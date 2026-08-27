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
// File: conv1d_control_reg.sv
// Author(s):
//   Michele Caon
// Date: 07/11/2024
// Description: Conv1d control register wrapper

module conv1d_control_reg (
  input  logic                      clk_i,
  input  logic                      rst_ni,

  // Register interface
  input  conv1d_reg_pkg::reg_req_t  req_i,   // from host system
  output conv1d_reg_pkg::reg_rsp_t  rsp_o,   // to host system

  // Interface to/from the accelerator
  output logic                      start_o,      // Start signal to accelerator
  output logic                      accumulate_o, // Accumulate mode
  input  logic                      running_i,    // Running status from accelerator
  input  logic                      done_i        // Done status from accelerator
);
  // INTERNAL SIGNALS
  // ----------------
  // Registers <--> Accelerator
  conv1d_control_reg_pkg::conv1d_control_reg2hw_t reg2hw;
  conv1d_control_reg_pkg::conv1d_control_hw2reg_t hw2reg;

  // -----------------
  // CONTROL REGISTERS
  // -----------------

  // Registers top module
  conv1d_control_reg_top #(
    .reg_req_t(conv1d_reg_pkg::reg_req_t),
    .reg_rsp_t(conv1d_reg_pkg::reg_rsp_t)
  ) u_conv1d_control_reg_top (
    .clk_i    (clk_i),
    .rst_ni   (rst_ni),
    .reg_req_i(req_i),
    .reg_rsp_o(rsp_o),
    .reg2hw   (reg2hw),
    .hw2reg   (hw2reg),
    .devmode_i(1'b0)
  );

  // Assign configuration outputs from registers
  
  // Control signals
  assign start_o      = reg2hw.control.start.q;
  assign accumulate_o = reg2hw.control.accumulate.q;

  // Status updates to registers
  assign hw2reg.status.running.d  = running_i;
  assign hw2reg.status.running.de = 1'b1;  // Always enable update
  assign hw2reg.status.done.d      = done_i;
  assign hw2reg.status.done.de     = 1'b1;  // Always enable update

endmodule
