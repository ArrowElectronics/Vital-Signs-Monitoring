# Button Module

The button module is an input module. There are 2 buttons allocated for the ADI Study Watch design:

* Navigation button
* Action button

## Hardware
The button hardware for each design implementation is described in here:
* [button hardware]() on the engineering board
* [button hardware]() on the dvt1 board
* [button hardware]() on the dvt2 board


## Interfacing Drivers
The button module is supported by the [button driver](../drv/bttn_drv.md).

## Button Logic

### Short Press
The button module shall identify a button press having a duration from 10ms to 1000ms as "short press"

### Long press
The button module shall identify a button press having a duration greater than 1000ms as "long press"

The button module supports the following functional combinations:

Button 1 (Navigation) | Button 2 (Action) | Example Assignment
------------| ------------|-------------
short press | no activity | navigation (through menu)
no activity | short press | select, confirm
short press | short press | _not to use_
long press  | no activity | return to previous
no activity | long press  | _available_
long press  | long press  | reboot


## API


## Usage
