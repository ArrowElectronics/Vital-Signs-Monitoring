Fw 1.0.1 16 June 2021

## Contents of WatchV4 Source Framework

SES version used is "SEGGER Embedded Studio for ARM Release 4.12  Build 2018112601.37855 Windows x64"

The SES project file location is  nrf5_sdk_15.2.0\adi_study_watch\app\nRF52840_app\ses\watchv4_nrf52840.emProject, where $PERSEUS_PROJ_LOCATION is the local file path where it has been downloaded.
HR library project is located at  nrf5_sdk_15.2.0\adi_study_watch\algo\ppg_loop1_algo\ses
To build this library you need to add C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 4.12\gcc\arm-none-eabi\bin to the system environment $PATH variable

nRF5_SDK_15.2.0 is used in this project

FreeRTOS V10.0.0 source code is being used from  nrf5_SDK_15.2.0\external\freertos

The command line interface may be started using  nrf5_SDK_15.2.0\adi_study_watch\cli\m2m2\tools\CLI.py

This project has FreeRTOS Framework with ADXL, ADPD4000, PPG, USBD CDC ACM BLE UART, Post Office and System Task as FreeRTOS tasks.

### FAQ with Firmware build from SES
1.	SES project build fails:
	The SES .emProject file is created and tested only from SES IDE "SEGGER Embedded Studio for ARM Release 4.12  Build 2018112601.37855 Windows x64"
	Please don't try from any other SES IDE version.
	
	 nrf5_sdk_15.2.0\adi_study_watch\app\nRF52840_app\ses\watchv4_nrf52840.emProject maybe opened and build to prepare the Watch firmware hex file.
	Other library project dependencies have been set already and gets built from watchv4_nrf52840.emProject
	
2.	HR library build fails:
	Make sure that C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 4.12\gcc\arm-none-eabi\bin is added to the system environment $PATH variable
	If SES IDE is already kept opened and after that a $PATH change is done, then please restart the SES IDE before a project rebuild.
	
3.	Facing SES license issue due to having multiple SES IDE versions installed:
	Uninstall completely all SES versions from the Windows (5.50 and 4.12)
	Reinstall only 4.12 SES windows 64 bit, request for license.
	Then the build issues would be resolved.
	
4.	SES build issue due to license expiry:
	The SES build issue, following license expiry can be fixed after renewing the license.
	The SES license can be renewed by using the following link:
	https://license.segger.com/Nordic.cgi 
	by filling in basic details(name, mail, company etc.) and PC MAC ID. Upon submitting the details, the renewed license will be sent to the mail id given.


### Python Tool Setup

1.	Install the latest version of Python 2.7 (the 32 bit is tested, 64 bit version might have some dependency problems so it is highly recommended to use 32 bit version): https://www.python.org/downloads/
2.	Add Python to the system PATH: http://stackoverflow.com/questions/6318156/adding-python-path-on-windows-7

     a.	Be sure to also add the "C:\Python27\Scripts" to the system PATH as well.
3.	Install the Microsoft Visual C++ Compiler for Python 2.7: aka.ms/vcpython27  << You must finish this before you can complete the following steps.
4.	Use pip (Python package manager, installed along with Python) to install the CLI dependencies ( It is possible that previous older Python, pre 2.7.13  installs  do not have the "pip" included) Step 1)  is recommended)

a)	Basic dependencies (Using the CLI only)

     i.	Install the pywin package using the executable installer: https://sourceforge.net/projects/pywin32/files/pywin32/Build%20220/pywin32-220.win32-py2.7.exe/download
	 
    ii.	pip install  pyserial colorama numpy
	
b)	Plotting dependencies (If you want to plot data) 

     i.	Install PyQT4 using the executable installer: https://sourceforge.net/projects/pyqt/files/PyQt4/PyQt-4.11.4/PyQt4-4.11.4-gpl-Py2.7-Qt4.8.7-x32.exe/download
	 
    ii.	pip install pyqtgraph PySide
	


### CLI Usage

__NOTE__ down the COM port number from Device Manager in PC after connecting study watch with USB cable. This is to be used with the CLI.
Here are some CLI commands to get started with getting raw data from ADPD and ADXL sensors. 

From Windows cmd prompt or Windows power shell, start the python code
>python CLI.py

Run the tool and peruse the help text:
>cd  nrf5_SDK_15.2.0\adi_study_watch\cli\m2m2\tools\

>python CLI.py

This is m2m2 UART shell. Type help or ? to list commands

#help

#?connect

#connect_usb COM12

#msg_verbose 3

#quickstart adpd4000

#plot radpd6

#quickstart adxl

#plot radxl

#quickstop adpd4000

#quickstop adxl

### SES Project options

1.	SES project of Debug configuration RTT logging is enabled by default using ENAB_RTT_LOGGING preprocessor macro.
nRF logs come on SES Debug Terminal.

### Robot Framework

1.	Added basic robot framework tests for PM, ADXL, ADPD4000 and PPG path nrf5_sdk_15.2.0\adi_study_watch\tst\robot_framework\
After installing “pip install robotframework”
Read up more on robot framework from http://robotframework.org/

