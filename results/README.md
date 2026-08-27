# Saved results

`accelerator-full-run.vcd.gz` is the standalone-accelerator trace. Its decompressed SHA-256 is recorded in [`vcd_summary.csv`](vcd_summary.csv). The trace uses a `1 ps` timescale and a two-unit testbench clock period.

Direct inspection of `start_i`, `accumulate_i`, and `done_o` gives 176 complete accelerator invocations: 44 overwrite runs at 1,369 clock cycles each and 132 accumulation runs at 1,417 clock cycles each. This matches the expected `4 x 11 x 4` tile schedule in [`accelerator/tb/conv1d_tb.cpp`](../accelerator/tb/conv1d_tb.cpp).

The Lab 3 report records the full 992-output comparison, SoC hardware and CPU cycle counts, and Yosys/OpenROAD implementation results. `vcd_summary.csv` collects these values together with direct VCD measurements and source locations.

The complete testbench averaged 1,638 cycles per run including OBI input/weight transfers and output reads. Direct accelerator `start_i`-to-`done_o` latency is 1,369 cycles for overwrite runs and 1,417 cycles for accumulation runs.
