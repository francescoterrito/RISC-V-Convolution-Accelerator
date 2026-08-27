#ifndef CONV1D_HH_
#define CONV1D_HH_

#include <stdlib.h>
#include <stdint.h>

// ------
// MACROS
// ------

// Access to 2D and 3D arrays
#define mem2d(data,data_len,i,j)                    data[((i) * (data_len)) + (j)]
#define mem3d(filter,filter_depth,filter_len,k,i,j) filter[((k) * (filter_depth) + (i)) * (filter_len) + (j)]

// -------------------
// FUNCTION PROTOTYPES
// -------------------

/**
 * Performs 1D convolution on input data on the system CPU.
 *
 * @param input_data Pointer to input data array of size input_ch * input_len.
 *                   The data is organized in row-major order: input_data[n][i] = input_data[n * input_len + i].
 * @param filter Pointer to filter array of size output_ch * input_ch * KERNEL_LEN.
 *               The data is organized in row-major order: filter[m][n][k] = filter[((m * input_ch + n) * KERNEL_LEN) + k].
 * @param output_data Pointer to output data array of size output_ch * output_len, where output_len depends on convolution parameters.
 * @param input_len Length of each input channel.
 * @param input_ch Number of input channels.
 * @param output_ch Number of output channels (number of filter sets).
 */
void conv1d_sw(const int8_t * const in_data, const int8_t * const filter, int32_t *output_data, const uint32_t input_len, const uint32_t input_ch, const uint32_t output_ch);

/**
 * Performs 1D convolution on input data using the hardware accelerator.
 *
 * @param input_data Pointer to input data array of size input_ch * input_len.
 *                   The data is organized in row-major order: input_data[n][i] = input_data[n * input_len + i].
 * @param filter Pointer to filter array of size output_ch * input_ch * KERNEL_LEN.
 *               The data is organized in row-major order: filter[m][n][k] = filter[((m * input_ch + n) * KERNEL_LEN) + k].
 * @param output_data Pointer to output data array of size output_ch * output_len, where output_len depends on convolution parameters.
 * @param input_len Length of each input channel.
 * @param input_ch Number of input channels.
 * @param output_ch Number of output channels (number of filter sets).
 */
void conv1d_hw(const int8_t * const in_data, const int8_t * const filter, int32_t *output_data, const uint32_t input_len, const uint32_t input_ch, const uint32_t output_ch);

#endif /* CONV1D_HH_ */
