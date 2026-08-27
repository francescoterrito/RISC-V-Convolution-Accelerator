# Source provenance

This repository combines two stages of one course project. The directory split records the technical boundary, not two separate projects.

## Project-specific implementation

- `accelerator/hw/conv1d.sv`: tiled convolution FSM, four-lane datapath, address generation, accumulation mode, and scratchpad arbitration
- `accelerator/hw/conv1d_obi.sv` and `accelerator/hw/control-reg/`: bus/control integration and extended register map
- `accelerator/tb/conv1d_tb.cpp`: full-workload tiling, accelerator control, output collection, and golden-model comparison
- `soc/rtl/user_domain.sv`: Croc address-map, register-bus, accelerator, and interrupt integration
- `soc/rtl/user_domain/conv1d/`: accelerator sources imported into the SoC
- `soc/sw/applications/conv1d/conv1d.c`: firmware tiling and scratchpad data movement
- `soc/sw/drivers/conv1d/`: memory-mapped control driver
- `soc/ihp13/tc_sram_impl.sv`: logical 128 x 32-bit SRAM mapping onto the available 64 x 64 macro
- `soc/openroad/scripts/` and `soc/openroad/src/`: project backend constraints, macro placement, and power-grid adaptations

## Supplied course and upstream material

The standalone bus infrastructure, testbench framework, register generator, and vendor packages were supplied as the accelerator starting point. The SoC stage is based on the Croc MCU and includes upstream RTL and scripts from ETH Zurich, the University of Bologna, PULP Platform, lowRISC/OpenTitan, and related projects. File-level copyright, author, SPDX, and license notices have been preserved.

The main license texts inherited with the two starting trees remain at `accelerator/LICENSE` and `soc/LICENSE.md`. No new top-level license has been added. Users must follow the notices that apply to each file and dependency.

## Generated evidence

- Generated register RTL and headers required by the checked-in design
- `results/accelerator-full-run.vcd.gz`, a compressed control/scheduling trace
- `results/vcd_summary.csv`, values collected from the testbench, VCD, and Lab 3 report
- `output/pdf/lab3-report.pdf`, the Lab 3 technical report

## Reproduction dependencies

Full SoC and physical-flow reproduction requires the RISC-V toolchain, Bender dependencies, and an IHP Open PDK installation. Generated test vectors, firmware binaries, tool databases, and physical-design databases are not included.
