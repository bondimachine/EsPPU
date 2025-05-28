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
    l32i    a2, a0, 0

    // if it is a write, wait for data to be stable
    bbci    a2, PIN_RW, wait_data_stable

    // for the moment we consider every read to be PPU STATUS read

    // WTF? we got here but no /CS?
    bbsi    a2, PIN_CS, exit

    // load ppu_status_read
    movi    a1, ppu_status_read
    l32i    a1, a1, 0
    movi    a0, GPIO_OUT_W1TS_REG
    s32i    a1, a0, 0

    // set D7 pin to output
    movi    a1, 1 << PIN_D7
    movi    a0, GPIO_ENABLE_W1TS_REG
    s32i    a1, a0, 0

    // we store the command anyways as PPU STATUS read is used to reset the address latch.
    j       process_command

wait_data_stable:

    // here we are about ~100ns since started

    // we only should need to cycle here iif we are on a slow clock, but it shuldn't hurt that much to check.
    bbsi    a2, PIN_CLK, wait_clock_up_done

wait_clock_up:
    l32i    a2, a0, 0
    bbci    a2, PIN_CLK, wait_clock_up

wait_clock_up_done:
    // data stabilization is 140 ns max after clock high. from the 100ns above plus the checks we only need ~8 more 240mhz clocks
    nop; nop; nop;
    l32i    a2, a0, 0

process_command:

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

    // if it was a read, we need to release the data bus
    bbci    a2, PIN_RW, exit

    // wait until clock is down
    movi    a0, GPIO_IN_REG
wait_clock_down:
    l32i    a2, a0, 0
    bbsi    a2, PIN_CLK, wait_clock_down

    // set all data pins to 0 (so we only need to set the 1s in the next read, if any)
    movi    a1, 1 << PIN_D0 | 1 << PIN_D1 | 1 << PIN_D2 | 1 << PIN_D3 | 1 << PIN_D4 | 1 << PIN_D5 | 1 << PIN_D6 | 1 << PIN_D7
    movi    a0, GPIO_OUT_W1TC_REG
    s32i    a1, a0, 0

    // put them in input to stop driving the bus
    movi    a0, GPIO_ENABLE_W1TC_REG
    s32i    a1, a0, 0

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