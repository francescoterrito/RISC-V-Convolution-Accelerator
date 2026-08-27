// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Philippe Sauter <phsauter@iis.ee.ethz.ch>
//
// Modified by Marco Penno <marco.penno@polito.it>, 2025.

#pragma once

#include <stdarg.h>

extern void putchar(char);

// simple printf with support for %x and %u formatter but no others
void printf(char *fmt, ...);
