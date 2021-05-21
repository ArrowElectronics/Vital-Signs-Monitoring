# Python serial terminal with parts pilfered from
# http://stackoverflow.com/questions/1258566/how-to-get-user-input-during-a-while-loop-without-blocking
# Used to test uart access to the ADI adpd DA14580.
# 3rd party modules needed:
# PySerial: https://pypi.python.org/pypi/pyserial/2.7
# COBS: https://pypi.python.org/pypi/cobs/1.0.0
import threading, Queue, serial
from cobs import cobs
ser = serial.Serial('COM7', 57600, timeout=None)
# this will allow us to communicate between the two threads
# Queue is a FIFO list, the param is the size limit, 0 for infinite
tx_q = Queue.Queue(0)
rx_q = Queue.Queue(0)
prompt_flag = threading.Event()
# the thread reading the command from the user input
def input_getter(tx_q) :
    while 1 :
        tx_q.put(raw_input("#>")) # put the command in the queue so the other thread can read it
        prompt_flag.clear()
        prompt_flag.wait()

def serial_monitor(data):
    while 1:
        rx_byte = ser.read(1)
        rx_q.put(rx_byte)

# your function displaying the words in an infinite loop
def display(tx_q, rx_q):
    print 'A simple UART interface to send/receive data to the DA14580 with the ADI ADPD BLE profile.\n\
Note that you have to send data in blocks of 20 bytes.\n\
Also note that this tool may blow up if it starts receiving \n\
halfway through a packet (DecodeError "not enough input bytes...").'
    prompt_flag.clear()
    while 1 :

        # parsing the command queue
        try:
            data_out = tx_q.get(False)
            if len(data_out) != 20:
                print 'The message is not 20 bytes long! Try again.'
                prompt_flag.set()
                raise Queue.Empty
            print 'ASCII output data:'
            print "\t" + data_out
            print 'Raw output data:'
            print "\t" + ":".join("{:02x}".format(ord(c)) for c in data_out)
            data_out = cobs.encode(data_out)
            print 'COBS-ified output data with 0x00 delimiters:'
            data_out = b'\x00' + data_out + b'\x00'
            print "\t" + ":".join("{:02x}".format(ord(c)) for c in data_out)
            ser.write(data_out)
            print 'de-COBS-ified output data:'
            try:
                data_out = cobs.decode(data_out[1:-1])
            except:
                print 'The packet was malformed!'
                prompt_flag.set()
            print "\t" + data_out
            prompt_flag.set()
        except Queue.Empty, e:
               pass

        try:
            data_in = rx_q.get(False)
            if(data_in != ''):
                print 'RECEIVED DATA##########'
                print 'Raw data:'
                print "\t" + ":".join("{:02x}".format(ord(c)) for c in data_in)
                print 'De-COBS-ified data:'
                try:
                    data_in = cobs.decode(data_in[1:-1])
                except cobs.DecodeError:
                        print 'Got a malformed packet! There was a non-zero byte in the input!'
                        print 'DONE RECEIVED DATA######'
                        raise(Queue.Empty)
                print "\t" + ":".join("{:02x}".format(ord(c)) for c in data_in)
                print 'ASCII data:'
                print "\t" + data_in
                print 'DONE RECEIVED DATA######'
                prompt_flag.set()
        except Queue.Empty, e:
            pass



# then start the two threads
displayer = threading.Thread(None, # always to None since the ThreadGroup class is not implemented yet
                            display, # the function the thread will run
                            None, # doo, don't remember and too lazy to look in the doc
                            (tx_q,rx_q,), # *args to pass to the function
                             {}) # **kwargs to pass to the function

sender = threading.Thread(None, input_getter, None, (tx_q,), {})
receiver = threading.Thread(None, serial_monitor, None, (rx_q,), {})

if __name__ == "__main__" :
    displayer.start()
    sender.start()
    receiver.start()
