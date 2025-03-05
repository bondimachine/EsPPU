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
    bbsi    a2, PIN_CS, exit

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