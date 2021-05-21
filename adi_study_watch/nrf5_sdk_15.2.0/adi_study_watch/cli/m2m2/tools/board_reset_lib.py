#!/bin/python
import sys, time
try:
    import RPi.GPIO as GPIO
    use_serial = False
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    RPI_RESET_GPIO_PIN = 17
    RPI_BMODE_GPIO_PIN = 27
except ImportError:
    import serial
    use_serial = True

CMD_RESET = 'a'
CMD_BOOTMODE = 'b'

class board_reset_lib():
    def __init__(self, com_port=None):
        if use_serial and com_port != None:
            self.ser = serial.Serial(com_port, 9600)
        else:
            GPIO.setup(RPI_RESET_GPIO_PIN, GPIO.OUT, initial=GPIO.HIGH)
            GPIO.setup(RPI_BMODE_GPIO_PIN, GPIO.OUT, initial=GPIO.HIGH)

    def boardReset(self):
        if use_serial:
            self.ser.write(CMD_RESET)
            self.ser.read(1)
        else:
            GPIO.output(RPI_RESET_GPIO_PIN, GPIO.LOW)
            time.sleep(0.1)
            GPIO.output(RPI_RESET_GPIO_PIN, GPIO.HIGH)
            time.sleep(0.1)

    def boardBootmode(self):
        if use_serial:
            self.ser.write(CMD_BOOTMODE)
            self.ser.read(1)
        else:
            GPIO.output(RPI_RESET_GPIO_PIN, GPIO.LOW)
            time.sleep(0.1)
            GPIO.output(RPI_BMODE_GPIO_PIN, GPIO.LOW)
            time.sleep(0.1)
            GPIO.output(RPI_RESET_GPIO_PIN, GPIO.HIGH)
            time.sleep(0.1)
            GPIO.output(RPI_BMODE_GPIO_PIN, GPIO.HIGH)
            time.sleep(0.1)


if __name__ == '__main__':
    if len(sys.argv) > 2:
        brd = board_reset_lib(sys.argv[1])
        if sys.argv[2] == "reset":
            brd.boardReset()
        if sys.argv[2] == "bootmode":
            brd.boardBootmode()
    else:
        print "Incorrect arguments! python_handler SERIAL_PORT COMMAND"
        sys.exit(1)
