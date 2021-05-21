import serial, Queue, threading
from cobs import cobs
from m2m2_common import *
from time import sleep
import time
import binascii


# An abstract m2m2 server base class.
class _m2m2_server_base():
    def __init__(self, rx_q, tx_q, vrb):
        tx_ack_evt = threading.Event()
        self.vrb = vrb
        self.serial_rx_thread = threading.Thread(target=self.rx_run, args=[rx_q,tx_ack_evt])
        self.serial_rx_thread.setDaemon(True)
        self.serial_tx_thread = threading.Thread(target=self.tx_run, args=[tx_q,tx_ack_evt])
        self.serial_tx_thread.setDaemon(True)
        self.verbose = False
        self.server_setup()
        self.vrb.write("m2m2 server up!", 4)

    def server_setup():
        '''
        Defines any internal setup that a server has to do.
        DO NOT put anything here that touches a real interface; that should go in connect()
        '''
        raise Exception.NotImplementedError()

    def connect():
        '''
        Defines how a connection should be opened to the interface.
        Returns True on success, and False on failure.
        This function must start the receive threads:
            self._start()
        '''
        raise Exception.NotImplementedError()

    def is_connected():
        '''
        Returns True if the interface is currently connected, False otherwise.
        '''
        raise Exception.NotImplementedError()

    def rx_run():
        '''
        The thread which receives packets from the interface
        '''
        raise Exception.NotImplementedError()

    def tx_run():
        '''
        The thread which sends packets to the interface
        '''
        raise Exception.NotImplementedError()

    def scan():
        '''
        Fetches a list of available instances of the interface.
        Examples include:
            - COM ports available on the PC
            - BLE devices available for connection
        '''
        raise Exception.NotImplementedError()

    def quit():
        '''
        Closes the interface. Closes ports, cleans up in-progress transactions, etc.
        '''
        raise Exception.NotImplementedError()

    def _start(self):
        self.serial_tx_thread.start()
        self.serial_rx_thread.start()

class m2m2_uart_server(_m2m2_server_base):

    def __init__(self, rx_q, tx_q, vrb):
        self.serial_name = None
        tx_ack_evt = threading.Event()
        self.vrb = vrb
        self.serial_rx_thread = threading.Thread(target=self.rx_run, args=[rx_q,tx_ack_evt])
        self.serial_rx_thread.setDaemon(True)
        self.serial_tx_thread = threading.Thread(target=self.tx_run, args=[tx_q,tx_ack_evt])
        self.serial_tx_thread.setDaemon(True)
        self.verbose = False
        self.server_setup()
        self.vrb.write("m2m2 server up!", 4)

    def server_setup(self):
        self.serial_device = None

    #def connect(self, serial_address, serial_baud):
    def connect(self, serial_address):
        try:
            #self.serial_device = serial.Serial(serial_address, serial_baud, timeout=None)
            self.serial_device = serial.Serial(serial_address, timeout=None)
        except Exception as e:
            self.vrb.err("Server error!: {}".format(e))
            return False
        self.verbose = False
        self._start()
        self.vrb.write("Serial server up!", 4)
        return True

    def is_connected(self):
        if self.serial_device == None:
            return False
        else:
            return True

    def rx_run(self, rx_q,tx_ack_evt):
        self.rx_q = rx_q
        while True:
            self.data = self._read_packet(tx_ack_evt)
            rx_q.put(self.data)


    def tx_run(self, tx_q, tx_ack_evt):
        self.tx_q = tx_q
        while True:
            pkt = self.tx_q.get()
            try:
                self.serial_device.write(pkt)
            except Exception as e:
                self.vrb.write("Failed to do a serial write, did you forget to connect?: {}".format(e), 4)

    def scan(self):
        import sys, glob
        result = []
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass
        return result

    def quit(self):
        self.serial_device.close()

    '''def _read_packet(self,tx_ack_evt):
        #eol = b'\x00'
        #leneol = len(eol)
        line = bytearray()
        header = self.serial_device.read(8)
        line += header
        #print   "\nread packet"
        #print   binascii.hexlify(line)
        #for n in (header):
            #line += n
        pkt_len = (line[4] << 8) + (line[5])
        Payload = self.serial_device.read((pkt_len-8))
        line += Payload
        #for n in Payload:
		#line += n
        return bytes(line)'''

    def _read_packet(self,tx_ack_evt):
        line = bytearray()

        for n in range(8):
            c = self.serial_device.read(1)
            line += c
        pkt_len = (line[4] << 8) + (line[5]);
        for n in range(pkt_len-8):
            c = self.serial_device.read(1)
            line += c
        return bytes(line)