Need to run from path nrf5_sdk_15.2.0\adi_study_watch\tst\robot_framework,
>robot -v DUT_COMPORT:COM28 --timestampoutputs --outputdir ./log m2m2_tests.txt

Before making commits, it will be good if you run robot tests once for a basic sanity test.
You can add more tests onto m2m2_tests.txt once it has been added and tested from CLI.py


### Patches to ADXL, ADPD400x Drivers, nRF5 SDK15.2.0

ADXL Driver Changes:
--------------------
Added following lines to Adxl362.h:
```c
#ifdef NRF52840_SPI_FIX
#define SPI_ADXL_DUMMY_BYTES    2
#else
#define SPI_ADXL_DUMMY_BYTES    0
#endif //NRF52840_SPI_FIX
```

Made changes in Adxl362.c to change SPI receive data buffer size to be with refernce to SPI_ADXL_DUMMY_BYTES.

For eg: in uint8_t GetDevIdAdAdxl362(void) API,
```c
uint8_t        nRxData[SPI_ADXL_DUMMY_BYTES+3];
ADXL362_SPI_Receive(nTxData, nRxData, nLengthWrite, nLengthRead);
nValue = nRxData[SPI_ADXL_DUMMY_BYTES+0];
```

ADPD400x Driver Changes:
------------------------
```c

diff --git a/ADPD40xx/src/Adpd400xDrv.c b/ADPD40xx/src/Adpd400xDrv.c
index d69e82a..b83976c 100644
--- a/ADPD40xx/src/Adpd400xDrv.c
+++ b/ADPD40xx/src/Adpd400xDrv.c
@@ -96,7 +96,7 @@ typedef struct _adpd400xDrv_slot_t {
 static adpd400xDrv_slot_t gsSlot[SLOT_NUM];
 static uint8_t gsTotalSlotSize;         //!< Total active slot size in bytes
 static uint16_t gsHighestSelectedSlot;  //!< Highes selected slot
-static Adpd400xComMode_t nAdpd400xCommMode = ADPD400x_UNKNOWN_BUS;
+static Adpd400xComMode_t nAdpd400xCommMode = ADPD400x_SPI_BUS;
 
 static uint8_t gnAdpdFifoWaterMark = 1;
 static uint32_t gnAccessCnt[5];
@@ -246,7 +246,11 @@ int16_t Adpd400xDrvRegWrite(uint16_t nAddr, uint16_t nRegValue) {
 * @return int16_t A 16-bit integer: 0 - success; < 0 - failure
 */
 int16_t Adpd400xDrvRegRead(uint16_t nAddr, uint16_t *pnData) {
+#ifdef NRF52840_SPI_FIX
+  uint8_t anRxData[4];
+#else
   uint8_t anRxData[2];
+#endif //NRF52840_SPI_FIX
   uint8_t txData[2];
   uint16_t i = 0;
 
@@ -297,7 +301,11 @@ int16_t Adpd400xDrvRegRead(uint16_t nAddr, uint16_t *pnData) {
   } else {
     return ADPD400xDrv_ERROR;
   }
+#ifdef NRF52840_SPI_FIX
+  *pnData = (anRxData[2] << 8) + anRxData[3];
+#else
   *pnData = (anRxData[0] << 8) + anRxData[1];
+#endif //NRF52840_SPI_FIX
   return ADPD400xDrv_SUCCESS;
 }
 
@@ -656,6 +664,7 @@ int16_t adi_adpddrv_ReadFifoData(uint8_t *pnData, uint16_t nDataSetSize) {
   uint8_t nAddr;
   uint8_t txData[2];
   uint8_t i = 0;
+  uint16_t nTmpAddr;
   
   Adpd400xDrvRegRead(ADPD400x_REG_INT_STATUS_FIFO, &gnFifoLevel);
   gnFifoLevel = gnFifoLevel & 0x7FF;
@@ -670,7 +679,7 @@ int16_t adi_adpddrv_ReadFifoData(uint8_t *pnData, uint16_t nDataSetSize) {
     switch(nAdpd400xCommMode){
     case ADPD400x_SPI_BUS:
       i = 0;
-      uint16_t nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation
+      nTmpAddr = (nAddr << 1 ) & ADPD400x_SPI_READ; // To set the last bit low for read operation
       txData[i++] = (uint8_t)(nTmpAddr >> 8);
       txData[i++] = (uint8_t)(nTmpAddr);
       
diff --git a/ADPD40xx/src/Adpd400xDrv.h b/ADPD40xx/src/Adpd400xDrv.h
index 90ba23f..4fbcfa0 100644
--- a/ADPD40xx/src/Adpd400xDrv.h
+++ b/ADPD40xx/src/Adpd400xDrv.h
@@ -187,4 +187,5 @@ int16_t adi_adpddrv_SetLedCurrent(uint16_t nLedCurrent,
 int16_t Adpd400xDrvGetLedCurrent(uint16_t *pLedCurrent, 
                                  ADPD400xDrv_LedId_t nLedId,
                                  ADPD400xDrv_SlotNum_t nSlotNum);
+void adi_adpddrv_ISR();
 #endif
```
 
