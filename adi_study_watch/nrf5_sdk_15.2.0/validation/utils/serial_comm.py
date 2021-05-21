from serial import Serial
from utils.test_utils import util_logger
import time


class SerialIface(Serial):

    def __init__(self, port, baudrate=115200, timeout=0.1):
        """
        This function initializes the serial class and configures the serial port
        :param port:
        :param baudrate:
        :param timeout:
        """
        super(SerialIface, self).__init__(port=port, baudrate=baudrate, timeout=timeout)
        time.sleep(2)  # Delay added for the port to be configured

    @util_logger
    def serial_write(self, cmd, get_response=True, resp_delay_secs=1):
        """
        This function takes in a string command and writes it to the serial port.
        If get_response is True, the function will also read back a response.
        :param cmd:
        :param get_response:
        :param resp_delay_secs:
        :return:
        """
        if type(cmd) is str:
            # converting command string to encoded string which serial write expects
            cmd = cmd.encode()
        status = self.write(cmd)
        if get_response:
            time.sleep(resp_delay_secs)
            response = self.read(self.inWaiting())
        else:
            response = None
        return response


if __name__ == '__main__':
    # Below is an example to demonstrate the SerialIface class usage
    iface = SerialIface(port='COM7')
    command = '3fsend'
    response = iface.serial_write(command)
    print(response)
