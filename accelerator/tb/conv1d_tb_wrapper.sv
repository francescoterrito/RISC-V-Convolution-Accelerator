// Copyright 2024 Politecnico di Torino.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 2.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-2.0. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// File: conv1d_tb_wrapper.sv
// Author(s):
//   Luigi Giuffrida
// Date: 10/11/2024
// Description: Wrapper for Verilator TB

module conv1d_tb_wrapper (
  input logic clk_i,
  input logic rst_ni,

  // OBI interface (counter value)
  input  logic        obi_req_i,
  input  logic        obi_we_i,
  input  logic [ 3:0] obi_be_i,
  input  logic [31:0] obi_addr_i,
  input  logic [31:0] obi_wdata_i,
  output logic        obi_gnt_o,
  output logic        obi_rvalid_o,
  output logic [31:0] obi_rdata_o,

  // Register Interface (configuration registers)
  input  logic        reg_valid_i,
  input  logic        reg_write_i,
  input  logic [ 3:0] reg_wstrb_i,
  input  logic [31:0] reg_addr_i,
  input  logic [31:0] reg_wdata_i,
  output logic        reg_error_o,
  output logic        reg_ready_o,
  output logic [31:0] reg_rdata_o,

  // Terminal count interrupt
  output logic done_int_o  // interrupt to host system
);
  // INTERNAL SIGNALS
  croc_pkg::sbr_obi_req_t           obi_req;  // from host system
  croc_pkg::sbr_obi_rsp_t           obi_rsp;  // to host system
  conv1d_reg_pkg::reg_req_t         reg_req;  // from host system
  conv1d_reg_pkg::reg_rsp_t         reg_rsp;  // to host system

  // -------
  // COUNTER
  // -------
  // OBI request
  always_comb begin
    obi_req.req      = obi_req_i;
    obi_req.a.we     = obi_we_i;
    obi_req.a.be     = obi_be_i;
    obi_req.a.addr   = obi_addr_i;
    obi_req.a.wdata  = obi_wdata_i;
  end
  
  // Register interface request
  assign reg_req = '{
          valid: reg_valid_i,
          write: reg_write_i,
          wstrb: reg_wstrb_i,
          addr: reg_addr_i,
          wdata: reg_wdata_i
      };

  // Counter instance
  conv1d_obi u_conv1d_obi (
      .clk_i     (clk_i     ),
      .rst_ni    (rst_ni    ),
      .obi_req_i (obi_req ),
      .obi_rsp_o (obi_rsp ),
      .reg_req_i (reg_req ),
      .reg_rsp_o (reg_rsp ),
      .done_int_o  (done_int_o  )
  );

  // OBI response
  assign obi_gnt_o    = obi_rsp.gnt;
  assign obi_rvalid_o = obi_rsp.rvalid;
  assign obi_rdata_o  = obi_rsp.r.rdata;

  // Register interface response
  assign reg_error_o = reg_rsp.error;
  assign reg_ready_o = reg_rsp.ready;
  assign reg_rdata_o = reg_rsp.rdata;
endmodule
