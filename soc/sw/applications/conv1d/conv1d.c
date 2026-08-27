#include <stdint.h>
#include <limits.h>
#include "conv1d.h"
#include "data.h" // for PAD, STRIDE, KERNEL_LEN
#include "config.h"
#include "conv_accel.h"

void conv1d_sw(const int8_t * const input_data, const int8_t * const filter, int32_t *output_data, const uint32_t input_len, const uint32_t input_ch, const uint32_t output_ch)
{
    // Calculate the length of the input data after padding
    uint32_t padded_input_len = input_len + 2 * PAD;

    // Calculate the length of the output data
    uint32_t output_len = ((padded_input_len - KERNEL_LEN) / STRIDE) + 1;

    // Loop over each output channel (output_ch times)
    for (uint32_t m = 0; m < output_ch; m++) {
        // Loop over each output position
        for (uint32_t o = 0; o < output_len; o++) {
            int32_t accumulated_output = 0;

            // Loop over each input channel
            for (uint32_t n = 0; n < input_ch; n++) {
                // Compute the starting index in the input data for this convolution window
                int32_t input_start = o * STRIDE - PAD;

                int32_t sum = 0;

                // Loop over the kernel elements
                for (uint32_t k_idx = 0; k_idx < KERNEL_LEN; k_idx++) {
                    int32_t input_index = input_start + k_idx;
                    int8_t input_value;

                    // Handle padding: if the input index is outside the valid range, use zero
                    if (input_index < 0 || input_index >= (int32_t)input_len) {
                        // Input value is zero due to padding
                        input_value = 0;
                    } else {
                        // Access the input data using the mem2d macro
                        input_value = mem2d(input_data, input_len, n, input_index);
                    }

                    // Access the filter coefficient using the mem3d macro
                    int8_t filter_value = mem3d(filter, input_ch, KERNEL_LEN, m, n, k_idx);

                    // Multiply and accumulate
                    sum += input_value * filter_value;
                }

                // Accumulate the sum from all input channels
                accumulated_output += sum;
            }

            // Store the accumulated result in the output data array
            // The output data is organized in row-major order: output_data[m][o] = output_data[m * output_len + o]
            output_data[m * output_len + o] = accumulated_output;
        }
    }
}

// Hardware accelerator tiling parameters (must match RTL)
#define TILE_CIN   4   // Input channels per tile
#define TILE_COUT  2   // Output channels per tile
#define TILE_T     12  // Output time samples per tile
#define HW_K       5   // Kernel length (fixed in hardware)
#define INPUT_WINDOW (TILE_T + HW_K - 1)  // 16 input samples needed for 12 outputs

// Scratchpad memory layout (word addresses, multiply by 4 for byte address)
#define SP_INPUT_BASE   0
#define SP_WEIGHT_BASE  64
#define SP_OUTPUT_BASE  104

