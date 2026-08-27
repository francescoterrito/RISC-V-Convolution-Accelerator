// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: conv1d_sram_pkg.sv
// Author: Michele Caon
// Date: 08/12/2022
// Description: SRAM request and response data types

package conv1d_sram_pkg;
  // SRAM request
  typedef struct packed {
    logic        req;
    logic        we;
    logic [3:0]  be;
    logic [31:0] addr;
    logic [31:0] wdata;
  } sram_req_t;

  // SRAM response
  typedef struct packed {logic [31:0] rdata;} sram_rsp_t;
endpackage
