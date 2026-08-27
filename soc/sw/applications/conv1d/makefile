# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Michele Caon, Luigi Giuffrida
# Date: 20/11/2024
# Description: Makefile for conv1d application.

#############################
# ----- CONFIGURATION ----- #
#############################

APP_PARAMS 		?= --in_len 8 --in_ch 4 --k_len 3 --k_num 2 --stride 1 --padding 0
PYTHONPATH      := $(PYTHONPATH):../../common

#####################
# ----- RULES ----- #
#####################

# Default alias
.PHONY: all
all: data.h

# Generate data.h
data.h: .force
	@echo "## Generating data.h..."
	PYTHONPATH=$(PYTHONPATH) python3 datagen.py $(APP_PARAMS)

# Dummy rule to force recompilation (because of parameters)
.force:

# Clean
.PHONY: clean
clean:
	$(RM) data.h