void conv1d_hw(const int8_t * const in_data, const int8_t * const filter, int32_t *output_data, const uint32_t input_len, const uint32_t input_ch, const uint32_t output_ch) {

    // Calculate output length (valid convolution, no padding)
    uint32_t output_len = input_len - HW_K + 1;

    // Calculate number of tiles
    uint32_t num_cin_tiles = (input_ch + TILE_CIN - 1) / TILE_CIN;
    uint32_t num_cout_tiles = (output_ch + TILE_COUT - 1) / TILE_COUT;
    uint32_t num_t_tiles = (output_len + TILE_T - 1) / TILE_T;

    // Pointer to scratchpad memory
    volatile uint32_t *scratchpad = (volatile uint32_t *)USER_CONV1D_START_ADDRESS;

    // Loop over output channel tiles
    for (uint32_t cout_tile = 0; cout_tile < num_cout_tiles; cout_tile++) {
        uint32_t m_start = cout_tile * TILE_COUT;
        uint32_t m_end = (m_start + TILE_COUT > output_ch) ? output_ch : m_start + TILE_COUT;

        // Loop over time tiles
        for (uint32_t t_tile = 0; t_tile < num_t_tiles; t_tile++) {
            uint32_t t_start = t_tile * TILE_T;
            uint32_t t_end = (t_start + TILE_T > output_len) ? output_len : t_start + TILE_T;
            uint32_t actual_t_samples = t_end - t_start;

            // Loop over input channel tiles
            for (uint32_t cin_tile = 0; cin_tile < num_cin_tiles; cin_tile++) {
                uint32_t n_start = cin_tile * TILE_CIN;
                uint32_t n_end = (n_start + TILE_CIN > input_ch) ? input_ch : n_start + TILE_CIN;
                uint32_t actual_cin = n_end - n_start;

                // === Copy inputs to scratchpad ===
                // Layout: inputs[cin][t] at address cin * INPUT_WINDOW + t
                for (uint32_t c = 0; c < actual_cin; c++) {
                    uint32_t global_c = n_start + c;
                    for (uint32_t t = 0; t < INPUT_WINDOW; t++) {
                        uint32_t global_t = t_start + t;
                        int8_t input_val = 0;
                        if (global_t < input_len) {
                            input_val = mem2d(in_data, input_len, global_c, global_t);
                        }
                        // Store as 32-bit word (HW sign-extends lower 8 bits)
                        scratchpad[SP_INPUT_BASE + c * INPUT_WINDOW + t] = (int32_t)input_val;
                    }
                }
                // Zero-pad remaining input channels if needed
                for (uint32_t c = actual_cin; c < TILE_CIN; c++) {
                    for (uint32_t t = 0; t < INPUT_WINDOW; t++) {
                        scratchpad[SP_INPUT_BASE + c * INPUT_WINDOW + t] = 0;
                    }
                }

                // === Copy weights to scratchpad ===
                // Layout: weights[cout][cin][k] at address cout * (TILE_CIN * HW_K) + cin * HW_K + k
                for (uint32_t m = 0; m < (m_end - m_start); m++) {
                    uint32_t global_m = m_start + m;
                    for (uint32_t c = 0; c < actual_cin; c++) {
                        uint32_t global_c = n_start + c;
                        for (uint32_t k = 0; k < HW_K; k++) {
                            int8_t weight_val = mem3d(filter, input_ch, KERNEL_LEN, global_m, global_c, k);
                            scratchpad[SP_WEIGHT_BASE + m * (TILE_CIN * HW_K) + c * HW_K + k] = (int32_t)weight_val;
                        }
                    }
                    // Zero-pad remaining input channels if needed
                    for (uint32_t c = actual_cin; c < TILE_CIN; c++) {
                        for (uint32_t k = 0; k < HW_K; k++) {
                            scratchpad[SP_WEIGHT_BASE + m * (TILE_CIN * HW_K) + c * HW_K + k] = 0;
                        }
                    }
                }
                // Zero-pad remaining output channels if needed
                for (uint32_t m = (m_end - m_start); m < TILE_COUT; m++) {
                    for (uint32_t c = 0; c < TILE_CIN; c++) {
                        for (uint32_t k = 0; k < HW_K; k++) {
                            scratchpad[SP_WEIGHT_BASE + m * (TILE_CIN * HW_K) + c * HW_K + k] = 0;
                        }
                    }
                }

                // === Set accumulate mode ===
                // First cin_tile: overwrite (ACCUMULATE=0), others: accumulate (ACCUMULATE=1)
                conv_accel_set_accumulate(cin_tile > 0 ? 1 : 0);

                // === Start accelerator and wait ===
                conv_accel_start();
                conv_accel_wait();
            }

            // === Read outputs from scratchpad ===
            // Layout: outputs[cout][t] at address cout * TILE_T + t
            for (uint32_t m = 0; m < (m_end - m_start); m++) {
                uint32_t global_m = m_start + m;
                for (uint32_t t = 0; t < actual_t_samples; t++) {
                    uint32_t global_t = t_start + t;
                    output_data[global_m * output_len + global_t] =
                        (int32_t)scratchpad[SP_OUTPUT_BASE + m * TILE_T + t];
                }
            }
        }
    }

    return;
}
