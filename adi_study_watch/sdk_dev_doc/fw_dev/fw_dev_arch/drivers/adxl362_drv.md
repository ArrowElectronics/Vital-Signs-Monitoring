# ADXL362 Driver


## Hardware Interface

The nRF52840 MCU uses the SPIM (Serial Communication Master) peripheral to communicate with the ADXL362 accelerometer.

The ADXL362 SPI operates in the following mode:
* CPHA = CPOL = 0

SCLK frequency
* min 2.4Khz
* max 8MHz

## Communication Protocol
### Register Read and Write
The command structure for the read register and write register commands is as follows:
1. CS down
2. command byte
    * 0x0A for read command
    * 0x0B for write command
3. address byte
4. data byte 0
    * additional data byte 1
    * additional data byte n
5. CS up

### FIFO Read
Reading from the FIFO buffer is a command structure that does not have an address:
1. CS down
2. command byte
    * 0x0D for FIFO read
3. data bytes
4. CS up




## Data Rate

12.5
25
50
100
200
400


# Driver Structure



# Dependencies


Accelerometer Range
* +/-2G


nrf_drv_spi_t

# easy dma





# Macros
