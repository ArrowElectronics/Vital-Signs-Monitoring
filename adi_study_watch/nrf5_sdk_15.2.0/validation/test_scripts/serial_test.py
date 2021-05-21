import serial
import time
arduino = serial.Serial('COM7', 115200, timeout=.1)
time.sleep(2)
setFreq = '5f'
data = arduino.write((setFreq + "send").encode())
time.sleep(1)
#while True:
data = arduino.read(arduino.inWaiting())
if data:
	print (data)
print ("Exiting")
exit()
nordic = serial.Serial('COM10', 115200, timeout = .1)
time.sleep(2)
check = 'q'
data1 = nordic.write(check)
time.sleep(1)

data1 = nordic.read(nordic.inwaiting())
if data:
	print data1
print "Exiting"