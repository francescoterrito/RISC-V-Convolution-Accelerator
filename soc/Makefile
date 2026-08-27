# Copyright (c) 2024 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Authors:
# - Philippe Sauter <phsauter@iis.ee.ethz.ch>
# Modified by Marco Penno <marco.penno@polito.it>, 2025

# Tools
BENDER	  ?= bender
PYTHON3   ?= python3
VERILATOR ?= /foss/tools/bin/verilator
YOSYS     ?= yosys
OPENROAD  ?= openroad
KLAYOUT   ?= klayout
REGGEN    ?= $(PYTHON3) $(shell $(BENDER) path register_interface)/vendor/lowrisc_opentitan/util/regtool.py

# Directories
# directory of the path to the last called Makefile (this one)
PROJ_DIR  := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))


default: help

################
# Dependencies #
################
# Download RCX file used for parasitic extraction from ORFS (configuration got ok by IHP)
IHP_RCX_URL  := "https://raw.githubusercontent.com/The-OpenROAD-Project/OpenROAD-flow-scripts/7747f88f70daaeb63f43ce36e71829707b7e3fa7/flow/platforms/ihp-sg13g2/IHP_rcx_patterns.rules"
IHP_RCX_FILE := $(PROJ_DIR)/openroad/IHP_rcx_patterns.rules

## Checkout/update dependencies using Bender
checkout: $(IHP_RCX_FILE)
	$(BENDER) checkout
	git submodule update --init --recursive
	@mkdir -p ihp13/pdk && ln -sf /foss/pdks/ihp-sg13g2 ihp13/pdk/ihp-sg13g2
	
$(IHP_RCX_FILE): 
	curl -L -o $@ $(IHP_RCX_URL)

## Reset dependencies (without updating Bender.lock)
clean-deps:
	rm -rf .bender
	git submodule deinit -f --all

.PHONY: checkout clean-deps


############
# Software #
############
SW_HEX := sw/bin/main.hex

PROG ?= example

software: sw

sw:
	$(MAKE) -C sw clean
	$(MAKE) -C sw compile PROG=$(PROG)

.PHONY: software sw

## Generate input data
APP_PARAMS 		?= --in_len 8 --in_ch 4 --k_len 3 --k_num 2 --stride 1 --padding 0
.PHONY: gen-data
gen-data:
	@echo "Generating input data..."
	@$(MAKE) -C sw/applications/conv1d/ APP_PARAMS="$(APP_PARAMS)" 2>&1 || { \
	    echo ""; \
	    echo "\033[91mData generation failed!\033[0m"; \
	    exit 1; \
	}
	@echo "\033[92mData generated successfully\033[0m"

##################
# RTL Simulation #
##################
# Vendoring
## Update vendored IPs
.PHONY: bender-update
bender-update:
	@echo "Vendoring all target files..."
	@bender update
	@bender vendor init

# Verilator
# Turn off style warnings and well-defined SystemVerilog warnings that should be part of -Wno-style
VERILATOR_ARGS = -Wno-fatal -Wno-style \
	-Wno-BLKANDNBLK -Wno-WIDTHEXPAND -Wno-WIDTHTRUNC -Wno-WIDTHCONCAT -Wno-ASCRANGE

VERILATOR_ARGS += --binary -j 0
VERILATOR_ARGS += --timing --autoflush --trace-fst --trace-threads 2 --trace-structs
VERILATOR_ARGS +=  --unroll-count 2 --unroll-stmts 1
VERILATOR_ARGS += --x-assign fast --x-initial fast
VERILATOR_CFLAGS += -O3 -march=native -mtune=native

verilator/croc.f: Bender.lock Bender.yml
	$(BENDER) script verilator -t rtl -t verilator -DSYNTHESIS -DVERILATOR > $@

verilator/obj_dir/Vtb_croc_soc: verilator/croc.f $(SW_HEX)
	cd verilator; $(VERILATOR) $(VERILATOR_ARGS) -O3 --top tb_croc_soc -f croc.f

