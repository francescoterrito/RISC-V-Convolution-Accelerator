# Copyright 2023 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

# Authors:
# - Tobias Senti <tsenti@ethz.ch>
# - Jannis Schönleber <janniss@iis.ee.ethz.ch>
# - Philippe Sauter   <phsauter@iis.ee.ethz.ch>

source scripts/floorplan_util.tcl

##########################################################################
# Reset (mark everything as unplaced)
##########################################################################

set block [ord::get_db_block]
set insts [odb::dbBlock_getInsts $block]
foreach inst $insts {
  set master [[$inst getMaster] getName]
  # delete IO filler and unplace the rest
  if {[lsearch -exact $iofill $master] != -1 || $master eq $iocorner} {
    odb::dbInst_destroy $inst
    continue
  } else {
    odb::dbInst_setPlacementStatus $inst "none"
  }
}

##########################################################################
# Pads/IOs 
##########################################################################
utl::report "Create Padring"
source src/padring.tcl


##########################################################################
# RAM sizes
##########################################################################
set RamMaster1024x64   [[ord::get_db] findMaster "RM_IHPSG13_1P_1024x64_c2_bm_bist"]
set RamSize1024x64_W   [ord::dbu_to_microns [$RamMaster1024x64 getWidth]]
set RamSize1024x64_H   [ord::dbu_to_microns [$RamMaster1024x64 getHeight]]

# Conv1d accelerator SRAM (uses 64x64 macro for 128x32 logical memory with bit interleaving)
set RamMaster64x64     [[ord::get_db] findMaster "RM_IHPSG13_1P_64x64_c2_bm_bist"]
set RamSize64x64_W     [ord::dbu_to_microns [$RamMaster64x64 getWidth]]
set RamSize64x64_H     [ord::dbu_to_microns [$RamMaster64x64 getHeight]]


##########################################################################
# Chip and Core Area
##########################################################################
# core gets snapped to site-grid -> get real values
set coreArea      [ord::get_core_area]
set core_leftX    [lindex $coreArea 0]
set core_bottomY  [lindex $coreArea 1]
set core_rightX   [lindex $coreArea 2]
set core_topY     [lindex $coreArea 3]


##########################################################################
# Tracks 
##########################################################################
# We need to define the metal tracks 
# (where the wires on each metal should go)
# this function is defined in init_tech.tcl
makeTracks

# the height of a standard cell, useful to align things
set siteHeight        [ord::dbu_to_microns [[dpl::get_row_site] getHeight]]


##########################################################################
# Paths to the instances of macros
##########################################################################
utl::report "Macro Names"
source src/instances.tcl

##########################################################################
# Placing 
##########################################################################
# use these for macro placement
set floorPaddingX      20.0
set floorPaddingY      20.0
set floor_leftX       [expr $core_leftX + $floorPaddingX]
set floor_bottomY     [expr $core_bottomY + $floorPaddingY]
set floor_rightX      [expr $core_rightX - $floorPaddingX]
set floor_topY        [expr $core_topY - $floorPaddingY]
set floor_midpointX   [expr $floor_leftX + ($floor_rightX - $floor_leftX)/2]
set floor_midpointY   [expr $floor_bottomY + ($floor_topY - $floor_bottomY)/2]

utl::report "Place Macros"

set numBanks 16 
set banksPerSide 8

# spacing between macros (moderately reduced)
set spacingY 100
set spacingY_group 130

for {set i 0} {$i < $numBanks} {incr i} {
   if {$i < $banksPerSide} {
      # left column
      set X $floor_leftX
      set row $i
   } else {
      # right column
      set X [expr $floor_rightX - $RamSize1024x64_W]
      set row [expr $i - $banksPerSide]
   }
   set groupOffset [expr ($row / 4) * ($spacingY_group - $spacingY)]
   set Y [expr $floor_topY - ($row + 1) * ($RamSize1024x64_H + $spacingY) - $groupOffset]
   set bankVar [set bank${i}_sram0]
   placeInstance $bankVar $X $Y R0
}

# Place conv1d accelerator SRAM (64x64) at bottom center
set conv1d_X [expr $floor_midpointX - $RamSize64x64_W/2]
set conv1d_Y [expr $floor_bottomY + 50]
placeInstance $CONV1D_SRAM $conv1d_X $conv1d_Y R0

cut_rows -halo_width_x 4 -halo_width_y 8