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
// File: conv1d.sv
// Author(s):
//   Luigi Giuffrida
//   Michele Caon
// Date: 08/11/2024
// Description: conv1d accelerator top module

module conv1d (
  input logic clk_i,
  input logic rst_ni,

  // Interface towards internal memory
  input  croc_pkg::sbr_obi_req_t mem_req_i,
  output croc_pkg::sbr_obi_rsp_t mem_rsp_o,

  // Interface from configuration registers
  input  logic start_i,
  input  logic accumulate_i,  // 1=read/write, 0=skip read
  output logic running_o,
  output logic done_o
);
  // PARAMETERS
  localparam int unsigned NumWords = 32'd128;  // DO NOT CHANGE THIS!
  localparam int unsigned AddrWidth = (NumWords > 32'd1) ? unsigned'($clog2(NumWords)) : 32'd1;

  // Tiling parameters (from project.tex)
  localparam int unsigned TILE_CIN  = 4;
  localparam int unsigned TILE_COUT = 2;
  localparam int unsigned TILE_T    = 12;
  localparam int unsigned KERNEL_K  = 5;

  // Memory layout
  localparam int unsigned INPUT_BASE  = 0;
  localparam int unsigned WEIGHT_BASE = 64;
  localparam int unsigned OUTPUT_BASE = 104;

  // Derived
  localparam int unsigned INPUT_WINDOW = TILE_T + KERNEL_K - 1;  // 16
  localparam int unsigned NUM_OUTPUTS  = TILE_COUT * TILE_T;      // 24

  // INTERNAL SIGNALS
  // ----------------
  // Memory multiplexer signals
  conv1d_sram_pkg::sram_req_t int_mem_req, ext_mem_req, mem_req;
  conv1d_sram_pkg::sram_rsp_t mem_rsp;
  logic                       ext_mem_gnt;

  // FSM states
  typedef enum logic [3:0] {
    IDLE,
    READ_PREV_OUT,      // Read previous output for accumulation
    WAIT_PREV_OUT,      // Wait for previous output data
    FETCH_INPUTS,       // Fetch 4 inputs from SRAM (4 cycles)
    FETCH_WEIGHTS,      // Fetch 4 weights from SRAM (4 cycles)
    WAIT_CAPTURE,       // Wait for last weight to be captured
    COMPUTE,            // Compute 4 MACs + accumulate
    WRITE_OUTPUT,       // Write result to SRAM
    NEXT_KERNEL,        // Move to next kernel position
    NEXT_OUTPUT,        // Move to next output
    DONE
  } state_t;
  state_t state_q, state_d;

  // Counters
  logic [4:0] out_idx_q, out_idx_d;       // Output index (0-23)
  logic [2:0] kernel_idx_q, kernel_idx_d; // Kernel position (0-4)
  logic [1:0] fetch_cnt_q, fetch_cnt_d;   // Fetch counter (0-3)
  logic       mem_valid_q;                // Memory read valid (1-cycle delay)
  logic [1:0] capture_idx_q;              // Which pipeline register to capture into
  logic       capture_is_input_q;         // True if capturing input, false if weight

  // Pipeline registers - ONLY 9 DATA REGISTERS (36 bytes) - FAQ COMPLIANT
  logic signed [31:0] in_pipe_q  [0:3];   // 4 input pipeline registers
  logic signed [31:0] wt_pipe_q  [0:3];   // 4 weight pipeline registers
  logic signed [31:0] accum_q, accum_d;   // 1 accumulator register

  // Datapath signals
  logic signed [31:0] mul_result [0:3];   // 4 multiplier outputs
  logic signed [31:0] add_tree_out;       // Adder tree output

  // Address calculation signals
  logic        out_ch;                    // Output channel (0 or 1)
  logic [3:0]  t_pos;                     // Time position within output channel
  logic [6:0]  input_addr, weight_addr;   // Computed addresses

  // ---------------------
  // INTERNAL ARCHITECTURE
  // ---------------------
  
  // Addresses
  // Output channel and time position from output index
  assign out_ch = (out_idx_q >= 5'd12);
  assign t_pos  = out_ch ? (out_idx_q[3:0] - 4'd12) : out_idx_q[3:0];
  
  always_comb begin
    input_addr = INPUT_BASE + (fetch_cnt_q * INPUT_WINDOW) + t_pos + kernel_idx_q;
    weight_addr = WEIGHT_BASE + (out_ch * (TILE_CIN * KERNEL_K)) + (fetch_cnt_q * KERNEL_K) + kernel_idx_q;
  end

  // FSM SEQUENTIAL LOGIC
  
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      state_q           <= IDLE;
      out_idx_q         <= '0;
      kernel_idx_q      <= '0;
      fetch_cnt_q       <= '0;
      accum_q           <= '0;
      mem_valid_q       <= 1'b0;
      capture_idx_q     <= '0;
      capture_is_input_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        in_pipe_q[i] <= '0;
        wt_pipe_q[i] <= '0;
      end
    end else begin
      state_q      <= state_d;
      out_idx_q    <= out_idx_d;
      kernel_idx_q <= kernel_idx_d;
      fetch_cnt_q  <= fetch_cnt_d;
      accum_q      <= accum_d;
      mem_valid_q  <= int_mem_req.req && !int_mem_req.we;

      // Track which register to capture into (delayed by 1 cycle to match memory latency)
      capture_idx_q      <= fetch_cnt_q;
      capture_is_input_q <= (state_q == FETCH_INPUTS);

      // Capture input data into pipeline register (using delayed index)
      if (mem_valid_q && capture_is_input_q) begin
        in_pipe_q[capture_idx_q] <= mem_rsp.rdata;
      end

      // Capture weight data into pipeline register (using delayed index)
      if (mem_valid_q && !capture_is_input_q) begin
        wt_pipe_q[capture_idx_q] <= mem_rsp.rdata;
      end
    end
  end

  // FSM COMBINATIONAL LOGIC
  
  always_comb begin
    // Next to present state assignment
    state_d      = state_q;
    out_idx_d    = out_idx_q;
    kernel_idx_d = kernel_idx_q;
    fetch_cnt_d  = fetch_cnt_q;
    accum_d      = accum_q;

    // Init to no memory request
    int_mem_req = '{req: 1'b0, we: 1'b0, be: 4'hF, addr: '0, wdata: '0};

    case (state_q)
      IDLE: begin
        out_idx_d    = '0;
        kernel_idx_d = '0;
        fetch_cnt_d  = '0;
        accum_d      = '0;
        if (start_i) begin
          if (accumulate_i) begin
            state_d = READ_PREV_OUT;
          end else begin
            state_d = FETCH_INPUTS;
          end
        end
      end

      READ_PREV_OUT: begin
        // Issue read request for previous output value
        int_mem_req.req  = 1'b1;
        int_mem_req.addr = OUTPUT_BASE + out_idx_q;
        state_d = WAIT_PREV_OUT;
      end

      WAIT_PREV_OUT: begin
        // Wait for read data to arrive
        if (mem_valid_q) begin
          accum_d = $signed(mem_rsp.rdata);  // Load previous value into accumulator
          state_d = FETCH_INPUTS;
          fetch_cnt_d = '0;
        end
      end

      FETCH_INPUTS: begin
        // Fetch 4 inputs sequentially
        int_mem_req.req  = 1'b1;
        int_mem_req.addr = input_addr;

        if (fetch_cnt_q == 2'd3) begin
          // Last input fetched => move to weights
          state_d = FETCH_WEIGHTS;
          fetch_cnt_d = '0;
        end else begin
          fetch_cnt_d = fetch_cnt_q + 1;
        end
      end

      FETCH_WEIGHTS: begin
        // Fetch 4 weights sequentially
        int_mem_req.req  = 1'b1;
        int_mem_req.addr = weight_addr;

        if (fetch_cnt_q == 2'd3) begin
          // Last weight fetched do to wait for capture
          state_d = WAIT_CAPTURE;
          fetch_cnt_d = '0;
        end else begin
          fetch_cnt_d = fetch_cnt_q + 1;
        end
      end

      WAIT_CAPTURE: begin
        // mem_valid_q = 1 when the last read data arrives
        if (mem_valid_q) begin
          state_d = COMPUTE;
        end
      end

      COMPUTE: begin
        // Compute: 4 MACs using adder tree and accumulate result
        accum_d = accum_q + add_tree_out; // 4° adder
        state_d = NEXT_KERNEL;
      end

      NEXT_KERNEL: begin
        if (kernel_idx_q == KERNEL_K - 1) begin
          // All kernel positions done go to write output
          state_d = WRITE_OUTPUT;
        end else begin
          // Go to next kernel position
          kernel_idx_d = kernel_idx_q + 1;
          fetch_cnt_d = '0;
          state_d = FETCH_INPUTS;
        end
      end

      WRITE_OUTPUT: begin
        // Write result to SRAM
        int_mem_req.req   = 1'b1;
        int_mem_req.we    = 1'b1;
        int_mem_req.addr  = OUTPUT_BASE + out_idx_q;
        int_mem_req.wdata = accum_q;
        state_d = NEXT_OUTPUT;
      end

      NEXT_OUTPUT: begin
        if (out_idx_q == NUM_OUTPUTS - 1) begin
          // All outputs done
          state_d = DONE;
        end else begin
          // Move to next output
          out_idx_d = out_idx_q + 1;
          kernel_idx_d = '0;
          fetch_cnt_d = '0;
          if (accumulate_i) begin
            accum_d = '0;  // Will be loaded from SRAM
            state_d = READ_PREV_OUT;
          end else begin
            accum_d = '0;
            state_d = FETCH_INPUTS;
          end
        end
      end

      DONE: begin
        if (!start_i) state_d = IDLE;
      end

      default: state_d = IDLE;
    endcase
  end

  // DATAPATH: 4 mul + adder tree
  
  // 4 Multipliers (one per input channel)
  genvar ic;
  generate
    for (ic = 0; ic < TILE_CIN; ic++) begin : gen_mul
      assign mul_result[ic] = in_pipe_q[ic] * wt_pipe_q[ic];
    end
  endgenerate

  // Adder tree: 3 adders to sum 4 products (adders 1-3)
  // 4° adder used for accumulation
  assign add_tree_out = (mul_result[0] + mul_result[1]) + (mul_result[2] + mul_result[3]); // 1, 2, 3 adder

  // Status outputs
  assign running_o = (state_q != IDLE) && (state_q != DONE);
  assign done_o    = (state_q == DONE);

  // Memory
  // CPU can access only when accelerator is in IDLE or DONE state
  assign ext_mem_gnt = (state_q == IDLE) || (state_q == DONE);

  // Memory request multiplexer
  // If accelerator is in IDLE or DONE state, CPU can communicate with SRAM
  always_comb begin : mem_req_mux
    mem_req = ext_mem_gnt ? ext_mem_req : int_mem_req;
  end

  // OBI to SRAM bridge
  obi_sram_shim #(
    .ObiCfg    (croc_pkg::SbrObiCfg),
    .obi_req_t (croc_pkg::sbr_obi_req_t),
    .obi_rsp_t (croc_pkg::sbr_obi_rsp_t)
  ) u_obi_bridge (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .obi_req_i (mem_req_i),
    .obi_rsp_o (mem_rsp_o),
    .req_o     (ext_mem_req.req),
    .we_o      (ext_mem_req.we),
    .addr_o    (ext_mem_req.addr),
    .wdata_o   (ext_mem_req.wdata),
    .be_o      (ext_mem_req.be),
    .gnt_i     (ext_mem_gnt),
    .rdata_i   (mem_rsp.rdata)
  );

  // Internal SRAM instance (scratchpad memory)
  // OBI uses byte addresses, SRAM uses word addresses

  /*SIMULATION SRAM — uncomment for Verilator HW vs SW testing
  conv1d_sram_wrapper #(
    .NUM_WORDS (NumWords),
    .DATA_WIDTH(32'd32)
  ) u_internal_mem (
    .clk_i  (clk_i),
    .rst_ni (rst_ni),
    .req_i  (mem_req.req),
    .we_i   (mem_req.we),
    .addr_i (ext_mem_gnt ? ext_mem_req.addr[AddrWidth-1+2:2] : int_mem_req.addr[AddrWidth-1:0]),
    .wdata_i(mem_req.wdata),
    .be_i   (mem_req.be),
    .rdata_o(mem_rsp.rdata)
  );
  */

  // SYNTHESIS SRAM — uses IHP SG13G2 SRAM macros via tc_sram_impl
  tc_sram_impl #(
    .NumWords  (NumWords),
    .DataWidth (32),
    .NumPorts  (1),
    .Latency   (1)
  ) u_internal_mem (
    .clk_i   (clk_i),
    .rst_ni  (rst_ni),
    .impl_i  ('0),
    .impl_o  (),
    .req_i   (mem_req.req),
    .we_i    (mem_req.we),
    .addr_i  (ext_mem_gnt ? ext_mem_req.addr[AddrWidth-1+2:2] : int_mem_req.addr[AddrWidth-1:0]),
    .wdata_i (mem_req.wdata),
    .be_i    (mem_req.be),
    .rdata_o (mem_rsp.rdata)
  );
  
endmodule
