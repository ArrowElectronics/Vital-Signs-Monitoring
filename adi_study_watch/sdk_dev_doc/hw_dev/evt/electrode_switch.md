# Electrode Switch

There are three independent electrode switches in the Study Watch. These switches controls connections from the electrode to inputs of the specific analog frontends.  Respectly, these three switches routes electrodes to:

* the AD8233 ECG signal chain
* the AD5940 impedance signal chain, and
* the ADPD4000 ECG and impedance signal chain.

## Component
The component for electrode switches is

[DG2502](https://www.vishay.com/docs/62962/dg2501.pdf)

This is a 4 channel single pole single throw (SPST) switch, with default connection __open__ when each switch input is pulled __low__.

## Electrode Switch for AD8233 Signal Chain

On the engineering board, the electrode switch configurations for the ad8233 signal chain (or ECG1) is capture in the following schematic:

![electrode switch for ad8233](./ele_swtch_ad8233.png)

### IOs
* Supply
    * AVDD_2V85
    * Ground
* Digital Input
    * 8233_SW_EN_1V8
* Analog Input
    * EL_T1, top electrode 1
    * EL_T2, top electrode 2
    * EL_B1, bottom electrode 1
    * EL_B2, bottom electrode 2
* Analog Output
    * 8233_IN+
    * 8233_IN-
    * 8233_RLD

### Resistor Options
On the engineering board as shown in the schematic above, resistors R75, R76, R77, and R78 provide configuration options at the board level.

R75 | R76 | Option
----|---- | ------
0   | nc  | default, ad8233 RLD output connects to EL_T1
nc  | 0   | EL_T1 and EL_T2 both connects to ad8233 IN+
0   | 0   | not allowed
nc  | nc  | EL_T1 disconnected from any ad8233 input or output

R77 | R78 | Option
----|---- | ------
0   | nc  | default, ad8233 RLD output connects to EL_B2
nc  | 0   | EL_B1 and EL_B2 both connects to ad8233 IN-
0   | 0   | not allowed
nc  | nc  | EL_B2 disconnected from any ad8233 input or output


## Electrode Switch for AD5940 Signal Chain

On the engineering board, the electrode switch configurations for the ad5940 signal chain (or EDA/Bioz) is capture in the following schematic:

![electrode switch for ad5940](./ele_swtch_ad5940.png)

### IOs
* Supply
    * AVDD_2V85
    * Ground
* Digital Input
    * 5940_SW_EN_1V8
* Analog Input
    * EL_T1, top electrode 1
    * EL_T2, top electrode 2
    * EL_B1, bottom electrode 1
    * EL_B2, bottom electrode 2
* Analog Output, each capacitively coupled to
    * AD5940_AIN1
    * AD5940_AIN3
    * AD5940_AIN2
    * AD5940_CE

### Capacitor Option
On the engineering board, there is an capacitor option via capacitor C105 as shown above to share the connection from EL_B2 to AD5940_AIN2 and AD5940_SE0. This options is intended to be removed for final watch build once we have verified that the AIN2 input of the AD5940 IC can be routed internally to SE0 using the engineering board.

## Electrode Switch for ADPD4000 ECG and Impedance Signal Chain

On the engineering board, the electrode switch configurations for the adpd4000 signal chain (or ECG2) is capture in the following schematic:

![electrode switch for ad8233](./ele_swtch_adpd4000.png)

### IOs
* Supply
    * AVDD_2V85
    * Ground
* Digital Input
    * 4K_SW_EN_1V8
* Analog Input
    * EL_T1, top electrode 1
    * EL_T2, top electrode 2
    * EL_B1, bottom electrode 1
    * EL_B2, bottom electrode 2
* Analog Output
    * 4K_PRE_FILTER+
    * 4K_PRE_FILTER-
    * 4K_VC2

### Resistor Options
On the engineering board as shown in the schematic above, resistors R79, R80, R81, and R82 provide configuration options at the board level.

__Note that R71 and R82 when populated, are intended to be capacitively coupled, e.g. use a 0.47uF capacitor instead of 0 Ohm resister.__

R75 | R76 | Option
----|---- | ------
cap | nc  | default, adpd4000 vc2 output connects to EL_T1
nc  | 0   | EL_T1 and EL_T2 both connects to adpd4000 4K_PRE_FILTER+
0   | 0   | do not use
nc  | nc  | EL_T1 disconnected from any adpd4000 input or output

R77 | R78 | Option
----|---- | ------
cap | nc  | default, apad4000 vc2 output connects to EL_B2
nc  | 0   | EL_B1 and EL_B2 both connects to adpd4000 4K_PRE_FILTER-
0   | 0   | do not use
nc  | nc  | EL_B2 disconnected from any adpd4000 input or output
