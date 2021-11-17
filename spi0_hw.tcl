# TCL File Generated by Component Editor 18.1
# Tue Nov 09 00:36:20 CST 2021
# DO NOT MODIFY


# 
# spi0 "spi0" v1.0
#  2021.11.09.00:36:20
# 
# 

# 
# request TCL package from ACDS 16.1
# 
package require -exact qsys 16.1


# 
# module spi0
# 
set_module_property DESCRIPTION ""
set_module_property NAME spi0
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property GROUP "JLosh Cores"
set_module_property AUTHOR ""
set_module_property DISPLAY_NAME spi0
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false
set_module_property REPORT_HIERARCHY false


# 
# file sets
# 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL spi
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
set_fileset_property QUARTUS_SYNTH ENABLE_FILE_OVERWRITE_MODE false
add_fileset_file spi.v VERILOG PATH spi.v TOP_LEVEL_FILE

add_fileset SIM_VERILOG SIM_VERILOG "" ""
set_fileset_property SIM_VERILOG ENABLE_RELATIVE_INCLUDE_PATHS false
set_fileset_property SIM_VERILOG ENABLE_FILE_OVERWRITE_MODE false
add_fileset_file spi.v VERILOG PATH spi.v


# 
# parameters
# 


# 
# display items
# 


# 
# connection point clk
# 
add_interface clk clock end
set_interface_property clk clockRate 0
set_interface_property clk ENABLED true
set_interface_property clk EXPORT_OF ""
set_interface_property clk PORT_NAME_MAP ""
set_interface_property clk CMSIS_SVD_VARIABLES ""
set_interface_property clk SVD_ADDRESS_GROUP ""

add_interface_port clk clk clk Input 1


# 
# connection point reset
# 
add_interface reset reset end
set_interface_property reset associatedClock clk
set_interface_property reset synchronousEdges DEASSERT
set_interface_property reset ENABLED true
set_interface_property reset EXPORT_OF ""
set_interface_property reset PORT_NAME_MAP ""
set_interface_property reset CMSIS_SVD_VARIABLES ""
set_interface_property reset SVD_ADDRESS_GROUP ""

add_interface_port reset reset reset Input 1


# 
# connection point avalon
# 
add_interface avalon avalon end
set_interface_property avalon addressUnits WORDS
set_interface_property avalon associatedClock clk
set_interface_property avalon associatedReset reset
set_interface_property avalon bitsPerSymbol 8
set_interface_property avalon burstOnBurstBoundariesOnly false
set_interface_property avalon burstcountUnits WORDS
set_interface_property avalon explicitAddressSpan 0
set_interface_property avalon holdTime 0
set_interface_property avalon linewrapBursts false
set_interface_property avalon maximumPendingReadTransactions 0
set_interface_property avalon maximumPendingWriteTransactions 0
set_interface_property avalon readLatency 0
set_interface_property avalon readWaitTime 1
set_interface_property avalon setupTime 0
set_interface_property avalon timingUnits Cycles
set_interface_property avalon writeWaitTime 0
set_interface_property avalon ENABLED true
set_interface_property avalon EXPORT_OF ""
set_interface_property avalon PORT_NAME_MAP ""
set_interface_property avalon CMSIS_SVD_VARIABLES ""
set_interface_property avalon SVD_ADDRESS_GROUP ""

add_interface_port avalon address address Input 2
add_interface_port avalon byteenable byteenable Input 4
add_interface_port avalon chipselect chipselect Input 1
add_interface_port avalon writedata writedata Input 32
add_interface_port avalon readdata readdata Output 32
add_interface_port avalon write write Input 1
add_interface_port avalon read read Input 1
set_interface_assignment avalon embeddedsw.configuration.isFlash 0
set_interface_assignment avalon embeddedsw.configuration.isMemoryDevice 0
set_interface_assignment avalon embeddedsw.configuration.isNonVolatileStorage 0
set_interface_assignment avalon embeddedsw.configuration.isPrintableDevice 0


# 
# connection point phy
# 
add_interface phy conduit end
set_interface_property phy associatedClock clk
set_interface_property phy associatedReset reset
set_interface_property phy ENABLED true
set_interface_property phy EXPORT_OF ""
set_interface_property phy PORT_NAME_MAP ""
set_interface_property phy CMSIS_SVD_VARIABLES ""
set_interface_property phy SVD_ADDRESS_GROUP ""

add_interface_port phy tx new_signal Output 1
add_interface_port phy clk_out new_signal_1 Output 1
add_interface_port phy baud_out new_signal_2 Output 1
add_interface_port phy cs_0 new_signal_3 Output 1
add_interface_port phy cs_1 new_signal_4 Output 1
add_interface_port phy cs_2 new_signal_5 Output 1
add_interface_port phy cs_3 new_signal_6 Output 1
add_interface_port phy rx new_signal_7 Input 1

