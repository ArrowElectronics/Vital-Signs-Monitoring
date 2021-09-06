
import ctypes, os

from os.path import dirname, basename, isfile
import glob, sys
from importlib import import_module

# Automagically import all generated Python style interface definitions.

interface_file_relative_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../inc/python/")
# Make sure we can do `from X import *`
sys.path.append(interface_file_relative_path)
# Find a list of interface files to import
modules = glob.glob(os.path.dirname(interface_file_relative_path) + "/*_def.py")
found_interfaces = [ os.path.basename(f)[:-3] for f in modules if os.path.isfile(f) and not f.endswith('__init__.py')]
for i in found_interfaces:
    exec("from {} import *".format(i))

from m2m2_core_def import *

# A LUT for taking an "informal" device name from the command line and getting an Application
# address that's associated with it.
application_name_map = {
"adxl":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, 'help':"The ADXL device."},
"ppg":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, 'help':"The PPG heart rate service."},
"agc":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, 'help':"The AGC service."},
"syncppg":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SYNC_ADPD_ADXL, 'help':"The sync PPG data stream service."},
"eda":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, 'help':"The EDA service."},
"ecg":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, 'help':"The ECG service."},
"bcm":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, 'help':"The BCM service."},
"temperature":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE, 'help':"The Temperature service."},
"ped":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PED, 'help':"The Pedometer service."},
"ad5940":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, 'help':"The AD5940 device."}, #Since we dont have ad5940 app separately, using EDA address to access ad5940
"adpd4000":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'help':"The ADPD400 device."},
"lt_dcb_config":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, 'help':"The low touch application service."},
"adp5360":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, 'help':"The ADP5360 device."},
"ad7156":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156, 'help':"The AD7156 device."},
"sqi":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI, 'help':"The SQI service."},
"lt_app_lcfg":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, 'help':"The lt_app_lcfg."},
"user0_config":{'address':M2M2_ADDR_ENUM_t.M2M2_ADDR_USER0_CONFIG_APP, 'help':"The user0_config lcfg."},
}

stream_name_map = {
"radpd1":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM1, 'help':"Raw ADPD data slot1"},
"radpd2":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM2, 'help':"Raw ADPD data slot2"},
"radpd3":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM3, 'help':"Raw ADPD data slot3"},
"radpd4":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM4, 'help':"Raw ADPD data slot4"},
"radpd5":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM5, 'help':"Raw ADPD data slot5"},
"radpd6":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM6, 'help':"Raw ADPD data slot6"},
"radpd7":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM7, 'help':"Raw ADPD data slot7"},
"radpd8":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM8, 'help':"Raw ADPD data slot8"},
"radpd9":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM9, 'help':"Raw ADPD data slot9"},
"radpd10":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM10, 'help':"Raw ADPD data slot10"},
"radpd11":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM11, 'help':"Raw ADPD data slot11"},
"radpd12":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM12, 'help':"Raw ADPD data slot12"},
"radxl":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL_STREAM, 'help':"Raw ADXL data."},
"rppg":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG_STREAM, 'help':"Raw PPG data."},
"rsyncppg":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SYNC_ADPD_ADXL, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SYNC_ADPD_ADXL_STREAM, 'help':"Raw sync PPG data."},
"reda":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA_STREAM, 'help':"Raw EDA data."},
"recg":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG_STREAM, 'help':"Raw ECG data."},
"rbcm":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM_STREAM, 'help':"Raw BCM data."},
"ragc":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_AGC_STREAM, 'help':"PPG AGC data."},
"radp":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_BATT_STREAM, 'help':"ADP5350 Battery data."},
"rtemperature":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE_STREAM, 'help':"Raw Temperature data."},
"rped":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PED, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PED_STREAM, 'help':"Raw Pedometer data."},
"rsqi":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI_STREAM, 'help':"Raw SQI data."},
"rhrv":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_HRV_STREAM, 'help':"PPG HRV data."},
#"rad5940":{'application':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD5940, 'stream':M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD5940_STREAM, 'help':"Raw ADPD data (support for dual-slot)"},
}

pkt_loss_dict = {'recg': False, 'rppg': False, 'radxl': False, 'rtemperature': False, 'reda': False, 'rsyncppg': False, 'radpd': False}

def init_pkt_loss_dict():
    global pkt_loss_dict
    for k in pkt_loss_dict.keys():
        pkt_loss_dict[k] = False

def update_pkt_loss(stream_name, state):
    global pkt_loss_dict
    if stream_name in pkt_loss_dict.keys():
        pkt_loss_dict[stream_name] = state
        print(pkt_loss_dict)
    else:
        print('Unable to update packet loss dictionary!')

def get_pkt_loss_dict():
    global pkt_loss_dict
    return pkt_loss_dict

# This is a placeholder for the address, until the CLI address makes its way
# into the M2M2_ADDR_ENUM_t definition.
#CLI_ADDRESS = M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI
CLI_ADDRESS = M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI_BLE

