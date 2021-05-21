Steps to load & work with IAR project:

1. Erase Watch's internal NOR Flash. Connect Watch via JLink cable to PC and execute the following command:
/c/Program\ Files\ \(x86\)/Nordic\ Semiconductor/nrf5x/bin/nrfjprog.exe --eraseall

2. Load softdevice binary onto the Watch. Keep Watch connected to PC via Jlink cable and execute the following command:
/c/Program\ Files\ \(x86\)/Nordic\ Semiconductor/nrf5x/bin/nrfjprog.exe --family nRF52 --program /C/projects/nRF5_SDK_15.2.0_9412b96/components/softdevice/s140/hex/s140_nrf52_6.1.0_softdevice.hex

3. Open watchv4_nrf52840.eww, then Rebuild heart_rate_coolidge, pedometer and watchv4_nrf52840 Debug projects. Use "Download & Debug" to load the watch.hex onto Watch & run the debug session.

4. Open C:\Program Files (x86)\SEGGER\JLink_V640\JLinkRTTViewer.exe and click Ok to connect via  USB port to JLink. Click on "All Terminals" tab to view the RTT logs.

5. For any further changes in IAR project and running debug session, steps 1,2,3,4 is required. Otherwise a HardFault exception keeps coming, as could be seen in the Call Stack.

6. Steps 1,2,3,4 could be followed accordingly for loading Release configuration as well.
