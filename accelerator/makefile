# Copyright 2024 Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Luigi Giuffrida, modified by Marco Penno
# Date: 03/10/2024 (original), modified 11/11/2025
# Description: Top-level makefile for conv1d accelerator.

# ----- CONFIGURATION ----- #
# Global configuration
ROOT_DIR            := $(realpath .)
BUILD_DIR           := obj_dir
LOG_DIR             := logs

# RTL simulation
LOG_LEVEL           ?= LOG_MEDIUM

# Verilator configuration
TRACE_ENABLED       ?= 1
TRACE_DEPTH         ?= 99
TRACE_MAX_ARRAY     ?= 128
VERILATOR_THREADS   ?= 1
OPTIMIZATION_LEVEL  ?= -O3
VERILATOR_WARNINGS  ?= -Wall -Wno-fatal

APP_PARAMS      	?= --in_len 8 --in_ch 4 --k_len 3 --k_num 2 --stride 1 --padding 0

# ----- TARGETS ----- #
## Default target
.PHONY: all
all: | gen-data verilator-sim

# Create log directory if it doesn't exist
$(LOG_DIR):
	@mkdir -p $(LOG_DIR)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

## @section Verilator RTL simulation

## Build simulation model (do not launch simulation)
.PHONY: verilator-build
verilator-build: | bender-flist $(BUILD_DIR)
	@echo "Building Verilator simulation model..."
	@verilator \
	    --cc \
	    --exe \
	    --build \
	    -f bender_files.f \
	    tb/conv1d_tb.cpp \
	    tb/tb_macros.cpp \
	    tb/tb_components.cpp \
	    tb/data.c \
	    --top-module conv1d_tb_wrapper \
	    --Mdir $(BUILD_DIR) \
	    -I./hw/vendor/pulp-platform-common-cells/include \
	    -I./hw/vendor/pulp-platform-register-interface/include \
	    -I./tb \
	    $(if $(filter 1,$(TRACE_ENABLED)),--trace --trace-fst --trace-structs --trace-max-array $(TRACE_MAX_ARRAY)) \
	    $(if $(filter-out 1,$(VERILATOR_THREADS)),--threads $(VERILATOR_THREADS)) \
	    -CFLAGS "-std=c++14 -I../tb -I../sw $(OPTIMIZATION_LEVEL)" \
	    -LDFLAGS "-lpthread" \
	    $(VERILATOR_WARNINGS) \
	    --x-assign unique \
	    --x-initial unique

## Build and run simulation
.PHONY: verilator-sim
verilator-sim: | verilator-build verilator-run

## Run simulation
.PHONY: verilator-run
verilator-run:
	@echo "Running Verilator simulation..."
	@$(BUILD_DIR)/Vconv1d_tb_wrapper; \
	EXIT_CODE=$$?; \
	if [ $$EXIT_CODE -eq 0 ]; then \
	    echo "\033[92mSimulation passed successfully\033[0m"; \
	else \
	    echo "\033[91mSimulation failed with exit code $$EXIT_CODE\033[0m" >&2; \
	    exit $$EXIT_CODE; \
	fi

## Generate input data
.PHONY: gen-data
gen-data:
	@echo "Generating input data..."
	@$(MAKE) -C tb APP_PARAMS="$(APP_PARAMS)" 2>&1 || { \
	    echo ""; \
	    echo "\033[91mData generation failed!\033[0m"; \
	    exit 1; \
	}
	@echo "\033[92mData generated successfully\033[0m"

# Open waveform dump with GTKWave
.PHONY: waves
waves: $(LOG_DIR)/waves.fst | .check-gtkwave
	gtkwave -a tb/waves.gtkw $<

## @section Hardware generation

## Generate configuration registers
.PHONY: gen-registers
gen-registers: hw/control-reg/data/control_reg.hjson 
	@echo "Generating configuration registers..."
	@command -v hjson >/dev/null 2>&1 || { \
	    echo "hjson not found, installing..."; \
	    pip install --user hjson; \
	};
	@bash hw/control-reg/gen-control-reg.sh 2>&1

## @section Utilities

## Update vendored IPs
.PHONY: bender-update
bender-update:
	@echo "Vendoring all target files..."
	@bender vendor init
.PHONY: bender-flist

## Generate filelist with bender
bender-flist:
	@echo "Generating file list with Bender..."
	@bender script verilator -t test > bender_files.f 
	@if [ ! -f bender_files.f ]; then \
	    echo "ERROR: Failed to generate file list" \
	    exit 1; \
	fi

# ----- HELPERS ----- #

## @section Helpers

# Check if GTKWave is available
.PHONY: .check-gtkwave
.check-gtkwave:
	@if [ ! `which gtkwave` ]; then \
	    printf -- "### ERROR: 'gtkwave' is not in PATH.\n" >&2; \
	    exit 1; fi

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) -r $(LOG_DIR)
	$(RM) tb/data.h tb/data.c

.PHONY: clean-all
clean-all: | clean
	$(RM) -r hw/vendor/*
	$(RM) -r hw/control-reg/rtl/*
	$(RM) -r sw/*
	$(RM) Bender.lock
	$(RM) bender_files.f

.PHONY: help
help:
	@echo ""
	@echo "\033[1;34mConv1D Accelerator Makefile\033[0m"
	@echo ""
	@echo "\033[1;33mTargets:\033[0m"
	@echo "  \033[32mall\033[0m                 Default target: runs gen-data and verilator-sim"
	@echo ""
	@echo "\033[1;35mVerilator RTL Simulation:\033[0m"
	@echo "  \033[32mverilator-build\033[0m     Build the simulation model without launching simulation"
	@echo "  \033[32mverilator-sim\033[0m       Build and run the simulation"
	@echo "  \033[32mverilator-run\033[0m       Run the simulation (requires prior build)"
	@echo "  \033[32mgen-data\033[0m            Generate input data for simulation using APP_PARAMS"
	@echo "  \033[32mwaves\033[0m               Open waveform dump with GTKWave"
	@echo ""
	@echo "\033[1;35mHardware Generation:\033[0m"
	@echo "  \033[32mgen-registers\033[0m       Generate configuration registers from HJSON specification"
	@echo ""
	@echo "\033[1;35mUtilities:\033[0m"
	@echo "  \033[32mbender-update\033[0m       Update vendored IPs"
	@echo "  \033[32mbender-flist\033[0m        Generate a file list of all source files for Verilator"
	@echo "  \033[32mclean\033[0m               Remove build directory and generated data files"
	@echo "  \033[32mclean-all\033[0m           Remove all automatically generated files. Use with caution"
	@echo "  \033[32mhelp\033[0m                Display this help message."
	@echo ""
	@echo "\033[1;33mExamples:\033[0m"
	@echo "  \033[90m# Standard simulation with tracing:\033[0m"
	@echo "  \033[90mmake verilator-sim LOG_LEVEL=LOG_HIGH\033[0m"
	@echo ""
	@echo "  \033[90m# Custom test parameters:\033[0m"
	@echo "  \033[90mmake gen-data APP_PARAMS=\"--in_len 16 --in_ch 8 --k_len 5 --k_num 4\"\033[0m"
	@echo ""
	@echo "  \033[90m# Complete workflow:\033[0m"
	@echo "  \033[90mmake all\033[0m"