#include "config.h"
#include "simple_cnt.h"
#include "uart.h"
#include "print.h"

// Main body
// ---------
int main(void)
{
    // Initialize the uart peripheral
    uart_init(); 

    uint32_t val = 255;
    uint32_t ret = 0;

    // Set the counter threshold
    simple_cnt_set_threshold(val);

    // Read back the counter threshold
    ret = simple_cnt_get_threshold();
    if (val != ret) {
        printf("Error: threshold value mismatch\n");
        uart_write_flush();
        return -1;
    }

    // Set the counter value
    val = 42;
    simple_cnt_set_value(val);

    // Read back the counter value
    ret = simple_cnt_get_value();
    if (val != ret) {
        printf("Error: value mismatch\n");
        uart_write_flush();
        return -1;
    }

    // Clear the counter
    simple_cnt_clear();

    // Read back the counter value
    ret = simple_cnt_get_value();
    if (0 != ret) {
        printf("Error: value mismatch\n");
        uart_write_flush();
        return -1;
    }

    // Enable the counter
    simple_cnt_enable();

    // Wait for the counter to reach the threshold
    simple_cnt_wait_poll();
    printf("TC set\n");
    uart_write_flush();

    // Disable the counter and clear it
    simple_cnt_disable();
    simple_cnt_clear();

    ret = simple_cnt_get_value();
    printf("Done. Counter value: %u\n", ret);
    uart_write_flush();

    return 1;
}
