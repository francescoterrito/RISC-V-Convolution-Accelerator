// Created by Marco Penno <marco.penno@polito.it>, 2025.

// System library headers
#include <string.h>

#include "uart.h"
#include "print.h"
#include "timer.h"
#include "gpio.h"
#include "util.h"
#include "data.h"
#include "conv1d.h"
#include "conv_accel.h"

uint32_t R_sw[K_NUM * R_ROWS * R_COLS] = {0};
uint32_t R_hw[K_NUM * R_ROWS * R_COLS] = {0};

int main() {

    // INITIALIZATION
    // --------------
    uint32_t t_start=0, t_end=0;

    // Initialize the uart peripheral
    uart_init(); 

    // Initialize the hardware accelerator
    // TODO: implement this function in `conv_accel.c`
    conv_accel_init();

    // HARDWARE-ACCELERATED 1D CONVOLUTION
    // -----------------------------------
    // Get performance counter value
    t_start = get_mcycle(); // DO NOT CHANGE THIS LINE
    
    // Launch hardware-accelerated 1D convolution
    // TODO: implement this function in `conv1d.c`
    conv1d_hw(A, F, R_hw, INPUT_LEN, INPUT_CH, K_NUM);
    
    // Get performance counter value
    t_end = get_mcycle(); // DO NOT CHANGE THIS LINE
    printf("HW cycles: %u\n", t_end - t_start); // DO NOT CHANGE THIS LINE

    
    // SOFTWARE 1D CONVOLUTION
    // -----------------------
#if 1  // Set to 1 to enable SW comparison
    // Get performance counter value
    t_start = get_mcycle(); // DO NOT CHANGE THIS LINE

    // Launch software 1D convolution
    conv1d_sw(A, F, R_sw, INPUT_LEN, INPUT_CH, K_NUM);

    // Get performance counter value
    t_end = get_mcycle(); // DO NOT CHANGE THIS LINE
    printf("CPU cycles: %u\n", t_end - t_start); // DO NOT CHANGE THIS LINE


    // RESULT CHECKING
    // ---------------
    // Compare the CPU output data with the golden output
    if (memcmp(R_sw, R, R_SIZE) != 0) {
        printf("CPU result mismatch!\n");
        return -1;
    }
#endif

    // Compare the HW output data with the golden output
    if (memcmp(R_hw, R, R_SIZE) != 0) {
        printf("HW result mismatch!\n");
        return -1;
    }

    // Exit with no errors
    printf("Success!\n");

    return 0;  
}
