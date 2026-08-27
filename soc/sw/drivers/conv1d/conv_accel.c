#include <stdint.h>

#include "conv_accel.h"
#include "config.h"
#include "conv1d_control_reg.h"

void conv_accel_init() {
    // Clear control register (START=0, ACCUMULATE=0)
    *(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_CONTROL_REG_OFFSET) = 0;
}

void conv_accel_start() {
    // Set START bit to trigger computation
    *(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_CONTROL_REG_OFFSET) |=
        (1 << CONV1D_CONTROL_CONTROL_START_BIT);
}

void conv_accel_wait() {
    // Poll DONE bit in STATUS register
    while (!(*(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_STATUS_REG_OFFSET) &
             (1 << CONV1D_CONTROL_STATUS_DONE_BIT))) {
        // Busy wait
    }
    // Clear START bit after completion
    *(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_CONTROL_REG_OFFSET) &=
        ~(1 << CONV1D_CONTROL_CONTROL_START_BIT);
}

void conv_accel_set_accumulate(uint8_t accumulate) {
    if (accumulate) {
        *(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_CONTROL_REG_OFFSET) |=
            (1 << CONV1D_CONTROL_CONTROL_ACCUMULATE_BIT);
    } else {
        *(volatile uint32_t *)(USER_CONV1D_CSR_START_ADDRESS + CONV1D_CONTROL_CONTROL_REG_OFFSET) &=
            ~(1 << CONV1D_CONTROL_CONTROL_ACCUMULATE_BIT);
    }
}
