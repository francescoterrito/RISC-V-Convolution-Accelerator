#include "tb_components.hh"
#include "tb_macros.hh"

ObiReqTx::ObiReqTx()
{
    this->obi_req = {0};
}

ObiReqTx::~ObiReqTx()
{
}

void ObiReqTx::reset()
{
    this->obi_req = {0};
}

ObiRspTx::ObiRspTx()
{
}

ObiRspTx::~ObiRspTx()
{
}

RegReqTx::RegReqTx()
{
    this->reg_req = {0};
}

RegReqTx::~RegReqTx()
{
}

void RegReqTx::reset()
{
    this->reg_req = {0};
}

RegRspTx::RegRspTx()
{
}

RegRspTx::~RegRspTx()
{
}

Drv::Drv(Vconv1d_tb_wrapper *dut)
{
    this->dut = dut;
}

Drv::~Drv()
{
}

void Drv::drive(ObiReqTx *obi_req, RegReqTx *reg_req)
{
    // OBI interface
    this->dut->obi_req_i = 0;
    if (obi_req != NULL) {
        this->dut->obi_req_i = obi_req->obi_req.req;
        this->dut->obi_we_i = obi_req->obi_req.we;
        this->dut->obi_be_i = obi_req->obi_req.be;
        this->dut->obi_addr_i = obi_req->obi_req.addr;
        this->dut->obi_wdata_i = obi_req->obi_req.wdata;
    }

    // Register interface
    this->dut->reg_valid_i = 0;
    if (reg_req != NULL) {
        this->dut->reg_valid_i = reg_req->reg_req.valid;
        this->dut->reg_write_i = reg_req->reg_req.write;
        this->dut->reg_wstrb_i = reg_req->reg_req.wstrb;
        this->dut->reg_addr_i = reg_req->reg_req.addr;
        this->dut->reg_wdata_i = reg_req->reg_req.wdata;
    }
}

Scb::Scb()
{
    this->tx_num = 0;
    this->err_num = 0;
}

Scb::~Scb()
{
}

void Scb::writeObiReq(ObiReqTx *req)
{
    this->obi_req_q.push_back(req);
}

void Scb::writeObiRsp(ObiRspTx *rsp)
{
    // Push the sampled response
    this->obi_rsp_q.push_back(rsp);
}

void Scb::writeRegReq(RegReqTx *req)
{
    this->reg_req_q.push_back(req);
}

void Scb::writeRegRsp(RegRspTx *rsp)
{
    // Push the sampled response
    this->reg_rsp_q.push_back(rsp);
}

bool Scb::scheduleObiCheck(vluint32_t exp_value)
{
    TB_DEBUG("SCB > Scheduling OBI read data check: 0x%08x", exp_value);

    // Push the new expected result
    this->obi_exp_q.push_back(exp_value);
    return false;
}

bool Scb::scheduleRegCheck(vluint32_t exp_value)
{
    TB_DEBUG("SCB > Scheduling REG read data check: 0x%08x", exp_value);

    // Push the new expected result
    this->reg_exp_q.push_back(exp_value);
    return false;
}

int Scb::checkObiData()
{
    vluint32_t exp_value;
    ObiRspTx *rsp;
    log_lvl_t log_lvl = LOG_MEDIUM;

    TB_DEBUG("SCB | OBI > response queue size: %d | result queue size: %d", this->obi_rsp_q.size(), this->obi_exp_q.size());

    // Check if read data shall be checked
    if (this->obi_exp_q.empty())
    {
        // If no check is scheduled, consume unused responses
        while (this->obi_rsp_q.size() > 0)
        {
            rsp = this->obi_rsp_q.front();
            this->obi_rsp_q.pop_front();
            delete rsp;
        }
        return 0;
    }

    // If the response is not available yet, return and wait for the next cycle
    if (this->obi_rsp_q.empty()) return 0;

    // Else, pop received data and the expected value
    rsp = this->obi_rsp_q.front();
    this->obi_rsp_q.pop_front();
    exp_value = this->obi_exp_q.front();
    this->obi_exp_q.pop_front();

    // Check that the received data has the expected value
    this->tx_num++;
    if (rsp->obi_rsp.rdata != exp_value)
    {
        TB_ERR("SCB | OBI > Received data (0x%08x) does not match the expected value (0x%08x)", rsp->obi_rsp.rdata, exp_value);
        this->err_num++;
        delete rsp;
        return -1;
    }

    // Reduce verbosity if data is zero
    if (exp_value == 0) log_lvl = LOG_HIGH;
    
    TB_SUCCESS(log_lvl, "SCB | OBI > Received data: 0x%08x (expected: 0x%08x)", rsp->obi_rsp.rdata, exp_value);

    // Clean up
    delete rsp;
    return 0;
}

