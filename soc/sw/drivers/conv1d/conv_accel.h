#ifndef CONV_ACCEL_HH_
#define CONV_ACCEL_HH_

#include <stdint.h>

/**
 * @brief Initialize the accelerator.
 *
 */
void conv_accel_init();

/**
 * @brief Trigger the start of processing.
 *
 */
void conv_accel_start();

/**
 * @brief Wait for the accelerator to complete execution.
 *
 */
void conv_accel_wait();

/**
 * @brief Set accumulate mode for partial sum handling.
 * @param accumulate 1 = read-modify-write (accumulate), 0 = overwrite
 */
void conv_accel_set_accumulate(uint8_t accumulate);

#endif /* CONV_ACCEL_HH_ */