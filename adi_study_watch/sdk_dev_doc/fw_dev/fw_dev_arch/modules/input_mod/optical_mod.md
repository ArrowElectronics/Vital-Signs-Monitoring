# Optical Module

 This module will provide optical sensor data from ADPD4002 optical device. The module configures the watch with preference given to the user-defined configuration available in NOR Flash of the watch over the configuration within the firmware.
Config: ADPD DCFG

As is the case for the gen3 software the ADPD will have the DCFG in the same format.
User specified CFGs will be stored in the Device Configuration Block area of the flash.
Loading a DCFG must happen in the following order:
1.	Use the global LoadCFG(CFG_index) –
a.	if a CFG is returned, then this CFG is used. Skip to step 3
b.	If a CFG is not found, then skip to step 2.
2.	Load the CFG applicable in local memory
3.	Configuration load end
Configs:  ADPD LCFG
The ADPD will have an LCFG as parameter values.
ADPD will have default LCFG. This default LCFG will be hardcoded in the source code – but must be easy to update – in other words, store them in appropriate include files.
User specified CFGs will be stored in the Device Config Block area of the flash.
LCFG should have an additional volatile storage in memory.
Loading a LCFG must happen in the following order:
1.	Use the global LoadCFG(CFG_index) –
a.	if an LCFG is returned then this CFG is used. Skip to step 3.
b.	If an LCFG is not found, then skip to step 2.
2.	Load the CFG applicable in local memory
3.	Apply the LCFG in volatile memory

LCFG parameter	Description	Value
ClockCal	Calibrate the clock for better accuracy 	0 : Clock calibration to be skipped
1 : Clock calibration to be performed
SlotConfig	Select slot, channel and dark/signal for the ADPD device	4:0 – Slot selection
5:  0 – Channel 1, 1 – Channel 2
8:6 – Signal Data size
11:9 – Dark Data Size

ADPD M2M2 API

M2M2 Commands

•	Like all M2M2 commands these commands are synchronous with a command and response phase
•	Each response should indicate the result of the command if successful, else an error if unsuccessful

LoadCFG
•	Load ADPD DCFG.
•	The device should have an ADPD DCFG hard coded in source.
•	LoadCFG()
•	Try to load the CFG from NOR Flash using the global LoadCFG() call.
If it’s unsuccessful, then the hard coded CFG in firmware is used.
ClockCal
•	Perform calibration of the ADPD device
Start
•	Start the ADPD based on the DCFG selection
•	Start should take no parameters
Stop
•	Stops the ADPD sensor module
Subscribe
•	Another app requests to start receiving data events from the ADPD Module.
•	The ID of the data event(s) must be specified.
Unsubscribe
•	Another app requests to stop receiving data events from the ADPD Module.
•	The ID of the data event(s) must be specified.
RegWrite
•	Write a series of register address-data pairs contained in the payload to the ADPD device.
RegRead
•	Read a series of register address values contained in the payload from the ADPD device.
Usage

Example usage:
1.	Subscribe
2.	LoadCFG
3.	ClockCalibration
4.	RegWrite/RegRead
5.	Start
6.	…
7.	Stop
8.	Unsubscribe
