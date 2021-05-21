# Button Hardware

![Button Schematic](./bttn_sch.png)

## Theory of Op

The schematic of the button circuit is shown above. Mechanical tact switches are used to interface with users. The output of this circuit is nominally pulled up to supply voltage when there is no activity, and pulled to ground via the mechanical contact of the tact switch during button press.  The resistor limits current from supply to ground during button press and in collaboration with the capacitor as shown in the schematic above creates a filtered output for debouncing.

The button implementation is based on the nRF52840 gpiote function. In addition, timer or timerapp are used to debounce and/or to distinguish between short and long press activities.

Developer could use the firmware examples included in the nRF5 SDK as the driver or module base.  The firmware documentation is linked below:
* [firmware button module](../../../fw_dev/fw_dev_arch/mod//input_mod/bttn_mod.md)
* [firmware button driver](../../../fw_dev/fw_dev_arch/drv/bttn_drv.md)

## IOs

* Supply:
    * [DVDD_1V8](), 1.8V
    * Ground
* Input:
    * Tact switch, active pull to ground
* Output
    * navigation, to P0.25 on nRF52840, active low
    * action, to P1.00 on nRF52840, active low
