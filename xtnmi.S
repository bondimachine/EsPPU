#include <xtensa/coreasm.h>
#include "soc/gpio_reg.h"
#include "pins.h"
#include "soc/timer_group_reg.h"
#include "config.h"

// sources https://github.com/espressif/esp-idf/blob/v5.4/examples/system/nmi_isr/main/asm_funcs.S 

    .data
_stack:
    .space      12

    .section .iram1,"ax"
    .global     xt_nmi
    .type       xt_nmi,@function
    .align      4
xt_nmi:
    movi    a0, _stack
    s32i    a1, a0, 0 // push a1
    s32i    a2, a0, 4 // push a2

    // until here we have ~350ns, even on NMI :-S

    // read pins status
    movi    a0, GPIO_IN_REG

wait_clock:
    l32i    a2, a0, 0

    // we only should need to cycle here iif we are on a slow clock, but it shuldn't hurt that much to check.
    bbci    a2, PIN_CLK, wait_clock

//     // if we are getting a PPU_STATUS read
//     movi    a1, PPU_STATUS_READ
//     bnall   a2, a1, wait_data_stable

//     // a PPU read, but not on 2002, move on.
//     movi    a1, (1 << PIN_A0) | (1 << PIN_A2)
//     bany    a2, a1, wait_data_stable

//     // make sure we signal vblank to our reader
//     // set D7 = vblank
//     movi    a1, vblank
//     l32i    a1, a1, 0
//     movi    a0, GPIO_OUT_W1TS_REG
//     s32i    a1, a0, 0

//     // set D7 pin to output
//     movi    a0, GPIO_ENABLE_W1TS_REG
//     movi    a1, 1 << PIN_D7
//     s32i    a1, a0, 0

//     // wait until clock is up
//     movi    a0, GPIO_IN_REG

// wait_clock_up:
//     l32i    a1, a0, 0
//     bbsi    a1, PIN_CLK, wait_clock_up


//     // set D7 = 0, for next round
//     movi    a1, 1 << PIN_D7
//     movi    a0, GPIO_OUT_W1TC_REG
//     s32i    a1, a0, 0

//     // put D7 in input
//     movi    a0, GPIO_ENABLE_W1TC_REG
//     s32i    a1, a0, 0

//     // we store the command anyways as it is used to reset the address latch.
//     j       write_command_to_buffer

wait_data_stable:
     // data stabilization is 140 ns max after clock high, so ~33 of our 240mhz clocks
    nop; nop; nop; nop; nop;
    nop; nop; nop; nop; nop;
    l32i    a2, a0, 0

#ifndef APU
    // WTF? we got here but no /CS?
    bbsi    a2, PIN_CS, exit
#else
    // check /CS. if set, avoid checking for /AS
    bbci    a2, PIN_CS, write_command_to_buffer

    // /CS not set, check for /AS. if not set, leave.
    bbsi    a2, PIN_AS, exit

    // /AS is set, read IN1
    movi    a0, GPIO_IN1_REG
    l32i    a1, a0, 0

    // remap bits
    slli    a1, a1, IN1_REMAP_SHIFT
    movi    a0, IN1_REMAP_MASK
    and     a1, a1, a0
    movi    a0, ~IN1_REMAP_MASK
    and     a2, a2, a0
    or      a2, a2, a1

    j       write_command_to_buffer

process_ppu_command:
#endif

    // for debugging
    // movi    a1, 1 << 12
    // movi    a0, GPIO_OUT_W1TS_REG
    // s32i    a1, a0, 0


write_command_to_buffer:
    movi    a0, command_buffer_write_index
    l32i    a1, a0, 0
    // Increment command_buffer_write_index, we do it here to spare memory access below
    addi    a1, a1, 1
    s32i    a1, a0, 0

    // cheap version of mod operator (%), as long as buffer size is power of 2
    movi    a0, (COMMAND_BUFFER_SIZE - 1)
    and     a1, a1, a0 // a1 = a1 % COMMAND_BUFFER_SIZE

    // store in command_buffer[a1 * 4]. wasting two bytes by saving all pins in favor of speed
    movi    a0, command_buffer
    addx4   a0, a1, a0 // a1 is x4 before adding, so it works as an uint32_t array index
    s32i    a2, a0, 0

exit:
    /* Clear interupt on CS/AS pins */
    movi    a0, GPIO_STATUS_W1TC_REG
#ifdef APU    
    movi    a1, (1 << PIN_CS) | (1 << PIN_AS)
#else
    movi    a1, (1 << PIN_CS)
#endif
    s32i    a1, a0, 0

    // until here we spent ~250ns

    // for debugging
    // movi    a1, 1 << 12
    // movi    a0, GPIO_OUT_W1TC_REG
    // s32i    a1, a0, 0

    movi    a0, _stack
    l32i    a1, a0, 0 // restore a1
    l32i    a2, a0, 4 // restore a2

    rsr     a0, EXCSAVE + XCHAL_NMILEVEL // restore a0
    rfi     XCHAL_NMILEVEL

    .global ld_include_xt_nmi
ld_include_xt_nmi: