import argparse
import os
import numpy as np
from c_gen import CFileGen


def main():
    descr = """\
# Generate the input data and the golden output for 1D convolution, used by main.c to run the kernel.
    R = conv1D(A, F, STRIDE, PAD)
"""

    # Create command line parser
    cmd_parser = argparse.ArgumentParser(
        prog="datagen",
        description="Conv1d golden model.", 
        epilog=descr
    )

    # Number of input channels (N)
    cmd_parser.add_argument(
        "--in_ch", "-n",
        type=int,
        default=4,
        help="Number of input channels."
    )

    # Length of each input channel (L)
    cmd_parser.add_argument(
        "--in_len", "-l",
        type=int,
        default=8,
        help="Length of each input channel."
    )

    # Length of each filter (K)
    cmd_parser.add_argument(
        "--k_len", "-k",
        type=int,
        default=3,
        help="Length of each filter."
    )

    # Number of filter sets (M)
    cmd_parser.add_argument(
        "--k_num",
        "-f",
        type=int,
        default=1,
        help="Number of filter sets (i.e., output channels).",
    )

    # Convolution stride
    cmd_parser.add_argument(
        "--stride", "-r",
        type=int, 
        default=1, 
        help="Stride of the convolution."
    )

    # Convolution padding
    cmd_parser.add_argument(
        "--padding", "-p",
        type=int, 
        default=0, 
        help="Padding of the convolution."
    )

    # Output directory
    cmd_parser.add_argument(
        "--outdir",
        "-o",
        type=str,
        default=os.path.dirname(os.path.abspath(__file__)),
        help="Directory where to store the otuput files.",
    )

    # Seed for random number generator
    cmd_parser.add_argument(
        "--seed", "-s",
        type=int,
        help="Seed for numpy PRG (normally used for debug)."
    )

    # Version
    cmd_parser.add_argument(
        "--version",
        "-v",
        action="version",
        version="%(prog)s 0.1.0",
        help="print the version and exit.",
    )

    # Parse command line arguments
    args = cmd_parser.parse_args()

    # Set parameters
    out_dir = args.outdir
    input_len = args.in_len
    input_ch = args.in_ch
    kernel_len = args.k_len
    k_num = args.k_num
    stride = args.stride
    pad = args.padding
    header_basename = "data"

    # Print arguments
    print("1D convolution golden model.")
    print("    R = conv1D(A, F, STRIDE, PAD)")
    print("Arguments:")
    print("- output directory: " + out_dir)
    print("- input length: " + str(input_len))
    print("- input channels: " + str(input_ch))
    print("- kernel length: " + str(kernel_len))
    print("- number of filter sets: " + str(k_num))
    print("- stride: " + str(stride))
    print("- padding: " + str(pad))

    # Generate random inputs
    if args.seed is not None:
        np.random.seed(args.seed)

    # Input matrix A [input_len x input_ch]
    A = np.random.randint(
        low=np.iinfo(np.int8).min, high=np.iinfo(np.int8).max, size=(input_ch, input_len), dtype=np.int8
    )

    # Filter F [kernel_len x input_ch x k_num]
    F = np.random.randint(
        low=np.iinfo(np.int8).min, high=np.iinfo(np.int8).max, size=(k_num, input_ch, kernel_len), dtype=np.int8
    )

    # -----------------------------------------------------------------------
    # Golden output [input_len x k_num]
    # -----------------------------------------------------------------------
    # Compute the number of output elements
    output_len = (input_len - kernel_len + 2 * pad) // stride + 1

    # Reset the outputs
    R = np.zeros((k_num, output_len), dtype=np.int32)
    
    # Loop over each set of filters (M times)
    for m in range(k_num):
        # Initialize an array to accumulate the convolution results for this filter set.
        # For 'same' convolution mode, the output length is the same as the input length (L).
        accumulated_output = np.zeros(output_len, dtype=np.int32)
        
        # Loop over each input channel (N channels)
        for n in range(input_ch):
            # Pad the input data with zeros at both ends
            input_padded = A[n]
            input_padded = np.pad(input_padded.astype(np.int32), (pad, pad), mode='constant', constant_values=0)
    
            # Perform convolution between input data[n] and filter filters[m][n].
            # - 'valid': Returns the convolution result without zero-padding (output length is L - F + 1).
            # NOTE: we use `correlate` instead of `convolve` because the latter flips the filter (following a common mathematical convention).
            filter_values = F[m][n]
            conv_result = np.correlate(input_padded, filter_values.astype(np.int32), mode='valid')

            # Apply stride by slicing (subsampling) the convolution output
            # NOTE: this is not an efficient way to apply stride, since involves wasteful computation when
            # computing conv_result. Do NOT use this algorithm in your hardware implementation.
            conv_strided = conv_result[::stride]

            # Accumulate the convolution results from each channel.
            accumulated_output += conv_strided[:output_len]
        
        # Store the accumulated convolution results for this filter set.
        R[m] = accumulated_output
        
    # At this point, 'R' contains M output arrays.
    # Each output array is the result of summing the convolutions of N input channels
    # with their respective filters in one filter set.
    # 'R[m]' corresponds to the output from filter set 'm'.

    print("- generated input data (A, F) and golden output (R).")

    # -----------------------------------------------------------------------
    # Generate C files
    # -----------------------------------------------------------------------
    data_gen = CFileGen(header_basename)

    data_gen.add_comment("1D convolution data")
    data_gen.add_comment("Input data")
    data_gen.add_comment("A: input matrix [input_len x input_ch]")
    data_gen.add_comment("F: filter matrix [kernel_len x input_ch x k_num]")
    data_gen.add_comment("Output data")
    data_gen.add_comment("R: output matrix [input_len x k_num]")
    data_gen.add_comment("Stride: " + str(stride))
    data_gen.add_comment("Padding: " + str(pad))

    data_gen.add_macro("INPUT_LEN", str(input_len))
    data_gen.add_macro("INPUT_CH", str(input_ch))
    data_gen.add_macro("KERNEL_LEN", str(kernel_len))
    data_gen.add_macro("K_NUM", str(k_num))
    data_gen.add_macro("STRIDE", str(stride))
    data_gen.add_macro("PAD", str(pad))

    data_gen.add_input_matrix("A", A)
    data_gen.add_input_matrix("F", F)
    data_gen.add_output_matrix("R", R)

    data_gen.write_header(out_dir)
    data_gen.write_source(out_dir)


if __name__ == "__main__":
    main()
