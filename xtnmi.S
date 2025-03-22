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
    .global     xt_highint5
    .type       xt_highint5,@function
    .align      4
xt_highint5:
    // sadly to clear the interrupt mark, we need two free registers (address and data), 
    // so we cannot help building some stack to store them even for discarded clocks.
    movi    a0, _stack
    s32i    a1, a0, 0 // push a1
    s32i    a2, a0, 4 // push a2

    // read pins status
    movi    a0, GPIO_IN_REG
    l32i    a2, a0, 0

    // check /CS. if not set, just leave
#ifndef APU
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

write_command_to_buffer:
#endif

    // if we are getting a PPU_STATUS read
    movi    a0, PPU_STATUS_READ
    bnall   a2, a0, do_write

    // make sure we signal vblank to our reader
    // set D7 pin to output
    movi    a0, GPIO_ENABLE_W1TS_REG
    movi    a1, 1 << PIN_D7
    s32i    a1, a0, 0

    // set D7 = vblank
    movi    a1, vblank
    l32i    a1, a1, 0
    movi    a0, GPIO_OUT_W1TS_REG
    s32i    a1, a0, 0

do_write:
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

    // this is poor man's version of waiting until clock goes down. Actually we should keep the value until 10 ns after that.
    // but with all these saving commands we should be already after clock down at a ~2mhz. don't judge me.

    movi    a0, PPU_STATUS_READ
    bnall   a2, a0, exit

    // put D7 in input
    movi    a1, 1 << PIN_D7
    movi    a0, GPIO_ENABLE_W1TC_REG
    s32i    a1, a0, 0

    // set D7 = 0, for next round
    movi    a0, GPIO_OUT_W1TC_REG
    s32i    a1, a0, 0

exit:
    /* Clear interupt on CLK pin */
    movi    a0, GPIO_STATUS_W1TC_REG
    movi    a1, (1 << PIN_CLK)
    s32i    a1, a0, 0

    movi    a0, _stack
    l32i    a1, a0, 0 // restore a1
    l32i    a2, a0, 4 // restore a2

    rsr     a0, EXCSAVE5 // restore a0
    rfi     5

    .global ld_include_xt_nmi
ld_include_xt_nmi: