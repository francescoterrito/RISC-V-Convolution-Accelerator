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
// File: conv1d.cpp
// Author(s):
//   Luigi Giuffrida
// Date: 08/11/2024
// Description: TB for the OBI CONV1D accelerator

#include <iostream>
#include <getopt.h>
#include <random>
#include <time.h>

// Verilator libraries
#include <verilated.h>
#include <verilated_fst_c.h>

// DUT header
#include "Vconv1d_tb_wrapper.h"
#include "conv1d_control_reg.h"

// Testbench components
#include "tb_macros.hh"
#include "tb_components.hh"

// Test data
#include "data.h"

// Defines
// -------
#define FST_FILENAME "logs/waves.fst"
#define END_OF_RESET_TIME 5
#define MAX_SIM_CYCLES 50e6
#define MAX_SIM_TIME (MAX_SIM_CYCLES * 2)
#define WATCHDOG_TIMEOUT 5000 // cycles to wait for a program step to complete
#define END_OF_TEST_TIMEOUT 10 // cycles between done assertion and simulation end

// Scratchpad layout
#define INPUT_BASE  0
#define WEIGHT_BASE 64
#define OUTPUT_BASE 104

// Tiling parameters
#define TILE_CIN    4   // Input channels per tile
#define TILE_COUT   2   // Output channels per tile
#define TILE_T      12  // Output temporal positions per tile
#define KERNEL_K    5   // Kernel size (fixed)
#define INPUT_WINDOW (TILE_T + KERNEL_K - 1)  // 16 input positions per channel
// INPUT_LEN, K_NUM, INPUT_CH, KERNEL_LEN taken from data.h

// Tile memory sizes
#define TILE_INPUT_SIZE  (TILE_CIN * INPUT_WINDOW)  // 64 words
#define TILE_WEIGHT_SIZE (TILE_COUT * TILE_CIN * KERNEL_K)  // 40 words
#define TILE_OUTPUT_SIZE (TILE_COUT * TILE_T)  // 24 words

// Generate clock and reset
void clkGen(Vconv1d_tb_wrapper *dut);
void rstDut(Vconv1d_tb_wrapper *dut, vluint64_t sim_time);

// Generate OBI transactions
ObiReqTx *genObiWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t be);
ObiReqTx *genObiReadReqTx(const vluint32_t addr_offs);
RegReqTx *genRegWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t wstrb);
RegReqTx *genRegReadReqTx(const vluint32_t addr_offs);

// Run a number of cycles
void runCycles(unsigned int ncycles, Vconv1d_tb_wrapper *dut, uint8_t gen_waves, VerilatedFstC *trace);

// Global variables
vluint64_t sim_cycles = 0;
TbLogger logger;    // testbench logger

