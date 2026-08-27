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
// File: conv1d_sram_wrapper.sv
// Author(s):
//   Michele Caon
// Date: 24/11/2024
// Description: Wrapper for the SRAM modules

module conv1d_sram_wrapper #(
  parameter int unsigned NUM_WORDS = 32'd128,  // Number of Words in data array
  parameter int unsigned DATA_WIDTH = 32'd32,  // Data signal width
  // DEPENDENT PARAMETERS, DO NOT OVERWRITE!
  localparam int unsigned AddrWidth = (NUM_WORDS > 32'd1) ? unsigned'($clog2(NUM_WORDS)) : 32'd1
) (
  input  logic                 clk_i,
  input  logic                 rst_ni,
  // input ports
  input  logic                 req_i,
  input  logic                 we_i,
  input  logic [AddrWidth-1:0] addr_i,
  input  logic [         31:0] wdata_i,
  input  logic [          3:0] be_i,
  // output ports
  output logic [         31:0] rdata_o
);
  // ---------------------
  // SRAM SIMULATION MODEL
  // ---------------------
  tc_sram #(
    .NumWords    (NUM_WORDS),
    .DataWidth   (DATA_WIDTH),
    .ByteWidth   (32'd8),
    .NumPorts    (32'd1),
    .Latency     (32'd1)
  ) u_tc_sram (
  	.clk_i   (clk_i),
    .rst_ni  (rst_ni),
    .req_i   (req_i),
    .we_i    (we_i),
    .addr_i  (addr_i),
    .wdata_i (wdata_i),
    .be_i    (be_i),
    .rdata_o (rdata_o)
  );
endmodule
