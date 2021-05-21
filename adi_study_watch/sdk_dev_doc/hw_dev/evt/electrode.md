# Electrode

## Convention on the Engineering Board
There are 4 stainless steel electrodes on the Study Watch. These electrodes are intend to be used in direct contact with human body for ECG, EDA and BioZ sensing applications.

On the Study Watch, two electrodes are placed on the top of the watch along the rim, and two electrodes are placed on the bottom plate of the watch. The electrode convention for the watch is shown below:

![watch electrode convention](../eletrode_convention_watch.png)

On the engineering board, electrodes are made with bare coper contact pads with dimension and placement that mimics the watch electrode configuration.  The electrode convention on the engineering board is depicted below:

![engineering board electrode convention](./electrode_convention_eng.png)


## Electrode Protection

Safety measures are incorporated to protect both the user and the watch circuit.

![Electrode protection](./electrode_protect.png)


## Electrode Switch
Electrode connections are wired to the inputs of three independent analog [switches](./electrode_switch.md), each through a series resistor as shown in figure above.

The value of the resistor for each electrode is currently 150Kohm.