int main(int argc, char *argv[])
{
    // Define command-line options
    const option longopts[] = {
        {"log_level", required_argument, NULL, 'l'},
        {"gen_waves", required_argument, NULL, 'w'},
        {"seed", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    // Process command-line options
    // ----------------------------
    int opt; // current option
    int prg_seed = time(NULL);
    bool gen_waves = true;
    while ((opt = getopt_long(argc, argv, "l:w:", longopts, NULL)) >= 0)
    {
        switch (opt)
        {
        case 'l': // set the log level
            logger.setLogLvl(optarg);
            TB_CONFIG("Log level set to %s", optarg);
            break;
        case 'w': // generate waves
            if (!strcmp(optarg, "true")) {
                gen_waves = 1;
                TB_CONFIG("Waves enabled");
            }
            else {
                gen_waves = 0;
                TB_CONFIG("Waves disabled");
            }
            break;
        case 's': // set the seed
            prg_seed = atoi(optarg);
            TB_CONFIG("Seed set to %d", prg_seed);
            break;
        default:
            TB_ERR("ERROR: unrecognised option %c.\n", opt);
            exit(EXIT_FAILURE);
        }
    }

    // Create Verilator simulation context
    VerilatedContext *cntx = new VerilatedContext;

    // Pass simulation context to the logger
    logger.setSimContext(cntx);

    if (gen_waves)
    {
        Verilated::mkdir("logs");
        cntx->traceEverOn(true);
    }

    // Instantiate DUT
    Vconv1d_tb_wrapper *dut = new Vconv1d_tb_wrapper(cntx);

    // Set the file to store the waveforms in
    VerilatedFstC *trace = NULL;
    if (gen_waves)
    {
        trace = new VerilatedFstC;
        dut->trace(trace, 10);
        trace->open(FST_FILENAME);
    }

    // TB components
    Drv *drv = new Drv(dut);
    Scb *scb = new Scb();
    ReqMonitor *reqMon = new ReqMonitor(dut, scb);
    RspMonitor *rspMon = new RspMonitor(dut, scb);

    // Initialize PRG
    srand(prg_seed);

    // Simulation program
    // ------------------
    // Simulation variables
    ObiReqTx *obi_req = NULL;
    vluint32_t obi_rdata = 0;
    uint32_t obi_data = 0;
    uint32_t obi_addr = 0;
    unsigned int data_idx = 0;
    bool obi_accepted = 0;

    RegReqTx *reg_req = NULL;
    vluint32_t reg_rdata = 0;
    bool reg_accepted = 0;

    bool irq_received = 0;

    bool end_of_test = false;
    unsigned int exit_timer = 0;
    unsigned int watchdog = 0;
    unsigned int prev_step_cnt = 0;
    unsigned int step_cnt = 0;

    int errors = 0;
    
    const uint32_t output_len = INPUT_LEN - KERNEL_LEN + 1;  // 128-5+1=124

    // Calculate number of tiles needed to cover all out_channel/out_values/in_channels (c_out_tiles/t_tiles/c_in_tiles)
    const uint32_t c_out_tiles = (K_NUM + TILE_COUT - 1) / TILE_COUT;     // (8+2-1)/2=4
    const uint32_t t_tiles = (output_len + TILE_T - 1) / TILE_T;       // (124+12-1)/12=11 (10 full + 1 partial)
    const uint32_t c_in_tiles = (INPUT_CH + TILE_CIN - 1) / TILE_CIN;     // (16+4-1)/4=4

    // Tile loop counters
    uint32_t oc_tile = 0;   // Output channel tile (0 to c_out_tiles-1)
    uint32_t t_tile = 0;    // Temporal tile (0 to t_tiles-1)
    uint32_t ic_tile = 0;   // Input channel tile (0 to c_in_tiles-1)

    // Current tile parameters
    uint32_t oc_base = 0;   // Base output channel for current tile
    uint32_t t_base = 0;    // Base temporal position for current tile
    uint32_t ic_base = 0;   // Base input channel for current tile
    uint32_t actual_t = 0;  // Actual temporal outputs for current tile (may be < TILE_T for last tile)

    // Output buffer to store computed results for verification
    int32_t *computed_outputs = new int32_t[K_NUM * output_len](); // Array of 124*8 element on 32 bit each

    TB_LOG(LOG_LOW, "Starting simulation...");
    while (!cntx->gotFinish() && cntx->time() < MAX_SIM_TIME)
    {
        // Generate clock and reset
        rstDut(dut, cntx->time());
        clkGen(dut);

        // Evaluate simulation step
        dut->eval();

        if (dut->clk_i == 1 && cntx->time() > END_OF_RESET_TIME)
        {
            switch (step_cnt)
            {

            // Load input data for current tile to scratchpad

            case 0:
                // Initialize tile parameters at each loop
                if (data_idx == 0) {
                    oc_base = oc_tile * TILE_COUT; // (0 to 4)*2
                    t_base = t_tile * TILE_T;     // (0 to 11)*12
                    ic_base = ic_tile * TILE_CIN; // (0 to 16)*4
                    if ((t_base + TILE_T) <= output_len) {
                        actual_t = TILE_T;                  // Full tile
                    } else {
                        actual_t = output_len - t_base;     // Partial tile
                    }
                }
                if (!obi_accepted) {
                    // Calculate which input channel and position within the tile
                    uint32_t tile_ic = data_idx / INPUT_WINDOW;  // 0 to 3
                    uint32_t tile_pos = data_idx % INPUT_WINDOW; // 0 to 15
                    uint32_t global_ic = ic_base + tile_ic;
                    uint32_t global_pos = t_base + tile_pos;

                    // Handle edge cases: pad with zero if out of bounds
                    int8_t input_val = 0;
                    if (global_ic < INPUT_CH && global_pos < INPUT_LEN) {
                        // A stored
                        input_val = (int8_t)A[global_ic * INPUT_LEN + global_pos]; // [ch * INPUT_LEN + pos]
                    }
                    // A stored for tile
                    obi_data = (uint32_t)(int32_t)input_val;  // Sign extention
                    obi_addr = (INPUT_BASE + data_idx) * 4;
                    obi_req = genObiWriteReqTx(obi_addr, obi_data, 0xf);
                    break;
                }
                obi_accepted = false;
                data_idx++;
                // Load until all data are loaded for a tile
                if (data_idx >= TILE_INPUT_SIZE) {
                    step_cnt++;
                    data_idx = 0;
                }
                break;

            // Load weight data for current tile to scratchpad

            case 1:
                if (!obi_accepted) {
                    // Scratchpad index
                    uint32_t tile_oc = data_idx / (TILE_CIN * KERNEL_K);  // 0 to 1
                    uint32_t rem = data_idx % (TILE_CIN * KERNEL_K);
                    uint32_t tile_ic = rem / KERNEL_K;  // 0 to 3
                    uint32_t k = rem % KERNEL_K;        // 0 to 4

                    uint32_t global_oc = oc_base + tile_oc;
                    uint32_t global_ic = ic_base + tile_ic;

                    // Handle edge cases: pad with zero if out of bounds
                    int8_t weight_val = 0;
                    if (global_oc < K_NUM && global_ic < INPUT_CH) {
                        // F stored
                        weight_val = (int8_t)F[(global_oc * INPUT_CH + global_ic) * KERNEL_LEN + k]; //[(oc * INPUT_CH + ic) * KERNEL_LEN + k]
                    }
                    // F stored for tile
                    obi_data = (uint32_t)(int32_t)weight_val;  // Sign extention
                    obi_addr = (WEIGHT_BASE + data_idx) * 4;
                    obi_req = genObiWriteReqTx(obi_addr, obi_data, 0xf);
                    break;
                }
                obi_accepted = false;
                data_idx++;
                // Load until all weights/kernel are loaded for a tile
                if (data_idx >= TILE_WEIGHT_SIZE) {
                    step_cnt++;
                    data_idx = 0;
                }
                break;

            // Wait cycles
            case 2 ... 4:
                step_cnt++;
                break;


            // Start accelerator
            
            // ACCUMULATE=0 for first input channel tile (ic_tile==0)
            // ACCUMULATE=1 for next input channel tiles (ic_tile>0)
            case 5:
                // Setting start bit to 1 and accumulate
                if (!reg_accepted) {
                    uint32_t accumulate = (ic_tile > 0) ? 1 : 0;
                    uint32_t control_val = 0x1 | (accumulate << 1);  // START=1, ACCUMULATE bit
                    reg_req = genRegWriteReqTx(CONV1D_CONTROL_CONTROL_REG_OFFSET, control_val, 0xf);
                    break;
                }
                reg_accepted = false;
                step_cnt++;
                break;

            // Wait for accelerator computation

            // Polling on DONE signal
            case 6:
                if (irq_received) {
                    step_cnt++;
                    data_idx = 0;
                    // Clear START bit
                    reg_req = genRegWriteReqTx(CONV1D_CONTROL_CONTROL_REG_OFFSET, 0x0, 0xf);
                }
                break;

            // STEP 7: Read outputs from scratchpad

            // Only read outputs after all input channel tiles are processed
            case 7:
                // If not last input channel for a tile, skip
                if (ic_tile < c_in_tiles - 1) {
                    step_cnt++;  // Go to tile advancement
                    break;
                }

                // Last ic_tile: read outputs from scratchpad and store in buffer
                if (!obi_accepted) {
                    obi_addr = (OUTPUT_BASE + data_idx) * 4;
                    obi_req = genObiReadReqTx(obi_addr);
                    break;
                }
                if (!rspMon->isDataReadyObi()) {
                    break;
                }
                obi_accepted = false;
                {
                    // Scratchpad index
                    uint32_t tile_oc = data_idx / TILE_T;  // 0 to 1
                    uint32_t tile_t = data_idx % TILE_T;   // 0 to 11

                    int32_t read_val = (int32_t)rspMon->getObiData();

                    // Only store valid outputs (handle partial tile)
                    if (tile_t < actual_t) {
                        uint32_t global_oc = oc_base + tile_oc;
                        uint32_t global_t = t_base + tile_t;
                        // Store in output buffer for checking with golden ones
                        computed_outputs[global_oc * output_len + global_t] = read_val;
                    }
                }
                data_idx++;
                // Read until all outputs values are readed for a tile
                if (data_idx >= TILE_OUTPUT_SIZE) {
                    step_cnt++;
                    data_idx = 0;
                }
                break;

            // Checking for end of the program

            case 8:
                // Check if all input channels are computed
                ic_tile++;
                if (ic_tile >= c_in_tiles) {
                    ic_tile = 0;
                    // check if all output values are computed
                    t_tile++;
                    if (t_tile >= t_tiles) {
                        t_tile = 0;
                        oc_tile++;
                        // check if all output are computed
                        if (oc_tile >= c_out_tiles) {
                            step_cnt++;
                            data_idx = 0;
                            break;
                        }
                    }
                }
                // No end of the program, start for a new tile
                step_cnt = 0;
                data_idx = 0;
                break;

            // Checking against golden model
            
            case 9:
                {
                    // Golden value and computed_outputs index
                    uint32_t total_outputs = K_NUM * output_len;
                    uint32_t idx = data_idx;
                    uint32_t oc = idx / output_len;
                    uint32_t t = idx % output_len;

                    int32_t computed = computed_outputs[idx];
                    int32_t expected = (int32_t)R[idx];

                    if (computed != expected) {
                        // Error
                        TB_ERR("Output mismatch at R[%d][%d] (idx=%d): got %d, expected %d",
                               oc, t, idx, computed, expected);
                        errors++;
                    } else {
                        // Good result
                        TB_LOG(LOG_HIGH, "R[%d][%d] = %d (correct)", oc, t, computed);
                    }
                }
                data_idx++;
                if (data_idx >= K_NUM * output_len) {
                    // Summary
                    TB_LOG(LOG_LOW, "Verification complete. Total errors: %d / %d outputs",
                           errors, K_NUM * output_len);
                    step_cnt++;
                }
                break;

            default:
                // Set simulation exit flag
                end_of_test = true;
                break;
            }

            // Drive DUT inputs
            drv->drive(obi_req, reg_req);
            delete obi_req;
            delete reg_req;
            obi_req = NULL;
            reg_req = NULL;

            // Update input signals
            dut->eval();

            // Monitor DUT signals
            reqMon->monitor();
            rspMon->monitor();

            irq_received = rspMon->irq();
            obi_accepted = reqMon->acceptedObi();
            reg_accepted = reqMon->acceptedReg();

            // Get register data (to be used in the test program above)
            if (reg_accepted) {
                reg_rdata = rspMon->getRegData();
            }
            if (rspMon->isDataReadyObi()) obi_rdata = rspMon->getObiData();

            // Check for exit conditions
            if (prev_step_cnt != step_cnt) watchdog = 0;
            else watchdog++;
            if (watchdog > WATCHDOG_TIMEOUT) {
                TB_WARN("Watchdog timeout reached: terminating simulation.");
                scb->notifyError();
                break;
            }
            prev_step_cnt = step_cnt;
            if (end_of_test)
            {
                if (exit_timer++ == END_OF_TEST_TIMEOUT) {
                    TB_LOG(LOG_MEDIUM, "End of simulation reached: terminating.");
                    break;
                }
            }
        }

        // Dump waveforms and advance simulation time
        if (gen_waves) trace->dump(cntx->time());
        if (dut->clk_i == 1) sim_cycles++;
        cntx->timeInc(1);
    }

    // Simulation complete
    dut->final();

    // Print simulation summary
    if (scb->getErrNum() > 0)
    {
        TB_ERR("CHECKS FAILED > errors: %u/%u", scb->getErrNum(), scb->getTxNum());
        if (gen_waves) trace->close();
        exit(EXIT_SUCCESS);
    }
    else if (!scb->isDone())
    {
        TB_ERR("CHECKS PENDING > errors: %u/%u", scb->getErrNum(), scb->getTxNum());
        if (gen_waves) trace->close();
        exit(EXIT_SUCCESS);
    }
    TB_SUCCESS(LOG_LOW, "CHECKS PASSED > errors: %u (checked %u transactions)", scb->getErrNum(), scb->getTxNum());

    // Clean up and exit
    if (gen_waves) trace->close();
    delete dut;
    delete cntx;
    delete obi_req;
    delete reg_req;
    delete[] computed_outputs;

    return 0;
}

void clkGen(Vconv1d_tb_wrapper *dut)
{
    dut->clk_i ^= 1;
}

void rstDut(Vconv1d_tb_wrapper *dut, vluint64_t sim_time)
{
    dut->rst_ni = 1;
    if (sim_time > 1 && sim_time < END_OF_RESET_TIME)
    {
        dut->rst_ni = 0;
    }
}

void runCycles(unsigned int ncycles, Vconv1d_tb_wrapper *dut, uint8_t gen_waves, VerilatedFstC *trace)
{
    VerilatedContext *cntx = dut->contextp();
    for (unsigned int i = 0; i < (2 * ncycles); i++)
    {
        // Generate clock
        clkGen(dut);

        // Evaluate the DUT
        dut->eval();

        // Save waveforms
        if (gen_waves)
            trace->dump(cntx->time());
        if (dut->clk_i == 1)
            sim_cycles++;
        cntx->timeInc(1);
    }
}

// Issue write OBI transaction
ObiReqTx *genObiWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t be)
{
    ObiReqTx *req = new ObiReqTx;

    // OBI write request
    req->obi_req.req = 1;
    req->obi_req.we = 1;
    req->obi_req.be = be;
    req->obi_req.addr = addr_offs;
    req->obi_req.wdata = wdata;

    return req;
}

// Issue read OBI transaction
ObiReqTx *genObiReadReqTx(const vluint32_t addr_offs)
{
    ObiReqTx *req = new ObiReqTx;

    // OBI read request
    req->obi_req.req = 1;
    req->obi_req.we = 0;
    req->obi_req.be = 0xf;
    req->obi_req.addr = addr_offs;
    req->obi_req.wdata = 0;

    return req;
}

// Issue write register interface transaction
RegReqTx *genRegWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t wstrb)
{
    RegReqTx *req = new RegReqTx;

    // OBI write request
    req->reg_req.valid = 1;
    req->reg_req.write = 1;
    req->reg_req.wstrb = wstrb;
    req->reg_req.addr = addr_offs;
    req->reg_req.wdata = wdata;

    return req;
}

// Issue read register interface transaction
RegReqTx *genRegReadReqTx(const vluint32_t addr_offs)
{
    RegReqTx *req = new RegReqTx;

    // OBI read request
    req->reg_req.valid = 1;
    req->reg_req.write = 0;
    req->reg_req.wstrb = 0xf;
    req->reg_req.addr = addr_offs;
    req->reg_req.wdata = 0;

    return req;
}