nRF5 SDK 15.2.0 Changes:
-----------------------
1. Change to nrf_drv_spi.h file:

```c
$ diff $PATH/nRF5_SDK_15.2.0_9412b96/nRF5_SDK_15.2.0_9412b96/integration/nrfx/legacy/nrf_drv_spi.h $PATH/study_watch_adi/System/Middleware/ThirdParty/nRF5_SDK_15.2.0/integration/nrfx/legacy/nrf_drv_spi.h
365c365
<                                 uint8_t         tx_buffer_length,
---
>                                 uint16_t         tx_buffer_length,
367c367
<                                 uint8_t         rx_buffer_length);
---
>                                 uint16_t         rx_buffer_length);
493c493
<                                 uint8_t         tx_buffer_length,
---
>                                 uint16_t         tx_buffer_length,
495c495
<                                 uint8_t         rx_buffer_length)
---
>                                 uint16_t         rx_buffer_length)
```

2. Change to nrf_drv_uart.h file:

```c
$ diff $PATH/nRF5_SDK_15.2.0_9412b96/nRF5_SDK_15.2.0_9412b96/integration/nrfx/legacy/nrf_drv_uart.h $PATH/study_watch_adi/System/Middleware/ThirdParty/nRF5_SDK_15.2.0/integration/nrfx/legacy/nrf_drv_uart.h
332c332
<                            uint8_t                length);
---
>                            uint32_t                length);
515c515
<                            uint8_t                length)
---
>                            uint32_t                length)

```

3. Change to nrf_drv_rtc.h file:

```c
diff --git a/nrf5_sdk_15.2.0/integration/nrfx/legacy/nrf_drv_rtc.h b/nrf5_sdk_15.2.0/integration/nrfx/legacy/nrf_drv_rtc.h
index 7224ece2..4eef181a 100644
+/** @brief Macro for forwarding the new implementation. */
+#define nrf_drv_rtc_int_is_enabled      nrf_rtc_int_is_enabled
+/** @brief Macro for forwarding the new implementation. */
+#define nrf_drv_rtc_event_pending       nrf_rtc_event_pending
```

4. Change to app_timer_freertos.c

```c
diff --git a/nrf5_sdk_15.2.0/components/libraries/timer/app_timer_freertos.c b/nrf5_sdk_15.2.0/components/libraries/timer/app_timer_freertos.c
index 45645f99..a17140fd 100644
--- a/nrf5_sdk_15.2.0/components/libraries/timer/app_timer_freertos.c
+++ b/nrf5_sdk_15.2.0/components/libraries/timer/app_timer_freertos.c
@@ -120,7 +120,6 @@ uint32_t app_timer_create(app_timer_id_t const *      p_timer_id,
                           app_timer_mode_t            mode,
                           app_timer_timeout_handler_t timeout_handler)
 {
-    app_timer_info_t * pinfo = (app_timer_info_t*)(*p_timer_id);
     uint32_t      err_code = NRF_SUCCESS;
     unsigned long timer_mode;

@@ -128,6 +127,8 @@ uint32_t app_timer_create(app_timer_id_t const *      p_timer_id,
     {
         return NRF_ERROR_INVALID_PARAM;
     }
+
+    app_timer_info_t * pinfo = (app_timer_info_t*)(*p_timer_id);
     if (pinfo->active)
     {
         return NRF_ERROR_INVALID_STATE;
```

5. Change to ble_conn_state.c

```c
diff --git a/nrf5_sdk_15.2.0/components/ble/common/ble_conn_state.c b/nrf5_sdk_15.2.0/components/ble/common/ble_conn_state.c
index c438ea2f..600ec274 100644
--- a/nrf5_sdk_15.2.0/components/ble/common/ble_conn_state.c
+++ b/nrf5_sdk_15.2.0/components/ble/common/ble_conn_state.c
@@ -96,7 +96,7 @@ void bcs_internal_state_reset(void)

 ble_conn_state_conn_handle_list_t conn_handle_list_get(nrf_atflags_t flags)
 {
-    ble_conn_state_conn_handle_list_t conn_handle_list;
+    ble_conn_state_conn_handle_list_t conn_handle_list = {};
     conn_handle_list.len = 0;

     if (flags != 0)
```

### Watch Bring Up Guide

How to bring up the Watch using Jlink debugger and loading the code from SES IDE is explained in nrf5_sdk_15.2.0/adi_study_watch/doc/ADI_VSM_Watch_IV_Bringup_Guide.pdf

### BLE Dongle

How to use BLE Dongle is explained in nrf5_sdk_15.2.0/adi_study_watch/tst/ble_dongle/ble_dongle_usage.docx

### Watch Firmware Update

How to update the Watch firmware through DFU is explained in nrf5_sdk_15.2.0/adi_study_watch/bootloader/Watch Firmware Upgrade.docx

### Watch Help Documents

Documents providing help of Watch Firmware and Watch usage is captured in nrf5_sdk_15.2.0/adi_study_watch/doc

