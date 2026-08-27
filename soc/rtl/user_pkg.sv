// Copyright 2024 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Authors:
// - Philippe Sauter <phsauter@iis.ee.ethz.ch>

`include "register_interface/typedef.svh"
`include "obi/typedef.svh"

package user_pkg;

  ////////////////////////////////
  // User Manager Address maps //
  ///////////////////////////////
  
  // None


  /////////////////////////////////////
  // User Subordinate Address maps ////
  /////////////////////////////////////

  localparam int unsigned NumUserDomainSubordinates = 4; // Cnt, Conv1d, CSR Cnt, CSR Conv1d

  localparam bit [31:0] UserCntAddrStart   = croc_pkg::UserBaseAddr; // 32'h2000_0000;
  localparam bit [31:0] UserCntAddrSize    = 32'h0001_0000;
  localparam bit [31:0] UserCntAddrEnd     = UserCntAddrStart + UserCntAddrSize;

  localparam bit [31:0] UserConv1dAddrStart   = UserCntAddrEnd; // 32'h2001_0000;
  localparam bit [31:0] UserConv1dAddrSize    = 32'h0001_0000;
  localparam bit [31:0] UserConv1dAddrEnd     = UserConv1dAddrStart + UserConv1dAddrSize;

  localparam bit [31:0] UserCntRegAddrStart   = UserConv1dAddrEnd; // 32'h2002_0000;
  localparam bit [31:0] UserCntRegAddrSize    = 32'h0000_1000;
  localparam bit [31:0] UserCntRegAddrEnd     = UserCntRegAddrStart + UserCntRegAddrSize;

  localparam bit [31:0] UserConv1dRegAddrStart   = UserCntRegAddrEnd; // 32'h2002_1000;
  localparam bit [31:0] UserConv1dRegAddrSize    = 32'h0000_1000;
  localparam bit [31:0] UserConv1dRegAddrEnd     = UserConv1dRegAddrStart + UserConv1dRegAddrSize;



  localparam int unsigned NumDemuxSbrRules  = NumUserDomainSubordinates; // number of address rules in the decoder
  localparam int unsigned NumDemuxSbr       = NumDemuxSbrRules + 1; // additional OBI error, used for signal arrays

  // Enum for bus indices
  typedef enum int {
    UserError = 0,
    UserCnt = 1,
    UserConv1d = 2,
    UserCntReg = 3,
    UserConv1dReg = 4
  } user_demux_outputs_e;

  // Address rules given to address decoder
  localparam croc_pkg::addr_map_rule_t [NumDemuxSbrRules-1:0] user_addr_map = '{
    '{ idx: UserCnt,        start_addr: UserCntAddrStart,       end_addr: UserCntAddrEnd}, // 1: Cnt
    '{ idx: UserConv1d,     start_addr: UserConv1dAddrStart,    end_addr: UserConv1dAddrEnd}, // 2: Conv1d
    '{ idx: UserCntReg,     start_addr: UserCntRegAddrStart,    end_addr: UserCntRegAddrEnd}, // 3: CSR Cnt
    '{ idx: UserConv1dReg,  start_addr: UserConv1dRegAddrStart, end_addr: UserConv1dRegAddrEnd} // 4: CSR Conv1d
  };

endpackage