int Scb::checkRegData()
{
    vluint32_t exp_value;
    RegRspTx *rsp;
    log_lvl_t log_lvl = LOG_MEDIUM;

    TB_DEBUG("SCB | REG > response queue size: %d | result queue size: %d", this->reg_rsp_q.size(), this->reg_exp_q.size());

    // Check if read data shall be checked
    if (this->reg_exp_q.empty())
    {
        // If no check is scheduled, consume unused responses
        while (this->reg_rsp_q.size() > 0)
        {
            rsp = this->reg_rsp_q.front();
            this->reg_rsp_q.pop_front();
            delete rsp;
        }
        return 0;
    }

    // If the response is not available yet, return and wait for the next cycle
    if (this->reg_rsp_q.empty()) return 0;

    // Else, pop received data and the expected value
    rsp = this->reg_rsp_q.front();
    this->reg_rsp_q.pop_front();
    exp_value = this->reg_exp_q.front();
    this->reg_exp_q.pop_front();

    // Check that the received data has the expected value
    this->tx_num++;
    if (rsp->reg_rsp.rdata != exp_value)
    {
        TB_ERR("SCB | REG > Received data (0x%08x) does not match the expected value (0x%08x)", rsp->reg_rsp.rdata, exp_value);
        this->err_num++;
        delete rsp;
        return -1;
    }

    // Reduce verbosity if data is zero
    if (exp_value == 0) log_lvl = LOG_HIGH;
    
    TB_SUCCESS(log_lvl, "SCB | REG > Received data: 0x%08x (expected: 0x%08x)", rsp->reg_rsp.rdata, exp_value);

    // Clean up
    delete rsp;
    return 0;
}

int Scb::checkData()
{
    if (this->checkObiData() != 0 || this->checkRegData() != 0) return -1;
    return 0;
}

void Scb::notifyError()
{
    this->err_num++;
}

unsigned int Scb::getTxNum()
{
    return this->tx_num;
}

unsigned int Scb::getErrNum()
{
    return this->err_num;
}

int Scb::isDone()
{
    return this->obi_exp_q.empty() && this->reg_exp_q.empty();
}

void Scb::clearQueues()
{
    while (!this->obi_req_q.empty())
    {
        ObiReqTx *obi_req = this->obi_req_q.front();
        this->obi_req_q.pop_front();
        delete obi_req;
    }
    while (!this->obi_rsp_q.empty())
    {
        ObiRspTx *obi_rsp = this->obi_rsp_q.front();
        this->obi_rsp_q.pop_front();
        delete obi_rsp;
    }
    while (!this->obi_exp_q.empty())
    {
        this->obi_exp_q.pop_front();
    }
    while (!this->reg_req_q.empty())
    {
        RegReqTx *reg_req = this->reg_req_q.front();
        this->reg_req_q.pop_front();
        delete reg_req;
    }
    while (!this->reg_rsp_q.empty())
    {
        RegRspTx *reg_rsp = this->reg_rsp_q.front();
        this->reg_rsp_q.pop_front();
        delete reg_rsp;
    }
    while (!this->reg_exp_q.empty())
    {
        this->reg_exp_q.pop_front();
    }
    this->err_num++;
}

void Scb::popObiReq()
{
    if (this->obi_req_q.empty()) return;
    ObiReqTx *req = this->obi_req_q.front();
    this->obi_req_q.pop_front();
    delete req;
}

void Scb::popRegReq()
{
    if (this->reg_req_q.empty()) return;
    RegReqTx *req = this->reg_req_q.front();
    this->reg_req_q.pop_front();
    delete req;
}

ReqMonitor::ReqMonitor(Vconv1d_tb_wrapper *dut, Scb *scb)
{
    this->dut = dut;
    this->scb = scb;
}

ReqMonitor::~ReqMonitor()
{
}

void ReqMonitor::monitorObi()
{
    // OBI request
    // -----------
    if (dut->obi_req_i && dut->obi_gnt_o)
    {
        // Fetch the data from the DUT interface
        ObiReqTx *obi_req = new ObiReqTx();
        obi_req->obi_req.req = dut->obi_req_i;
        obi_req->obi_req.we = dut->obi_we_i;
        obi_req->obi_req.be = dut->obi_be_i;
        obi_req->obi_req.addr = dut->obi_addr_i;
        obi_req->obi_req.wdata = dut->obi_wdata_i;

        // Print the request content
        TB_LOG(LOG_HIGH, "REQ | OBI > %-5s | req: %u | we: %u | be: 0x%1x | addr: 0x%08x | wdata: 0x%08x", (obi_req->obi_req.we) ? "WRITE" : "READ", obi_req->obi_req.req, obi_req->obi_req.we, obi_req->obi_req.be, obi_req->obi_req.addr, obi_req->obi_req.wdata);
        
        // Send the request to the scoreboard
        delete obi_req;
    }
}