"""
Wraps the m2m2_hdr_t(0) type with some convenient methods. Stores the header and
payload separately, so that the payload and its fields can be accessed directly.
"""
class m2m2_packet:
    def __init__(self, dest, payload):
        self.header = m2m2_hdr_t(0)
        self.header.src = CLI_ADDRESS
        self.header.dest = dest
        self.header.length = ctypes.sizeof(m2m2_hdr_t(0)) + ctypes.sizeof(payload)
        self.header.checksum = 0x0000
        self.payload = payload

    def _pack_data(self, data, length):
        """
        Internal method to turn a structure into a sequence of bytes.
        """
        return buffer(data)[0:length]

    def pack(self):
        """
        Take our header and payload structures and pack them into a sequence of bytes in the form of a Python string
        """
        return self._pack_data(self.header, 8) + self._pack_data(self.payload, self.header.length-8)

    def unpack(self, raw_packet, override_size = False):
        """
        Take a raw sequence of bytes and unpack it into our header and payload classes.
        """
        memmove(addressof(self.header), buffer(raw_packet)[:], ctypes.sizeof(self.header))
        if not override_size and ctypes.sizeof(self.payload) < self.header.length - ctypes.sizeof(self.header):
            print "CRITICAL ERROR! TRYING TO UNPACK A MESSAGE INTO THE WRONG PAYLOAD!"
            print "Src address: \t{}\t({})".format(self.header.src, hex(self.header.src))
            print "Dest address: \t{}\t({})".format(self.header.dest, hex(self.header.dest))
            print "Packet length: \t{}\t({})".format(self.header.length, hex(self.header.length))
            print "Checksum: \t{}\t({})".format(self.header.checksum, hex(self.header.checksum))
            print "Raw packet header__payload: {}__{}".format(format_hex(self._pack_data(self.header)), format_hex(raw_packet))
            print "================"
            print "Receiving structure type: {}".format(type(self.payload).__name__)
            print "Receiving structure size: {}".format(ctypes.sizeof(self.payload))
            raise Exception("Incorrect message size.")
        else:
            memmove(addressof(self.payload), buffer(raw_packet)[ctypes.sizeof(self.header):], ctypes.sizeof(self.payload))

"""
Set the global variable CLI_ADDRESS value. change based on USB_CDC/BLE connection
"""
def set_cli_addr(addr):
    global CLI_ADDRESS
    CLI_ADDRESS	= addr

"""
Get the global variable CLI_ADDRESS value which is based on USB_CDC/BLE connection
"""
def get_cli_addr():
    global CLI_ADDRESS
    return CLI_ADDRESS

"""
Returns a string of colon-separated hex values of an input string.
"""
def format_hex(str):
    return ":".join("{:02x}".format(ord(c)) for c in str)

"""
Returns a string with swapped characters at positions pos1 and pos2.
"""
def swap(str,pos1,pos2):
    lst = list(str);
    lst[pos1], lst[pos2] = lst[pos2], lst[pos1]
    return ''.join(lst)

"""
Returns an m2m2_hdr_t(0) object with the header information contained in raw_packet.
"""
def pack_header(raw_packet):
    header = m2m2_hdr_t(0)
    memmove(addressof(header), buffer(raw_packet)[:], ctypes.sizeof(header))
    return header

"""
    This function reads a csv file and returns the column data specified by the col_idx.
    :param file_path:
    :param col_idx:
    :param row_offset:
    :return:
"""
def read_csv_column(file_path, col_idx=0, row_offset=1):
    col_data_list=[]
    with open(file_path, 'r') as f_ref:
        line_list = f_ref.readlines()
    col_data_list = [float(line.split(',')[col_idx].strip())
                     for i, line in enumerate(line_list)
                     if (i >= row_offset) and line.strip()]
    return col_data_list

"""
Provides convenient methods for creating and displaying a table on the command line.
"""
class table:
    def __init__(self, headings, v_separator = "|", h_separator = "-", width = None):
        """
        headings: A list of table headings.
        v_separator: A string to use as the vertical separator between row entries.
        h_separator: A string to use as the horizontal separator between rows.
        width: The width of the table's row entries.
        """
        self.v_separator = v_separator
        self.h_separator = h_separator
        self.headings = headings
        self.width = width
        self.rows = []
    def add_row(self, row):
        """
        Takes a list of row entries, and adds them to the list of rows.
        """
        self.rows.append(row)
    def display(self):
        """
        Prints out the formatted table.
        """
        cells = []
        v = self.v_separator
        h = self.h_separator
        if self.width == None:
            for heading in self.headings:
                # Format the width formatter string based on the width of the heading
                w = "{{:<{}}}".format(len(heading))
                cells.append("{} {} ".format(v, w))
        else:
            # Format the width formatter string
            w = "{{:<{}}}".format(self.width)
            # The format of a "cell" in the table
            cells.append("{} {} ".format(v, w))
        # Create the header formatting string
        hdr_format = ""
        for cell in cells:
            hdr_format += cell
        hdr_format += v
        hdr = hdr_format.format(*self.headings)
        row_sep = h * len(hdr)
        print row_sep
        print hdr
        print row_sep
        for row in self.rows:
            row_format = ""
            for cell in cells:
                row_format = row_format + cell
            row_format = row_format + v
            print row_format.format(*row)
        print row_sep