class m2m2_adi_ble_server(_m2m2_server_base):
    def __init__(self, rx_q, tx_q, vrb):
        tx_ack_evt = threading.Event()
        self.vrb = vrb
        self.serial_rx_thread = threading.Thread(target=self.rx_run, args=[rx_q,tx_ack_evt])
        self.serial_rx_thread.setDaemon(True)
        self.serial_tx_thread = threading.Thread(target=self.tx_run, args=[tx_q,tx_ack_evt])
        self.serial_tx_thread.setDaemon(True)
        self.verbose = False
        self.server_setup()
        self.vrb.write("m2m2 server up!", 4)

    def server_setup(self):
        self.serial_device = None

    def connect(self,device_id):
        #self.serial_device = serial.Serial(serial_com_address, 115200, timeout=2)
        serial_address = self.serial_name
        self.serial_device = serial.Serial(serial_address,115200,timeout=None)
        msg = "AT+RECN=1\r\n"
        self.serial_device.write(msg)
        response = bytearray()
        response = self._read_ascii_packet(self.serial_device)
        if response == '+OK':
            msg = "AT+CONN=" + str(device_id) + "\r\n"
            self.serial_device.write(msg)
            print 'connecting...'
            sleep(4)
            response = bytearray()
            response = self._read_ascii_packet(self.serial_device)
            if response == '+OK':
                response = bytearray()
                response = self._read_ascii_packet(self.serial_device)
                if response == '+DEVCONN':
                    print 'ble connected'
                    print 'the link is ready to start'
                    #serial_device.close()
                    self.verbose = False
                    self._start()
                    self.vrb.write("Serial server up!", 4)
                    self.serial_device.reset_input_buffer()
                    return True
            print 'connection error'
            self.serial_device.close()
            return False

    def is_connected(self):
        if self.serial_device == None:
            return False
        else:
            return True

    def rx_run(self, rx_q,tx_ack_evt):
        self.rx_q = rx_q
        while True:
            self.data = self._read_packet(tx_ack_evt)
            rx_q.put(self.data)


    def tx_run(self, tx_q, tx_ack_evt):
        self.tx_q = tx_q
        while True:
            pkt = self.tx_q.get()
            try:
                self.serial_device.write(pkt)
            except Exception as e:
                self.vrb.write("Failed to do a serial write, did you forget to connect?: {}".format(e), 4)

    def scan(self,serial_address,scan_time = 5):
        self.serial_name = serial_address
        self.serial_device = serial.Serial(serial_address,115200,timeout=2)
        msg = "AT+SCAN\r\n"
        self.serial_device.write(msg)
        response = bytearray()
        response = self._read_ascii_packet(self.serial_device)
        if response == '+OK':
            print 'scanning ble devices during ' + str(scan_time) + ' seconds\r\n'
            sleep(scan_time)
            num_devices = 0
            ble_device_response = []
            while True:
                response = self._read_ascii_packet(self.serial_device)
                if not response:
                    break
                else:
                    ble_device_response.append(response)
                    num_devices += 1
            if(num_devices == 0):
                print 'no devices found, please, try again'
                self.serial_device.close()
            else:
                print '================================='
                print '=================================\r\n'
                print str(num_devices) + ' BLE devices found\r\n'
                for ble_device in ble_device_response:
                    print  ble_device
                print '================================='
                print '=================================\r\n'
                print 'select one device from the list\r\n'
        self.serial_device.close()


    def quit(self):
        self.serial_device.close()

    def _read_packet(self,tx_ack_evt):
        line = bytearray()

        for n in range(8):
            c = self.serial_device.read(1)
            line += c
        pkt_len = (line[4] << 8) + (line[5])
        for n in range(pkt_len-8):
            c = self.serial_device.read(1)
            line += c
        return bytes(line)

    def _read_ascii_packet(self,serial_device):
        packet = bytearray()
        while True:
            c = serial_device.read(1)
            if not c:
                break
            elif c != '\r' and c != '\n':
                packet += c
            elif c == '\r':
                c = serial_device.read(1)
                if c == '\n':
                    break
        return packet
