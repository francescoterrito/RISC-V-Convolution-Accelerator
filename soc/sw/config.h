// Copyright 2022 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Nils Wistoff <nwistoff@iis.ee.ethz.ch>
// Paul Scheffler <paulsc@iis.ee.ethz.ch>
// Modified by Marco Penno <marco.penno@polito.it>, 2025

#pragma once

/* Address map */
#define SOCCTRL_BASE_ADDR 0x03000000
#define UART_BASE_ADDR    0x03002000
#define GPIO_BASE_ADDR    0x03005000
#define TIMER_BASE_ADDR   0x0300A000
#define USER_BASE_ADDR    0x20000000


/* User-defined Address map */

// SIMPLE_CNT
#define USER_CNT_START_ADDRESS (USER_BASE_ADDR + 0x00000000)
#define USER_CNT_SIZE 0x00010000
#define USER_CNT_END_ADDRESS (USER_CNT_START_ADDRESS + USER_CNT_SIZE)

// CONV1D_ACCEL
#define USER_CONV1D_START_ADDRESS (USER_BASE_ADDR + 0x00010000)
#define USER_CONV1D_SIZE 0x00010000
#define USER_CONV1D_END_ADDRESS (USER_CONV1D_START_ADDRESS + USER_CONV1D_SIZE)

// CSR SIMPLE_CNT
#define USER_CNT_CSR_START_ADDRESS (USER_BASE_ADDR + 0x00020000)
#define USER_CNT_CSR_SIZE 0x00001000
#define USER_CNT_CSR_END_ADDRESS (USER_CNT_CSR_START_ADDRESS + USER_CNT_CSR_SIZE)

// CSR CONV1D_ACCEL
#define USER_CONV1D_CSR_START_ADDRESS (USER_BASE_ADDR + 0x00021000)
#define USER_CONV1D_CSR_SIZE 0x00001000
#define USER_CONV1D_CSR_END_ADDRESS (USER_CONV1D_CSR_START_ADDRESS + USER_CONV1D_CSR_SIZE)

/* Frequencies */ 
#define TB_FREQUENCY 20000000
#define TB_BAUDRATE    115200

/* Peripheral configs */ 
// UART
#define UART_BYTE_ALIGN 4
#define UART_FREQ       TB_FREQUENCY
#define UART_BAUD       TB_BAUDRATE