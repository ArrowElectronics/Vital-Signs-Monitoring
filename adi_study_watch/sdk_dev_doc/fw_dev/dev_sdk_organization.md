# Organization
## SDK Base
The study watch firmware SDK is based Nordic NRF5 SDK.  Current version is
[nRF5_SDK_15.3.0_59ac345](https://www.nordicsemi.com/?sc_itemid=%7B21C26716-5F2C-4E2D-9514-C9B87B711114%7D)

## Directory Structure
The directory organization is defined here:
```
nrf5_sdk_root/
    default_nrf_sdk_directories/
        :
    default_nrf_sdk_files/
        :
    adi_boot_loader/
        :
    adi_study_watch/
        configurations/
            watch_board_evt.c
            watch_board_dvt.c
            watch_board_pvt.c
        drivers/
            adxl362_drv
            adxl5940_drv
            adpd4000_drv
            adp5360_drv
            ad7156_drv
            lpm010m297b_drv
            buttons_drv
            electrode_switch_drv
            *usb_drv
        modules/
            dash_board_mod
            lpm010m297b_mod
            ad5940_mod
            ad5940_ecg_mod
            ad5940_eda_mod
            ad5940_bioz_mod
            adpd4000_mod
            adpd4000_ecg_mod_experimental
            adpd4000_ppg_mod
            adpd4000_sx02_mod_experimental
            adxl362_mod
            adp5360_mod
        libraries/
            m2m2_lib
            post_office_lib
            lfs_lib
            circular_buffer_lib
            memory_pool_lib
            memory_management_lib
        sdk_hex/
            headers
                header1.h
                header2.h
            hex_file
        algorithms/
            algorithm1/
            algorithm2/
            ppg_loop1_algo
            ppg_motion_reject_algo/
        applications/
            application1/
                ses/
                    appliation1.emProject
            application2/
            display/ * to be refined
                display_app.c * call this "dash.c?"
                display_app.h
                page/
                home_main/
                    home_main_page.c
                    hr_main/ * this is actually an app
                        hr_main_page.c
                        bpm_sub/
                        ...
                    setting/ * this is another app
                    ...
                font/
                draw/
        tests/
            unit_test1/
```

## Tool Chain
### Development IDE
[Segger Embedded Studio](https://www.nordicsemi.com/DocLib/Content/User_Guides/gsg_ses/latest/UG/gsg/intro)

### Development HW
0. [NRF52 DK 100056, for nRF52840](https://www.nordicsemi.com/?sc_itemid=%7B6BECB3CF-00F7-4B2D-8553-F1AD6AC458EF%7D)
1. Study Watch EVT board (link)
2. Study Watch DVT board (link)
3. Study Watch PVT board (link)


.a library (obj combine into one)
.c
.h
