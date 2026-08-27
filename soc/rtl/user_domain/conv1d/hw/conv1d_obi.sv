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
// File: conv1d_obi.sv
// Author(s):
//   Luigi Giuffrida
// Date: 07/11/2024
// Description: OBI bus wrapper for the conv1d accelerator

module conv1d_obi (
  input logic clk_i,
  input logic rst_ni,

  // OBI interface (scratchpad memory access)
  input  croc_pkg::sbr_obi_req_t obi_req_i,
  output croc_pkg::sbr_obi_rsp_t obi_rsp_o,

  // Register Interface (configuration registers)
  input  conv1d_reg_pkg::reg_req_t  reg_req_i,
  output conv1d_reg_pkg::reg_rsp_t  reg_rsp_o,

  // Conv1d completion interrupt
  output logic done_int_o  // interrupt to host system
);

  // INTERNAL SIGNALS
  // ----------------
  // Signals between registers and accelerator
  logic start_sig;
  logic accumulate_sig;
  logic running_sig;
  logic done_sig;

  // --------------
  // CONV1D MODULE
  // --------------
  // conv1d instance
  conv1d u_conv1d (
    .clk_i    (clk_i),
    .rst_ni   (rst_ni),

    .mem_req_i(obi_req_i),
    .mem_rsp_o(obi_rsp_o),

    // Interface to control registers
    .start_i     (start_sig),
    .accumulate_i(accumulate_sig),
    .running_o   (running_sig),
    .done_o      (done_sig)
  );

  // -----------------
  // CONTROL REGISTERS
  // -----------------

  // Control registers
  conv1d_control_reg u_conv1d_control_reg (
    .clk_i    (clk_i),
    .rst_ni   (rst_ni),
    .req_i    (reg_req_i),
    .rsp_o    (reg_rsp_o),

    // Interface to accelerator
    .start_o     (start_sig),
    .accumulate_o(accumulate_sig),
    .running_i   (running_sig),
    .done_i      (done_sig)
  );

  // Done interrupt: connected to done signal
  assign done_int_o = done_sig;

  // ----------
  // ASSERTIONS
  // ----------
`ifndef SYNTHESIS
  initial begin
    // Assertions can be added here if needed
  end
`endif  /* SYNTHESIS */
endmodule
