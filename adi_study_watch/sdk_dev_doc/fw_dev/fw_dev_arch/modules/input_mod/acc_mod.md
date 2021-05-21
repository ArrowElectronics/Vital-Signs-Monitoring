# Accelerometer Module

Theory of Op

### Hardware
This module uses the adxl362 IC

 This module will provide motion data from ADXL362 accelerometer device. The module configures the watch with preference given to the user-defined configuration available in NOR Flash of the watch over the configuration within the firmware.
Config: ADXL DCFG

As is the case for the gen3 software the ADXL will have the DCFGs in the same format.
User specified CFGs will be stored in the Device Config Block area of the non-volatile memory.
Loading a DCFG must happen in the following order:
1.	Use the global LoadCFG(CFG_index) –
a.	if a CFG is returned then this CFG is used. Skip to step 3.
b.	If a CFG is not found, then skip to step 2.
2.	Load the CFG applicable in local memory
3.	Configuration load end

ADXL M2M2 API

M2M2 Commands

•	Like all M2M2 commands these commands are synchronous with a command and response phase
•	Each response should indicate the result of the command if successful, else an error

LoadCFG
•	Load ADXL DCFG.
•	The device should have a default ADXL DCFG hard coded in source.
•	LoadCFG()
•	Try to load the CFG from NOR Flash using the global LoadCFG() call.
If it’s unsuccessful, then the hard coded CFG in firmware is used.
Start
•	Start the ADXL based on the DCFG selection
•	Start should take no parameters
Stop
•	Stops the ADXL sensor module
Subscribe
•	Another app requests to start receiving data events from the ADXL Module.
•	The ID of the data event(s) must be specified.
Unsubscribe
•	Another app requests to stop receiving data events from the ADXL Module.
•	The ID of the data event(s) must be specified.
RegWrite
•	Write a series of register address-data pairs contained in the payload to the ADXL device.
RegRead
•	Read a series of register address values contained in the payload from the ADXL device.
Usage

Example usage:
1.	Subscribe
2.	LoadCFG
3.	RegWrite/RegRead
4.	Start
5.	…
6.	Stop
7.	Unsubscribe