## Simulate RTL using Verilator
verilator: verilator/obj_dir/Vtb_croc_soc
	cd verilator; obj_dir/Vtb_croc_soc +binary="$(realpath $(SW_HEX))"

.PHONY: verilator

# Open waveform dump with GTKWave
.PHONY: waves
waves:
	gtkwave verilator/croc.fst $<

####################
# Open Source Flow #
####################
# Bender manages the different IPs and can be used to generate file-lists for synthesis
TOP_DESIGN     ?= croc_chip
DUT_DESIGN	   ?= croc_soc
BENDER_TARGETS ?= asic ihp13 rtl synthesis
SV_DEFINES     ?= VERILATOR SYNTHESIS COMMON_CELLS_ASSERTS_OFF

## Generate croc.flist used to read design in yosys
yosys-flist: Bender.lock Bender.yml rtl/*/Bender.yml
	$(BENDER) script flist-plus $(foreach t,$(BENDER_TARGETS),-t $(t)) $(foreach d,$(SV_DEFINES),-D $(d)=1) > $(PROJ_DIR)/croc.flist

include yosys/yosys.mk
include openroad/openroad.mk

klayout/croc_chip.gds: $(OR_OUT)/croc.def klayout/*.sh klayout/*.py
	./klayout/def2gds.sh

## Generate merged .gds from openroads .def output
klayout: klayout/croc_chip.gds

.PHONY: klayout yosys-flist


#################
# Documentation #
#################

.PHONY: help
help:
	@echo ""
	@echo "\033[1;34mCroc MCU Makefile\033[0m"
	@echo "\033[1;35mSoftware Build and RTL Simulation:\033[0m"
	@echo "  \033[32mgen-data\033[0m            Generate input data for simulation using APP_PARAMS"
	@echo "  \033[32msw\033[0m                  Build the selected bare-metal software (PROG=<name>)"
	@echo "  \033[32mverilator\033[0m           Build the RTL simulation model and run the testbench"
	@echo "  \033[32mwaves\033[0m               Open waveform dump with GTKWave"
	@echo ""
	@echo "\033[1;35mRTL-to-GDSII Flow:\033[0m"
	@echo "  \033[32myosys-flist\033[0m         Generate croc.flist using Bender"
	@echo "  \033[32myosys\033[0m               Run RTL → gate-level synthesis using Yosys"
	@echo "  \033[32mopenroad\033[0m            Perform floorplanning, placement, CTS, routing and signoff"
	@echo "  \033[32mklayout\033[0m             Convert the final DEF into a GDS layout using KLayout"
	@echo ""
	@echo "\033[1;35mUtilities:\033[0m"
	@echo "  \033[32mbender-update\033[0m       Update vendored IPs"
	@echo "  \033[32mclean\033[0m               Remove build files and temporary outputs"
	@echo "  \033[32mclean-all\033[0m           Remove all auto-generated files (use with caution)"
	@echo "  \033[32mhelp\033[0m                Display this help message"
	@echo ""
	@echo "\033[1;33mExamples:\033[0m"
	@echo "  \033[90m# Build the bare-metal software:\033[0m"
	@echo "  \033[90mmake sw PROG=example\033[0m"
	@echo ""
	@echo "  \033[90m# Custom data generation:\033[0m"
	@echo "  \033[90mmake gen-data APP_PARAMS=\"--in_len 16 --in_ch 8 --k_len 5 --k_num 4\"\033[0m"


###########
# Cleanup #
###########

## Delete generated files and directories
.PHONY: clean
clean: 
	rm -f $(SV_FLIST)
	rm -f klayout/croc_chip.gds
	rm -rf verilator/obj_dir/
	rm -f verilator/croc.f
	rm -f verilator/croc.vcd
	$(MAKE) ys_clean
	$(MAKE) or_clean

## Remove all generated files across the entire project
.PHONY: clean-all
clean-all: clean
	@echo "Running full recursive cleanup..."
	@$(MAKE) -C sw clean || true
	@$(MAKE) -C yosys clean || true
	@$(MAKE) -C openroad clean || true
	@$(MAKE) -C klayout clean || true
	@echo "Full cleanup completed."
