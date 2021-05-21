import ftd2xx, sys

class watch_ftdi(ftd2xx.FTD2XX):
    def set_pm(self):
        self.setBitMode(0xF0, 0x20)
    def set_ps(self):
        self.setBitMode(0xF1, 0x20)

def open_watch_ftdi():
    d = ftd2xx.open()
    d.__class__ = watch_ftdi
    return d

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print "Invalid arguments! Need the state to set the UART to ('pm' or 'ps'), and a COM port:"
        print "uart_connection_swap.py COM5 pm"
        quit()
    device_list = ftd2xx.listDevices()
    print "Found devices:"
    for device in device_list:
        print "\t{}".format(device)

    d = open_watch_ftdi()
    if sys.argv[2] == "ps":
        # Switch to PS
        d.set_ps()
    if sys.argv[2] == "pm":
        # Switch to PM
        d.set_pm()
    d.close()