void ReqMonitor::monitorReg()
{
    // REG request
    // -----------
    if (dut->reg_valid_i && dut->reg_ready_o)
    {
        // Fetch the data from the DUT interface
        RegReqTx *reg_req = new RegReqTx();
        reg_req->reg_req.valid = dut->reg_valid_i;
        reg_req->reg_req.write = dut->reg_write_i;
        reg_req->reg_req.wstrb = dut->reg_wstrb_i;
        reg_req->reg_req.addr = dut->reg_addr_i;
        reg_req->reg_req.wdata = dut->reg_wdata_i;

        // Print the request content
        TB_LOG(LOG_HIGH, "REQ | REG > %-5s | valid: %u | write: %u | wstrb: 0x%1x | addr: 0x%08x | wdata: 0x%08x", (reg_req->reg_req.write) ? "WRITE" : "READ", reg_req->reg_req.valid, reg_req->reg_req.write, reg_req->reg_req.wstrb, reg_req->reg_req.addr, reg_req->reg_req.wdata);
        
        // Send the request to the scoreboard
        delete reg_req;
    }
}

void ReqMonitor::monitor()
{
    this->monitorObi();
    this->monitorReg();
}

bool ReqMonitor::acceptedObi()
{
    return dut->obi_req_i & dut->obi_gnt_o;
}

bool ReqMonitor::acceptedReg()
{
    return dut->reg_valid_i & dut->reg_ready_o;
}

RspMonitor::RspMonitor(Vconv1d_tb_wrapper *dut, Scb *scb)
{
    this->dut = dut;
    this->scb = scb;
    for (int i = 0; i < 2; i++)
    {
        this->obi_pending_read[i] = false;
    }
}

RspMonitor::~RspMonitor()
{
}

void RspMonitor::monitorObi()
{
    // OBI requests
    // ------------
    bool new_obi_read = dut->obi_req_i & dut->obi_gnt_o & !dut->obi_we_i;
    
    // Check for correctly delivered response
    if (this->obi_pending_read[0] && !dut->obi_rvalid_o && dut->obi_req_i & dut->obi_gnt_o)
    {
        TB_ERR("RSP | OBI > Response not delivered");
        this->scb->notifyError();
        this->obi_pending_read[1] = this->obi_pending_read[0];
        this->obi_pending_read[0] = new_obi_read;
        // Clear any pending check in the scoreboard
        this->scb->popObiReq();
        return;
    }

    // Ignore the response if there's no pending read request
    if (!this->obi_pending_read[0] || !dut->obi_rvalid_o) 
    {
        this->obi_pending_read[1] = this->obi_pending_read[0];
        this->obi_pending_read[0] = new_obi_read;
        return;
    }
    
    // Fetch the data from the DUT interface
    ObiRspTx *obi_rsp = new ObiRspTx();
    obi_rsp->obi_rsp.rvalid = dut->obi_rvalid_o;
    obi_rsp->obi_rsp.rdata = dut->obi_rdata_o;

    // Print the response content
    TB_LOG(LOG_HIGH, "RSP | OBI > rvalid: %u | rdata: 0x%08x", obi_rsp->obi_rsp.rvalid, obi_rsp->obi_rsp.rdata);

    // Send the response to the scoreboard
    this->scb->writeObiRsp(obi_rsp);

    // Update previous read request flag
    this->obi_pending_read[1] = false; // clear previous request
    this->obi_pending_read[0] = new_obi_read;
}

void RspMonitor::monitorReg()
{
    // REG requests
    // ------------
    // Ignore unaccepted requests
    if (!dut->reg_valid_i || !dut->reg_ready_o) return;

    // Fetch the data from the DUT interface
    RegRspTx *reg_rsp = new RegRspTx();
    reg_rsp->reg_rsp.error = dut->reg_error_o;
    reg_rsp->reg_rsp.rdata = dut->reg_rdata_o;

    // Print the response content
    TB_LOG(LOG_HIGH, "RSP | REG > error: %u | rdata: 0x%08x", reg_rsp->reg_rsp.error, reg_rsp->reg_rsp.rdata);

    // Send the response to the scoreboard
    this->scb->writeRegRsp(reg_rsp);
}

void RspMonitor::monitor()
{
    this->monitorObi();
    this->monitorReg();
}

bool RspMonitor::isDataReadyObi()
{
    return this->dut->obi_rvalid_o;
}

bool RspMonitor::irq()
{
    return this->dut->done_int_o;
}

vluint32_t RspMonitor::getObiData()
{
    return dut->obi_rdata_o;
}

vluint32_t RspMonitor::getRegData()
{
    return dut->reg_rdata_o;
}
