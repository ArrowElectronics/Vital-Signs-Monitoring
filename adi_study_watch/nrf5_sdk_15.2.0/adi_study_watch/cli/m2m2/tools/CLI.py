try:
    import cmd, struct, serial, ctypes, threading, Queue, binascii, sys, glob, subprocess, errno
    import datetime, time, os
    import math
    import socket
    import m2m2_server
    from cobs import cobs
    from m2m2_common import *
    import colorama as cr
    import tqdm
    from cli_utils import *
except ImportError as e:
    print "Oops, looks like you're missing a Python module!"
    print "Try installing it with pip: `pip install MODULE_NAME`"
    print "Error message is: {}".format(e)
    print "Exiting..."
    raise ImportError(e)
class LowTouch():
    User_File = ''
    User_File_name = "USER_INPUT_CONFIG.LOG"
    DCB_File_name = "GEN_BLK_DCB_CONFIG.LOG"
    Startcmd = False
    Startcmdlen = 0
    Startcmdcount = 0
    Stopcmd = False
    Stopcmdlen = 0
    Stopcmdcount = 0
    Enable_lowtouch = False

enable_csv_logs = 0
lowtouch = LowTouch()
class verboser():
    msg_formatters = {
    0:None, # A placeholder for not printing anything
    1:{"fmt":"{}", "help":"Regular old prints"},
    2:{"fmt":cr.Back.WHITE + cr.Fore.BLACK + "{}", "help":"More chatter (i.e. Which command was just run)"},
    3:{"fmt":cr.Back.CYAN + cr.Fore.BLACK + "{}", "help": "Underlying transaction info (i.e. raw packet data)"},
    4:{"fmt":cr.Back.MAGENTA + cr.Fore.CYAN + "{}", "help": "CLI Interior workings (threads starting, sockets opened, etc)"},
    }
    err_formatter = cr.Back.RED + cr.Fore.GREEN + "ERR: {}"
    level = 1
    console_socket = None
    console = None
    port = 1069

    def __init__(self, console_level=2):
        self.console_level = console_level

    def __del__(self):
        self.stop_console()

    def write(self, msg, level = 1):
        if ((self.level == 0) or (level == 0)):
            return
        if level <= self.level:
            outstr = ""
            whitespace = ""
            whitespace = msg[:len(msg) - len(msg.lstrip())]
            msg_str = msg[len(msg) - len(msg.lstrip()):]
            output_str = whitespace + self.msg_formatters[level]["fmt"].format(msg_str)
            if level >= self.console_level and self.console_socket != None:
                self.console_write(output_str)
            else:
                print output_str

    def console_write(self, text, level = 1):
        if self.console != None:
            try:
                self.console_socket.send(text)
            except socket.error as e:
                if e.errno == errno.ENETRESET or e.errno == errno.ECONNABORTED or e.errno == errno.ECONNRESET:
                    self.err("Socket error: {}".format(e), force_print=True)
                    self.err("Attempting to restart the console...", force_print=True)
                    self.stop_console()
                    self.start_console()
                else:
                    self.err("Socket error: {}".format(e), force_print=True)

    def err(self, msg, level = 1, force_print=False):
        if lowtouch.Enable_lowtouch == True:
            print("  skip sending Command to device...")
            return None
        output_str = self.err_formatter.format(msg)
        if self.console_socket != None and self.level >= self.console_level and not force_print:
            self.console_write(output_str)
        else:
            print output_str

    def set_level(self, level):
        self.level = level
        if level >= self.console_level and self.console == None:
            self.start_console()
        elif level <= self.console_level and self.console_socket != None:
            self.stop_console()

    def start_console(self):
        if "nt" in os.name:
            self.console = subprocess.Popen("start python.exe console_window.py {}".format(self.port), shell=True)
        else:
            self.console = subprocess.Popen("exec xterm -e \"python console_window.py {}\"".format(self.port), shell=True)
        try:
            self.console_socket = socket.socket()
            self.console_socket.connect(('localhost', self.port))
        except socket.error as e:
            self.err("Couldn't open the socket to the secondary console: {}".format(e))

    def stop_console(self):
        self.console_socket.close()
        self.console.terminate()
        self.console_socket = None
        self.console = None


class m2m2_shell(cmd.Cmd):
    cr.init(autoreset=True)
    intro = 'This is the m2m2 UART shell. Type "help" or "?" to list commands.\n'
    prompt = cr.Fore.GREEN + '#>'
    # Make error messages stand out more.

    vrb = verboser()
    m2m2_server = None
    sock_map = {}
    tx_q = Queue.Queue()
    rx_q = Queue.Queue()

    dispatcher_map = {}
    for addr in vars(M2M2_ADDR_ENUM_t).keys():
        if addr.count("__") <= 0:
            addr_val = vars(M2M2_ADDR_ENUM_t)[addr]
            dispatcher_map[addr_val] = Queue.Queue()

    # A dictionary of useful/common command sequences to be executed. The 'commands' key contains a list of CLI commands to be run for the sequence.
    quickstarts = {
#    "adpd4000": {   "commands":["clockCalibration 6", "setSlot 1 1 1 0x04", "sub radpd1 add", "sensor adpd4000 start"],
#                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG."},
    "adpd4000": {   "commands":[ "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green LED."},
    "ecg4000": {   "commands":[ "controlECGElectrodeSwitch 4k_sw 1", "loadAdpdCfg 40","SetEcg4kLcfg 0:300", "clockCalibration 6", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"Setup the ADPD for measuring ecg in slot A."},
    "adpd4000_g_DVT2": {   "commands":["loadAdpdCfg 40", "clockCalibration 2","adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green LED."},
    "adpd4000_r_DVT2": {   "commands":["loadAdpdCfg 41", "clockCalibration 2", "adpdAGCControl 2:1", "sensor adpd4000 start", "sub radpd7 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Red LED."},
    "adpd4000_ir_DVT2": {   "commands":["loadAdpdCfg 42", "clockCalibration 2", "adpdAGCControl 3:1", "sensor adpd4000 start", "sub radpd8 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for IR LED."},
    "adpd4000_b_DVT2": {   "commands":["loadAdpdCfg 43", "clockCalibration 2", "adpdAGCControl 4:1", "sensor adpd4000 start", "sub radpd9 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Blue LED."},
    "adpd4000_g": {   "commands":["loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green LED."},
    "adpd4000_r": {   "commands":["loadAdpdCfg 41", "clockCalibration 6", "adpdAGCControl 2:1", "sensor adpd4000 start", "sub radpd7 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Red LED."},
    "adpd4000_ir": {   "commands":["loadAdpdCfg 42", "clockCalibration 6", "adpdAGCControl 3:1", "sensor adpd4000 start", "sub radpd8 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for IR LED."},
    "adpd4000_b": {   "commands":["loadAdpdCfg 43", "clockCalibration 6", "adpdAGCControl 4:1", "sensor adpd4000 start", "sub radpd9 add"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Blue LED."},
    "adpd4000_g_agc_off": {   "commands":["loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:0", "sensor adpd4000 start", "sub radpd6 add", "plot radpd6"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Green LED."},
    "adpd4000_r_agc_off": {   "commands":["loadAdpdCfg 41", "clockCalibration 6", "adpdAGCControl 2:0", "sensor adpd4000 start", "sub radpd7 add", "plot radpd7"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Red LED."},
    "adpd4000_ir_agc_off": {   "commands":["loadAdpdCfg 42", "clockCalibration 6", "adpdAGCControl 3:0", "sensor adpd4000 start", "sub radpd8 add", "plot radpd8"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for IR LED."},
    "adpd4000_b_agc_off": {   "commands":["loadAdpdCfg 43", "clockCalibration 6", "adpdAGCControl 4:0", "sensor adpd4000 start", "sub radpd9 add", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for Blue LED."},
    "adpd4000_g_r": {   "commands":["create_adpd4k_dcfg 6:4 7:5", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1 2:1", "sensor adpd4000 start", "sub radpd6 add","sub radpd7 add", "plot radpd6", "plot radpd7"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green + red LED."},
    "adpd4000_g_ir": {   "commands":["create_adpd4k_dcfg 6:4 8:6", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1 3:1", "sensor adpd4000 start", "sub radpd6 add","sub radpd8 add", "plot radpd6", "plot radpd8"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green + ir LED."},
    "adpd4000_r_ir": {   "commands":["create_adpd4k_dcfg 7:5 8:6", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 2:1 3:1", "sensor adpd4000 start", "sub radpd7 add","sub radpd8 add", "plot radpd7", "plot radpd8"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for red+ir LED."},
    "adpd4000_g_b": {   "commands":["create_adpd4k_dcfg 6:4 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1 4:1", "sensor adpd4000 start", "sub radpd6 add","sub radpd9 add", "plot radpd6", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green+blue LED."},
    "adpd4000_r_b": {   "commands":["create_adpd4k_dcfg 7:5 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 2:1 4:1", "sensor adpd4000 start", "sub radpd7 add","sub radpd9 add", "plot radpd7", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for red+blue LED."},
    "adpd4000_ir_b": {   "commands":["create_adpd4k_dcfg 8:6 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 3:1 4:1", "sensor adpd4000 start", "sub radpd8 add","sub radpd9 add", "plot radpd8", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for ir+blue LED."},
    "adpd4000_g_r_ir": {   "commands":["create_adpd4k_dcfg 6:4 7:5 8:6", "loadAdpdCfg 40", "adpdAGCControl 1:1 2:1 3:1", "clockCalibration 6", "sensor adpd4000 start", "sub radpd6 add", "sub radpd7 add", "sub radpd8 add", "plot radpd6", "plot radpd7", "plot radpd8"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green+red+ir LED."},
    "adpd4000_r_ir_b": {   "commands":["create_adpd4k_dcfg 7:5 8:6 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 2:1 3:1 4:1", "sensor adpd4000 start", "sub radpd7 add","sub radpd8 add","sub radpd9 add", "plot radpd7", "plot radpd8", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for red+ir+blue LED."},
    "adpd4000_g_r_b": {   "commands":["create_adpd4k_dcfg 6:4 7:5 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1 2:1 4:1", "sensor adpd4000 start", "sub radpd6 add", "sub radpd7 add","sub radpd9 add", "plot radpd6", "plot radpd7", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green+red+blue LED."},
    "adpd4000_g_ir_b": {   "commands":["create_adpd4k_dcfg 6:4 8:6 9:7", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1 3:1 4:1", "sensor adpd4000 start", "sub radpd6 add", "sub radpd8 add","sub radpd9 add", "plot radpd6", "plot radpd8", "plot radpd9"],
                "help":"Setup the ADPD in 32 bit summation mode with the default DCFG for green+ir+blue LED."},
    "ctr": {   "commands":["loadAdpdCfg 6", "clockCalibration", "setSlot 0x00:0x14", "getCtrValue"],
                "help":"Get ctr value."},
    "adxl": {   "commands":["sensor adxl start","sub radxl add"],
                "help":"Start ADXL"},
    "plot-adpd4000": {"commands":["quickstart adpd4000", "plot radpd6"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ADPD SlotF."},
    "plot-ecg4000": {"commands":["quickstart ecg4000", "plot radpd1"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ECG from SlotA."},
    "plot-adpd4000_g": {"commands":["quickstart adpd4000_g", "plot radpd6"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ADPD SlotF."},
    "plot-adpd4000_r": {"commands":["quickstart adpd4000_r", "plot radpd7"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ADPD SlotG."},
    "plot-adpd4000_ir": {"commands":["quickstart adpd4000_ir", "plot radpd8"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ADPD SlotH."},
    "plot-adpd4000_b": {"commands":["quickstart adpd4000_b", "plot radpd9"],
                "help":"Quickstarts the ADPD and starts a plot of the raw ADPD SlotI."},
    "temperature": {   "commands":["create_adpd4k_dcfg 4:2 5:3", "loadAdpdCfg 40", "sensor temperature start", "sub rtemperature add"],
                "help":"Start Temperature"},
    "ped": {   "commands":["sensor adxl start", "sensor ped start", "sub rped add"],
                "help":"Starts the Pedometer."},
    "sqi_ext":{   "commands":["plot rsqi", "SQISetSlot 6", "set_adpd_ext_datastream_odr 100", "sensor sqi start","sub rsqi add","send_ext_adpd_datastream 11173863_ADPDAppStream_SlotFChannel1.csv 6 2", "sub rsqi remove", "sensor sqi stop"],
                "help":"send external sqi data stream"},
    "sqi_green": {   "commands":["plot radpd6","plot rsqi","loadAdpdCfg 40", "reg w adpd4000 0x0D:0x2710", "clockCalibration 6","SQISetSlot 6","sensor sqi start","sub rsqi add","adpdAGCControl 1:1","sensor adpd4000 start","sub radpd6 add"],
                "help":"Starts the SQI with Green LED on slot F of ADPD4000 at 100Hz"},
    "sqi_green_50": {   "commands":["plot radpd6","plot rsqi","loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6","SQISetSlot 6","sensor sqi start","sub rsqi add","adpdAGCControl 1:1","sensor adpd4000 start","sub radpd6 add"],
                "help":"Starts the SQI with Green LED on slot F of ADPD4000 at 50Hz"},
    "sqi_green_25": {   "commands":["plot radpd6","plot rsqi","loadAdpdCfg 40", "reg w adpd4000 0x0D:0x9C40", "clockCalibration 6","SQISetSlot 6","sensor sqi start","sub rsqi add","adpdAGCControl 1:1","sensor adpd4000 start","sub radpd6 add"],
                "help":"Starts the SQI with Green LED on slot F of ADPD4000 at 25Hz"},
    "sqi_ppg":{   "commands":["plot rsyncppg","plot rppg", "plot rsqi", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "SQISetSlot 6","sensor sqi start", "sub rsqi add", "sensor ppg start","sub rppg add"],
                "help":"starts SQI along with PPG stream"},
    "start_log_sqi_green": {   "commands":["loadAdpdCfg 40","reg w adpd4000 0x0D:0x2710", "clockCalibration 6", "SQISetSlot 6", "fs_sub rsqi add", "fs_sub radpd6 add","sensor sqi start", "sensor adpd4000 start", "fs_log start"],
                "help":"log the SQI data with Green LED on slot F of ADPD4000 at 100Hz"},
    "sqi_agc_off_green": {   "commands":["plot radpd6","plot rsqi","loadAdpdCfg 40","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 6","sensor sqi start","sub rsqi add","adpdAGCControl 1:0","sensor adpd4000 start","sub radpd6 add"],
                "help":"Starts the SQI with Green LED on slot F of ADPD4000 at 100Hz"},
    "sqi_red": {   "commands":["plot radpd7","plot rsqi","loadAdpdCfg 41","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 7","sensor sqi start","sub rsqi add","adpdAGCControl 2:1","sensor adpd4000 start","sub radpd7 add"],
                "help":"Starts the SQI with Red LED on slot G of ADPD4000 at 100Hz"},
    "start_log_sqi_red": {   "commands":["loadAdpdCfg 41","reg w adpd4000 0x0D:0x2710", "clockCalibration 6", "SQISetSlot 7", "fs_sub rsqi add", "fs_sub radpd7 add","sensor sqi start", "sensor adpd4000 start", "fs_log start"],
                "help":"log the SQI data with Red LED on slot G of ADPD4000 at 100Hz"},
    "sqi_agc_off_red": {   "commands":["plot radpd7","plot rsqi","loadAdpdCfg 41","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 7","sensor sqi start","sub rsqi add","adpdAGCControl 2:0","sensor adpd4000 start","sub radpd7 add"],
                "help":"Starts the SQI with Red LED on slot G of ADPD4000 at 100Hz"},
    "sqi_ir": {   "commands":["plot radpd8","plot rsqi","loadAdpdCfg 42","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 8","sensor sqi start","sub rsqi add","adpdAGCControl 3:1","sensor adpd4000 start","sub radpd8 add"],
                "help":"Starts the SQI with IR LED on slot H of ADPD4000 at 100Hz"},
    "start_log_sqi_ir": {   "commands":["loadAdpdCfg 42","reg w adpd4000 0x0D:0x2710", "clockCalibration 6", "SQISetSlot 8", "fs_sub rsqi add", "fs_sub radpd8 add","sensor sqi start", "sensor adpd4000 start", "fs_log start"],
                "help":"log the SQI data with IR LED on slot H of ADPD4000 at 100Hz"},
    "sqi_agc_off_ir": {   "commands":["plot radpd8","plot rsqi","loadAdpdCfg 42","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 8","sensor sqi start","sub rsqi add","adpdAGCControl 3:0","sensor adpd4000 start","sub radpd8 add"],
                "help":"Starts the SQI with IR LED on slot H of ADPD4000 at 100Hz"},
    "sqi_blue": {   "commands":["plot radpd9","plot rsqi","loadAdpdCfg 43","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 9","sensor sqi start","sub rsqi add","adpdAGCControl 4:1","sensor adpd4000 start","sub radpd9 add"],
                "help":"Starts the SQI with Blue LED on slot I of ADPD4000 at 100Hz"},
    "start_log_sqi_blue": {   "commands":["loadAdpdCfg 43","reg w adpd4000 0x0D:0x2710", "clockCalibration 6", "SQISetSlot 9", "fs_sub rsqi add", "fs_sub radpd9 add","sensor sqi start", "sensor adpd4000 start", "fs_log start"],
                "help":"log the SQI data with Blue LED on slot I of ADPD4000 at 100Hz"},
    "sqi_agc_off_blue": {   "commands":["plot radpd9","plot rsqi","loadAdpdCfg 43","reg w adpd4000 0x0D:0x2710","clockCalibration 6","SQISetSlot 9","sensor sqi start","sub rsqi add","adpdAGCControl 4:0","sensor adpd4000 start","sub radpd9 add"],
                "help":"Starts the SQI with Blue LED on slot I of ADPD4000 at 100Hz"},
    "start_log_adp": {   "commands":["fs_sub radp add", "fs_log start"],
                "help":"Starts logging the Battery info."},
    "stop_log_adp": {   "commands":["fs_sub radp remove","fs_log stop"],
                "help":"Stops logging the Battery info."},
    "start_log_adxl": {   "commands":["fs_sub radxl add","sensor adxl start","fs_log start"],
                "help":"Starts logging the ADXL."},
    "start_reg_read_adxl": {   "commands":["reg r adxl 0x00","reg r adxl 0x01","reg r adxl 0x02",
                                           "reg r adxl 0x03","reg r adxl 0x08","reg r adxl 0x09",
                                           "reg r adxl 0x0A","reg r adxl 0x0B","reg r adxl 0x0C","reg r adxl 0x0D","reg r adxl 0x0E","reg r adxl 0x0F",
                                           # "reg r adxl 0x0A","reg r adxl 0x0B","reg r adxl 0x0C","reg r adxl 0x0D","reg r adxl 0x0E","reg r adxl 0x0F",
                                           "reg r adxl 0x10","reg r adxl 0x11","reg r adxl 0x12","reg r adxl 0x13","reg r adxl 0x14","reg r adxl 0x15",
                                           "reg r adxl 0x16","reg r adxl 0x17","reg r adxl 0x1F","reg r adxl 0x20","reg r adxl 0x21","reg r adxl 0x22",
                                           "reg r adxl 0x23","reg r adxl 0x24","reg r adxl 0x25","reg r adxl 0x26","reg r adxl 0x27","reg r adxl 0x28",
                                           "reg r adxl 0x29","reg r adxl 0x2A","reg r adxl 0x2B","reg r adxl 0x2C","reg r adxl 0x2D","reg r adxl 0x2E",
                                           ],
                "help":"Starts logging the ADXL."},                
    "start_log_adxl_252": {   "commands":["setDateTime","getDateTime",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x98","delay 20","quickstop stop_log_adxl",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x99","delay 20","quickstop stop_log_adxl",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x9A","delay 20","quickstop stop_log_adxl",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x9B","delay 20","quickstop stop_log_adxl",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x9C","delay 20","quickstop stop_log_adxl",
                            "quickstart start_log_adxl","reg w adxl 0x2C:0x9D","delay 20","quickstop stop_log_adxl"
                            # "quickstart adxl","delay 20","quickstop adxl"
                            ],
                "help":"Takes 1 min log of ADXL in the external trigger mode."},
                
    "start_log_ext_adxl": {   "commands":["fs_sub radxl add","sensor adxl start","fs_log start","delay 60","reg w adxl 0x2C:0x98", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x99","delay 60", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x9A","delay 60", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x9B","delay 60", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x9C","delay 60", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x9D","delay 60", "sensor adxl stop",
                            "sensor adxl start","reg w adxl 0x2C:0x9E","delay 60", "sensor adxl stop",
                            "fs_sub radxl remove", "fs_log stop"],
                "help":"Starts logging the ADXL."},                
    "start_log_adpd4000_g": {   "commands":["loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1", "fs_sub radpd6 add","sensor adpd4000 start","fs_log start"],
                "help":"Starts logging the ADPD4000."},
    "start_log_adpd4000_r": {   "commands":["loadAdpdCfg 41", "clockCalibration 6",  "adpdAGCControl 2:1", "fs_sub radpd7 add","sensor adpd4000 start","fs_log start"],
                "help":"Starts logging the ADPD4000."},
    "start_log_adpd4000_ir": {   "commands":["loadAdpdCfg 42", "clockCalibration 6",  "adpdAGCControl 3:1", "fs_sub radpd8 add","sensor adpd4000 start","fs_log start"],
                "help":"Starts logging the ADPD4000."},
    "start_log_adpd4000_b": {   "commands":["loadAdpdCfg 43", "clockCalibration 6",  "adpdAGCControl 4:1", "fs_sub radpd9 add","sensor adpd4000 start","fs_log start"],
                "help":"Starts logging the ADPD4000."},
    "start_log_ppg": {   "commands":["loadAdpdCfg 40", "clockCalibration 6","setPpgLcfg 40","fs_sub rppg add","sensor ppg start","fs_log start"],
                "help":"Starts logging the PPG."},
    "start_log_temperature": {   "commands":["create_adpd4k_dcfg 4:2 5:3", "loadAdpdCfg 40", "fs_sub rtemperature add", "sensor temperature start","fs_log start"],
                "help":"Start Temperature"},
    "start_log_ecg": {   "commands":["lcfgEcgWrite 0:100","fs_sub recg add","sensor ecg start","fs_log start"],
                "help":"Start ecg"},
    "start_log_ped": {"commands":["fs_sub rped add","sensor adxl start", "sensor ped start","fs_log start"],
                "help":"Starts the Pedometer. logging"},
    "start_log_adpd4000_r_adxl": {"commands":["loadAdpdCfg 41", "clockCalibration 6","fs_sub radpd7 add","fs_sub radxl add","fs_log start","sensor adpd4000 start","sensor adxl start"],
                    "help":"Starts the ADPD4000_r, ADXL logging"},
    #"start_log_mv_uc1": {   "commands":["loadAdpdCfg 40", "reg w adpd4000 0x0D:0x07D0", "clockCalibration 6","fs_sub radpd6 add","fs_sub radxl add","fs_sub rtemperature add","adpdAGCControl 1:1","sensor adpd4000 start","sensor adxl start","sensor temperature start","fs_log start"],
    #"start_log_mv_uc1": {   "commands":["loadAdpdCfg 40", "reg w adpd4000 0x0D:0x2710", "clockCalibration 6","fs_sub radpd6 add","fs_sub radxl add","fs_sub rtemperature add","adpdAGCControl 1:1","sensor adpd4000 start","sensor adxl start","sensor temperature start","fs_log start"],
    "start_log_mv_uc1": {   "commands":["loadAdpdUCDcfg 1 dvt1", "clockCalibration 6","fs_sub radpd6 add","fs_sub radxl add","fs_sub rtemperature add","adpdAGCControl 1:1","sensor adpd4000 start","sensor adxl start","sensor temperature start","fs_log start"],
                "help":"Starts logging for MV UC1 - Adpd@500Hz, Adxl, Temperature"},
    #"start_log_mv_uc2": {   "commands":["loadAdpdCfg 40", "reg w adpd4000 0x0D:0x2710", "clockCalibration 6","fs_sub radpd6 add","fs_sub radxl add","fs_sub reda add","fs_sub rtemperature add","lcfgEdaWrite 0:30","sensor eda start","sensor adxl start","adpdAGCControl 1:1","sensor adpd4000 start","sensor temperature start","fs_log start"],
    "start_log_mv_uc2": {   "commands":["loadAdpdUCDcfg 2 dvt1","clockCalibration 6","fs_sub radpd6 add","fs_sub radxl add","fs_sub reda add","fs_sub rtemperature add","lcfgEdaWrite 0:30","sensor eda start","sensor adxl start","adpdAGCControl 1:1","sensor adpd4000 start","sensor temperature start","fs_log start"],
                "help":"Starts logging for MV UC2 - Eda@30Hz, Adxl, Adpd@100Hz, Temperature"},
    #"start_log_mv_uc3": {   "commands":["fs_sub radpd6 add","fs_sub radxl add","fs_sub recg add","fs_sub rtemperature add","lcfgEcgWrite 0:250","sensor ecg start","loadAdpdCfg 40", "clockCalibration 6","reg w adpd4000 0x0D:0x2710","adpdAGCControl 1:1","sensor adpd4000 start","sensor adxl start","sensor temperature start","fs_log start"],
    "start_log_mv_uc3": {   "commands":["fs_sub radpd6 add","fs_sub radxl add","fs_sub recg add","fs_sub rtemperature add","lcfgEcgWrite 0:250","sensor ecg start","loadAdpdUCDcfg 3 dvt1", "clockCalibration 6","reg w adpd4000 0x0D:0x2710","adpdAGCControl 1:1","sensor adpd4000 start","sensor adxl start","sensor temperature start","fs_log start"],
                "help":"Starts logging for MV UC3 - Ecg@250Hz, Adpd@100Hz, Adxl, Temperature"},
    #"start_log_mv_uc4": {   "commands":["fs_sub rppg add","fs_sub recg add","fs_sub rtemperature add","lcfgEcgWrite 0:1000","sensor ecg start","loadAdpdCfg 40", "clockCalibration 6","setPpgLcfg 40","sensor ppg start","sensor temperature start","fs_log start"],
    "start_log_mv_uc4": {   "commands":["fs_sub rppg add","fs_sub recg add","fs_sub rtemperature add","lcfgEcgWrite 0:1000","sensor ecg start","loadAdpdUCDcfg 4 dvt1", "clockCalibration 6","setPpgLcfg 40","sensor ppg start","sensor temperature start","fs_log start"],
                "help":"Starts logging for MV UC4 - Ecg@1000Hz, ppg, Temperature"},
    #"start_log_mv_uc5": {   "commands":["loadAdpdCfg 44","reg w adpd4000 0x0D:0x2710","clockCalibration 6","fs_sub radpd6 add","fs_sub radpd7 add","fs_sub radpd8 add","fs_sub radpd9 add","fs_sub radxl add","adpdAGCControl 0:1","sensor adpd4000 start","sensor adxl start","fs_log start"],
    "start_log_mv_uc5": {   "commands":["loadAdpdUCDcfg 5 dvt1","reg w adpd4000 0x0D:0x2710","clockCalibration 6","fs_sub radpd6 add","fs_sub radpd7 add","fs_sub radpd8 add","fs_sub radpd9 add","fs_sub radxl add","adpdAGCControl 0:1","sensor adpd4000 start","sensor adxl start","fs_log start"],
                "help":"Starts logging for MV UC5 - 4 LED Slots at 100Hz, Adxl"},
    "ppg": {    "commands":["loadAdpdCfg 40", "clockCalibration 6","setPpgLcfg 40", "sensor ppg start", "sub rppg add"],
               "help":"Starts the PPG library with the default LCFG and DCFG."},
    "periodic_ppg": {    "commands":["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 6 0x000F001E", "sensor ppg start", "sub rppg add"],
               "help":"Starts Duty cycle based periodic PPG, Ton=15sec,Toff=30sec"},
    "ecg": {"commands": ["lcfgEcgWrite 0:100", "sensor ecg start", "sub recg add"],
                 "help": "Start ECG"},
    "ecg_dcb": {"commands":  ["set_ecg_dcb_lcfg", "sensor ecg start", "sub recg add"],
                 "help": "Start ECG, writes lcfg from DCB if present"},
    "eda": {"commands":["lcfgEdaWrite 0:4", "sensor eda start", "sub reda add"],
                "help":"Starts the eda."},
    "tst_issue_330_1": {"commands":["fs_log start","fs_log stop"],
                "help":"Open and close "}, 
    "tst_issue_330_2": {"commands":["pattern_write 16384 0 1 1 1"],
               "help":"pattern write with 2 pages "},                                     
    "eda_4": {"commands":["lcfgEdaWrite 0:4","sensor eda start","sub reda add"],
                "help":"Starts the eda."},
    "eda_8": {"commands":["lcfgEdaWrite 0:8","sensor eda start","sub reda add"],
                "help":"Starts the eda."},
    "eda_16": {"commands":["lcfgEdaWrite 0:16","sensor eda start","sub reda add"],
                "help":"Starts the eda."},
    "eda_25": {"commands":["lcfgEdaWrite 0:25","sensor eda start","sub reda add"],
                "help":"Starts the eda."},
    "eda_30": {"commands":["lcfgEdaWrite 0:30","sensor eda start","sub reda add"],
                "help":"Starts the eda."},  
    "use_case_2": {"commands":["quickstart start_log_eda","quickstart start_log_adxl","quickstart start_log_adpd4000_g","quickstart start_log_temperature"],
                "help":"Starts the use case 2."},                                                                                                                    
    "eda_dcb": {"commands":["set_eda_dcb_lcfg","sub reda add","sensor eda start"],
                "help":"Starts the eda, writes the lcfg from DCB if present"},
    "plot-eda": {"commands":["quickstart eda", "plot reda"],
                "help":"Quickstarts the EDA and starts a plot of the EDA data."},
    "start_log_eda": {"commands":["fs_sub reda add","sensor eda start","delay 5","fs_log start"],
                "help":"Start eda"},
    "bcm": {"commands": ["lcfgBcmWrite 0:4","sensor bcm start","sub rbcm add"],
                "help":"Starts BCM."},
    "plot-bcm": {    "commands":["quickstart bcm", "plot rbcm"],
                "help":"Quickstarts the BCM and starts a plot of the BCM data."},
    "plot-ped": {    "commands":["quickstart ped", "plot rped"],
                "help":"Quickstarts the Pedometer and starts a plot of the Ped count data."},            
    "start_log_bcm": {"commands":["fs_sub rbcm add","sensor bcm start","fs_log start"],
                "help":"Start bcm"},
    "start_stop_adpd4k": {"commands":["quickstart adpd4000","quickstop adpd4000"],
                "help":"Start - stop tests addp4k "},                
    "start_stop_230": {"commands":["quickstart adxl","quickstart adpd4000","quickstart eda", "sensor temperature start","sub  rtemperature add","quickstop adxl","quickstop adpd4000","quickstop eda", "sub  rtemperature remove","sensor temperature stop"],
                "help":"Start - stop tests addp4k "},
    "mv_uc1_streaming_start": {"commands":["loadAdpdCfg 40","reg w adpd4000 0x0D:0x07D0","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add", "delay 1", "sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add"],
                "help":"Start MV UC1 streaming "},
    "adpd500Hz_stream_start_stop": {"commands":["loadAdpdCfg 40","reg w adpd4000 0x0D:0x07D0","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add", "delay 2", "quickstop adpd4000"],
                "help":"Start adpd at 500Hz streaming "},
    "mv_uc1_245_issue": {"commands":["write_dcb_config adpd4000 UseCase1.dcfg","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop","delete_dcb_config adpd4000","sensor adpd4000 start","sensor adpd4000 stop","loadAdpdCfg 40","reg w adpd4000 0x0D:0x07D0","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop"],
                "help":"Start MV UC1 245 issue commands"},
    "mv_uc1_245_issue_200Hz": {"commands":["write_dcb_config adpd4000 UseCase1_200Hz.dcfg","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop","delete_dcb_config adpd4000","sensor adpd4000 start","sensor adpd4000 stop","loadAdpdCfg 40","reg w adpd4000 0x0D:0x1388","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop"],
                "help":"Start MV UC1 245 issue commands"},
    "mv_uc1_245_issue_300Hz": {"commands":["write_dcb_config adpd4000 UseCase1_300Hz.dcfg","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop","delete_dcb_config adpd4000","sensor adpd4000 start","sensor adpd4000 stop","loadAdpdCfg 40","reg w adpd4000 0x0D:0x0D05","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop"],
                "help":"Start MV UC1 245 issue commands"},
    "mv_uc1_245_issue_wo_dcb": {"commands":["loadAdpdCfg 40","reg w adpd4000 0x0D:0x07D0","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop","loadAdpdCfg 40","reg w adpd4000 0x0D:0x07D0","clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","quickstart adpd_reg_tab_update","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add","delay 1","quickstop mv_uc1_streaming_stop"],
                "help":"Start MV UC1 245 issue commands"},
	 "adpd_dcb_test": {"commands":["write_dcb_config adpd4000 adpd4000_dcb_test1.dcfg","delay 4","read_dcb_config adpd4000","delay 4","compare_cfg_files adpd4000_dcb_test1.dcfg adpd4000_dcb_get.dcfg","delete_dcb_config adpd4000","delay 4"],
                "help":"Starts the adpd dcb test."},
    "adxl_dcb_test": {"commands":["write_dcb_config adxl adxl_dcb_test1.dcfg","delay 4","read_dcb_config adxl","delay 4","compare_cfg_files adxl_dcb_test1.dcfg adxl_dcb_get.dcfg","delete_dcb_config adxl","delay 4"],
                "help":"Starts the adxl dcb test."},
    "ppg_dcb_test": {"commands":["write_dcb_config ppg ppg_dcb.lcfg","delay 4","read_dcb_config ppg","delay 4","compare_cfg_files ppg_dcb.lcfg ppg_dcb_get.lcfg","delete_dcb_config ppg","delay 4"],
                "help":"Starts the ppg dcb test."},
    "ecg_dcb_test": {"commands":["write_dcb_config ecg ecg_dcb.lcfg","delay 2","read_dcb_config ecg","delay 2","compare_cfg_files ecg_dcb.lcfg ecg_dcb_get.lcfg","delete_dcb_config ecg","delay 4"],
                "help":"Starts the ecg dcb test."},    
    "eda_dcb_test": {"commands":["write_dcb_config eda eda_dcb.lcfg","delay 2","read_dcb_config eda","delay 2","compare_cfg_files eda_dcb.lcfg eda_dcb_get.lcfg","delete_dcb_config eda","delay 4"],
                "help":"Starts the eda dcb test."},      
    "dcb_test": {"commands":["quickstart adpd_dcb_test","quickstart adxl_dcb_test","quickstart ecg_dcb_test","quickstart eda_dcb_test"],
                "help":"Starts the dcb test."},  
    "temp_delete_dcb_test": {"commands":["delete_dcb_config adpd4000","create_adpd4k_dcfg 4:2 5:3","loadAdpdCfg 40","delay 5","sub rtemperature remove",
                                        "sensor temperature stop","reg r adpd4000 0x0170 0x0190"],
                "help":"Starts the temp del dcb test."},       
    "temp_write_dcb_test": {"commands":["write_dcb_config adpd4000 adpd4000_dcb_temp.dcfg","loadAdpdCfg 40","create_adpd4k_dcfg 4:2 5:3",
                                        "loadAdpdCfg 40","delay 5","sub rtemperature remove","sensor temperature stop","reg r adpd4000 0x0170 0x0190"],
                "help":"Starts the temp write dcb test."},    
    "temp_dcb_test": {"commands": ["quickstart temp_delete_dcb_test","quickstart temp_write_dcb_test","quickstart temp_delete_dcb_test"],  
                "help":"Starts the temp robot dcb test."},        
    "combined_dcb_write_adpd_test": {"commands": ["delete_dcb_config adpd4000","delay 2","loadAdpdCfg 40","clockCalibration 6","sub radpd6 add","delay 5","sub radpd6 remove","sensor adpd4000 stop","reg r adpd4000 0x01B0",
                                            "write_dcb_config adpd4000 adpd4000_dcb_test1.dcfg","delay 2","loadAdpdCfg 40","clockCalibration 6","sensor adpd4000 start","sub radpd6 add","delay 5","reg r adpd4000 0x01B0",
                                            "delay 2","sub radpd6 remove","sensor adpd4000 stop"],
                "help":"Starts the adpd robot combined test."}, 
    "combined_dcb_write_adxl_test": {"commands": ["delete_dcb_config adxl","delay 2","sensor adxl start","sub radxl add","delay 5","delay 4","reg r adxl 0x020 0x2C 0x2E","sub radxl remove","sensor adxl stop",
	                                        "write_dcb_config adxl adxl_dcb_test1.dcfg","delay 2","sensor adxl start","sub radxl add","delay 5","reg r adxl 0x20 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop"],
                "help":"Starts the adxl robot combined test."},  
    "combined_dcb_write_ecg_test": {"commands": ["delete_dcb_config ecg","set_ecg_dcb_lcfg","sensor ecg start","sub recg add","delay 10","sub recg remove","sensor ecg stop",
	                                        "write_dcb_config ecg ecg_dcb.lcfg","delay 2","set_ecg_dcb_lcfg","sensor ecg start","sub recg add","delay 10",
                                            "sub recg remove","sensor ecg stop","lcfgEcgRead 0"],
                "help":"Starts the ecg robot combined test."},   
    "combined_dcb_write_eda_test": {"commands": ["delete_dcb_config eda","set_eda_dcb_lcfg","sensor eda start","sub reda add","delay 10","sub recg remove","sensor eda stop","lcfgEdaRead 0","lcfgEdaRead 2",
	                                        "write_dcb_config eda eda_dcb.lcfg","delay 2","set_eda_dcb_lcfg","sensor eda start","sub reda add","delay 10",
                                            "sub reda remove","sensor eda stop","lcfgEdaRead 0","lcfgEdaRead 2"],
                "help":"Starts the eda robot combined test."},  
    "combined_dcb_write_ppg_test": {"commands": ["delete_dcb_config ppg","loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","sensor ppg start","sub rppg add","sub rppg remove","sensor ppg stop","lcfgPpgCheck 40 ppg_dcb.lcfg",
	                                        "write_dcb_config ppg ppg_dcb.lcfg","delay 5","loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","sensor ppg start","sub rppg add",
                                            "sub rppg remove","sensor ppg stop","lcfgPpgCheck 40 ppg_dcb.lcfg"],
                "help":"Starts the ppg robot combined test."},    
    "combined_dcb_test": {"commands": ["quickstart combined_dcb_adpd_test","quickstart combined_dcb_adxl_test","quickstart combined_dcb_ecg_test","quickstart combined_dcb_eda_test","quickstart combined_dcb_ppg_test"],
                                            "help":"Starts the robot combined test."},                  
    "combined_dcb_ad7156_test": {"commands": ["delete_dcb_config ad7156","delay 2","loadAd7156Cfg","reg r ad7156 0xA 0xB",
	                                        "write_dcb_config ad7156 ad7156_dcb_test.dcfg","delay 2","loadAd7156Cfg","reg r ad7156 0xA 0xB",
                                            "delete_dcb_config ad7156","delay 2","loadAd7156Cfg","reg r ad7156 0xA 0xB"],
                "help":"Starts the ad7156 robot combined test."},
    "combined_del_adpd_dcb_test": {"commands": ["delete_dcb_config adpd4000	","delay 2","loadAdpdCfg 40","clockCalibration 6","sensor adpd4000 start",
                                                "sub radpd6 add","delay 5","sub radpd6 remove","sensor adpd4000 stop","reg r adpd4000 0x01B0"],  
                    "help":"Starts the deletion adpd dcb robot combined test."},    
    "combined_del_adxl_dcb_test": {"commands": ["delete_dcb_config adxl","delay 2","sensor adxl start","sub radxl add","delay 5","reg r adxl 0x020 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop"],  
                    "help":"Starts the deletion adxl dcb robot combined test."},                 
    "combined_del_ppg_dcb_test": {"commands": ["delete_dcb_config ppg","loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40",
	                                        "sensor ppg start","sub rppg add","delay 5","sub rppg remove",
                                            "sensor ppg stop","lcfgPpgCheck 40 ppg_dcb.lcfg"],  
                    "help":"Starts the deletion ppg dcb robot combined test."},   
    "combined_del_ecg_dcb_test": {"commands": ["delete_dcb_config ecg","set_ecg_dcb_lcfg","sensor ecg start","sub recg add","delay 10",
                                            "sub recg remove","sensor ecg stop","lcfgEcgRead 0"],  
                    "help":"Starts the deletion ecg dcb robot combined test."},       
    "combined_del_eda_dcb_test": {"commands": ["delete_dcb_config eda","set_eda_dcb_lcfg","sensor eda start","sub reda add","delay 10",
                                            "sub reda remove","sensor eda stop","lcfgEdaRead 0","lcfgEdaRead 2"],  
                    "help":"Starts the deletion eda dcb robot combined test."},     
    "combined_dcb_adpd_test": {"commands": ["quickstart combined_dcb_write_adpd_test","quickstart combined_del_adpd_dcb_test"],  
                    "help":"Starts adpd dcb robot combined test."},   
    "combined_dcb_adxl_test": {"commands": ["quickstart combined_dcb_write_adxl_test","quickstart combined_del_adxl_dcb_test"],  
                    "help":"Starts adxl dcb robot combined test."},   
    "combined_dcb_ppg_test": {"commands": ["quickstart combined_dcb_write_ppg_test","quickstart combined_del_ppg_dcb_test"],  
                    "help":"Starts ppg dcb robot combined test."}, 
    "combined_dcb_ecg_test": {"commands": ["quickstart combined_dcb_write_ecg_test","quickstart combined_del_ecg_dcb_test"],  
                    "help":"Starts ecg dcb robot combined test."}, 
    "combined_dcb_eda_test": {"commands": ["quickstart combined_dcb_write_eda_test","quickstart combined_del_eda_dcb_test"],  
                    "help":"Starts eda dcb robot combined test."}, 
    "adxl_dcb_test_2_chunk1": {"commands": ["delete_dcb_config adxl","delay 2","sensor adxl start","sub radxl add","delay 4","reg r adxl 0x020 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop"],  
                    "help":"Starts the adxl dcb test 2 chunk1 robot test."},  
    "adxl_dcb_test_2_chunk2": {"commands": ["write_dcb_config adxl adxl_dcb_test1.dcfg","delay 2","sensor adxl start","sub radxl add","delay 5","reg r adxl 0x020 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop","delay 2"],  
                    "help":"Starts the adxl dcb test 2 chunk3 robot test."},  
    "adxl_dcb_test_2_chunk3": {"commands": ["write_dcb_config adxl adxl_dcb_test2.dcfg","delay 2","sensor adxl start","sub radxl add","delay 4","reg r adxl 0x020 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop"],  
                    "help":"Starts the adxl dcb test 2 chunk3 robot test."},   
    "adxl_dcb_test_2_chunk4": {"commands": ["delete_dcb_config adxl	","delay 2","sensor adxl start","sub radxl add","delay 4","reg r adxl 0x020 0x2C 0x2E",
                                            "sub radxl remove","sensor adxl stop"],  
                    "help":"Starts the adxl dcb test 2 chunk4 robot test."},
    "adxl_dcb_test_2": {"commands": ["quickstart adxl_dcb_test_2_chunk1","quickstart adxl_dcb_test_2_chunk2","quickstart adxl_dcb_test_2_chunk3","quickstart adxl_dcb_test_2_chunk4"],  
                    "help":"Starts the adxl dcb test 2 robot test."},    
    "ppg_status_check_robot_test": {"commands": ["sensor ppg start","sub rppg add","sub rppg remove","sensor ppg stop","status ppg","sub rppg add","sub rppg add","sub rppg add",
                                                "status ppg","sub rppg remove","sub rppg remove","status ppg","sub rppg remove","status ppg","sensor ppg start","sensor ppg start",
                                                "sensor ppg start","status ppg","sensor ppg stop","sensor ppg stop","status ppg","sensor ppg stop","status ppg"],
                    "help":"Starts the ppg robot test."},                                                                                                                                                                                             
    #"adpd_reg_tab_update": {"commands":["reg r adpd4000 0x00000 0x0001 0x0002 0x0003 0x0004 0x0005 0x0006 0x0007 0x0008 0x0009", "delay 1", "reg r adpd4000 0x000a 0x000b 0x000c 0x000d  0x000e 0x000F 0x0010 0x0011 0x0012 0x0013", "delay 1", "reg r adpd4000 0x0014 0x0015 0x0016 0x0017 0x0018 0x0019 0x001a 0x001b 0x001c 0x001d", "delay 1", "reg r adpd4000 0x001E 0x0020 0x0021 0x0022 0x0023 0x0024 0x0025 0x002E 0x002F 0x0030", "delay 1", "reg r adpd4000 0x0031 0x0032 0x0033 0x0034 0x0035 0x0036 0x0037 0x0038 0x0039 0x003A", "delay 1", "reg r adpd4000 0x003B 0x003C 0x003D 0x003E 0x003F 0x0040 0x0041 0x0042 0x0043 0x0044", "delay 1", "reg r adpd4000 0x0045 0x0046 0x0047 0x0048 0x0049 0x004A 0x004B 0x004C 0x004D 0x004E", "delay 1", "reg r adpd4000 0x004F 0x0050 0x0051 0x0052 0x0053 0x0054 0x0055 0x0056 0x0057 0x0058", "delay 1", "reg r adpd4000 0x0059 0x005A 0x005B 0x005C 0x005D 0x005E 0x005F 0x0060 0x0061 0x0062", "delay 1", "reg r adpd4000 0x0063 0x0064 0x0065 0x0066 0x0067 0x0068 0x0069 0x006A 0x006B 0x006C", "delay 1", "reg r adpd4000 0x006D 0x006E 0x006F 0x0070 0x0071 0x0072 0x0073 0x0074 0x0075 0x0076", "delay 1", "reg r adpd4000 0x0077 0x0078 0x0079 0x007A 0x007B 0x007C 0x007D 0x007E 0x007F 0x0080", "delay 1", "reg r adpd4000 0x0081 0x0082 0x0083 0x0084 0x0085 0x0086 0x0087 0x0088 0x0089 0x008A", "delay 1", "reg r adpd4000 0x008B 0x008C 0x008D 0x008E 0x008F 0x00A0 0x00A1 0x00A2 0x00A3 0x00A4", "delay 1", "reg r adpd4000 0x00A5 0x00A6 0x00A7 0x00A8 0x00A9 0x00AA 0x00AB 0x00AC 0x00AD 0x00AE", "delay 1", "reg r adpd4000 0x00AF 0x00B0 0x00B1 0x00B2 0x00B3 0x00B4 0x00B5 0x00B6 0x00B7 0x00B8", "delay 1", "reg r adpd4000 0x0100 0x0101 0x0102 0x0103 0x0104 0x0105 0x0106 0x0107 0x0108 0x0109", "delay 1", "reg r adpd4000 0x010A 0x010B 0x010C 0x010D 0x010E 0x010F 0x0110 0x0111 0x0112 0x0113" , "reg r adpd4000 0x0114 0x0115 0x0116 0x0117 0x0120 0x0121 0x0122 0x0123 0x0124 0x0125", "delay 1", "reg r adpd4000 0x0126 0x0127 0x0128 0x0129 0x012A 0x012B 0x012C 0x012D 0x012E 0x012F", "delay 1", "reg r adpd4000 0x0130 0x0131 0x0132 0x0133 0x0134 0x0135 0x0136 0x0137 0x0140 0x0141", "delay 1", "reg r adpd4000 0x0142 0x0143 0x0144 0x0145 0x0146 0x0147 0x0148 0x0149 0x014A 0x014B", "delay 1", "reg r adpd4000 0x014C 0x014D 0x014E 0x014F 0x0150 0x0151 0x0152 0x0153 0x0154 0x0155", "delay 1", "reg r adpd4000 0x0156 0x0157 0x0160 0x0161 0x0162 0x0163 0x0164 0x0165 0x0166 0x0167", "delay 1", "reg r adpd4000 0x0168 0x0169 0x016A 0x016B 0x016C 0x016D 0x016E 0x016F 0x0170 0x0171", "delay 1", "reg r adpd4000 0x0172 0x0173 0x0174 0x0175 0x0176 0x0177 0x0180 0x0181 0x0182 0x0183", "delay 1", "reg r adpd4000 0x0184 0x0185 0x0186 0x0187 0x0188 0x0189 0x018A 0x018B 0x018C 0x018D", "delay 1", "reg r adpd4000 0x018E 0x018F 0x0190 0x0191 0x0192 0x0193 0x0194 0x0195 0x0196 0x0197", "delay 1", "reg r adpd4000 0x01A0 0x01A1 0x01A2 0x01A3 0x01A4 0x01A5 0x01A6 0x01A7 0x01A8 0x01A9", "delay 1", "reg r adpd4000 0x01AA 0x01AB 0x01AC 0x01AD 0x01AE 0x01AF 0x01B0 0x01B1 0x01B2 0x01B3", "delay 1", "reg r adpd4000 0x01B4 0x01B5 0x01B6 0x01B7 0x01C0 0x01C1 0x01C2 0x01C3 0x01C4 0x01C5", "delay 1", "reg r adpd4000 0x01C6 0x01C7 0x01C8 0x01C9 0x01CA 0x01CB 0x01CC 0x01CD 0x01CE 0x01CF", "delay 1", "reg r adpd4000 0x01D0 0x01D1 0x01D2 0x01D3 0x01D4 0x01D5 0x01D6 0x01D7 0x01E0 0x01E1", "delay 1", "reg r adpd4000 0x01E2 0x01E3 0x01E4 0x01E5 0x01E6 0x01E7 0x01E8 0x01E9 0x01EA 0x01EB", "delay 1", "reg r adpd4000 0x01EC 0x01ED 0x01EE 0x01EF 0x01F0 0x01F1 0x01F2 0x01F3 0x01F4 0x01F5", "delay 1", "reg r adpd4000 0x01F6 0x01F7 0x0200 0x0201 0x0202 0x0203 0x0204 0x0205 0x0206 0x0207", "delay 1", "reg r adpd4000 0x0208 0x0209 0x020A 0x020B 0x020C 0x020D 0x020E 0x020F 0x0210 0x0211", "delay 1", "reg r adpd4000 0x0212 0x0213 0x0214 0x0215 0x0216 0x0217 0x0220 0x0221 0x0222 0x0223", "delay 1", "reg r adpd4000 0x0224 0x0225 0x0226 0x0227 0x0228 0x0229 0x022A 0x022B 0x022C 0x022D", "delay 1", "reg r adpd4000 0x022E 0x022F 0x0230 0x0231 0x0232 0x0233 0x0234 0x0235 0x0236 0x0237", "delay 1", "reg r adpd4000 0x0240 0x0241 0x0242 0x0243 0x0244 0x0245 0x0246 0x0247 0x0248 0x0249", "delay 1", "reg r adpd4000 0x024A 0x024B 0x024C 0x024D 0x024E 0x024F 0x0250 0x0251 0x0252 0x0253", "delay 1", "reg r adpd4000 0x0254 0x0255 0x0256 0x0257 0x0260 0x0261 0x0262 0x0263 0x0264 0x0265", "delay 1", "reg r adpd4000 0x0266 0x0267 0x0268 0x0269 0x026A 0x026B 0x026C 0x026D 0x026E 0x026F", "delay 1", "reg r adpd4000 0x0270 0x0271 0x0272 0x0273 0x0274 0x0275 0x0276 0x0277"],
    "adpd_reg_tab_update": {"commands":["reg r adpd4000 0x00000 0x0001 0x0002 0x0003 0x0004 0x0005 0x0006 0x0007 0x0008 0x0009", "reg r adpd4000 0x000a 0x000b 0x000c 0x000d  0x000e 0x000F 0x0010 0x0011 0x0012 0x0013", "reg r adpd4000 0x0014 0x0015 0x0016 0x0017 0x0018 0x0019 0x001a 0x001b 0x001c 0x001d", "reg r adpd4000 0x001E 0x0020 0x0021 0x0022 0x0023 0x0024 0x0025 0x002E 0x002F 0x0030", "reg r adpd4000 0x0031 0x0032 0x0033 0x0034 0x0035 0x0036 0x0037 0x0038 0x0039 0x003A", "reg r adpd4000 0x003B 0x003C 0x003D 0x003E 0x003F 0x0040 0x0041 0x0042 0x0043 0x0044", "reg r adpd4000 0x0045 0x0046 0x0047 0x0048 0x0049 0x004A 0x004B 0x004C 0x004D 0x004E", "reg r adpd4000 0x004F 0x0050 0x0051 0x0052 0x0053 0x0054 0x0055 0x0056 0x0057 0x0058", "reg r adpd4000 0x0059 0x005A 0x005B 0x005C 0x005D 0x005E 0x005F 0x0060 0x0061 0x0062", "reg r adpd4000 0x0063 0x0064 0x0065 0x0066 0x0067 0x0068 0x0069 0x006A 0x006B 0x006C", "reg r adpd4000 0x006D 0x006E 0x006F 0x0070 0x0071 0x0072 0x0073 0x0074 0x0075 0x0076", "reg r adpd4000 0x0077 0x0078 0x0079 0x007A 0x007B 0x007C 0x007D 0x007E 0x007F 0x0080", "reg r adpd4000 0x0081 0x0082 0x0083 0x0084 0x0085 0x0086 0x0087 0x0088 0x0089 0x008A", "reg r adpd4000 0x008B 0x008C 0x008D 0x008E 0x008F 0x00A0 0x00A1 0x00A2 0x00A3 0x00A4", "reg r adpd4000 0x00A5 0x00A6 0x00A7 0x00A8 0x00A9 0x00AA 0x00AB 0x00AC 0x00AD 0x00AE", "reg r adpd4000 0x00AF 0x00B0 0x00B1 0x00B2 0x00B3 0x00B4 0x00B5 0x00B6 0x00B7 0x00B8", "reg r adpd4000 0x0100 0x0101 0x0102 0x0103 0x0104 0x0105 0x0106 0x0107 0x0108 0x0109", "reg r adpd4000 0x010A 0x010B 0x010C 0x010D 0x010E 0x010F 0x0110 0x0111 0x0112 0x0113" , "reg r adpd4000 0x0114 0x0115 0x0116 0x0117 0x0120 0x0121 0x0122 0x0123 0x0124 0x0125", "reg r adpd4000 0x0126 0x0127 0x0128 0x0129 0x012A 0x012B 0x012C 0x012D 0x012E 0x012F", "reg r adpd4000 0x0130 0x0131 0x0132 0x0133 0x0134 0x0135 0x0136 0x0137 0x0140 0x0141", "reg r adpd4000 0x0142 0x0143 0x0144 0x0145 0x0146 0x0147 0x0148 0x0149 0x014A 0x014B", "reg r adpd4000 0x014C 0x014D 0x014E 0x014F 0x0150 0x0151 0x0152 0x0153 0x0154 0x0155", "reg r adpd4000 0x0156 0x0157 0x0160 0x0161 0x0162 0x0163 0x0164 0x0165 0x0166 0x0167", "reg r adpd4000 0x0168 0x0169 0x016A 0x016B 0x016C 0x016D 0x016E 0x016F 0x0170 0x0171", "reg r adpd4000 0x0172 0x0173 0x0174 0x0175 0x0176 0x0177 0x0180 0x0181 0x0182 0x0183", "reg r adpd4000 0x0184 0x0185 0x0186 0x0187 0x0188 0x0189 0x018A 0x018B 0x018C 0x018D", "reg r adpd4000 0x018E 0x018F 0x0190 0x0191 0x0192 0x0193 0x0194 0x0195 0x0196 0x0197", "reg r adpd4000 0x01A0 0x01A1 0x01A2 0x01A3 0x01A4 0x01A5 0x01A6 0x01A7 0x01A8 0x01A9", "reg r adpd4000 0x01AA 0x01AB 0x01AC 0x01AD 0x01AE 0x01AF 0x01B0 0x01B1 0x01B2 0x01B3", "reg r adpd4000 0x01B4 0x01B5 0x01B6 0x01B7 0x01C0 0x01C1 0x01C2 0x01C3 0x01C4 0x01C5", "reg r adpd4000 0x01C6 0x01C7 0x01C8 0x01C9 0x01CA 0x01CB 0x01CC 0x01CD 0x01CE 0x01CF", "reg r adpd4000 0x01D0 0x01D1 0x01D2 0x01D3 0x01D4 0x01D5 0x01D6 0x01D7 0x01E0 0x01E1", "reg r adpd4000 0x01E2 0x01E3 0x01E4 0x01E5 0x01E6 0x01E7 0x01E8 0x01E9 0x01EA 0x01EB", "reg r adpd4000 0x01EC 0x01ED 0x01EE 0x01EF 0x01F0 0x01F1 0x01F2 0x01F3 0x01F4 0x01F5", "reg r adpd4000 0x01F6 0x01F7 0x0200 0x0201 0x0202 0x0203 0x0204 0x0205 0x0206 0x0207", "reg r adpd4000 0x0208 0x0209 0x020A 0x020B 0x020C 0x020D 0x020E 0x020F 0x0210 0x0211", "reg r adpd4000 0x0212 0x0213 0x0214 0x0215 0x0216 0x0217 0x0220 0x0221 0x0222 0x0223", "reg r adpd4000 0x0224 0x0225 0x0226 0x0227 0x0228 0x0229 0x022A 0x022B 0x022C 0x022D", "reg r adpd4000 0x022E 0x022F 0x0230 0x0231 0x0232 0x0233 0x0234 0x0235 0x0236 0x0237", "reg r adpd4000 0x0240 0x0241 0x0242 0x0243 0x0244 0x0245 0x0246 0x0247 0x0248 0x0249", "reg r adpd4000 0x024A 0x024B 0x024C 0x024D 0x024E 0x024F 0x0250 0x0251 0x0252 0x0253", "reg r adpd4000 0x0254 0x0255 0x0256 0x0257 0x0260 0x0261 0x0262 0x0263 0x0264 0x0265", "reg r adpd4000 0x0266 0x0267 0x0268 0x0269 0x026A 0x026B 0x026C 0x026D 0x026E 0x026F", "reg r adpd4000 0x0270 0x0271 0x0272 0x0273 0x0274 0x0275 0x0276 0x0277"],
                "help":"ADPD Reg Table update - 448 registers are updated"},   			
    #"start_config_log_test": {   "commands":[ "fs_config_log start","quickstart start_log_syncppg","quickstart start_log_ecg","quickstart start_log_eda","quickstart start_log_temperature","quickstop stop_log_syncppg","quickstop stop_log_ecg","quickstop stop_log_eda","quickstop stop_log_temperature","fs_config_log_file write","fs_config_log stop", "pm_activate_touch_sensor"],
    "start_config_log_test": {   "commands":[ "fs_config_log start","quickstart start_log_ecg","quickstop stop_log_ecg","fs_config_log_file write","fs_config_log stop", "pm_activate_touch_sensor"],
                "help":"Starts logging pre-defined user config data into DCFG sectors of LFS"},
    "config_write_test": {   "commands":[ "fs_config_log start","quickstart start_log_adpd4000_g","quickstop stop_log_adpd4000_g","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of pre-defined user config data into DCFG sectors of LFS"},
    "nand_config_file_create_mv_uc1": {   "commands":[ "fs_config_log start","quickstart start_log_mv_uc1", "quickstop stop_log_mv_uc1","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of MV UC1 log commands, as user config file into DCFG sectors of LFS"},
    "nand_config_file_create_mv_uc2": {   "commands":[ "fs_config_log start","quickstart start_log_mv_uc2", "quickstop stop_log_mv_uc2","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of MV UC2 log commands, as user config file into DCFG sectors of LFS"},
    "nand_config_file_create_mv_uc3": {   "commands":[ "fs_config_log start","quickstart start_log_mv_uc3", "quickstop stop_log_mv_uc3","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of MV UC3 log commands, as user config file into DCFG sectors of LFS"},
    "nand_config_file_create_mv_uc4": {   "commands":[ "fs_config_log start","quickstart start_log_mv_uc4", "quickstop stop_log_mv_uc4","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of MV UC4 log commands, as user config file into DCFG sectors of LFS"},
    "nand_config_file_create_mv_uc5": {   "commands":[ "fs_config_log start","quickstart start_log_mv_uc5", "quickstop stop_log_mv_uc5","fs_config_log_file write","fs_config_log stop"],
                "help":"Starts creation of MV UC5 log commands, as user config file into DCFG sectors of LFS"},
    "eda_freq_seq_test": {"commands":["quickstart eda_4","delay 7","quickstop eda","quickstart eda_8","delay 7","quickstop eda", "quickstart eda_4","delay 7","quickstop eda","quickstart eda_16","delay 7","quickstop eda", "quickstart eda_8","delay 7","quickstop eda", "quickstart eda_4","delay 7","quickstop eda", "quickstart eda_16","delay 7","quickstop eda"],
                "help":"Start EDA frquency change test: 4->8->4->16->8->4->16"},
    "gen_blk_dcb_file_create_test1": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_adpd4000_r_adxl", "quickstop stop_log_adpd4000_r_adxl", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with adpd4000_r_adxl log commands, that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_test2": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_eda", "quickstop stop_log_eda", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with EDA log commands, that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_test3": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_adpd4000_r", "quickstop stop_log_adpd4000_r", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with adpd4000_r log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_test4": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_ecg", "quickstop stop_log_ecg", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with ECG log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_mv_uc1": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_mv_uc1", "quickstop stop_log_mv_uc1", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with MV UC1 log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_mv_uc2": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_mv_uc2", "quickstop stop_log_mv_uc2", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with MV UC2 log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_mv_uc3": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_mv_uc3", "quickstop stop_log_mv_uc3", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with MV UC3 log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_mv_uc4": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_mv_uc4", "quickstop stop_log_mv_uc4", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with MV UC4 log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "gen_blk_dcb_file_create_mv_uc5": {   "commands":[ "create_gen_blk_dcb start", "quickstart start_log_mv_uc5", "quickstop stop_log_mv_uc5", "gen_blk_dcb_file_create write", "create_gen_blk_dcb stop"],
                "help":"Starts the creation of file with MV UC5 log commands,  that will be put in General Block DCB, which will be used for LT application; Copies this file to dcb_cfg folder as gen_blk_dcb.lcfg"},
    "start_dcb_low_touch_test": {   "commands":[ "quickstart gen_blk_dcb_file_create_test", "write_dcb_config low_touch gen_blk_dcb.lcfg", "pm_activate_touch_sensor"],
                "help":"Generate DCB file, write DCB, Start Low touch with DCB configurations"},
    "ppg_dark_test": {"commands":["delete_dcb_config adpd4000","write_dcb_config adpd4000 ppg_dark_test.dcfg","toggleSaveCSV","quickstart adpd4000","plot radpd6","delay 5","quickstop adpd4000","toggleSaveCSV","delete_dcb_config adpd4000"],
                "help":"Quickstarts the PPG dark test for 5 secs and saves the PPG data as CSV file"},
    "ppg_static_agc_dis_50Hz_test": {"commands":["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010 0x7 0x32", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg", "delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to disable STATIC AGC, run ADPD at 50Hz "},
    "ppg_static_agc_en_50Hz_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210 0x7 0x32", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to enable STATIC AGC, run ADPD at 50Hz "},
    "ppg_static_agc_en_100Hz_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210 0x7 0x64", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to enable STATIC AGC, run ADPD at 100Hz "},
    "ppg_static_agc_en_500Hz_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210 0x7 0x1F4", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to enable STATIC AGC, run ADPD at 500Hz "},
    "ppg_static_agc_dis_100Hz_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010 0x7 0x64", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to disable STATIC AGC, run ADPD at 100Hz "},
    "ppg_static_agc_dis_500Hz_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010 0x7 0x1F4", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 120", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to disable STATIC AGC, run ADPD at 500Hz "},
    "ppg_static_agc_en_50Hz_recalibrate_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210 0x7 0x32", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 5", "sensor ppg stop", "sensor ppg start", "delay 15", "quickstop ppg"],
                "help":"Quickstarts the PPG with changes in lcfg to enable STATIC AGC, run ADPD at 50Hz, run for 5secs, Do AGC recalibrate, run for 15 sec"},
    "ppg_static_agc_on": {"commands": ["setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210", "sensor ppg start", "sub rppg add", "plot rsyncppg","plot rppg"],
                "help":"Turns PPG static AGC ON in ppg lcfg- ppg app start to follow, after this command"},
    "ppg_static_agc_off": {"commands": ["setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add", "plot rsyncppg","plot rppg"],
                "help":"Turns PPG static AGC OFF in ppg lcfg- ppg app start to follow, after this command"},
    "ppg_static_agc_on_off_test": {"commands": ["loadAdpdCfg 40","clockCalibration 6","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1210", "sensor ppg start", "sub rppg add", "plot rppg", "plot rsyncppg","delay 10","sub rppg remove","sensor ppg stop","setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010","sub rppg add","sensor ppg start","delay 20","quickstop ppg"],
                "help":"Turns PPG static AGC ON, starts ppg app, stops ppg app, Turns PPG static AGC OFF, starts ppg app, stops ppg app"},
    "mwl_view": {"commands": ["loadAdpdCfg 44","clockCalibration 6", "adpdAGCControl 0:1", "sensor adpd4000 start", "sub radpd6 add","sub radpd7 add","sub radpd8 add","sub radpd9 add", "plot radpd6", "plot radpd7","plot radpd8","plot radpd9"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "mwl_view_agc_off": {"commands": ["loadAdpdCfg 44","clockCalibration 6","adpdAGCControl 0:0", "sensor adpd4000 start", "sub radpd6 add","sub radpd7 add","sub radpd8 add","sub radpd9 add","plot radpd6", "plot radpd7","plot radpd8","plot radpd9"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot F, G, H, I of ADPD4000, with static AGC OFF"},
###################################################################################################################
#### Commands for Slot Switching, to be used only when Watch is loaded with FW built with "SLOT_SELECT" macro ####
#################################################################################################################
    "adpd4000_g_A": {   "commands":["create_adpd4k_dcfg 1:4", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6","adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for green LED in slot-A, with Static AGC ON"},
    "adpd4000_g_B": {   "commands":["create_adpd4k_dcfg 2:4", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for green LED in slot-B, with Static AGC ON"},
    "adpd4000_r_A": {   "commands":["create_adpd4k_dcfg 1:5", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 2:1", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for red LED in slot-A, with Static AGC ON"},
    "adpd4000_r_B": {   "commands":["create_adpd4k_dcfg 2:5", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 2:1", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for red LED in slot-B, with Static AGC ON"},
    "adpd4000_ir_A": {   "commands":["create_adpd4k_dcfg 1:6", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 3:1", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for ir LED in slot-A, with Static AGC ON"},
    "adpd4000_ir_B": {   "commands":["create_adpd4k_dcfg 2:6", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 3:1", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for ir LED in slot-B, with Static AGC ON"},
    "adpd4000_b_A": {   "commands":["create_adpd4k_dcfg 1:7", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 4:1", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for blue LED in slot-A, with Static AGC ON"},
    "adpd4000_b_B": {   "commands":["create_adpd4k_dcfg 2:7", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x4E20", "clockCalibration 6", "adpdAGCControl 4:1", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for blue LED in slot-B, with Static AGC ON"},
    "adpd4000_g_A_agc_off": {   "commands":["create_adpd4k_dcfg 1:4", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for green LED in slot-A, with Static AGC OFF"},
    "adpd4000_g_B_agc_off": {   "commands":["create_adpd4k_dcfg 2:4", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for green LED in slot-B, with Static AGC OFF"},
    "adpd4000_r_A_agc_off": {   "commands":["create_adpd4k_dcfg 1:5", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for red LED in slot-A, with Static AGC OFF"},
    "adpd4000_r_B_agc_off": {   "commands":["create_adpd4k_dcfg 2:5", "loadAdpdCfg 40", "clockCalibration 6",  "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for red LED in slot-B, with Static AGC OFF"},
    "adpd4000_ir_A_agc_off": {   "commands":["create_adpd4k_dcfg 1:6", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for ir LED in slot-A, with Static AGC OFF"},
    "adpd4000_ir_B_agc_off": {   "commands":["create_adpd4k_dcfg 2:6", "loadAdpdCfg 40", "clockCalibration 6","reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for ir LED in slot-B, with Static AGC OFF"},
    "adpd4000_b_A_agc_off": {   "commands":["create_adpd4k_dcfg 1:7", "loadAdpdCfg 40", "clockCalibration 6","reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"starts adpd4k with the DCFG for blue LED in slot-A, with Static AGC OFF"},
    "adpd4000_b_B_agc_off": {   "commands":["create_adpd4k_dcfg 2:7", "loadAdpdCfg 40", "clockCalibration 6","reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start", "sub radpd2 add"],
                "help":"starts adpd4k with the DCFG for blue LED in slot-B, with Static AGC OFF"},
    "ppg_A": {"commands":["create_adpd4k_dcfg 1:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot A, with Static AGC ON"},
    "ppg_B": {"commands":["create_adpd4k_dcfg 2:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot B, with Static AGC ON"},
    "ppg_C": {"commands":["create_adpd4k_dcfg 3:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot C, with Static AGC ON"},
    "ppg_F": {"commands":["create_adpd4k_dcfg 6:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot F, with Static AGC ON"},
    "ppg_A_agc_off": {"commands":["create_adpd4k_dcfg 1:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot A"},
    "ppg_B_agc_off": {"commands":["create_adpd4k_dcfg 2:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot B"},
    "ppg_C_agc_off": {"commands":["create_adpd4k_dcfg 3:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot C"},
    "ppg_F_agc_off": {"commands":["create_adpd4k_dcfg 6:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add"],
                "help": "starts ppg in slot F"},
    "temp_AB": {   "commands":["create_adpd4k_dcfg 1:2 2:3", "loadAdpdCfg 40", "sensor temperature start", "sub rtemperature add"],
                "help":"Start Temperature in slot A&B"},
    "temp_BC": {   "commands":["create_adpd4k_dcfg 2:2 3:3", "loadAdpdCfg 40", "sensor temperature start", "sub rtemperature add"],
                "help":"Start Temperature in slot B&C"},
    "temp_DE": {   "commands":["create_adpd4k_dcfg 4:2 5:3", "loadAdpdCfg 40", "sensor temperature start", "sub rtemperature add"],
                "help":"Start Temperature in slot D&E"},
    "ecg4k_A": {   "commands":[ "controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0", "loadAdpdCfg 40","SetEcg4kLcfg 0:300", "clockCalibration 6", "sensor adpd4000 start", "sub radpd1 add"],
                "help":"Setup the ADPD for measuring ecg in slot A."},
    "ppg_temp_ABC_agc_off": {"commands":["create_adpd4k_dcfg 1:1 2:2 3:3", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add", "sensor temperature start", "sub rtemperature add"],
            "help": "starts ppg in Slot-A and temp using slot B and C"},
    "temp_ppg_ABC_agc_off": {"commands":["create_adpd4k_dcfg 1:2 2:3 3:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010",  "sensor ppg start", "sub rppg add", "sensor temperature start", "sub rtemperature add"],
            "help": "starts ppg in Slot-C and temp using slot A and B"},
    "ecg4k_ppg_temp_agc_off": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:1 3:2 4:3", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add", "delay 1","sensor adpd4000 start", "sub radpd1 add", "delay 1",  "sensor temperature start", "sub rtemperature add"],
            "help": "starts ecg in Slot A, ppg in Slot-B and temp using slot C and D"},
    "ecg4k_temp_ppg_agc_off": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:2 3:3 4:1", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add", "delay 1","sensor adpd4000 start", "sub radpd1 add", "delay 1",  "sensor temperature start", "sub rtemperature add"],
            "help": "starts ecg in Slot A, ppg in Slot-D and temp using slot B and C"},
    "ecg4k_temp": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:2 3:3", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "sensor adpd4000 start", "sub radpd1 add", "sensor temperature start", "sub rtemperature add"],
            "help": "starts ecg in Slot A, and temp using slot B and C"},
    "ecg4k_ppg_agc_off": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:1", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40","lcfgPpgWrite 0x4 0x1010", "sensor ppg start", "sub rppg add", "sensor adpd4000 start", "sub radpd1 add"],
            "help": "starts ecg in Slot A and ppg in Slot-B"},
    "ppg_temp_ABC": {"commands":["create_adpd4k_dcfg 1:1 2:2 3:3", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add", "sensor temperature start", "sub rtemperature add"],
            "help": "starts ppg in Slot-A and temp using slot B and C"},
    "temp_ppg_ABC": {"commands":["create_adpd4k_dcfg 1:2 2:3 3:1", "loadAdpdCfg 40", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add", "sensor temperature start", "sub rtemperature add"],
            "help": "starts ppg in Slot-C and temp using slot A and B"},
    "ecg4k_ppg_temp": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:1 3:2 4:3", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add", "delay 1","sensor adpd4000 start", "sub radpd1 add", "delay 1",  "sensor temperature start", "sub rtemperature add"],
            "help": "starts ecg in Slot A, ppg in Slot-B and temp using slot C and D"},
    "ecg4k_temp_ppg": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 1:0 2:2 3:3 4:1", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add", "delay 1","sensor adpd4000 start", "sub radpd1 add", "delay 1",  "sensor temperature start", "sub rtemperature add"],
            "help": "starts ecg in Slot A, ppg in Slot-D and temp using slot B and C"},
    "ecg4k_ppg": {"commands":["controlECGElectrodeSwitch 4k_sw 1", "create_adpd4k_dcfg 0:0 1:1", "loadAdpdCfg 40", "SetEcg4kLcfg 0:300", "clockCalibration 6", "setPpgLcfg 40", "sensor ppg start", "sub rppg add", "sensor adpd4000 start", "sub radpd1 add"],
            "help": "starts ecg in Slot A and ppg in Slot-B"},
    "mwl_view_ABCD": {"commands": ["create_adpd4k_dcfg 1:4 2:5 3:6 4:7", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20","sensor adpd4000 start", "sub radpd1 add","sub radpd2 add","sub radpd3 add","sub radpd4 add", "plot radpd1", "plot radpd2","plot radpd3","plot radpd4"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot A, B, C, D of ADPD4000"},
    "mwl_view_ABCD_agc_off": {"commands": ["create_adpd4k_dcfg 1:4 2:5 3:6 4:7", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "adpdAGCControl 0:0", "sensor adpd4000 start","sub radpd1 add","sub radpd2 add","sub radpd3 add","sub radpd4 add", "plot radpd1", "plot radpd2","plot radpd3","plot radpd4"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot A, B, C, D of ADPD4000"},
    "mwl_view_FGHI": {"commands": ["create_adpd4k_dcfg 6:4 7:5 8:6 9:7", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "sensor adpd4000 start","sub radpd6 add","sub radpd7 add","sub radpd8 add","sub radpd9 add", "plot radpd6", "plot radpd7","plot radpd8","plot radpd9"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "mwl_view_FGHI_agc_off": {"commands": ["create_adpd4k_dcfg 6:4 7:5 8:6 9:7", "loadAdpdCfg 40", "clockCalibration 6", "reg w adpd4000 0x0D:0x4E20", "adpdAGCControl 0:0", "sensor adpd4000 start","sub radpd6 add","sub radpd7 add","sub radpd8 add","sub radpd9 add", "plot radpd6", "plot radpd7","plot radpd8","plot radpd9"],
                "help":"Opens MWL view with Green, Red, IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "uc_hr_enab_adpd50_adxl50": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "sensor adxl start","sub radxl add"],
                "help":"Starts UC HR enable test"},
    "uc_hr_enab_adpd100_adxl50": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x2710", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "quickstart adxl"],
                "help":"Starts UC HR enable test"},
    "uc_hr_enab_adpd500_adxl50": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x07D0", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "quickstart adxl"],
                "help":"Starts UC HR enable test"},
    "uc_hr_enab_adpd50_adxl100": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "quickstart adxl","reg w adxl 0x2C:0x9B"],
                "help":"Starts UC HR enable test"},
    "uc_hr_enab_adpd500_adxl100": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x07D0", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "quickstart adxl","reg w adxl 0x2C:0x9B"],
                "help":"Starts UC HR enable test"},
    "uc_hr_enab_adpd100_adxl100": {"commands": ["setPpgLcfg 40","sub rppg add", "setUCHREnab 1 6", "plot rppg", "plot radpd6", "plot radxl", "loadAdpdCfg 40", "reg w adpd4000 0x0D:0x02710", "clockCalibration 6", "adpdAGCControl 1:1", "sensor adpd4000 start", "sub radpd6 add", "quickstart adxl","reg w adxl 0x2C:0x9B"],
                "help":"Starts UC HR enable test"},
    "start_stream_mv_uc1": {   "commands":["loadAdpdUCDcfg 1 dvt1", "clockCalibration 6","adpdAGCControl 1:1","sensor adpd4000 start","sub radpd6 add","sensor adxl start","sub radxl add","sensor temperature start","sub rtemperature add",],
                "help":"Starts streaming for MV UC1 - Adpd@500Hz, Adxl, Temperature"},
    "start_stream_mv_uc4_1": {   "commands":["lcfgEcgWrite 0:1000","sensor ecg start","loadAdpdUCDcfg 4 dvt1", "clockCalibration 6","setPpgLcfg 40","sensor ppg start","sensor temperature start","sub rppg add","sub recg add","sub rtemperature add"],
                "help":"Start MV UC4 start stream cmd sequence"},
    "start_log_mv_uc4_1": {   "commands":["fs_log start","fs_sub rppg add","fs_sub recg add","fs_sub rtemperature add"],
                "help":"Start MV UC4 start log cmd sequence, in b/w streaming"},}

    # A dictionary of useful/common command sequences to be executed for stopping applications/sensors. The 'commands' key contains a list of CLI commands to be run for the sequence.
    quickstops = {
    "adpd4000": {   "commands":["sub radpd6 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "ecg4000": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0"],
                "help":"Stops and Unsubscribes the ADPD that is measuring ecg in slot A."},
    "adpd4000_g": {   "commands":["sub radpd6 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_r": {   "commands":["sub radpd7 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_ir": {   "commands":["sub radpd8 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_b": {   "commands":["sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_agc_off": {   "commands":["sub radpd6 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_r_agc_off": {   "commands":["sub radpd7 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_ir_agc_off": {   "commands":["sub radpd8 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_b_agc_off": {   "commands":["sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_r": {   "commands":["sub radpd6 remove", "sub radpd7 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_ir": {   "commands":["sub radpd6 remove", "sub radpd8 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_r_ir": {   "commands":["sub radpd7 remove", "sub radpd8 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_b": {   "commands":["sub radpd6 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_r_b": {   "commands":["sub radpd7 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_ir_b": {   "commands":["sub radpd8 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_r_ir": {   "commands":["sub radpd6 remove", "sub radpd7 remove", "sub radpd8 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_r_ir_b": {   "commands":["sub radpd7 remove", "sub radpd8 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_r_b": {   "commands":["sub radpd6 remove", "sub radpd7 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adpd4000_g_ir_b": {   "commands":["sub radpd6 remove", "sub radpd8 remove", "sub radpd9 remove", "sensor adpd4000 stop"],
                "help":"Stops the ADPD application and unsubscribes it."},
    "adxl": {"commands":["sub radxl remove", "sensor adxl stop"],
                "help":"Stop ADXL"},
    "use_case_2": {"commands":["quickstop stop_log_eda","quickstop stop_log_adxl","quickstop stop_log_temperature","quickstop stop_log_adpd4000_g",],
                "help":"Stops the use case 2."},
    "mv_uc1_streaming_stop": {"commands":["sub rtemperature remove","sensor temperature stop","sub radxl remove","sensor adxl stop","sub radpd6 remove","sensor adpd4000 stop"],
                "help":"Stop MV UC1 streaming "},				
    "temperature": {   "commands":["sub rtemperature remove", "sensor temperature stop"],
                "help":"Stop Temperature"},
    "ped": {   "commands":["sensor ped stop", "sub rped remove", "sensor adxl stop"],
                "help":"Stops the Pedometer application, unsubscribes it and disables the ADXL sensor."},
    "sqi_green": {   "commands":["sub radpd6 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor running @100Hz"},
    "sqi_green_50": {   "commands":["sub radpd6 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor running @50Hz"},
    "sqi_green_25": {   "commands":["sub radpd6 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor running @25Hz"},
    "sqi_ppg": {   "commands":["sub rppg remove","sensor ppg stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI & PPG application, & un-subscribes them"},
    "stop_log_sqi_green": {   "commands":["fs_sub radpd6 remove", "fs_sub rsqi remove", "sensor adpd4000 stop", "sensor sqi stop","fs_log stop"],
                "help":"log the SQI data with Green LED on slot F of ADPD4000 at 100Hz"},
    "sqi_agc_off_green": {   "commands":["sub radpd6 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "sqi_red": {   "commands":["sub radpd7 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "stop_log_sqi_red": {   "commands":["fs_sub radpd7 remove", "fs_sub rsqi remove", "sensor adpd4000 stop", "sensor sqi stop","fs_log stop"],
                "help":"Stops the logging of SQI data with Red LED on slot G of ADPD4000 at 100Hz"},
    "sqi_agc_off_red": {   "commands":["sub radpd7 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "sqi_ir": {   "commands":["sub radpd8 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "stop_log_sqi_ir": {   "commands":["fs_sub radpd8 remove", "fs_sub rsqi remove", "sensor adpd4000 stop", "sensor sqi stop","fs_log stop"],
                "help":"log the SQI data with IR LED on slot H of ADPD4000 at 100Hz"},
    "sqi_agc_off_ir": {   "commands":["sub radpd8 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "sqi_blue": {   "commands":["sub radpd9 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "stop_log_sqi_blue": {   "commands":["fs_sub radpd9 remove", "fs_sub rsqi remove", "sensor adpd4000 stop", "sensor sqi stop","fs_log stop"],
                "help":"log the SQI data with Blue LED on slot I of ADPD4000 at 100Hz"},
    "sqi_agc_off_blue": {   "commands":["sub radpd9 remove","sensor adpd4000 stop","sub rsqi remove","sensor sqi stop"],
                "help":"Stops the SQI application, un-subscribes it and disables the ADPD sensor."},
    "stop_log_adxl": {"commands":["fs_sub radxl remove", "sensor adxl stop","fs_log stop"],
                "help":"Stops logging the ADXL"},
    "stop_log_adp": {"commands": ["fs_sub radp remove", "fs_log stop"],
                         "help": "Stops logging the Battery info."},
    "ppg": {    "commands":["sub rppg remove", "sensor ppg stop"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "periodic_ppg": {    "commands":["sub rppg remove", "sensor ppg stop"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "stop_log_adpd4000_g": {   "commands":["sensor adpd4000 stop","fs_sub radpd6 remove","fs_log stop"],
                "help":"Starts logging the ADPD."},
    "stop_log_adpd4000_r": {   "commands":["sensor adpd4000 stop","fs_sub radpd7 remove","fs_log stop"],
                "help":"Starts logging the ADPD."},
    "stop_log_adpd4000_ir": {   "commands":["sensor adpd4000 stop","fs_sub radpd8 remove","fs_log stop"],
                "help":"Starts logging the ADPD."},
    "stop_log_adpd4000_b": {   "commands":["sensor adpd4000 stop","fs_sub radpd9 remove","fs_log stop"],
                "help":"Starts logging the ADPD."},
    "stop_log_ppg": {   "commands":["sensor ppg stop","fs_sub rppg remove","fs_log stop"],
                "help":"Starts logging the ADPD."},
    "stop_log_temperature": {   "commands":["sensor temperature stop","fs_sub rtemperature remove","fs_log stop"],
                "help":"Stop Temperature"},
    "stop_log_ecg": {   "commands":["sensor ecg stop","fs_sub recg remove","fs_log stop"],
                "help":"Stop ecg"},
    "ecg": {"commands": ["sub recg remove", "sensor ecg stop"],
                 "help": "Stop ECG"},
    "ecg_dcb": {"commands": ["sub recg remove", "sensor ecg stop"],
                 "help": "Stop ECG"},             
    "eda": {"commands":["sub reda remove", "sensor eda stop"],
                "help":"Stops the EDA application and unsubscribes it."},
    "eda_dcb": {"commands":["sub reda remove", "sensor eda stop"],
                "help":"Stops the EDA application and unsubscribes it."},
    "stop_log_eda": {"commands":["sensor eda stop","fs_sub reda remove","fs_log stop"],
                "help":"Start eda"},
    "stop_log_bcm": {"commands":["sensor bcm stop","fs_sub rbcm remove","fs_log stop"],
                "help":"Start eda"},
    "stop_log_ped":{"commands":["sensor ped stop", "sensor adxl stop","fs_sub rped remove","fs_log stop"],
                "help":"Stops the Pedometer application, unsubscribes it and disables the ADXL sensor."}, 
    "stop_log_adpd4000_r_adxl": {"commands":["fs_sub radpd7 remove","fs_sub radxl remove","fs_log stop","sensor adpd4000 stop","sensor adxl stop"],
                    "help":"Stops the ADPD4000_r, ADXL logging"},
    "stop_log_mv_uc1": {   "commands":["sensor temperature stop","sensor adxl stop","sensor adpd4000 stop","fs_sub radpd6 remove","fs_sub radxl remove","fs_sub rtemperature remove","fs_log stop"],
                "help":"Stops logging for MV UC1 - Adpd, Adxl, Temperature"},         
    "stop_log_mv_uc2": {   "commands":["sensor temperature stop","sensor adpd4000 stop","sensor adxl stop","sensor eda stop","fs_sub radpd6 remove","fs_sub radxl remove","fs_sub reda remove","fs_sub rtemperature remove","fs_log stop"],
                "help":"Stops logging for MV UC2 - Eda, Adxl, Adpd, Temperature"},
    "stop_log_mv_uc3": {   "commands":["sensor temperature stop","sensor adxl stop","sensor adpd4000 stop","sensor ecg stop","fs_sub radpd6 remove","fs_sub radxl remove","fs_sub recg remove","fs_sub rtemperature remove","fs_log stop"],
                "help":"Stops logging for MV UC3 - Ecg, Adpd, Adxl, Temperature"},
    "stop_log_mv_uc4": {   "commands":["sensor temperature stop","sensor ppg stop","sensor ecg stop","fs_sub rppg remove","fs_sub recg remove","fs_sub rtemperature remove","fs_log stop"],
                "help":"Stops logging for MV UC4 - Ecg, ppg, Temperature"},
    "stop_log_mv_uc5": {   "commands":["sensor adxl stop","sensor adpd4000 stop","fs_sub radpd6 remove","fs_sub radpd7 remove","fs_sub radpd8 remove","fs_sub radpd9 remove","fs_sub radxl remove","fs_log stop"],
                "help":"Stops logging for MV UC5- 4 LED Slots at 100Hz."},
    "bcm": {"commands": ["sub rbcm remove", "sensor bcm stop"],
                 "help": "Stop BCM"},
    "mwl_view": {"commands": ["sub radpd6 remove","sub radpd7 remove","sub radpd8 remove","sub radpd9 remove","sensor adpd4000 stop"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "mwl_view_agc_off": {"commands": ["sub radpd6 remove","sub radpd7 remove","sub radpd8 remove","sub radpd9 remove","sensor adpd4000 stop"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "ppg_static_agc_on": {    "commands":["sub rppg remove", "sensor ppg stop"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_static_agc_off": {    "commands":["sub rppg remove", "sensor ppg stop"],
                "help":"Stops the PPG library with Unsubscribes it."},
###################################################################################################################
#### Commands for Slot Switching, to be used only when Watch is loaded with FW built with "SLOT_SELECT" macro ####
#################################################################################################################
    "adpd4000_g_A": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_g_B": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_r_A": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_r_B": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_ir_A": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_ir_B": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_b_A": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_b_B": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_g_A_agc_off": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_g_B_agc_off": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_r_A_agc_off": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_r_B_agc_off": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_ir_A_agc_off": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_ir_B_agc_off": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_b_A_agc_off": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "adpd4000_b_B_agc_off": {   "commands":["sub radpd2 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Stops the ADPD application, unsubscribes it and disables the slot"},
    "temp_AB": {   "commands":["sub rtemperature remove", "sensor temperature stop","disable_adpd4k_slots"],
                "help":"Stops Temperature app and disables the slot"},
    "temp_BC": {   "commands":["sub rtemperature remove", "sensor temperature stop","disable_adpd4k_slots"],
                "help":"Stops Temperature app and disables the slot"},
    "temp_DE": {   "commands":["sub rtemperature remove", "sensor temperature stop","disable_adpd4k_slots"],
                "help":"Stops Temperature app and disables the slot"},
    "ppg_A": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_B": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_C": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_F": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_A_agc_off": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_B_agc_off": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_C_agc_off": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ppg_F_agc_off": {    "commands":["sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops the PPG library with Unsubscribes it."},
    "ecg4k_A": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "disable_adpd4k_slots"],
                "help":"Stops and Unsubscribes the ADPD that is measuring ecg in slot A."},
    "ecg4k_ppg_temp": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rppg remove", "sensor ppg stop", "sub rtemperature remove", "sensor temperature stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ecg4k_temp_ppg": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rtemperature remove", "sensor temperature stop", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ecg4k_ppg": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ecg4k_temp": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rtemperature remove", "sensor temperature stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k and temperature app and disables the slot"},
    "ppg_temp_ABC": {   "commands":["sub rppg remove", "sensor ppg stop", "sub rtemperature remove", "sensor temperature stop", "disable_adpd4k_slots"],
                "help":"Stops ppg and temperature app and disables the slot"},
    "temp_ppg_ABC": {   "commands":["sub rtemperature remove", "sensor temperature stop", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ppg and temperature app and disables the slot"},
    "ecg4k_ppg_temp_agc_off": {   "commands":[ "sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rppg remove", "sensor ppg stop", "sub rtemperature remove", "sensor temperature stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ecg4k_temp_ppg_agc_off": {   "commands":["sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rtemperature remove", "sensor temperature stop", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ecg4k_ppg_agc_off": {   "commands":[ "sub radpd1 remove", "sensor adpd4000 stop", "controlECGElectrodeSwitch 4k_sw 0", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ecg4k, ppg and temperature app and disables the slot"},
    "ppg_temp_ABC_agc_off": {   "commands":["sub rppg remove", "sensor ppg stop", "sub rtemperature remove", "sensor temperature stop", "disable_adpd4k_slots"],
                "help":"Stops ppg and temperature app and disables the slot"},
    "temp_ppg_ABC_agc_off": {   "commands":["sub rtemperature remove", "sensor temperature stop", "sub rppg remove", "sensor ppg stop", "disable_adpd4k_slots"],
                "help":"Stops ppg and temperature app and disables the slot"},
    "mwl_view_ABCD": {"commands": [ "sub radpd1 remove","sub radpd2 remove","sub radpd3 remove","sub radpd4 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot A, B, C, D of ADPD4000"},
    "mwl_view_ABCD_agc_off": {"commands": [ "sub radpd1 remove","sub radpd2 remove","sub radpd3 remove","sub radpd4 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot A, B, C, D of ADPD4000"},
    "mwl_view_FGHI": {"commands": [ "sub radpd6 remove","sub radpd7 remove","sub radpd8 remove","sub radpd9 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "mwl_view_FGHI_agc_off": {"commands": ["sub radpd6 remove","sub radpd7 remove","sub radpd8 remove","sub radpd9 remove", "sensor adpd4000 stop", "disable_adpd4k_slots"],
                "help":"Opens MWL view with Green, Red IR, Blue LED from Slot F, G, H, I of ADPD4000"},
    "uc_hr_enab": {"commands": ["sub rppg remove","quickstop adpd4000", "quickstop adxl", "setUCHREnab 0 6"],
                "help":"Stops UC HR enable test"},
    "stop_stream_mv_uc1": {   "commands":["sub rtemperature remove","sensor temperature stop","sub radxl remove","sensor adxl stop","sub radpd6 remove","sensor adpd4000 stop"],
                "help":"Stops streaming for MV UC1 - Adpd, Adxl, Temperature"},
    "stop_log_mv_uc4_1": {   "commands":["fs_sub rppg remove","fs_sub recg remove","fs_sub rtemperature remove","fs_log stop"],
                "help":"Give the MV UC4 stop stream cmd sequence"},
    "stop_stream_mv_uc4_1": {   "commands":["sensor temperature stop","sensor ppg stop","sensor ecg stop","sub rppg remove","sub recg remove","sub rtemperature remove"],
                "help":"Give the MV UC4 stop log cmd sequence, in b/w streaming"},}

    def precmd(self, line):
        """
        This function overrides the cmd.Cmd base class method. It gets called
        after every command that the user gives.
        """
        if (not "connect" in line) and (self.m2m2_server == None):
            self.vrb.write("NOTE: Not connected to a serial device!")
        self.vrb.console_write(self.prompt + cr.Style.RESET_ALL + line)
        return line

    def postcmd(self, stop, line):
        """
        This function overrides the cmd.Cmd base class method. It gets called
        after every command that the user gives.
        :param stop:
        :param line:
        :return:
        """
        return None
        '''
        if type(stop) == tuple:
            if len(stop) > 0:
                return stop[0]
            else:
                return None
        else:
            return stop
        '''

    def do_flush(self, arg):
        """
Flush all of the CLI's message queues. This is useful when you receive messages
that aren't handled by a command. For example: if a command times out but a
response is eventually received, then the next time that command is run, the
previous response will be in the queue and will be returned instead of the newer
response.
        """
        for addr in self.dispatcher_map:
            q = self.dispatcher_map[addr].queue
            name = self._get_enum_name(M2M2_ADDR_ENUM_t, addr)
            self.vrb.write("Flushing Queue with {} items: {}({})".format(len(q), name, addr), 2)
            q.clear()

    @cli_logger
    def do_connect_usb(self, arg):
        """
Connect to serial device over USB CDC.
Provide a serial device identifier appropriate for your platform (COMX for Windows, /dev/ttyX for Linux and OSX).
#>connect_usb COM7
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        try:
            self.vrb.write("Starting serial interface threads...", 4)
            self.m2m2_server = m2m2_server.m2m2_uart_server(self.rx_q, self.tx_q, self.vrb)
            #if not self.m2m2_server.connect(args[0], int(args[1])):
            if not self.m2m2_server.connect(args[0]):
                self.vrb.write("Failed to connect to the serial port!")
                return
            self.vrb.write("Starting dispatcher thread...", 4)
            dispatcher_thread = threading.Thread(target=self._dispatcher, args=[self.dispatcher_map, self.sock_map])
            dispatcher_thread.setDaemon(True)
            dispatcher_thread.start()
        except serial.serialutil.SerialException as e:
            self.vrb.err("Error opening the serial device!")
            self.vrb.err("You might be connected already, or have given an incorrect serial device identifier.")
            self.vrb.err("Error was:\n\t{}".format(e))
            return
        set_cli_addr(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI)
        self.onecmd("getVersion")

    def do_connect_dongle(self, arg):
        """
Connect to remote device via BLE through BLE Dongle COM Port.
Provide a serial device identifier appropriate for your platform (COMX for Windows, /dev/ttyX for Linux and OSX).
#>connect_dongle COM8
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        try:
            self.vrb.write("Starting serial interface threads...", 4)
            self.m2m2_server = m2m2_server.m2m2_uart_server(self.rx_q, self.tx_q, self.vrb)
            #if not self.m2m2_server.connect(args[0], int(args[1])):
            if not self.m2m2_server.connect(args[0]):
                self.vrb.write("Failed to connect to the serial port!")
                return
            self.vrb.write("Starting dispatcher thread...", 4)
            dispatcher_thread = threading.Thread(target=self._dispatcher, args=[self.dispatcher_map, self.sock_map])
            dispatcher_thread.setDaemon(True)
            dispatcher_thread.start()
        except serial.serialutil.SerialException as e:
            self.vrb.err("Error opening the serial device!")
            self.vrb.err("You might be connected already, or have given an incorrect serial device identifier.")
            self.vrb.err("Error was:\n\t{}".format(e))
            return
        set_cli_addr(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI_BLE)
        self.onecmd("getVersion")

    def help_connect(self):
        print "Connect to a serial device."
        print "Provide a serial device identifier appropriate for your platform (COMX for Windows, /dev/ttyX for Linux and OSX)."
        print "Example usage for usb connection with Watch: \n\t#>connect_usb COM7\n"
        print "Example usage for ble connection with Watch: \n\t#>connect_dongle COM7\n"
        print "Wait a moment, searching for available serial ports..."
        # Find a list of available serial ports
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

        print "Available serial ports are:"
        for p in result:
            print "==> {}".format(p)


    def do_msg_verbose(self, arg):

        args = self._parse_args(arg, 1)
        if args == None:
            return
        try:
            lvl = int(args[0])
            self.vrb.set_level(lvl)
        except:
            self.vrb.err("Invalid argument!", 1)

    def help_msg_verbose(self):
        print "Sets the verbosity level of the CLI. Valid levels are from 0-4."
        print "Verbosity levels are cumulative, meaning that a level of 3 will"
        print "also print messages from levels 1 and 2."
        print "Setting a verbosity level >= {} will open a separate window that".format(self.vrb.console_level)
        print "will display the prints."
        print "In general, the different levels are used for:"
        print "The different levels are printed with different formatting:"
        for i in range(1, 5):
            print "  " + self.vrb.msg_formatters[i]["fmt"].format("Level {}: {}".format(i, self.vrb.msg_formatters[i]["help"]))
        print "Note that error messages and help text are not affected by this setting."

    def do_raw_msg(self, arg):
        """
Send a raw packet. Bytes are specified in hex, with a colon separating each byte.
Note that there is no receive method associated with this command. If verbosity is
disabled, or there is no other handler, then there will be no handling of a received
response.
    #>raw_msg AB:CD:EF:AB
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        data = binascii.unhexlify(args[0].replace(":", ""))
        self._send_packet(data)

    def do_exit(self, arg):
        """Exit the shell."""
        try:
            self.m2m2_server.quit()
        except:
            pass
        quit()


    def do_plot(self, arg):
	global enable_csv_logs
        args = self._parse_args(arg, 1)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                app_name = a
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
            else:
                address = None

        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        s = socket.socket()
        if "nt" in os.name:
            # If this is on windows, run the plotter automatically.
            Portnumber = self._get_free_port()
            if(Portnumber!= None):
                plotter_path = os.path.join(os.path.abspath(__file__), '../plotter.py')
                os.system("start cmd /k python {} {} {} {}".format(plotter_path, app_name,Portnumber,enable_csv_logs))
                time.sleep(0)
            else:
                self.vrb.err("Could not find a free socket for plotting")
                return
        else:
            self.vrb.write("Automatic plot starting is not yet implemented on non-Windows platforms! Feel free to do it yourself ;)")
        for i in range(5):
            try:
                self.vrb.write("Connecting to the plotter...", 4)
                s.connect(('localhost',Portnumber))
                self.sock_map[stream] = s
                self.vrb.write("Successfully connected to the plotter!", 4)
                return
            except socket.error:
                self.vrb.err("Count not connect to the plotter! Retrying {}...".format(5-i))
                time.sleep(2)
        self.vrb.err("Failed to connect to the plotter!")


    def help_plot(self):
        device_str_list = ""
        for device in stream_name_map:
            application_name = self._get_enum_name(M2M2_ADDR_ENUM_t, int(stream_name_map[device]["application"]))
            if application_name == None:
                application_name = stream_name_map[device]["application"]
            stream_name = self._get_enum_name(M2M2_ADDR_ENUM_t, int(stream_name_map[device]["stream"]))
            if stream_name == None:
                stream_name = stream_name_map[device]["stream"]
            device_str_list += "\n'{}':{}:{}\n\t{}\n".format(device, application_name, stream_name, stream_name_map[device]["help"])
        print "Plot a data stream."
        print "Note that you must start the stream separately."
        print
        print "Available streams: ('name':application:stream){}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>plot [stream]"
        print
        print "    #>plot radpd1"
        print "    #>plot radxl"
        print "    #>plot rppg"
        print "    #>plot reda"
        print "    #>plot recg"
        print

    def do_getVersion(self, arg):
        """
Get the system application version information. Gets both the PS and PM versions.
#>getVersion
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        version = None
        version = self._get_version(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM)
        if version != None:
            self._print_version_pkt(version)
            try:
                fw_ver_dict = {'major': int(version.payload.major), 'minor': int(version.payload.minor),
                               'patch': int(version.payload.patch),
                               'info': cast(version.payload.verstr, c_char_p).value,
                               'date': cast(version.payload.str, c_char_p).value.split('|')[-1]}
                err_stat = 0
            except:
                fw_ver_dict = {}
                err_stat = 1
        else:
            self.vrb.err("Timed out waiting for the PM version response.")
            fw_ver_dict = {}
            err_stat = 1
        return err_stat, fw_ver_dict

    def do_getMcuVersion(self, arg):
        """
Get the MCU type information (M3/M4).
#>getMcuVersion
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_mcu_version_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_MCU_VERSION_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_mcu_version_t(), 10)

        if reply_msg != None:
            status = self._get_enum_name(M2M2_PM_SYS_MCU_TYPE_ENUM_t, reply_msg.payload.mcu)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Get MCU version failed!")

    def do_ping(self, arg):
        """
Ping a system, to start getting a particular pkt_size every 20 ms. The approximate round-trip response time is recorded
to calculate the throughput of BLE/USB.
    #>ping [system] [number of pings] [pkt size]
    #>ping PM 10 70
Pkt size Min Value: should be greater than or equal to 15
Pkt size Max Value: should be less than or equal to 244
        """
        if len(arg.split()) == 4:
            arg_len = 4
        else:
            arg_len = 3
        args = self._parse_args(arg, arg_len)
        enable_pong_print = True
        if args == None:
            self.vrb.write("Incorrect usage! Please check help.")
            return
        for a in args:
            if "pm" in a.lower():
                addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        try:
            num_pings = int(args[1])
            pkt_sz = int(args[2])
            if len(args) >= 4:
                enable_pong_print = eval(args[3])
            else:
                enable_pong_print = True
            if (pkt_sz < 15) | (pkt_sz >= 245):
              self.vrb.write("Incorrect pkt size, Min Value: should be greater than or equal to 15; Max value: must be less than or equal to 244")
              return
        except:
            return
        self.vrb.write("Starting ping for count:{} pkt sz:{}".format(num_pings, pkt_sz))
        missed_seq_no_list = []
        total_time = 0
        total_bytes = 0
        src_addr = get_cli_addr()
        retry_cnt = 0

        tx_pkt = m2m2_packet(addr, m2m2_app_common_ping_t())
        tx_pkt.header.src = src_addr
        tx_pkt.header.dest = addr
        tx_pkt.header.length = pkt_sz
        tx_pkt.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_PING_REQ
        tx_pkt.payload.sequence_num = num_pings
        self._send_packet(tx_pkt)
        total_time = time.time()
        for i in range(num_pings):
          start_time = time.clock()
          rx_pkt = self._get_packet(addr, m2m2_app_common_ping_t(), 2)
          if rx_pkt == None :
            missed_seq_no_list.append(i+1)
            self.vrb.err("Request timed out for rx pkt {}!".format(i+1))
            self.vrb.write("Request timed out for rx pkt {}!".format(i+1))
            self.vrb.write("Missed packet sequence numbers: {}!".format(missed_seq_no_list))
            retry_cnt += 1
            if retry_cnt == 3:
              return
          elif rx_pkt.payload.sequence_num != i + 1:
            missed_seq_no_list.append(i+1)
            retry_cnt += 1
            self.vrb.write("Incorrect ping response received! Expected: {} Got: {}".format(i + 1, rx_pkt.payload.sequence_num))
            self.vrb.write("Missed packet sequence numbers: {}!".format(missed_seq_no_list))
            if retry_cnt == 3:
              return
          else:
            elapsed_time = time.clock() - start_time
            if enable_pong_print:
                self.vrb.write("\tpong# {}: took {}s".format(i, elapsed_time), 1)
            total_bytes+=(pkt_sz)
          i += 1

        total_time = time.time() - total_time
        self.vrb.write("Missed packet sequence numbers: {}!".format(missed_seq_no_list))
        self.vrb.write("Total Bytes: {} byte Total time: {}sec!".format(total_bytes,total_time))
        throughput = float(total_bytes)/total_time
        self.vrb.write("Throughput from CLI: {}byte/sec!".format(throughput))
        self.vrb.write("Throughput theoretical: {}byte/sec!".format(pkt_sz*1000/20))

        ping_results_dict = {'missed_pkt_seq_num': missed_seq_no_list, 'total_bytes': total_bytes,
                             'total_time': total_time, 'throughput_cli': throughput,
                             'throughput_theoretical': pkt_sz * 1000 / 20}
        return 0, ping_results_dict

    def do_getAdpdVersion(self, arg):
        """
Get the ADPD application version information.
    #>getAdpdVersion
        """
        version = self._get_version(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000)
        if version != None:
            self._print_version_pkt(version)
        else:
            self.vrb.err("Timed out waiting for the ADPD version response.")


    def help_loadAdpdCfg(self):
      enum_dict = self.get_enum_fields_vals(M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t)
      for key in enum_dict:
        print("{}::{}".format(key, enum_dict[key]))

    def get_enum_fields_vals(self,enum):
    # Save a copy of the dictionary so that we can remove items from it (can't remove items from a dictionary while iterating over that dictionary)
      d = dict(vars(enum))
      for item in vars(enum):
        # Remove the internal variables that are used by the language (i.e. the __name__ field), which by convention start and end with double underscores
        if item.startswith("__") and item.endswith("__"):
          del(d[item])
      return d

    def do_testCmd(self, arg):
        """
Issue a test command.
#>testCmd [1] [2] [3]
#>testCmd 1 0
        """
        args = self._parse_args(arg, 1)
        version = None
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd_testcommand_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_DO_TEST1_REQ
        msg.payload.retdata[0] = int(args[0])

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd_testcommand_resp_t(), 20)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Data1 '{}'".format(hex(reply_msg.payload.retdata[0])), 2)
            self.vrb.write("  Data2 '{}'".format(reply_msg.payload.retdata[1]), 2)
            self.vrb.write("  Data3 '{}'".format(reply_msg.payload.retdata[2]), 2)
        else:
            self.vrb.err("Test command failed!")


    def do_loadAdpdCfg(self, arg):
        args = self._parse_args(arg,1)
        if args == None:
            self.vrb.err("No configuration specified, loading the default ADPD4000 config", 2)
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_LOAD_CFG_REQ
        if args == None:
            msg.payload.deviceid = M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_G
        else:
            try:
                msg.payload.deviceid = int(args[0])
            except ValueError:
                self._LoadCfg(".\\cfgs\\"+args[0])
                return
        if (msg.payload.deviceid != M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_B) and (msg.payload.deviceid != M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_R) \
        and (msg.payload.deviceid != M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_IR) and (msg.payload.deviceid != M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_G) \
        and (msg.payload.deviceid != M2M2_SENSOR_ADPD_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADPD4000_DEVICE_4000_G_R_IR_B):
            self.vrb.write("Invalid device type")
            return
        self._send_packet(msg)
        time.sleep(3)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("Loaded adpd device configuration:", 2)
            self.vrb.write("  Device: '{}'".format(int(reply_msg.payload.deviceid)), 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Loading Adpd device configuration failed!")


    def do_getCtrValue(self, arg):
        """
Get ctr value of a reflective object
    #>getCtrValue
        """
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000
        stream = M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD_STREAM1
        msg = m2m2_packet(address, m2m2_sensor_adpd_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_GET_CTR_REQ
        msg.payload.stream = stream
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd_resp_t(), 20)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Get CTR Value:", 2)
            self.vrb.write("  Status: '{}'".format(status))
            self.vrb.write("  CTR =: '{}'".format(int(reply_msg.payload.retdata[0])), 2)
        else:
            self.vrb.err("Get CTR Value failed!")

    def do_getSlot(self, arg):
        """
Get the Adpd device slots. Return slot num, its data format.
    #>getSlot : show all the slots
    #>getSlot slot_num : show only this slot
        """
        args = self._parse_args(arg, 1)
        if args == None:
            i_start = 1
            i_stop = 13
        else:
            i_start = int(args[0])
            i_stop = int(args[0]) + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_REQ
        for i in range(i_start, i_stop):
            msg.payload.slot_num = i
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_resp_t(), 20)
            if reply_msg != None:
                slotNum = reply_msg.payload.slot_num
                slotActive = reply_msg.payload.slot_enable
                slotChannel = reply_msg.payload.channel_num
                slotSz = reply_msg.payload.slot_format
                self.vrb.write("  Slot Num:='{}' Enable='{}' Channel='{}' Format='{}'".format(slotNum, slotActive, slotChannel, hex(slotSz)), 2)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))

    def do_setSlot(self, arg):
        """
Set the Adpd device slot. Slot settings are:
  Slot Size = 0IDS
    #>setSlot SLOT_NUM ENABLE CHANNEL_NUM SLOT_FORMAT
    #>SLOT_FORMAT 0IDS
        """
        args = self._parse_args(arg, 4)
        if args == None:
            self.vrb.err("No slot settings supplied")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_REQ
        msg.payload.slot_num = int(args[0])
        msg.payload.slot_enable = int(args[1])
        msg.payload.channel_num = int(args[2])
        msg.payload.slot_format = int(args[3], 16)
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_resp_t(), 30)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            slotNum = reply_msg.payload.slot_num
            slotActive = reply_msg.payload.slot_enable
            slotChannel = reply_msg.payload.channel_num
            slotSz = reply_msg.payload.slot_format
            self.vrb.write("  Slot Num='{}', Enable='{}', Channel= '{}', Format='{}'".format(slotNum, slotActive, slotChannel, hex(slotSz)), 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Slot setting failed!")

    def do_loadAdxlCfg(self, arg):
        """
Load the ADXL device configuration. The argument is the device ID to choose the dcfg file:
  '362' for ADXL362
    #>loadAdxlCfg [device id]
    #>loadAdxlCfg 362
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No configuration specified, loading the default ADXL362 config", 2)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, m2m2_sensor_adxl_resp_t())
        msg.payload.command = M2M2_SENSOR_ADXL_COMMAND_ENUM_t.M2M2_SENSOR_ADXL_COMMAND_LOAD_CFG_REQ
        if args == None:
            msg.payload.deviceid = M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADXL_DEVICE_362
        else:
            msg.payload.deviceid = int(args[0])
	if msg.payload.deviceid != M2M2_SENSOR_ADXL_DEVICE_ID_ENUM_t.M2M2_SENSOR_ADXL_DEVICE_362:
            self.vrb.write("Invalid device type")
	    return
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, m2m2_sensor_adxl_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Loaded adxl device configuration:", 2)
            self.vrb.write("  Device: '{}'".format(int(reply_msg.payload.deviceid)), 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Loading Adxl device configuration failed!")

    def do_adxl_self_test(self, arg):
        """
Do ADXL362 self test
    #>adxl_self_test
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, m2m2_sensor_adxl_resp_t())
        msg.payload.command = M2M2_SENSOR_ADXL_COMMAND_ENUM_t.M2M2_SENSOR_ADXL_COMMAND_SELF_TEST_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL, m2m2_sensor_adxl_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("ADXL self test Done", 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Adxl self test failed!")

    def do_getSlotActive(self, arg):
        """
Get the Adpd slots active state.
    #>getSlotActive : show all the slots
    #>getSlotActive slot_num : show only this slot
    #> 0 = inactive, 1 = active
        """
        args = self._parse_args(arg, 1)
        if args == None:
            i_start = 1
            i_stop = 13
        else:
            i_start = int(args[0])
            i_stop = int(args[0]) + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_active_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_GET_SLOT_ACTIVE_REQ
        for i in range(i_start, i_stop):
            msg.payload.slot_num = i
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_active_resp_t(), 20)
            if reply_msg != None:
                slotNum = reply_msg.payload.slot_num
                slotActive = reply_msg.payload.slot_active
                self.vrb.write("  Slot Num:='{}' Active='{}' ".format(slotNum, slotActive), 2)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))

    def do_setSlotActive(self, arg):
        """
Set the Adpd slot active.
    #>setSlotActive SLOT_NUM ENABLE ACTIVE
    #> 0 = inactive, 1 = active
        """
        args = self._parse_args(arg, 2)
        if args == None:
            self.vrb.err("No slot settings supplied")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_active_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_SET_SLOT_ACTIVE_REQ
        msg.payload.slot_num = int(args[0])
        msg.payload.slot_active = int(args[1])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_slot_active_resp_t(), 30)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            slotNum = reply_msg.payload.slot_num
            slotActive = reply_msg.payload.slot_active
            self.vrb.write("  Slot Num='{}', Active='{}'".format(slotNum, slotActive), 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Slot setting failed!")

    def do_setDecSample(self, arg):
        """
Set the decimation factor for stream samples. The argument is the stream and decimation factor:
Note that decimation of samples is limited to radpd, radxl, recg, rsyncppg streams
      Eg: = setDecSample radpd 2
        """
        address = None
        args = self._parse_args(arg, 2)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        if address in [M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000 , M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL,\
        M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG ]:
            msg = m2m2_packet(address, m2m2_sensor_common_decimate_stream_t())
            msg.payload.command = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_SET_STREAM_DEC_FACTOR_REQ
            msg.payload.stream = stream
            if args[1] != None:
                msg.payload.dec_factor = int(args[1])
            else:
                self.vrb.write("No decimation factor specified, setting the default to 1", 2)
                msg.payload.dec_factor = 1
            self._send_packet(msg)
            reply_msg = self._get_packet(address, m2m2_sensor_common_decimate_stream_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                self.vrb.write("Decimation factor set: '{}'".format(reply_msg.payload.dec_factor))
                self.vrb.write("  Status: '{}'".format(status))
            else:
                self.vrb.err("Failed setting the decimation factor!")
        else:
            self.vrb.err("Unsupported for this application!")
            return

    def do_getDecSample(self, arg):
        """
Get the decimation factor for stream samples. The argument is the stream
Note that decimation of samples is limited to radpd, radxl, recg, rsyncppg streams
      Eg: = getDecSample radpd
        """
        address = None
        args = self._parse_args(arg, 1)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        if address in [M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000 , M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL,\
        M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG ]:
            msg = m2m2_packet(address, m2m2_sensor_common_decimate_stream_t())
            msg.payload.command = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_GET_STREAM_DEC_FACTOR_REQ
            msg.payload.stream = stream
            self._send_packet(msg)
            reply_msg = self._get_packet(address, m2m2_sensor_common_decimate_stream_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                dec_factor = reply_msg.payload.dec_factor
                self.vrb.write("Got the Decimation factor:")
                self.vrb.write("  Decimation Factor: '{}'".format(dec_factor))
                self.vrb.write("  Status: '{}'".format(status))
            else:
                self.vrb.err("Failed getting the decimation factor!")
        else:
            self.vrb.err("Unsupported for this application!")
            return

    def do_setPause(self, arg):
        """
Set device to pause or unpause
    #>setPause 1|0
        """
        args = self._parse_args(arg, 1)
        if args != None:
            pauseEnable = int(args[0])
        else:
            pauseEnable = 1

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_SET_PAUSE_REQ
        msg.payload.retdata[0] = pauseEnable
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpdCl_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Set pause device: {}".format(status))
        else:
            self.vrb.err("Set pause device failed!")

    def do_fs_DebugInfo(self, arg):
        """
File system debug information, Command to get the packet loss and count information.
#>fs_DebugInfo radpd
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                stream = stream_name_map[a]["stream"]
        if stream == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_debug_info_req_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_FS_DEBUG_INFO_REQ
        msg.payload.stream = stream
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_debug_info_resp_t(), 10)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
            self.vrb.write("  Total Packets received : '{}'".format(int(reply_msg.payload.packets_received)))
            self.vrb.write("  Total Packets missed : '{}'".format(int(reply_msg.payload.packets_missed)))
            self.vrb.write("  last page read : '{}'".format(int(reply_msg.payload.last_page_read)))
            self.vrb.write("  last page read offset : '{}'".format(int(reply_msg.payload.last_page_read_offset)))
            self.vrb.write("  number of bytes transferred : '{}'".format(int(reply_msg.payload.num_bytes_transferred)))
            self.vrb.write("  usb cdc write failed status : '{}'".format(int(reply_msg.payload.usb_cdc_write_failed)))
            self.vrb.write("  bytes processed from fs task : '{}'".format(int(reply_msg.payload.bytes_read)))
        else:
            self.vrb.err("No response from device.Getting volume info failed.")

    def do_getSystemInfo(self, arg):
        """
Get the PM device system Information
#>getSystemInfo
        """
        sys_info_dict = {}
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_INFO_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_info_t(), 10)
        if reply_msg != None:
            mac_string = ''.join('{:02X}'.format(x) for x in reply_msg.payload.mac_addr)
            self.vrb.write("System Information")
            self.vrb.write("  Version:     '{}'".format(reply_msg.payload.version))
            self.vrb.write("  MAC Address: '{}'".format(mac_string))
            self.vrb.write("  Device ID:   '{}'".format(reply_msg.payload.device_id))
            self.vrb.write("  Model Number:'{}'".format(reply_msg.payload.model_number))
            self.vrb.write("  PS Hardware: '{}'".format(reply_msg.payload.hw_id))
            self.vrb.write("  BOM:         '{}'".format(reply_msg.payload.bom_id))
            self.vrb.write("  Batch:       '{}'".format(reply_msg.payload.batch_id))
            self.vrb.write("  Date:        '{}'".format(reply_msg.payload.date))
            status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))
            sys_info_dict['version'] = reply_msg.payload.version
            sys_info_dict['mac_addr'] = mac_string
            sys_info_dict['device_id'] = reply_msg.payload.device_id
            sys_info_dict['model_number'] = reply_msg.payload.model_number
            sys_info_dict['hw_id'] = reply_msg.payload.hw_id
            sys_info_dict['bom_id'] = reply_msg.payload.bom_id
            sys_info_dict['batch_id'] = reply_msg.payload.batch_id
            sys_info_dict['date'] = reply_msg.payload.date
            error_stat = 0
        else:
            self.vrb.err("response timeout from device.")
            error_stat = 1
        return error_stat, sys_info_dict

    def do_msg_debug(self, arg):

        args = self._parse_args(arg.lower(), 1)
        if args == None:
            self.vrb.err("No argument was supplied!")
            return

        addr = M2M2_ADDR_ENUM_t.M2M2_ADDR_POST_OFFICE
        pkt = m2m2_packet(addr, post_office_config_t())
        if "on" in args:
            pkt.payload.cmd = POST_OFFICE_CFG_CMD_ENUM_t.POST_OFFICE_CFG_CMD_MAILBOX_SUBSCRIBE
        else:
            pkt.payload.cmd = POST_OFFICE_CFG_CMD_ENUM_t.POST_OFFICE_CFG_CMD_MAILBOX_UNSUBSCRIBE
        pkt.payload.sub = M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI
        pkt.payload.box = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_DBG_STREAM

        self._send_packet(pkt)

    def help_msg_debug(self):
        print "Enable/Disable debug messages output from the firmware"
        print "Note that you must use Verbosity levels 2 and above."
        print
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>msg_debug [on/off]"
        print

    def do_clockCalibration(self, arg):
        """
Calibrate the 32M and 1M or 32K clock to reduce the deviation to a minimum
    #>clockCalibration
    '0' for no calibration
    '1' for 32K clock calibration
    '2' for 1M clock calibration
    '4' for 32M clock calibration
    '5' for 32K and 32M clock calibration
    '6' for 1M and 32M clock calibration
    -----------------------------------------------
Usage:
    #>clockCalibration [clockcalid]
    #>clockCalibration 6
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No clockcalid specified", 2)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_clockcal_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_CLOCK_CAL_REQ
        if args == None:
            msg.payload.clockcalid = 0
        else:
            msg.payload.clockcalid = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_clockcal_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Clock Calibration:", 2)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Clock Calibration failed!")

    def do_getAdpdComModeCl(self, arg):
        """
Get ctr value of a reflective object
    #>getAdpdComModeCl
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_com_mode_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_com_mode_resp_t(), 20)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Get ADPD4000 Communication Mode:", 2)
            self.vrb.write("  Status: '{}'".format(status))
            self.vrb.write("  Com Mode =: '{}'".format(int(reply_msg.payload.com_mode)), 2)
        else:
            self.vrb.err("Get ADPD Communication mode failed!")

    def do_getAdpdComMode(self, arg):
        """
Get ctr value of a reflective object
    #>getAdpdComMode
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD, m2m2_sensor_com_mode_resp_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMUNICATION_MODE_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD, m2m2_sensor_com_mode_resp_t(), 20)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Get ADPD Communication Mode:", 2)
            self.vrb.write("  Status: '{}'".format(status))
            self.vrb.write("  Com Mode =: '{}'".format(int(reply_msg.payload.com_mode)), 2)
        else:
            self.vrb.err("Get ADPD Communication mode failed!")

    def do_sensor(self, arg):
        address = None
        args = self._parse_args(arg, 2)
        max_retry_cnt=1
        if args == None:
            return
        for a in args:
            if a in application_name_map:
                address = application_name_map[a]["address"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return
        msg = m2m2_packet(address, m2m2_app_common_sub_op_t())
        if "start" in args:
            msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_START_REQ
        elif "stop" in args:
            msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_STOP_REQ

        while max_retry_cnt > 0:
            self._send_packet(msg)
            reply_msg = self._get_packet(address, m2m2_app_common_sub_op_t(), 20)
            if reply_msg != None:
                self._print_packet_status(reply_msg)
                break
            else:
                self.vrb.err("The device did not respond!")
                max_retry_cnt = max_retry_cnt-1

    def help_sensor(self):
        device_str_list = ""
        for device in application_name_map:
            device_str_list += "\n'{}': {}".format(device, application_name_map[device]["help"])
        print "Start or stop a sensor."
        print
        print "Available devices:{}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>sensor [device] [start/stop]"
        print
        print "    #>sensor adxl start"
        print "    #>sensor adpd stop"
        print

    def do_sub(self, arg):
        address = None
        args = self._parse_args(arg, 2)
        max_retry_cnt=1
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        msg = m2m2_packet(address, m2m2_app_common_sub_op_t())
        if "r" in args or "remove" in args:
            msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ
        elif "a" in args or "add" in args:
            msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ
        else:
            self.vrb.err("Incorrect usage! You did not provide a valid subscription operation.")
            return

        msg.payload.stream = stream
        while max_retry_cnt > 0:
            self._send_packet(msg)
            reply_msg = self._get_packet(address, m2m2_app_common_sub_op_t(), 20)
            if reply_msg != None:
                self._print_subscription_status(reply_msg)
                break
            else:
                self.vrb.err("No response from device. Subscription operation failed.")
            max_retry_cnt=max_retry_cnt-1

    def help_sub(self):
        device_str_list = ""
        for device in stream_name_map:
            application_name = self._get_enum_name(M2M2_ADDR_ENUM_t, int(stream_name_map[device]["application"]))
            if application_name == None:
                application_name = stream_name_map[device]["application"]
            stream_name = self._get_enum_name(M2M2_ADDR_ENUM_t, int(stream_name_map[device]["stream"]))
            if stream_name == None:
                stream_name = stream_name_map[device]["stream"]
            device_str_list += "\n'{}':{}:{}\n\t{}\n".format(device, application_name, stream_name, stream_name_map[device]["help"])
        print "Subscribe to or unsubscribe from a data stream."
        print "Note that you must start the stream separately."
        print
        print "Available streams: ('name':application:stream){}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>sub [stream] [a(dd)/r(emove)]"
        print
        print "    #>sub radxl add"
        print "    #>sub radpd remove"
        print

    def do_reg(self, arg):
        '''
Perform a register operation on a device.
        '''
        args = self._parse_args(arg, None)
        address = None
        ops = []
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return 1, []
        if (("r" or "read") in args and ("w" or "write") in args):
            self.vrb.err("Incorrect usage! You can read OR write, not both.")
            return 1, []
        for l in args:
            if l in application_name_map:
                address = application_name_map[l]["address"]

        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return 1, []

        if ("r" or "read") in args:
            do_write = False
        elif ("w" or "write") in args:
            do_write = True
        else:
            self.vrb.err("Incorrect usage! You did not specify if you want to read or write!")
            return 1, []
        for arg in args:
            # Filter out the device argument
            if ("0x" or ":") in arg:
                # See if it's a address:value pair
                if ":" in arg:
                    reg_addr = int(arg.split(':')[0], 16)
                    reg_val = int(arg.split(':')[1], 16)

                # See if it's just an address
                elif "0x" in arg:
                    reg_addr = int(arg, 16)
                    reg_val = 0
                ops.append({"address":reg_addr, "value":reg_val})

        resp = self._reg_op(address, ops, do_write)
        reg_result_list = []
        if resp != None:
            self._print_reg_result(resp)
            for i in range(resp.payload.num_ops):
                reg_result_list.append((hex(resp.payload.ops[i].address), hex(resp.payload.ops[i].value)))
            err_stat = 0
        else:
            self.vrb.err("The device did not respond!")
            err_stat = 1
        return err_stat, reg_result_list

    def help_reg(self):
        device_str_list = ""
        for device in application_name_map:
            device_str_list += "\n'{}': {}".format(device, application_name_map[device]["help"])
        print "Operate on a register. You can read OR write, and pass up to 10 register operations per command."
        print
        print "For adxl, only a range of registers are writable. Refer AdxlDcfg.h for this register map"
        print "In the register map, bits CANNOT be written in fields which are mentioned as UNUSED in"
        print "data sheet. Any write to the UNUSED bits are silently ignored during this command."
        print "List of UNUSED bits for various ADXL362 registers:"
        print "---------------------------------"
        print "|    Register   |  UNUSED bits  |"
        print "---------------------------------"
        print "|     0x21      |   [7:3]       |"
        print "---------------------------------"
        print "|     0x24      |   [7:3]       |"
        print "---------------------------------"
        print "|     0x28      |   [7:4]       |"
        print "---------------------------------"
        print "|     0x2E      |   [7:1]       |"
        print "---------------------------------"
        print
        print "Available devices:{}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>reg [read/write] [device] [address](:[value])"
        print
        print "    #>reg r adpd 0x11 0x25"
        print "    #>reg r ad5940 0x000021E0"
        print "    #>reg w adxl 0x01:0x34 0x20:0x69 0x69:0xAB"
        print "    #>reg w adpd 0x01:0x1234 0x20:0x6959 0x69:0xBEEF"
        print "    #>reg w ad5940 0x000021E0:0x00000002"
        print
        print "  Note that if you perform a write operation and give only an address, a value of 0 will be written."
        print "  These two commands do the same thing:"
        print "    #>reg w adxl 0x01:0x00"
        print "    #>reg w adxl 0x01"
        print

    def do_getDcfg(self, arg):
        """
Get the DCFG of the device. The actual contents of dcfg registers is returned in RegisterValue format as a 32-bit value
        """
        args = self._parse_args(arg, None)
        address = None
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return
        for device in args:
            if device in application_name_map:
                address = application_name_map[device]["address"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return
        resp = []
        msg = m2m2_packet(address, m2m2_sensor_dcfg_data_t())
        msg.payload.command = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_GET_DCFG_REQ
        self._send_packet(msg)
        if address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000 or address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_TEMPERATURE:
            pkt_count = 2
            while (pkt_count > 0):
                _resp = self._get_packet(address, m2m2_sensor_dcfg_data_t(), 20)
                if _resp == None:
                    self.vrb.err("Error! Timed out waiting for the device!")
                    return 1                
                pkt_count = _resp.payload.num_tx_pkts;
                resp.append(_resp)                
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, _resp.payload.status)
                if _resp.payload.num_tx_pkts == 0: 
                    break                 

            print "Obtained the ADPD Device Configuration:"
            self.vrb.write("  Num of dcfg registers: '{}'".format(int(sum(obj.payload.size for obj in resp))))
            self._print_dcfg_result(resp, device)
            print "Status: '{}'".format(status)
        elif address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL:
            _resp = self._get_packet(address, m2m2_sensor_dcfg_data_t(), 20)
            if _resp == None:
                self.vrb.err("Error! Timed out waiting for the device!")
                return 1                
            else:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, _resp.payload.status)
                print "Obtained the ADXL Device Configuration:"
                resp.append(_resp)
                self._print_dcfg_result(resp, device)
                print "Status: '{}'".format(status)
        else:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
                
    def do_dumpAdpdCfg(self, arg):
        """
Dump all the register values to a file
#>dumpAdpdCfg [filename]
        """
        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            filename_dump = ".\\cfgs\\dcfg_dump.dcfg"
        else:
            filename_dump = ".\\cfgs\\" + args[0]
        try:
            fileHandler = open(filename_dump, "w")
        except IOError:
            print filename_dump + " not found"
        else:
            print "dump data to " + filename_dump

        # range [0, 2e], [a0-b8], [100, 117] [120, 137], [140, 157]
        start_a = 0
        end_a = 0x2E
        length = end_a - start_a + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length))
        cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ
        msg.payload.command = cmd
        msg.payload.num_ops = length

        for i in range(0, length):
            msg.payload.ops[i].address = start_a + i
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length), 1)

        for i in range (0, length):
            address = '{:04x}'.format(reply_msg.payload.ops[i].address)
            value = '{:04x}'.format(reply_msg.payload.ops[i].value)
            message = address + "    " + value + "\n"
            fileHandler.write(message)

        # range [0, 2e], [a0-b8], [100, 117] [120, 137], [140, 157]
        start_a = 0xA0
        end_a = 0xB8
        length = end_a - start_a + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length))
        cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ
        msg.payload.command = cmd
        msg.payload.num_ops = length

        for i in range(0, length):
            msg.payload.ops[i].address = start_a + i
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length), 1)

        for i in range (0, length):
            address = '{:04x}'.format(reply_msg.payload.ops[i].address)
            value = '{:04x}'.format(reply_msg.payload.ops[i].value)
            message = address + "    " + value + "\n"
            fileHandler.write(message)

        # range [0, 2e], [a0-b8], [100, 117] [120, 137], [140, 157]
        start_a = 0x100
        end_a = 0x117
        length = end_a - start_a + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length))
        cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ
        msg.payload.command = cmd
        msg.payload.num_ops = length

        for i in range(0, length):
            msg.payload.ops[i].address = start_a + i
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(length), 1)

        for i in range (0, length):
            address = '{:04x}'.format(reply_msg.payload.ops[i].address)
            value = '{:04x}'.format(reply_msg.payload.ops[i].value)
            message = address + "    " + value + "\n"
            fileHandler.write(message)

        fileHandler.close()

    def help_getDcfg(self):
        device_str_list = ""
        for device in application_name_map:
            device_str_list += "\n'{}': {}".format(device, application_name_map[device]["help"])
        print "Get the dcfg"
        print
        print "Available devices:{}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>getDcfg [device]"
        print
        print "    #>getDcfg adpd"
        print "    #>getDcfg adxl"
        print

    def do_status(self, arg):
        address = None
        args = self._parse_args(arg, 1)
        if args == None:
            return
        for a in args:
            if a in application_name_map:
                address = application_name_map[a]["address"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return
        msg = m2m2_packet(address, m2m2_app_common_status_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_SENSOR_STATUS_QUERY_REQ
        msg.payload.stream = address
        self._send_packet(msg)
        reply_msg = self._get_packet(address, m2m2_app_common_status_t(), 10)
        if reply_msg != None:
            self._print_sensor_app_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def help_status(self):
        device_str_list = ""
        for device in application_name_map:
            device_str_list += "\n'{}': {}".format(device, application_name_map[device]["help"])
        print "Get status of a sensor or application."
        print
        print "Available devices:{}".format(device_str_list)
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>status [device]"
        print
        print "    #>status adpd"
        print "    #>status adxl"
        print "    #>status ppg"
        print

    def do_get_sensor_apps_status(self, arg):
        address =  M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PS
        msg = m2m2_packet(address, m2m2_app_common_status_t())
        msg.payload.command = M2M2_PS_SYS_COMMAND_ENUM_t.M2M2_PS_SYS_COMMAND_GET_PS_APPS_INFO_REQ
        msg.payload.stream = address
        self._send_packet(msg)
        reply_msg = self._get_packet(address, m2m2_ps_sys_sensor_apps_info_req_t(), 100)
        if reply_msg != None:
            self._print_sensor_apps_info_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    @cli_logger
    def do_quickstart(self, arg):
        '''
        Execute pre-set startup sequences for different sensors.
        '''

        args = self._parse_args(arg, None)
        if args == None:
            self.vrb.err("Incorrect usage! You did not provide any arguments.")

        if args[0] in self.quickstarts:
            for command in self.quickstarts[args[0]]["commands"]:
                if(lowtouch.Enable_lowtouch == True) and lowtouch.Stopcmd == False:
                    lowtouch.Startcmd = True
                self.onecmd(command)

    def help_quickstart(self):
        sequence_str_list = ""
        for sequence in self.quickstarts:
            sequence_str_list += "\n==============================="
            sequence_str_list += "\n'{}': {}".format(sequence, self.quickstarts[sequence]["help"])
            for cmd in self.quickstarts[sequence]["commands"]:
                sequence_str_list += "\n\t->{}".format(cmd)
        print "Execute pre-defined startup sequences for different sensors."
        print "Implemented sequences are:{}".format(sequence_str_list)

    @cli_logger
    def do_quickstop(self, arg):
        '''
        Execute pre-set stop sequences for different applications/sensors.
        '''

        args = self._parse_args(arg, None)
        if args == None:
            self.vrb.err("Incorrect usage! You did not provide any arguments.")

        if args[0] in self.quickstops:
            for command in self.quickstops[args[0]]["commands"]:
                if(lowtouch.Enable_lowtouch == True) and lowtouch.Startcmd == True:
                    lowtouch.Stopcmd = True
                    lowtouch.Startcmd = False
                self.onecmd(command)

    def help_quickstop(self):
        sequence_str_list = ""
        for sequence in self.quickstops:
            sequence_str_list += "\n==============================="
            sequence_str_list += "\n'{}': {}".format(sequence, self.quickstops[sequence]["help"])
            for cmd in self.quickstops[sequence]["commands"]:
                sequence_str_list += "\n\t->{}".format(cmd)
        print "Execute pre-defined stop sequences for different applications/sensors."
        print "Implemented sequences are:{}".format(sequence_str_list)
    def do_setPpgLcfg(self, arg):
        """
Set the PPG LCFG. The argument is the LCFG ID to choose the ppg lcfg file.
 Supported values are either:
    '105' for ADPD105
    '107' for ADPD107
      Eg: = setPpgLcfg 107
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No configuration specified, setting the default ADPD107 library config", 2)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_set_lcfg_req_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_SET_LCFG_REQ
        if args == None:
            msg.payload.lcfgid = M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD4000
        else:
            msg.payload.lcfgid = int(args[0])
            if msg.payload.lcfgid not in (M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD107,
                        M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD185,
                        M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD188,
                        M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD108,
                        M2M2_SENSOR_PPG_LCFG_ID_ENUM_t.M2M2_SENSOR_PPG_LCFG_ID_ADPD4000):
                self.vrb.write("Invalid lcfg id")
                return
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_set_lcfg_resp_t(), 40)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("Set the PPG library configuration:", 2)
            self.vrb.write("  Device: '{}'".format(int(msg.payload.lcfgid)), 1)
            self.vrb.write("  Status: '{}'".format(status), 1)
        else:
            self.vrb.err("Loading Ppg library configuration failed!")

    def do_lcfgPpgRead(self, arg):
        """
Read the PPG LCFG used in the Watch. The argument are the PPG LCFG addresses or the LCFG ID to choose from the ppg lcfg file:
	Note that the the range of addr varies from 0x0 to 0x2E

      Eg: = lcfgPpgRead addr1 addr2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self._p_err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lcfg_op_hdr_t(num_ops), 60)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg == None:
            print "Reading PPG LCFG failed!"
	    return
        self._print_ppg_lcfg_result(reply_msg)

    def do_lcfgPpgWrite(self, arg):
        """
Set the PPG LCFG. The argument is the PPG LCFG addresses or the LCFG ID, then VALUE to modify the ppg lcfg value:
	Note that the the range of addr varies from 0x0 to 0x2E

      Eg: = lcfgPpgWrite addr1 value1 addr2 value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self._p_err("No arguments supplied!")
            return

        num_ops = len(args)
        num_ops >>= 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i*2]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            tempVal = args[i*2+1]
            if ("0x") in tempVal:
                reg_val = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_val = int(tempVal, 16)
            else:
                reg_val = int(tempVal)
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lcfg_op_hdr_t(num_ops), 60)
        if reply_msg == None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            print "Writing PPG LCFG failed!"
	    return
        self._print_ppg_lcfg_result(reply_msg)

    def do_getPpgStates(self, arg):
        """
Get the states of PPG.
      Eg: = getPpgStates
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lib_state_t())
        msg.payload.command = M2M2_PPG_APP_CMD_ENUM_t.M2M2_PPG_APP_CMD_GET_LAST_STATES_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, ppg_app_lib_state_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Got the PPG States:\n")
            self.vrb.write("{} -> {} -> {} -> {} -> {} -> {} -> {} -> {} -> {} -> {}\n".format(reply_msg.payload.states[0], 
                        reply_msg.payload.states[1], reply_msg.payload.states[2], reply_msg.payload.states[3], reply_msg.payload.states[4],
                        reply_msg.payload.states[5], reply_msg.payload.states[6], reply_msg.payload.states[7], reply_msg.payload.states[8],
                        reply_msg.payload.states[9]))
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed getting the ppg states!")

    def do_SQISetSlot(self, arg):
        """
Set the ADPD Slot on which SQI algo needs to run
Argument -> slot_no
slot_no -> ranges from 1 to 12
		   6 --> slotF
		   7 --> slotG
		   8 --> slotH
		   9 --> slotI
    -----------------------------------------------
Usage:
#>SQISetSlot [slot_no]
To run SQI Algo on slot F:
#>SQISetSlot 6
To run SQI Algo on slot G:
#>SQISetSlot 7
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.err("Invalid argument! please type help <command>(help SQISetSlot) to know the usage.")
            return
        if int(args[0]) < 1 or int(args[0]) > 12:
            self.vrb.err("Invalid Argument, out of range. please type help <command>(help SQISetSlot) to know the valid arguments")
            return
        slot_no = int(args[0])

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI, sqi_app_set_slot_t())
        msg.payload.command = M2M2_SQI_APP_CMD_ENUM_t.M2M2_SQI_APP_CMD_SET_SLOT_REQ
        msg.payload.nSQISlot = pow(2, (slot_no-1))
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_SQI, sqi_app_set_slot_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Done with SQI Slot Set:")
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed doing SQI Slot Set!")

    def do_setUCHREnab(self, arg):
        """
Command to enable/disable HR calculation from UC1,2,3,5 from a particular slot
UC HR is to be started when raw adxl, adpd applications are started
Usage:
#>setUCHREnab [enab] [slotNum]

Argument -> enab
1 --> to enable
0 --> to disable

Argument -> slotNum
slot_no -> ranges from 1 to 12
		   6 --> slotF
		   7 --> slotG
		   8 --> slotH
		   9 --> slotI

To enable UC HR calculation from slot F:
#>setUCHREnab 1 6
To disable UC HR calculation from slot F:
#>setUCHREnab 0 6

        """
        args = self._parse_args(arg, 2)
        if args == None:
            self.vrb.err("Invalid argument! please type help <command>(help setUCHREnab) to know the usage.")
            return
        if int(args[0]) < 0 or int(args[0]) > 1:
            self.vrb.err("Invalid Argument, please type help <command>(help setUCHREnab) to know the valid arguments")
            return
        enab = int(args[0])
        slotNum = int(args[1])

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_set_uc_hr_enab_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_UC_HR_ENAB_REQ
        msg.payload.control = enab
        msg.payload.slotNum = slotNum
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_set_uc_hr_enab_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Done with setUCHREnab:")
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed doing setUCHREnab!")


    def do_adpdAGCControl(self, arg):
        """
Control the Static AGC ON/OFF for ADPD app.
Arguments to be specified in following format->adpdAGCControl agc_type:agc_cntrl agc_type:agc_cntrl ...

agc_type->  1. To Select between g/b/r/ir/mwl
                                        mwl = 0
                                        g = 1
                                        r = 2
                                        ir = 3
                                        b = 4
agc_cntrl-> 2.  To Enable AGC, arg. = 1
                To Disable AGC, arg. = 0

    -----------------------------------------------
Usage:
#>adpdAGCControl [agc_cntrl]
To Turn OFF STATIC AGC for MWL_VIEW
#>adpdAGCControl 0:0
To Turn ON STATIC AGC for MWL VIEW
#>adpdAGCControl 0:1 
To Turn ON STATIC AGC for Green LED and Turn OFF STATIC AGC for Red LED
#>adpdAGCControl 1:1 2:0
To Turn ON STATIC AGC for IR LED and Blue LED
#>adpdAGCControl 3:1 4:1
        """
        args = self._parse_args(arg, None)

        if args == None:
            print("Invalid arguments! please type help <command>(help ppgAGCControl) to know the usage.")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_cntrl_t(num_ops))
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_AGC_ON_OFF_REQ
        msg.payload.num_ops = num_ops
        	
        for i in range(num_ops):
            tempVal = args[i]
            if (':' not in tempVal):
                self.vrb.err("Invalid Argument Format, check help!")
                return
            elif ("0x") in tempVal:
                agc_type = int(tempVal.split(':')[0], 16)
                agc_cntrl = int(tempVal.split(':')[1], 16)
            elif ("0X") in tempVal:
                agc_type = int(tempVal.split(':')[0], 16)
                agc_cntrl = int(tempVal.split(':')[1], 16)
            else:
                agc_type = int(tempVal.split(':')[0])
                agc_cntrl = int(tempVal.split(':')[1])
            if(agc_cntrl < 0 or agc_cntrl > 1 or agc_type < 0 or agc_type > 4):
                self.vrb.err("Invalid arguments! please type help <command>(help ppgAGCControl) to know the usage.")
                return
            if(agc_type == 0 and num_ops > 1):
                self.vrb.err("Invalid, When AGC Control is done for MWL View, AGC Control for individual LEDs should be avoided")
                return
            msg.payload.ops[i].agc_cntrl = agc_cntrl
            msg.payload.ops[i].agc_type = agc_type
        
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000,m2m2_adpd_agc_cntrl_t(num_ops), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Done with Static AGC Control:")
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed doing Static AGC COntrol!")

    def do_adpdAGCRecalibrate(self, arg):
        """
Do AGC recalibration, while adpd streaming is in progress
    -----------------------------------------------
Usage:
#>adpdAGCRecalibrate
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_cntrl_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_PPG_APP_CMD_AGC_RECALIBRATE_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_cntrl_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Done with Static AGC Recalibrate:")
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed doing Static AGC Recalibrate!")

    def do_adpdGetAGCInfo(self, arg):
        """
Get AGC Algo Info for 4 LEDs after AGC is done and save it as file.
LED 1 -> Green
LED 2 -> Red
LED 3 -> IR
LED 4 -> Blue
No argument
    -----------------------------------------------
Usage:
#>adpdGetAGCInfo
        """	
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_info_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_AGC_INFO_REQ
        led_index = 0
        led=4
        f = open('agc_info.csv','w') 
        while led_index < led:
            msg.payload.led_index = led_index+1
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_info_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                self.vrb.write("Received AGC Info")
                self.vrb.write("  Status: '{}'".format(status))
                #f = open('agc_info.csv','w') 
                sample_cnt = 0
                sample=10			
                f.write('LED {}\n'.format(led_index+1));
                f.write('CH1, CH2\n');
                while sample_cnt < sample:
                    f.write('{:02d}, {:02d}\n'.format((reply_msg.payload.green_ch1[sample_cnt]),(reply_msg.payload.green_ch2[sample_cnt])))
                    sample_cnt+=1
                f.write('DC0_LEDcurrent, {:02d}\n'.format((reply_msg.payload.DC0_LEDcurrent)))
                f.write('TIA_ch1_i, {:02d}\n'.format((reply_msg.payload.TIA_ch1_i)))
                f.write('TIA_ch2_i, {:02d}\n\n'.format((reply_msg.payload.TIA_ch2_i)))
                #f.close()
            else:
                self.vrb.err("Failed doing Static AGC COntrol!")
                f.close()
                return
            led_index += 1
        f.close()

    def do_adpdAGCStatus(self, arg):
        """
Get AGC ON/OFF status for 4 LEDs and MWL.
It accepts One Argument -> agc_type  = 0 ->MWL
                                       1 ->Green
                                       2 ->Red
                                       3 ->IR
                                       4 ->Blue
    -----------------------------------------------
Ex Usage:
To get MWL AGC Status
#>adpdAGCStatus 0

To get Blue LED AGC Status
#>adpdAGCStatus 4
        """	
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.err("Invalid argument! please type help <command>(help adpdAGCStatus) to know the usage.")
            return
        if int(args[0]) < 0 or int(args[0]) > 4:
            self.vrb.err("Invalid Argument, out of range. please type help <command>(help adpdAGCStatus) to know the valid arguments")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_agc_status_t())
        msg.payload.agc_type = int(args[0])
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_AGC_STATUS_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000,m2m2_adpd_agc_status_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
             
            if (reply_msg.payload.agc_status is 0):
                agc_status = "OFF"
            elif(reply_msg.payload.agc_status is 1):
                agc_status = "ON"
            else:
                agc_status = "Unknown"
            self.vrb.write("  AGC Status : {}".format(agc_status))
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Failed in getting AGC Status!")

    def do_set_adpd_ext_datastream_odr(self, arg):
        """
        specify/set odr of externally fed adpd data for sqi stream
        for eg: data logged @50Hz, cmd will be -
#>set_adpd_ext_datastream_odr 50
        """
        args      = self._parse_args(arg, 1)
        sampling_freq = int(args[0])
        if(sampling_freq < 25 or sampling_freq > 100):
            self.vrb.err("sampling frequency not in range")
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000,adpd_ext_data_stream_odr_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_SET_EXT_DATA_STREAM_ODR_REQ
        msg.payload.sampling_freq = sampling_freq
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000,adpd_ext_data_stream_odr_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))
        else:
         self.vrb.err("Failed in setting odr for externally fed adpd data!")

        
    def do_send_ext_adpd_datastream(self, arg):
        """
Send external adpd data to device 
(csv file should contain adpd data logged in AWT)
#>send_ext_adpd_datastream csvfilename start_row column_index
        """
        args      = self._parse_args(arg, 3)
        num_ops   = len(args)
        src_addr  = get_cli_addr()
        start_row = int(args[1])
        column_index = int(args[2])
        sequence_counter=0
        delay        = 0.01
        max_rtc_ticks_24_hr = 2764800000
        column_data_list = read_csv_column(str(args[0]),column_index,start_row)
        with open(str(args[0]), 'r') as f_ref:
            line_list = f_ref.readline()
            time_hrmmss=line_list.split(',')[2].strip()
            tz_seconds=int(line_list.split(',')[4].strip())
            hours=int(time_hrmmss[0:2])
            minutes=int(time_hrmmss[3:5])
            seconds=int(time_hrmmss[6:8])
            absolute_time_ms = ((hours*3600)+(minutes*60)+seconds+tz_seconds)*1000
        column_time_list = read_csv_column(str(args[0]),0,start_row)
        for row in range(len(column_data_list)):
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000,adpd_ext_data_stream_t())
            msg.header.src = src_addr
            msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_EXT_ADPD_DATA_STREAM
            msg.payload.data = int(column_data_list[row])
            msg.payload.timestamp = int(((column_time_list[row]+absolute_time_ms) * 32.768) % max_rtc_ticks_24_hr) # converting ms to ticks
            sequence_counter = sequence_counter + 1
            msg.payload.sequence_num = sequence_counter
            self._send_packet(msg)
            time.sleep(delay)

    def do_LTAppReadCh2Cap(self, arg):
        """
Read the AD7156 CH2 Capacitace in uF. This returns SUCCESS, if LT app is ruuning, else ERROR
	This is used to note the Cap value in different surfaces
    #>LTAppReadCh2Cap [ntimes]
    where 'ntimes' is the no:of times Cap value needs to be read
    -----------------------------------------------
Usage:
    #>LTAppReadCh2Cap [ntimes]
    #>LTAppReadCh2Cap 6
        """
        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.write("No arguments supplied!")
            return
			
        ntimes = int(args[0])

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_rd_ch2_cap())
        msg.payload.command = M2M2_LT_COMMAND_ENUM_t.M2M2_LT_COMMAND_RD_CH2_CAP_REQ

        avgCap = 0
        minCap = 1490
        maxCap =0

        for i in range(ntimes):
          self._send_packet(msg)
          reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_rd_ch2_cap(), 60)
          if reply_msg != None:
            avgCap += reply_msg.payload.capVal
            if reply_msg.payload.capVal > maxCap:
              maxCap = reply_msg.payload.capVal
            if reply_msg.payload.capVal < minCap:
              minCap = reply_msg.payload.capVal
            self.vrb.write("Ch2 Cap Value:{}".format(reply_msg.payload.capVal))
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))
          else:
            print "Read Ch2 Cap failed!"
            return
        avgCap = avgCap/ntimes
        self.vrb.write("Average Ch2 Cap: '{}'".format(avgCap))
        self.vrb.write("Minimum Ch2 Cap: '{}'".format(minCap))
        self.vrb.write("Maximum Ch2 Cap: '{}'".format(maxCap))
        return avgCap, minCap, maxCap

    def do_lcfgLTAppRead(self, arg):
        """
Read the LT application LCFG used in the Watch. The argument are the LT LCFG addresses:
    Note that the the range of addr varies from 0x0 to 0x3, as given below:
    LT_APP_LCFG_ONWR_TIME = 0x0
    LT_APP_LCFG_OFFWR_TIME = 0x1
    LT_APP_LCFG_AIR_CAP_VAL = 0x2
    LT_APP_LCFG_SKIN_CAP_VAL = 0x3

      Eg: = lcfgLTAppRead addr1 addr2 ...
        """
        args = self._parse_args(arg, None)
        if len(args) == 0:
            self._p_err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_lcfg_op_hdr_t(num_ops), 20)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg == None:
            print "Reading LT App LCFG failed!"
	    return
        self._print_lt_app_lcfg_result(reply_msg)

    def do_lcfgLTAppWrite(self, arg):
        """
Set the LT application LCFG. The argument is the LT app LCFG addresses or the LCFG ID, then VALUE to modify the lt app lcfg value:
    Note that the the range of addr varies from 0x0 to 0x3 as given below:
    LT_APP_LCFG_ONWR_TIME = 0x0
    LT_APP_LCFG_OFFWR_TIME = 0x1
    LT_APP_LCFG_AIR_CAP_VAL = 0x2
    LT_APP_LCFG_SKIN_CAP_VAL = 0x3

      Eg: = lcfgLTAppWrite addr1 value1 addr2 value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self._p_err("No arguments supplied!")
            return

        num_ops = len(args)
        num_ops >>= 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i*2]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            tempVal = args[i*2+1]
            if ("0x") in tempVal:
                reg_val = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_val = int(tempVal, 16)
            else:
                reg_val = int(tempVal)
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP, lt_app_lcfg_op_hdr_t(num_ops), 60)
        if reply_msg == None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            print "Writing LT App LCFG failed!"
	    return
        self._print_lt_app_lcfg_result(reply_msg)

    def _print_lt_app_lcfg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))

        t = table(["Field", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].field), packet.payload.ops[i].value])
        t.display()

    def do_LTAppTuning(self, arg):
        """
1. Do the tuning of LT App lcfg paramters to change either fw lcfg or add DCB lcfg
2. Write the LT Configuration NAND/DCB & then do LTAppTuning
For the LT application LCFG, Note that the range of addr varies from 0x0 to 0x3 as given below:
    LT_APP_LCFG_ONWR_TIME = 0x0
    LT_APP_LCFG_OFFWR_TIME = 0x1
    LT_APP_LCFG_AIR_CAP_VAL = 0x2
    LT_APP_LCFG_SKIN_CAP_VAL = 0x3
Usage:
    LTAppTuning [option] [airCap] [skinCap]
    [option]
        dcb --> To Tune and write to WR_detect_DCB_blk 
        fw  --> To Tune and replace default fw lcfg
    [airCap]
        Value in uF as observed from Display page which reads Ch2 Cap, when Watch is placed in Air, without touching bottom touch electrodes
        Try to keep a min value of the lot, observed in about 10 trials
    [skinCap]
        Value in uF as observed from Display page which reads Ch2 Cap, when Watch is placed in skin, touching the bottom touch electrodes
        Try to keep a max value of the lot, observed in about 10 trials
      Eg: = LTAppTuning fw  1400 1380
      Eg: = LTAppTuning dcb 1285 1250
        """

        args = self._parse_args(arg, None)
        if len(args) == 0 or len(args) != 3:
            self.vrb.write("Wrong arguments supplied!")
            return

        option = args[0]
        minCap_air = args[1]
        maxCap_skin = args[2]

        #self.vrb.write("Deactivating LT app")
        #self.do_pm_deactivate_touch_sensor("0")
        if option == "fw" :
            #self.vrb.write("Place Watch on Air, without touching bottom touch electrodes")
            #self.do_delay("5")
            #avgCap_air, minCap_air, maxCap_air = self.do_LTAppReadCh2Cap("100")
            #
            #self.vrb.write("Place Watch bottom touch electrodes on skin & tighten the straps")
            #self.do_delay("5")
            #avgCap_skin, minCap_skin, maxCap_skin = self.do_LTAppReadCh2Cap("100")
            #
            #cmd = "lcfgLTAppWrite 0x2 " + str(minCap_air)
            #self.onecmd(cmd)
            #cmd = "lcfgLTAppWrite 0x3 " + str(maxCap_skin)
            #self.onecmd(cmd)

            cmd = "lcfgLTAppWrite 0x2 " + minCap_air
            self.onecmd(cmd)
            cmd = "lcfgLTAppWrite 0x3 " + maxCap_skin
            self.onecmd(cmd)
            self.do_pm_activate_touch_sensor("0")
        elif option == "dcb" :
            #self.vrb.write("Place Watch on Air, without touching bottom touch electrodes")
            #self.do_delay("5")
            #avgCap_air, minCap_air, maxCap_air = self.do_LTAppReadCh2Cap("100")
            #
            #self.vrb.write("Place Watch bottom touch electrodes on skin & tighten the straps")
            #self.do_delay("5")
            #avgCap_skin, minCap_skin, maxCap_skin = self.do_LTAppReadCh2Cap("100")

            #Write to wrist_detect_dcb.lcfg file
            str1 = "#wrist_detect_dcb.lcfg"
            str2 = "00 1B58 #ONWR_TIME hex value in ms;    7000ms = 0x1B58"
            str3 = "01 1388 #OFFWR_TIME hex value in ms;   5000ms = 0x1388"
            #airCap = str(hex(minCap_air))
            #airCap = airCap.replace('0x','0')
            #str4 = "02 " +  airCap + " #AIR_CAP_VAL hex value in uF;"
            #skinCap = str(hex(maxCap_skin))
            #skinCap = skinCap.replace('0x','0')
            #str5 = "03 " + skinCap + " #SKIN_CAP_VAL hex value in uF;"

            airCap = str(hex(int(minCap_air,10)))
            airCap = airCap.replace('0x','0')
            str4 = "02 " +  airCap + " #AIR_CAP_VAL hex value in uF;"
            skinCap = str(hex(int(maxCap_skin,10)))
            skinCap = skinCap.replace('0x','0')
            str5 = "03 " + skinCap + " #SKIN_CAP_VAL hex value in uF;"

            f = open('dcb_cfg/wrist_detect_dcb.lcfg','w')
            #f.write('{:02X} {:02X}'.format((reply_msg.payload.dcbdata[ECnt]>>8),(reply_msg.payload.dcbdata[ECnt] & 0xff)))
            f.write(str1)
            f.write('\n')
            f.write(str2)
            f.write('\n')
            f.write(str3)
            f.write('\n')
            f.write(str4)
            f.write('\n')
            f.write(str5)
            f.write('\n')
            f.close()

            #Write to wrist_detect_dcb block
            cmd = "write_dcb_config wrist_detect wrist_detect_dcb.lcfg"
            self.onecmd(cmd)
        else:
            self.vrb.write("Wrong option given, check help & retry")
        #self.vrb.write("Activating LT app")
        #self.do_pm_activate_touch_sensor("0")

    def _print_ppg_lcfg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))

        t = table(["Field", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].field), hex(packet.payload.ops[i].value)])
        t.display()

    def _print_file_system_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, packet.payload.status)
            if status == None:
                self.vrb.err("Error decoding a packet's return status value!")
                return
        self.vrb.write("  Status: '{}'".format(status))

    def _print_file_count_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, packet.payload.status)
            if status == None:
                self.vrb.err("Error decoding a packet's return status value!")
                return
        self.vrb.write("  Status: '{}'".format(status))
        self.vrb.write("  No. of Files Present: '{}'".format(packet.payload.file_count))

    def _print_file_info_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, packet.payload.status)
            if status == None:
                self.vrb.err("Error decoding a packet's return status value!")
                return
            self.vrb.write("  Status: '{}'".format(status))
            self.vrb.write("  File Name: {}".format(cast(packet.payload.file_name, c_char_p).value))
            self.vrb.write("  Start Page used: '{}'".format(packet.payload.start_page))
            self.vrb.write("  End Page used: '{}'".format(packet.payload.end_page))
            self.vrb.write("  File Size: '{}'".format(packet.payload.file_size))

    def _print_page_read_test_status(self, packet,print_en):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, packet.payload.status)
            if status == None:
                self.vrb.err("Error decoding a packet's return status value!")
                return
            self.vrb.write("  Page Number Read: '{}'".format(packet.payload.page_num))
            self.vrb.write("  Data Region status: '{}'".format(packet.payload.data_region_status))            
            self.vrb.write("  ECC Zone Status: '{}'".format(packet.payload.ecc_zone_status))
            if(print_en):
                self.vrb.write("  Next Writeable Page: {}".format(packet.payload.next_page))
                self.vrb.write("  Is Page Occupied: '{}'".format(packet.payload.occupied))
                self.vrb.write("  Number of bytes to read: '{}'".format(packet.payload.num_bytes))
                print(" Samples of Data read : ")
                cnt = 0
                for item in packet.payload.sample_data[:packet.payload.num_bytes]:
                    cnt +=1
                    print( "{}: '{}'".format(cnt,item))
            
    def _print_packet_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            self.vrb.err("Error decoding a packet's return status value!")
            return
        self.vrb.write("  Status: '{}'".format(status))

    def _print_low_touch_status(self, packet):
        #status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, packet.payload.status)
        #if status == None:
        #    status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            self.vrb.err("Error decoding a packet's return status value!")
            return
        self.vrb.write("  Status: '{}'".format(status))

    def _print_reg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))

        t = table(["Address", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].address), hex(packet.payload.ops[i].value)])
        t.display()

    def _print_dcfg_result(self, resp_packets, sensor):
        t = table(["Address", "Value"])
        for j in range(len(resp_packets)):
            packet = resp_packets[j]
            for i in range(packet.payload.size):
                if sensor == 'adpd4000' or sensor == 'temperature':
                    firstbyte = packet.payload.dcfgdata[4*i];
                    secondbyte = packet.payload.dcfgdata[4*i+1];
                    thirdbyte = packet.payload.dcfgdata[4*i+2];
                    fourthbyte = packet.payload.dcfgdata[4*i+3];
                    address = (thirdbyte | (fourthbyte << 8));
                    value = (firstbyte | (secondbyte << 8));
                    t.add_row([hex(address), hex(value)])
                elif sensor == 'adxl':
                    firstbyte = packet.payload.dcfgdata[2*i];
                    secondbyte = packet.payload.dcfgdata[2*i+1];
                    address = secondbyte;
                    value = firstbyte;
                    t.add_row([hex(address), hex(value)])
        t.display()

    def _print_sensor_app_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = int(packet.payload.status)
        stream = self._get_enum_name(M2M2_ADDR_ENUM_t, packet.header.src)
        if stream == None:
            stream = hex(packet.payload.stream)
        num_subscribers = int(packet.payload.num_subscribers)
        num_start_reqs = int(packet.payload.num_start_reqs)
        self.vrb.write("Application: {}:".format(stream))
        self.vrb.write("  Status: '{}'".format(status))
        self.vrb.write("  Stream ID: '{}'".format(stream))
        self.vrb.write("  Number of Subscribers: '{}'".format(num_subscribers))
        self.vrb.write("  Number of Start Requests: '{}'".format(num_start_reqs))

    def _print_sensor_apps_info_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = int(packet.payload.status)
        stream = self._get_enum_name(M2M2_ADDR_ENUM_t, packet.header.src)
        if stream == None:
            stream = hex(packet.payload.stream)
        #self.vrb.write("Resp Pkt: {}:".format((packet)))
        num_sensor_apps = int(packet.payload.num_sensor_apps)
        self.vrb.write("Number of sensor apps on PS board: {} \n".format(num_sensor_apps))
        for i in range(num_sensor_apps):
            sensor_application = self._get_enum_name(M2M2_ADDR_ENUM_t,packet.payload.app_info[i].sensor_app_id)
            num_subscribers = int(packet.payload.app_info[i].num_subscribers)
            num_start_reqs = int(packet.payload.app_info[i].num_start_reqs)
            self.vrb.write("Sensor Application: {}:".format(sensor_application))
            self.vrb.write("  Number of Subscribers: '{}'".format(num_subscribers))
            self.vrb.write("  Number of Start Requests: '{}'\n\n".format(num_start_reqs))

    def _reg_op(self, address, ops, do_write):
        '''
        Operate on some registers. The address is 'address', 'ops' is a list of
        address:value dictionaries, and do_write is True to write, False to read.
        '''
        num_ops = len(ops)
        if address == 49412:
            msg = m2m2_packet(address, m2m2_sensor_common_reg_op_32_hdr_t(num_ops))
            if do_write == True:
                cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_WRITE_REG_32_REQ
            elif do_write == False:
                cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_READ_REG_32_REQ
        else:
            msg = m2m2_packet(address, m2m2_sensor_common_reg_op_16_hdr_t(num_ops))
            if do_write == True:
                cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ
            elif do_write == False:
                cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_READ_REG_16_REQ

        msg.payload.command = cmd
        msg.payload.num_ops = num_ops

        for i in range(0, num_ops):
            msg.payload.ops[i].address = ops[i]['address']
            msg.payload.ops[i].value = ops[i]['value']
        self._send_packet(msg)
        if address == 49412:
            return self._get_packet(address, m2m2_sensor_common_reg_op_32_hdr_t(num_ops))
        else:
            return self._get_packet(address, m2m2_sensor_common_reg_op_16_hdr_t(num_ops))

    def _dispatcher(self, q_map, s_map):
        """
        The 'dispatcher' thread function. This takes messages from the server, and
        inserts them into a queue based on their source address.
        """
        self.vrb.write("Dispatcher up!", 4)
        while True:
          packet = self.rx_q.get()
          pkt_header = pack_header(packet)
          if int(pkt_header.src) == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_DBG_STREAM:
            pkt = m2m2_packet(0, m2m2_debug_data_t())
            pkt.unpack(packet)
            self.vrb.write("##DBG:{}".format((cast(pkt.payload.str, c_char_p).value)), 2)
          if int(pkt_header.src) in self.sock_map:
              self.vrb.write("RX::{}".format(format_hex(packet)), 4)
              try:
                  self.sock_map[pkt_header.src].send(packet)
              except:
                  self.sock_map.pop(pkt_header.src, None)
                  addr = self._get_enum_name(M2M2_ADDR_ENUM_t, int(pkt_header.src))
                  if addr == None:
                      addr = ""
                  self.vrb.err("The socket {}({}) was broken!".format(addr, int(pkt_header.src)))
          else:
              self.vrb.write("RX::{}".format(format_hex(packet)), 3)
              try:
                  destination_q = q_map[pkt_header.src].put(packet)
              except:
                  self.vrb.err("Error routing a packet: {}".format(format_hex(packet)))

    def _get_enum_name(self, enum, value):
        """
        Search an enum class "enum" to see if it contains an attribute "value". Use this
        to get enum name strings from their raw integer values.
        """
        try:
            name = vars(enum).keys()[vars(enum).values().index(value)]
        except Exception as e:
            self.vrb.write("Couldn't find value {} in enum {}".format(value, type(enum).__name__), 4)
            name = None
        return name

    def _send_packet(self, msg):
        """
        Send a packet to the server.
        """
        if self.m2m2_server == None or not self.m2m2_server.is_connected():
            self.vrb.err("Not connected to a serial device!")
            return
        try:
            pkt = msg.pack()
        except:
            pkt = msg
        if lowtouch.Enable_lowtouch == True:
            if lowtouch.Startcmd == True:
                lowtouch.Startcmdcount += 1
                lowtouch.Startcmdlen += len(pkt)
            elif lowtouch.Stopcmd == True:
                lowtouch.Stopcmdcount += 1
                lowtouch.Stopcmdlen += len(pkt)
            lowtouch.User_File += pkt
            self.vrb.write("TX::{}".format(format_hex(pkt)))
            return None
        self.vrb.write("TX::{}".format(format_hex(pkt)), 3)
        self.tx_q.put(pkt)

    def _get_packet(self, source, payload, timeout = 1):
        """
        Get block for 'timeout' seconds waiting for a message from the 'source'
        address. Returns None if it times out.
        """
        if lowtouch.Enable_lowtouch == True:
            return None
        packet = m2m2_packet(0, payload)
        try:
            raw_pkt = self.dispatcher_map[source].get(timeout=timeout)
        except Queue.Empty:
            return None
        try:
            packet.unpack(raw_pkt)
            return packet
        except Exception as e:
            self.vrb.err("Error unpacking a packet: {}".format(e))
            return None


    def _parse_args(self, arg, num_args = None):
        """
        Check that an 'arg' string contains the correct number of arguments,
        and returns a list of those arguments. Set num_args = None to prevent
        checking.
        """
        args = arg.split()
        if num_args == None:
            return args
        if len(args) != num_args:
            self.vrb.err("Incorrect number of arguments! You passed: '{}'".format(arg))
            return None
        else:
            return args

    def _get_version(self, address):
        msg = m2m2_packet(address, m2m2_app_common_version_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_GET_VERSION_REQ
        self._send_packet(msg)
        return self._get_packet(address, m2m2_app_common_version_t(),1000)

    def _print_version_pkt(self, packet):
        app_name = self._get_enum_name(M2M2_ADDR_ENUM_t, packet.header.src)
        if app_name == None:
            app_name = "{}".format(hex(packet.header.src))
        try:
            self.vrb.write("Version info from '{}':".format(app_name))
            self.vrb.write("  Major: '{}'".format(int(packet.payload.major)))
            self.vrb.write("  Minor: '{}'".format(int(packet.payload.minor)))
            self.vrb.write("  Patch: '{}'".format(int(packet.payload.patch)))
            self.vrb.write("  String: '{}'".format(cast(packet.payload.verstr, c_char_p).value))
            # Cast the git_version string to a c_char_p pointer, and access its value as a Python string
            self.vrb.write("  String: '{}'".format(cast(packet.payload.str, c_char_p).value))
        except Exception as e:
            self.vrb.err("There was an error printing the version information: {}".format(e))

    def _print_file_list(self, packet):
        file_info_dict = {}
        try:
            filetype = self._get_enum_name(FILE_TYPE_ENUM_t, packet.payload.filetype)
            filesize = int(packet.payload.filesize)
            self.vrb.write("FILE: {}".format(cast(packet.payload.full_file_name, c_char_p).value))
            self.vrb.write("FILE_TYPE: '{}'".format(filetype))
            self.vrb.write("FILE_SIZE: '{}'\n".format(filesize))
            file_info_dict = {'file': cast(packet.payload.full_file_name, c_char_p).value,
                              'file_type': filetype, 'file_size': filesize}
        except Exception as e:
            self.vrb.err("There was an error printing the file list: {}".format(e))
        return file_info_dict

    def _subscribe(self, address, stream, sub_unsub):
        msg = m2m2_packet(address, m2m2_app_common_sub_op_t())
        if sub_unsub == True:
            command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_SUBSCRIBE_REQ
        elif sub_unsub == False:
            command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_STREAM_UNSUBSCRIBE_REQ
        else:
            return None
        msg.payload.command = command
        msg.payload.stream = stream
        self._send_packet(msg)
        return self._get_packet(address, m2m2_app_common_sub_op_t())

    def _print_subscription_status(self, packet):
        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, packet.payload.status)
        if status == None:
            status = int(packet.payload.status)
        stream = self._get_enum_name(M2M2_ADDR_ENUM_t, packet.payload.stream)
        if stream == None:
            stream = hex(packet.payload.stream)
        source = self._get_enum_name(M2M2_ADDR_ENUM_t, packet.header.src)
        if source == None:
            source = hex(packet.header.src)
        self.vrb.write("Application: {}:".format(source), 2)
        self.vrb.write("  Status: '{}'".format(status))
        self.vrb.write("  Stream ID: '{}'".format(stream), 2)

    def _LoadCfg(self, filename):
        print filename
        try:
            f = open(filename)
        except IOError:
            print filename + "not found"
        else:
            lines = f.readlines()
            f.close()

            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(1))
            cmd = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_WRITE_REG_16_REQ
            msg.payload.command = cmd
            msg.payload.num_ops = 1
            timeStart = time.time()
            for line in lines:
                if not line.strip():
                    continue
                splitline = line.rstrip('\n').split(None,2)
                try:
                    address = int(splitline[0], 16)
                    value = int(splitline[1], 16)
                    if (len(splitline)>2):
                        comment = splitline[2]
                    else:
                        comment = ''
                    #Bridge.writeReg(address, value)
                    print ("Set [0x%04x] = 0x%04x    " % (address, value) + comment)

                    msg.payload.ops[0].address = address
                    msg.payload.ops[0].value = value
                    self._send_packet(msg)
                    reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_common_reg_op_16_hdr_t(1), 1)

                except ValueError:
                    print "    " + line
            elaspTime = time.time()
            elaspTime -= timeStart
            print "Loading configure file done. Elasp time=%f" %(elaspTime)
        return
    def _get_free_port(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.bind(('localhost', 0))
        except socket.error as msg:
            print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
            return None
        s.listen(1)
        port = s.getsockname()[1]
        s.close()
        return port
    def do_setDateTime(self, arg):
        """
Set date and time. Current PC time will be sent to the device.
    #>setDateTime
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_date_time_req_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SET_DATE_TIME_REQ

        now = datetime.datetime.now()
        is_dst = time.daylight and time.localtime().tm_isdst > 0
        utc_offset = - (time.altzone if is_dst else time.timezone)
        msg.payload.year   = now.year
        msg.payload.month  = now.month
        msg.payload.day    = now.day
        msg.payload.hour   = now.hour
        msg.payload.minute = now.minute
        msg.payload.second = now.second
        msg.payload.TZ_sec = utc_offset
        self._send_packet(msg)
        self.vrb.write("date and time: {}".format(now), 2)
        self.vrb.write("timezone: {}".format(utc_offset), 2)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))

    def do_getDateTime(self, arg):
        """
Get date and time.Getting PM current date and time.
    #>getDateTime
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_date_time_req_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("date and time: {}-{:02}-{:02} {:02}:{:02}:{:02}".format((int(reply_msg.payload.year)),(int(reply_msg.payload.month)),(int(reply_msg.payload.day)),(int(reply_msg.payload.hour)),(int(reply_msg.payload.minute)),(int(reply_msg.payload.second))))
        self.vrb.write("timezone: {}".format(int(reply_msg.payload.TZ_sec)))
        self.vrb.write("Command return status: {}".format(status))
        now = datetime.datetime.now()
        self.vrb.write("PC Time: {}".format(now))
        date_time_dict = {'year': int(reply_msg.payload.year), 'month': int(reply_msg.payload.month), 'day': int(reply_msg.payload.day),
                          'hour': int(reply_msg.payload.hour), 'min': int(reply_msg.payload.minute), 'sec': int(reply_msg.payload.second),
                          'timezone': int(reply_msg.payload.TZ_sec)}
        err_stat = int(status, 16)
        return err_stat, date_time_dict

    def do_setManufactureDate(self, arg):
        """
Set the Manufacture Date of Watch, from the current PC time.
    #>setManufactureDate
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_manufacture_date_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SET_MANUFACTURE_DATE_REQ

        now = datetime.datetime.now()
        msg.payload.year   = now.year
        msg.payload.month  = now.month
        msg.payload.day    = now.day
        self._send_packet(msg)
        self.vrb.write("Manufacture date being set to : {} {} {}".format(now.year,now.month,now.day), 1)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_manufacture_date_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))

    def do_getManufactureDate(self, arg):
        """
Get the Manufacture Date of Watch.
    #>getDateTime
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_manufacture_date_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_MANUFACTURE_DATE_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_manufacture_date_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Manufacture date: {}-{:02}-{:02}".format((int(reply_msg.payload.year)),(int(reply_msg.payload.month)),(int(reply_msg.payload.day))))
        self.vrb.write("Command return status: {}".format(status))

    def do_system_reset(self, arg):
        """
Send a command to the Device to Restart - software reset.
    #>system_reset
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SYSTEM_RESET_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t(), 3)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))
        #self.m2m2_server.quit()

    def do_system_hw_reset(self, arg):
        """
Send a command to the Device to Restart - hardware reset.
    #>system_hw_reset
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SYSTEM_HW_RESET_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t(), 3)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))
        self.vrb.write("Wait for 25.6 secs for reset to happen")
        #self.m2m2_server.quit()

    def get_dcfg_data_list(self, f_path):
        """
	    To get address & variable list from a file that contains dcfg
        :param f_path:
        :return:
        """
        with open(f_path, 'r') as f_ref:
            line_list = f_ref.readlines()
        addr_list, val_list = [], []
        for line in line_list:
            if line.strip() and line.strip()[0] != '#':
                addr_val_list = line.split(' ')
                addr_list.append(addr_val_list[0].strip())
                val_list.append(addr_val_list[1].strip())
        return addr_list, val_list

    def do_loadAdpdUCDcfg(self, arg):
        """
Load ADPD device with UC dcfg using "reg w add:val" command. Argument to be passed is:
 [uc] as defined below:
        1 --> UC1
        2 --> UC2
        ...
        5 --> UC5
 [dvt_revision]:
        dvt1 --> To take UC dcfg for DVT1 board from mv_uc_dcfg folder
        dvt2 --> To take UC dcfg for DVT2 board from mv_uc_dcfg folder
-----------------------------------------------
Usage:
        #>loadAdpdUCDcfg [uc] [dvt_revision]
        #>loadAdpdUCDcfg 2 dvt1
        #>loadAdpdUCDcfg 5 dvt2
        """
        args = self._parse_args(arg, None)
        if len(args) == 0 or len(args) != 2:
            self._p_err("Wrong arguments supplied!")
            return
        uc = int(args[0])
        dvt_revision = args[1]
        curr_dir = os.getcwd()
        dcb_cfg_dir = os.path.join(curr_dir, 'mv_uc_dcfg')
        if uc == 1 and dvt_revision == "dvt1":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT1_UseCase1.dcfg')
        elif uc == 1 and dvt_revision == "dvt2":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT2_UseCase1.dcfg')
        elif uc == 2 and dvt_revision == "dvt1":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT1_UseCase2.dcfg')
        elif uc == 2 and dvt_revision == "dvt2":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT2_UseCase2.dcfg')
        elif uc == 3 and dvt_revision == "dvt1":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT1_UseCase3.dcfg')
        elif uc == 3 and dvt_revision == "dvt2":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT2_UseCase3.dcfg')
        elif uc == 4 and dvt_revision == "dvt1":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT1_UseCase4.dcfg')
        elif uc == 4 and dvt_revision == "dvt2":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT2_UseCase4.dcfg')
        elif uc == 5 and dvt_revision == "dvt1":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT1_UseCase5.dcfg')
        elif uc == 5 and dvt_revision == "dvt2":
            dcb_file = os.path.join(dcb_cfg_dir, 'DVT2_UseCase5.dcfg')
        else:
            self.vrb.write("Wrong UC/DVT revision selected. Check help & retry")
            return
        addr_list, val_list = self.get_dcfg_data_list(dcb_file)
        j=0
        add_val_list = "adpd4000 w"
        for i, addr in enumerate(addr_list):    
            add_val = " 0x" + addr + ":0x" + val_list[i]
            add_val_list = add_val_list + add_val
            j+=1
            if j == 30:
                self.do_reg(add_val_list)
                j=0
                add_val_list = "adpd4000 w "
            #self.do_reg('{} w 0x{}:0x{}'.format(dev, addr, val_list[i]))
            #self.do_reg('adpd4000 w 0x{}:0x{}'.format(addr, val_list[i]))
        self.do_reg(add_val_list)

    def do_delay(self, arg):
        """
Give a fixed delay to be used in between running the test
-----------------------------------------------
Usage:
    #>delay 2
        """
        args = self._parse_args(arg)
        try:
            delay = int(args[0])
        except:
            delay = 5
        time.sleep(delay)

    def do_msg_verbose(self, arg):

        args = self._parse_args(arg, 1)
        if args == None:
            return
        try:
            lvl = int(args[0])
            self.vrb.set_level(lvl)
        except:
            self.vrb.err("Invalid argument!", 1)
    
    def do_read_dcb_config(self,arg):
        """
Send a command to Read the DCB Configurations of the specific sensor of the Board, 
which is then saved into a file with name [sensor_name]_dcb_get.dcfg (ex. - adxl_dcb_get.dcfg), which will be present in 'tools/dcb_dcfg/' directory.
Currently dcb configurations can be read for adpd4000, adxl, ppg, ecg, eda, low_touch, ad7156, wrist_detect.
ex. read_dcb_config [sensor_name]
    #>read_dcb_config adpd4000
        """
        Sensor_Address = None
        args = self._parse_args(arg,1)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return 1

        for a in args:
            if a in application_name_map:
                Sensor_Address = application_name_map[a]["address"]
        if Sensor_Address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return 1

        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adxl_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ppg_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ecg_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_eda_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_bcm_data_t())    
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ad7156_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t())
        else:
             print "The requested config dcb is not supported"
             return
        Array_Element = None
        msg.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_READ_CONFIG_REQ
        #msg.payload.size = rdSize #len(Adpd_org_4000_g) + 1
        self._send_packet(msg)

        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adxl_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ppg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ecg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_eda_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_bcm_data_t(), 20)    
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ad7156_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t(), 20)
        else:
            pass
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return 1
        status = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')

        ECnt = 0
        Array_Element_Count_r = int(reply_msg.payload.size)
        
        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            f = open('dcb_cfg/adxl_dcb_get.dcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:04X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:02X}'.format((reply_msg.payload.dcbdata[ECnt]>>8),(reply_msg.payload.dcbdata[ECnt] & 0xff)))
                f.write('\n')
                ECnt+=1
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            num_of_pkts = int(reply_msg.payload.num_of_pkts)
            if num_of_pkts > 1:
                reply_msg2 = self._get_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t(), 20)
                if reply_msg2 == None:
                    self.vrb.err("Error! Timed out waiting for the device! at pkt 2")
                    return 1
                status2 = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg2.payload.status)
                if status2 == None:
                    status2 = format(reply_msg2.payload.status, '#04x')
            f = open('dcb_cfg/adpd4000_dcb_get.dcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:04X} {:04X}'.format((reply_msg.payload.dcbdata[ECnt]>>16),(reply_msg.payload.dcbdata[ECnt] & 0xffff)))
                f.write('\n')
                ECnt+=1
            if num_of_pkts > 1:
                Array_Element_Count_r = int(reply_msg2.payload.size)
                ECnt = 0
                while ECnt < Array_Element_Count_r:
                    self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg2.payload.dcbdata[ECnt]),ECnt))
                    f.write('0x{:08X}'.format(((reply_msg2.payload.dcbdata[ECnt]))))
                    f.write('\n')
                    ECnt+=1
                Array_Element_Count_r = int(reply_msg.payload.size) + int(reply_msg2.payload.size) #Update total read size
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            f = open('dcb_cfg/ppg_dcb_get.lcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:08X}'.format(ECnt,(reply_msg.payload.dcbdata[ECnt])))
                f.write('\n')
                ECnt+=1
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            f = open('dcb_cfg/ecg_dcb_get.lcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:04X}'.format(((reply_msg.payload.dcbdata[ECnt]>>16) & 0xff),(reply_msg.payload.dcbdata[ECnt] & 0xffff)))
                f.write('\n')
                ECnt+=1
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            f = open('dcb_cfg/eda_dcb_get.lcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:04X}'.format(((reply_msg.payload.dcbdata[ECnt]>>16) & 0xff),(reply_msg.payload.dcbdata[ECnt] & 0xffff)))
                f.write('\n')
                ECnt+=1
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            f = open('dcb_cfg/bcm_dcb_get.lcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:04X}'.format(((reply_msg.payload.dcbdata[ECnt]>>16) & 0xff),(reply_msg.payload.dcbdata[ECnt] & 0xffff)))
                f.write('\n')
                ECnt+=1        
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            num_of_pkts = int(reply_msg.payload.num_of_pkts)
            reply_msg_list = []
            status_list = []
            p = 1 #Start with second packet
            while p < num_of_pkts:
                _reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
                if _reply_msg == None:
                    self.vrb.err("Error! Timed out waiting for the device! at pkt 2")
                    return 1
                status = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, _reply_msg.payload.status)
                if status == None:
                    status = format(_reply_msg.payload.status, '#04x')
                reply_msg_list.append(_reply_msg)
                status_list.append(status)
                p = p+1
            f = open('dcb_cfg/gen_blk_dcb_get.lcfg','w')
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('0x{:08X}'.format(((reply_msg.payload.dcbdata[ECnt]))))
                f.write('\n')
                ECnt+=1
            print(len(reply_msg_list))
            Array_Element_Count_r = int(reply_msg.payload.size)
            p = 0 #Start with second packet, index=0 from reply_msg_list
            while p < (num_of_pkts-1):
                reply_msg = reply_msg_list[p]
                Array_Element_Count_r_pkt = int(reply_msg.payload.size)
                ECnt = 0
                while ECnt < Array_Element_Count_r_pkt:
                    self.vrb.write("Read Settings : 0x{:08X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                    f.write('0x{:08X}'.format(((reply_msg.payload.dcbdata[ECnt]))))
                    f.write('\n')
                    ECnt+=1
                Array_Element_Count_r = Array_Element_Count_r + int(reply_msg.payload.size) #Update total read size
                p = p+1
            status = status_list[p-2]
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            f = open('dcb_cfg/ad7156_dcb_get.dcfg','w')            
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:04X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:02X}'.format((reply_msg.payload.dcbdata[ECnt]>>8),(reply_msg.payload.dcbdata[ECnt] & 0xff)))
                f.write('\n')
                ECnt+=1
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            f = open('dcb_cfg/wrist_detect_dcb_get.dcfg','w')      
            while ECnt < Array_Element_Count_r:
                self.vrb.write("Read Settings : 0x{:04X} {}".format(int(reply_msg.payload.dcbdata[ECnt]),ECnt))
                f.write('{:02X} {:02X}'.format((reply_msg.payload.dcbdata[ECnt]>>8),(reply_msg.payload.dcbdata[ECnt] & 0xff)))
                f.write('\n')
                ECnt+=1
        else:
           pass
        f.close
        self.vrb.write("Size  : {:02}".format(int(Array_Element_Count_r)))
        self.vrb.write("Command return status: {}".format(status))
        if status.upper() == 'M2M2_DCB_STATUS_OK':
            return 0
        else:
            return 1
        

    def do_write_dcb_config(self,arg):
        """
Send a command to Write the DCB Configurations of the specific sensor of the Board from its dcfg file.
Currently the dcb configuration to be written is read from a dcfg file, stored in 'tools/dcb_dcfg/' directory.
The dcb configurations can be written for adpd4000, adxl, ppg, ecg eda, low_touch, ad7156, wrist_detect.
ex. write_dcb_config [sensor_name] [file_name]
    #>write_dcb_config adxl adxl_dcb.dcfg
        """
        Sensor_Address = None
        pkt_cnt = 1
        args = self._parse_args(arg,2)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return 1
        filename = 'dcb_cfg/' + args[1]
        for a in args:
            if a in application_name_map:
                Sensor_Address = application_name_map[a]["address"]
        if Sensor_Address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return 1
            
        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            adpd4000_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    dcb =str[0].replace(' ','').replace('\t','').replace('\n','')
                    adpd4000_dcb_cfg.append(int(dcb,16))
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t())
            if( len(adpd4000_dcb_cfg) <= MAXADPD4000DCBSIZE ):
                msg.payload.num_of_pkts = 1
            elif( len(adpd4000_dcb_cfg) > MAXADPD4000DCBSIZE) and ( len(adpd4000_dcb_cfg) <= (2*MAXADPD4000DCBSIZE) ):
                pkt_cnt = 2
                msg.payload.num_of_pkts = 2
                msg2 = m2m2_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t())
                msg2.payload.num_of_pkts = msg.payload.num_of_pkts
                msg2.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
            else:
                print "ADPD4000 DCB File Size exceed. Retry with smaller files"
                return
            print "Number of pkts:{}".format(msg.payload.num_of_pkts)
            Array_Element = adpd4000_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            adxl_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    dcb = str[0].replace(' ','').replace('\t','').replace('\n','')
                    adxl_dcb_cfg.append(int(dcb,16))
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adxl_data_t())
            Array_Element = adxl_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            ppg_dcb_lcfg = arr.array('i',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t' or line[0]=='/'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split('/')
                    str = str[0].split(' ')
                    str =str[1].replace(' ','').replace('\t','').replace('\n','')
                    ppg_dcb_lcfg.append(int(str,16))
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ppg_data_t())
            Array_Element = ppg_dcb_lcfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            ecg_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split(' ')
                    val1 = str[0]
                    val2 = str[1].replace(' ','').replace('\t','').replace('\n','')
                    val = (int(val1,16) << 16) | (int(val2,16))
                    ecg_dcb_cfg.append(val)
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ecg_data_t())
            Array_Element = ecg_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            eda_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split(' ')
                    val1 = str[0]
                    val2 = str[1].replace(' ','').replace('\t','').replace('\n','')
                    val = (int(val1,16) << 16) | (int(val2,16))
                    eda_dcb_cfg.append(val)
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_eda_data_t())
            Array_Element = eda_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            bcm_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split(' ')
                    val1 = str[0]
                    val2 = str[1].replace(' ','').replace('\t','').replace('\n','')
                    val = (int(val1,16) << 16) | (int(val2,16))
                    bcm_dcb_cfg.append(val)
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_bcm_data_t())
            Array_Element = bcm_dcb_cfg    
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            gen_blk_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename,"rb")
            except:
                self.vrb.err("Invalid File Name")
                return 1
            chunk_len = 256
            bytes_read = 0
            if f != None:
                while True:
                    config_buffer = f.read(chunk_len)
                    if not config_buffer:
                        break
                    temp_array = list(bytearray(config_buffer))
                    index = 0
                    while index < len(temp_array):
                        data = [ temp_array[index], temp_array[index+1] , temp_array[index+2], temp_array[index+3] ]
                        index += 4
                        # Show hex values of data.
                        #print(list('%02x' % b for b in data))
                        #Convert to 4 byte unsigned integer data interpreting data as being in little-endian byte order.
                        value = struct.unpack("<I", bytearray(data))[0]
                        #print(hex(value))
                        gen_blk_dcb_cfg.append(value)
                    bytes_read = bytes_read  + len(config_buffer)
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
            pkt_cnt = len(gen_blk_dcb_cfg)/MAXGENBLKDCBSIZE
            if len(gen_blk_dcb_cfg)%MAXGENBLKDCBSIZE :
                pkt_cnt =pkt_cnt + 1
            if pkt_cnt > 4 :
              print "GEN_BLK DCB File Size exceed.Retry with smaller files"
              return
            print "Number of pkts:{}".format(pkt_cnt)

            if pkt_cnt == 1 :
                msg.payload.size = 0
                msg.payload.num_of_pkts = 1
            elif pkt_cnt == 2 :
                msg.payload.size = 0
                msg.payload.num_of_pkts = 2
                #2nd pkt#
                msg2 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg2.payload.size = 0
                msg2.payload.num_of_pkts = msg.payload.num_of_pkts
                msg2.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
            elif pkt_cnt == 3 :
                msg.payload.size = 0
                msg.payload.num_of_pkts = 3
                #2nd pkt#
                msg2 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg2.payload.size = 0
                msg2.payload.num_of_pkts = msg.payload.num_of_pkts
                msg2.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
                #3rd pkt#
                msg3 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg3.payload.size = 0
                msg3.payload.num_of_pkts = msg.payload.num_of_pkts
                msg3.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
            elif pkt_cnt == 4 :
                msg.payload.size = 0
                msg.payload.num_of_pkts = 4
                #2nd pkt#
                msg2 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg2.payload.size = 0
                msg2.payload.num_of_pkts = msg.payload.num_of_pkts
                msg2.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
                #3rd pkt#
                msg3 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg2.payload.size = 0
                msg3.payload.num_of_pkts = msg.payload.num_of_pkts
                msg3.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
                #4th pkt#
                msg4 = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
                msg4.payload.size = 0
                msg4.payload.num_of_pkts = msg.payload.num_of_pkts
                msg4.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
            Array_Element = gen_blk_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            ad7156_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    dcb = str[0].replace(' ','').replace('\t','').replace('\n','')
                    ad7156_dcb_cfg.append(int(dcb,16))
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ad7156_data_t())
            Array_Element = ad7156_dcb_cfg
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            wrist_detect_dcb_cfg = arr.array('I',[])
            try:
                f = open(filename)
            except:
                self.vrb.err("Invalid File Name")
                return 1
            for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                    continue
                else:
                    str = line.split('#')
                    dcb = str[0].replace(' ','').replace('\t','').replace('\n','')
                    wrist_detect_dcb_cfg.append(int(dcb,16))
            f.close()
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t())
            Array_Element = wrist_detect_dcb_cfg
        else:
            Array_Element = NULL
            print "Wrong blk name selected"
            return 1
        msg.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_WRITE_CONFIG_REQ
        ECnt = 0
        Array_Element_Count_w = len(Array_Element)
        while ECnt < Array_Element_Count_w:
            #print "{} {}".format(Array_Element_Count_w, ECnt)
            if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
                if ECnt >= MAXADPD4000DCBSIZE:
                    msg2.payload.dcbdata[ECnt%MAXADPD4000DCBSIZE] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg2.payload.dcbdata[ECnt%MAXADPD4000DCBSIZE]), int(ECnt%(MAXADPD4000DCBSIZE))))
                else:
                    msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:04X} {}".format(int(msg.payload.dcbdata[ECnt]),int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]),int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif (Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
                if ECnt >= (3*MAXGENBLKDCBSIZE) and ECnt <= (4*MAXGENBLKDCBSIZE):
                    msg4.payload.size = msg4.payload.size + 1
                    msg4.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg4.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE]), int(ECnt%(MAXGENBLKDCBSIZE))))
                elif ECnt >= (2*MAXGENBLKDCBSIZE) and ECnt <= (3*MAXGENBLKDCBSIZE):
                    msg3.payload.size = msg3.payload.size + 1
                    msg3.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg3.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE]), int(ECnt%(MAXGENBLKDCBSIZE))))
                elif ECnt >= (1*MAXGENBLKDCBSIZE) and ECnt <= (2*MAXGENBLKDCBSIZE):
                    msg2.payload.size = msg2.payload.size + 1
                    msg2.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg2.payload.dcbdata[ECnt%MAXGENBLKDCBSIZE]), int(ECnt%(MAXGENBLKDCBSIZE))))
                else:
                    msg.payload.size = msg.payload.size + 1
                    msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                    self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
                msg.payload.dcbdata[ECnt] = int(Array_Element[ECnt])
                self.vrb.write("Write Settings : 0x{:08X} {}".format(int(msg.payload.dcbdata[ECnt]), int(ECnt)))
            else:
                pass
            ECnt += 1

        if pkt_cnt == 1:
            msg.payload.size = Array_Element_Count_w
        else:
            if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
                msg.payload.size = MAXADPD4000DCBSIZE
            '''else:
                print"Something went wrong"
                return'''
        self._send_packet(msg)

        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adxl_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ppg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ecg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_eda_data_t(), 20)
        elif (Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_bcm_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
           reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
           reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ad7156_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
           reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t(), 20)
        else:
            pass
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            print"Error! Timed out waiting for the device!"
            return 1
        status = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Size  : {:02}".format(int(msg.payload.size)))
        self.vrb.write("Command return status: {}".format(status))
        if status.upper() != 'M2M2_DCB_STATUS_OK':
            return 1

        if pkt_cnt >= 2:
            if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
                self.vrb.write("Sending gen_blk_dcb packet 2 of size:{}".format(msg2.payload.size))
                #time.sleep(0.01)
                self._send_packet(msg2)
                reply_msg2 = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
            elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
                msg2.payload.size = (ECnt%MAXADPD4000DCBSIZE)
                self.vrb.write("Sending ADPD4000 DCB packet 2 of size:{}".format(msg2.payload.size))
                #time.sleep(0.04)
                self._send_packet(msg2)
                reply_msg2 = self._get_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t(), 20)

            if reply_msg2 == None:
                self.vrb.err("Error! Timed out waiting for the device, with 2nd pkt receive!")
                return 1
            status2 = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg2.payload.status)
            if status2 == None:
                status2 = format(reply_msg2.payload.status, '#04x')
            self.vrb.write("Size  : {:02}".format(int(msg2.payload.size)))
            self.vrb.write("Command return status: {}".format(status2))
            if status2.upper() != 'M2M2_DCB_STATUS_OK':
                return 1
				
        if pkt_cnt >= 3:
            if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
                self.vrb.write("Sending gen_blk_dcb packet 3 of size:{}".format(msg3.payload.size))
                #time.sleep(0.01)
                self._send_packet(msg3)
                reply_msg3 = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
            
            if reply_msg3 == None:
                self.vrb.err("Error! Timed out waiting for the device, with 3rd pkt receive!")
                return 1
            status3 = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg3.payload.status)
            if status3 == None:
                status3 = format(reply_msg3.payload.status, '#04x')
            self.vrb.write("Size  : {:02}".format(int(msg3.payload.size)))
            self.vrb.write("Command return status: {}".format(status3))
            if status3.upper() != 'M2M2_DCB_STATUS_OK':
                return 1
				
        if pkt_cnt >= 4:
            if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
                self.vrb.write("Sending gen_blk_dcb packet 4 of size:{}".format(msg4.payload.size))
                #time.sleep(0.01)
                self._send_packet(msg4)
                reply_msg4 = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
            
            if reply_msg4 == None:
                self.vrb.err("Error! Timed out waiting for the device, with 4th pkt receive!")
                return 1
            status4 = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg4.payload.status)
            if status4 == None:
                status4 = format(reply_msg4.payload.status, '#04x')
            self.vrb.write("Size  : {:02}".format(int(msg4.payload.size)))
            self.vrb.write("Command return status: {}".format(status4))
            if status4.upper() == 'M2M2_DCB_STATUS_OK':
                return 1

        return 0
				
    def do_delete_dcb_config(self,arg):
        """
Send a command to Delete the DCB Configurations of the specific sensor of the Board.
Currently dcb configurations is supported for adpd4000, adxl, ppg, ecg, eda, low_touch, ad7156, wrist_detect.
ex. delete_dcb_config [sensor_name]
    #>delete_dcb_config adpd4000
        """
        Sensor_Address = None
        args = self._parse_args(arg,1)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return 1

        for a in args:
            if a in application_name_map:
                Sensor_Address = application_name_map[a]["address"]
        if Sensor_Address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid device.")
            return 1

        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_adxl_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ppg_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ecg_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_eda_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_bcm_data_t())    
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_ad7156_data_t())
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            msg = m2m2_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t())
        else:
            Array_Element = NULL
        msg.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_ERASE_CONFIG_REQ
        self._send_packet(msg)

        if(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adpd4000_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADXL):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_adxl_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ppg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ecg_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_eda_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_bcm_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_LT_APP):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_gen_blk_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_ad7156_data_t(), 20)
        elif(Sensor_Address == M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM):
            reply_msg = self._get_packet(Sensor_Address, m2m2_dcb_wrist_detect_data_t(), 20)
        else:
            pass
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return 1
        status = self._get_enum_name(M2M2_DCB_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))
        if status.upper() == 'M2M2_DCB_STATUS_OK':
            return 0
        else:
            return 1

    def do_query_dcb_blk_status(self,arg):
        """
Send a command to Query the Status of DCB Block index within the Watch - whether DCB is present / absent.
Currently dcb configurations is supported for adpd4000, adxl, ppg, ecg, eda, low_touch, ad7156, wrist_detect.
All supported DCB Blocks are queried and presented here
ex. query_dcb_blk_status
    #>query_dcb_blk_status
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_dcb_cmd_t())
        msg.payload.command = M2M2_DCB_COMMAND_ENUM_t.M2M2_DCB_COMMAND_QUERY_STATUS_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_dcb_block_status_t(), 20)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Command return status: {}".format(status))
        self.vrb.write("-------------------------------------")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_GENERAL_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_GENERAL_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_GENERAL_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_ADPD4000_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_ADPD4000_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_ADPD4000_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_ADXL362_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_ADXL362_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_ADXL362_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_PPG_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_PPG_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_PPG_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_ECG_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_ECG_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_ECG_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_EDA_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_EDA_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_EDA_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_AD7156_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_AD7156_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_AD7156_BLOCK_IDX Absent")
        if reply_msg.payload.dcb_blk_array[M2M2_DCB_CONFIG_BLOCK_INDEX_t.ADI_DCB_WRIST_DETECT_BLOCK_IDX]:
            self.vrb.write("ADI_DCB_WRIST_DETECT_BLOCK_IDX Present")
        else:
            self.vrb.write("ADI_DCB_WRIST_DETECT_BLOCK_IDX Absent")
        self.vrb.write("-------------------------------------")

    def do_getTimeOffset(self, arg):
        """
Send a command to get the time offset from the Board

    #>getTimeOffset
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_DATE_TIME_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_date_time_req_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')

        """Convert the Read Time from the board to seconds"""
        sHr = int(int(reply_msg.payload.hour - int(12)) * int(3600))
        sMin = int(reply_msg.payload.minute * int(60))
        sSec = int(reply_msg.payload.second)
        TosSec = int(sHr) + int(sMin) + int(sSec)

        """Convert the Time of PC to seconds"""
        Get_Time_now = datetime.datetime.now()
        nHr = int(int(Get_Time_now.hour - int(12)) * int(3600))
        nMin = int(Get_Time_now.minute * int(60))
        nSec = int(Get_Time_now.second)
        TonSec = int(nHr) + int(nMin) + int(nSec)

        """Calculate the Offset Value"""
        ToOffValue = int(TonSec) - int(TosSec)
        if ToOffValue > int(3599):
            ToOffvalue_Dec = float(float(ToOffValue) / float(60)) / float(60)
            ToOffvalueHr = int(ToOffvalue_Dec)
            ToOffvalueMin = int((ToOffvalue_Dec - int(ToOffvalueHr)) * int(60))
            ToOffvalueSec = ((float(ToOffvalue_Dec - int(ToOffvalueHr)) * float(60)) - int(ToOffvalueMin)) * int(60)
            self.vrb.write("OFFSET Raw Value: {}".format((int(ToOffValue))))
            self.vrb.write("OFFSET Value: {:02}:{:02}:{:02}".format((int(ToOffvalueHr)),(int(ToOffvalueMin)), (int(ToOffvalueSec))))
        else:
            ToOffvalue_Dec = float(float(ToOffValue) / float(60))
            ToOffvalueMin = int(ToOffvalue_Dec)
            ToOffvalueSec = int((ToOffvalue_Dec - int(ToOffvalueMin)) * int(60))
            self.vrb.write("OFFSET Raw Value: {}".format((int(ToOffValue))))
            self.vrb.write("OFFSET Value: 00:{:02}:{:02}".format((int(ToOffvalueMin)),(int(ToOffvalueSec))))
    def do_toggleSaveCSV(self,arg):
        """
Enable or disable save as CSV file option while plotting ADXL/ADPD4000/PPG streams
        """
        global enable_csv_logs
        enable_csv_logs ^= 1
        print "Save CSV logs option set to '{}'".format(enable_csv_logs)

    @cli_logger
    def do_controlECGElectrodeSwitch(self, arg):
        """
Control(enable/disable) the switches between ECG electrodes and adpd4k/ad5940/ad8233.
The argument is 'sw_name':
   either of '8233_sw'/'5940_sw'/'4k_sw'
'sw_enable':
   '1' to turn ON Switch
   '0' to turn OFF Switch.
    -----------------------------------------------
Usage:
#>controlECGElectrodeSwitch [sw_name] [sw_enable]
#>controlECGElectrodeSwitch 8233_sw 1
#>controlECGElectrodeSwitch 5940_sw 0
#>controlECGElectrodeSwitch 4k_sw 1
        """
        args = self._parse_args(arg, 2)

        if args == None:
            print("Invalid arguments! please type help <command>(help controlECGElectrodeSwitch) to know the usage.")
            return

        sw_name   = None
        sw_enable = None

        if "8233_sw" in args[0]:
            sw_name = 0
        elif "5940_sw" in args[0]:
            sw_name = 1
        elif "4k_sw" in args[0]:
            sw_name = 2
        else:
            print("Invalid arguments! please type help <command>(help controlECGElectrodeSwitch) to know the usage.")
            return

        if "1" in args[1]:
            sw_enable = 1
        elif "0" in args[1]:
            sw_enable = 0
        else:
            print("Invalid arguments! please type help <command>(help controlECGElectrodeSwitch) to know the usage.")
            return

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_dg2502_sw_ctrl_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_DG2502_SW_CNTRL_REQ
        msg.payload.sw_enable = sw_enable
        msg.payload.sw_name = sw_name
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_dg2502_sw_ctrl_cmd_t(), 10)

        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Boost enable Failed!")

    def do_LDOControl(self, arg):
        """
Control the enabling / disabling of the LDO. The argument is the ldo_num:
    '1' for FS LDO
    '2' for Optical LDO
    '3' for EPHYZ LDO
ldo_ctrl:
    '0' for disable
    '1' for enable
    -----------------------------------------------
Usage:
    #>LDOControl [ldo_num] [ldo_ctrl]
    #>LDOControl 3 1
    #>LDOControl 3 0
        """
        args = self._parse_args(arg, 2)
        if args == None:
            self.vrb.write("No ldo number, ldo control specified, check help and retry")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_ldo_ctrl_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_LDO_CNTRL_REQ
        msg.payload.ldo_num = int(args[0])
        msg.payload.ldo_enable = int(args[1])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_ldo_ctrl_cmd_t(), 10)
        if reply_msg != None:
            status = reply_msg.payload.status
            if status != M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK:
              print "Putting LDO into the desired state failed!"
            else:
              print "Putting LDO into the desired state..  Status: '{}'".format(status)
        else:
            print "Putting LDO into the desired state failed! Please check the arguments"

    def do_getChipID(self, arg):
        """
Get the chip ID from a chip. The argument is the chip name:
    '1' for ADXL362 Part ID
    '2' for ADPD4K Chip ID
    '3' for ADP5360 Device ID
    '4' for AD5940 Chip ID
    '5' for NAND Flash Device ID
    '6' for AD7156 Chip ID
    -----------------------------------------------
Usage:
    #>getChipID [chip_name]
    #>getChipID 1
        """
        err_stat = 0
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No chip name specified, check help and retry")
            return 1, None
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_chip_id_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CHIP_ID_REQ
        msg.payload.chip_name = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_chip_id_cmd_t(), 10)
        chip_id = None
        if reply_msg != None:
            status = reply_msg.payload.status
            if status != M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK:
              print "Getting chip ID failed!"
              err_stat = 1
            else:
              if msg.payload.chip_name == 1:
                chip_name = "ADXL362"
              elif msg.payload.chip_name == 2:
                chip_name = "ADPD4K"
              elif msg.payload.chip_name == 3:
                chip_name = "ADP5360"
              elif msg.payload.chip_name == 4:
                chip_name = "AD5940"
              elif msg.payload.chip_name == 5:
                chip_name = "Nand Flash"
              elif msg.payload.chip_name == 6:
                chip_name = "AD7156"
              print "Fetched chip ID:{} for {}".format(hex(reply_msg.payload.chip_id), chip_name)
              chip_id = reply_msg.payload.chip_id
        else:
            err_stat = 1
            print "Getting chip ID failed! Please check the arguments"
        return err_stat, chip_id

    def do_getBatteryInfo(self, arg):
        """
Get Battery information.
Fetches the device's battery status a given number of times.
-----------------------------------------------
Usage:
    #>getBatteryInfo [count]
    
    #>getBatteryInfo 5
        """
        args = self._parse_args(arg, 1)
        try:
            count = int(args[0])
        except:
            count = 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_GET_BAT_INFO_REQ
        for i in range(count):
            time.sleep(1)
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_bat_info_resp_t(), 10)
            if reply_msg == None:
                self.vrb.err("Error! Timed out waiting for the device!")
                break
            self.vrb.write("Watch Battery information:")
            chrg_status = self._get_enum_name(M2M2_PM_SYS_BAT_STATE_ENUM_t, reply_msg.payload.bat_chrg_stat)
            if chrg_status != None:
                self.vrb.write("Battery Status: '{}'".format(chrg_status))
            self.vrb.write("Battery Level: '{}'%".format(int(reply_msg.payload.bat_lvl)))
            self.vrb.write("Battery Voltage: '{}'mV".format(int(reply_msg.payload.bat_mv)))
            #self.vrb.write("Battery Temp: '{}' C".format(int(reply_msg.payload.bat_temp)))
            self.vrb.write("Date and Time    : {}".format(datetime.datetime.now().replace(microsecond=0)))
            self.vrb.write("---------------------------------------------------------------------")

    def help_getBatteryInfo(self):
        print
        print "Fetches the device's battery status a given number of times."
        print "-----------------------------------------------"
        print "Usage:"
        print "    #>getBatteryInfo [count]"
        print
        print "    #>getBatteryInfo 5"
        print

    def do_setBatteryThreshold(self, arg):
        """
Set Battery Low level and Critical levels in percentage(%).
-----------------------------------------------
Usage:
    #>setBatteryThreshold [low_level critical_level]
    #>setBatteryThreshold 15 10
        """
        args = self._parse_args(arg, 2)
        if args == None:
            return
        try:
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_bat_thr_req_t())
            msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SET_BAT_THR_REQ
            msg.payload.bat_level_low = int(args[0])
            msg.payload.bat_level_critical = int(args[1])
        except ValueError:
            self.vrb.err("Error! Invalid argument!")
            return
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 10)
        if reply_msg == None:
            self.vrb.err("Error! Timed out waiting for the device!")
            return
        status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("status: {}".format(status))

	'''def do_setBatteryCharging(self, arg):
        """
Enable or Disable battery charging. The argument is the option:
    '1' for Enable
    '0' for Disable
-----------------------------------------------
Usage:
    #>setBatteryCharging [option]

    #>setBatteryCharging 1
    #>setBatteryCharging 0
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t())
        opt = args[0]
        if "1" in opt :
            msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_ENABLE_BAT_CHARGE_REQ
        elif "0" in opt:
            msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_DISABLE_BAT_CHARGE_REQ
        else:
            self.vrb.err("Invalid arguments")
            self.help_BatteryCharging()
            return
        self._send_packet(msg)
        resp = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 10)
        if resp == None:
            self.vrb.err("Timed out waiting for a response from the device!")
            return
        self._print_packet_status(resp)'''

    def do_setPowerMode(self, arg):
        """
Set the device into a desired power state. The argument is the state:
    '0' for active mode
    '2' for hibernate mode              - same as Power off from Watch display page.
    '3' for shutdown mode/shipment mode - same as the Shipment Mode from Watch display page.
                                          To see it in action, Watch needs to be removed from a USB connection.
                                          Only then it will enter shipment mode. To bring the Watch up, you need to plug in the USB cable.
    -----------------------------------------------
Usage:
    #>SetPowerMode [state]
    #>setPowerMode 2
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No state specified, putting the device in shutdown mode", 2)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_SET_PWR_STATE_REQ
        if args == None:
            msg.payload.state = 0
        else:
            msg.payload.state = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_pwr_state_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_PM_SYS_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Put device into the state:", 2)
            self.vrb.write("  Device: '{}'".format(int(reply_msg.payload.state)), 2)
            self.vrb.write("  Status: '{}'".format(status), 2)
        else:
            self.vrb.err("Putting device into the desired state failed!")

    def do_setDisplayColour(self, arg):
        """
Set LCD Display Colour to all white, R, G, B and black, to check whether there is dead pixel. The argument is the disp_colour:
    '0' for White
    '1' for Black
    '2' for Red
    '3' for Green
    '4' for Blue
    -----------------------------------------------
Usage:
    #>setDisplayColour [disp_colour]
    #>setDisplayColour 0
    #>setDisplayColour 4
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No disp_colour specified, check help and retry")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_display_set_command_t())
        msg.payload.command = M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_SET_DISPLAY_REQ
        msg.payload.colour = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_display_set_command_t(), 10)
        if reply_msg != None:
            status = reply_msg.payload.status
            if status != 0:
              print "Putting Display into the desired colour failed!"
            else:
              print "Putting Display into the desired colour..  Status: '{}'".format(status)
        else:
            print "Putting Display into the desired colour failed! Please check the arguments"

    def do_backlightCntrl(self, arg):
        """
Do backlight control: enable/disable. The argument is the bl_cntrl:
    '0' for BL OFF
    '1' for BL ON
    -----------------------------------------------
Usage:
    #>backlightCntrl [bl_cntrl]
    #>backlightCntrl 0
    #>backlightCntrl 1
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No bl_cntrl specified, check help and retry")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_backlight_cntrl_command_t())
        msg.payload.command = M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_BACKLIGHT_CNTRL_REQ
        msg.payload.control = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_backlight_cntrl_command_t(), 10)
        if reply_msg != None:
            status = reply_msg.payload.status
            if status != 0:
              print "Backlight control for the desired state failed!"
            else:
              print "Backlight control for the desired state..  Status: '{}'".format(status)
        else:
            print "Backlight control for the desired state failed! Please check the arguments"

    def do_keyPressTest(self, arg):
        """
Do key press test: The argument is the 'duration' of test in secs.
After the test key presses of application will be unusable. 
system_Reset needs to be given after the test
    -----------------------------------------------
Usage:
    #>keyPressTest [duration]
    #>keyPressTest 5
    #>keyPressTest 10
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No duration specified, check help and retry")
            return
        #Start the Test
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_key_test_command_t())
        msg.payload.command = M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_KEY_TEST_REQ
        msg.payload.enable = 1
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_key_test_command_t(), 10)
        if reply_msg != None:
            if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK and reply_msg.payload.command == M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_KEY_TEST_RESP:
              print "Key press test enabled..  Status: '{}'".format(reply_msg.payload.status)
            else:
              print "Key Press test start failed!..Status: '{}'".format(reply_msg.payload.status)
        else:
            print "Key press start test failed! No response received"
            return

        #Get key stream Data
        duration = int(args[0])
        start_time = time.time()
        while True:
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_pm_sys_key_test_data_t(), 1)
            if reply_msg != None:
              if reply_msg.payload.command == M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_KEY_STREAM_DATA:
                print "Received key code: {}".format(reply_msg.payload.key_value)
            end_time = time.time()
            if end_time-start_time > duration:
                break
            # end of loop

        #End the Test
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_key_test_command_t())
        msg.payload.command = M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_KEY_TEST_REQ
        msg.payload.enable = 0
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_DISPLAY, m2m2_key_test_command_t(), 10)
        if reply_msg != None:
            if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK and reply_msg.payload.command == M2M2_DISPLAY_APP_CMD_ENUM_t.M2M2_DISPLAY_APP_CMD_KEY_TEST_RESP:
              print "Key press test disabled..  Status: '{}'".format(reply_msg.payload.status)
            else:
              print "Key Press end failed!..Status: '{}'".format(reply_msg.payload.status)
        else:
            print "Key press end-test failed! No response received"

    def do_capSenseTest(self, arg):
        """
Do Cap sense AD7156 test: The argument is the 'duration' of test in secs.
    -----------------------------------------------
Usage:
    #>capSenseTest [duration]
    #>capSenseTest 5
    #>capSenseTest 10
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("No duration specified, check help and retry")
            return

        #start the test
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cap_sense_test_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_REQ
        msg.payload.enable = 1
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cap_sense_test_cmd_t(), 20)
        if reply_msg != None:
            if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK and reply_msg.payload.command == M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_RESP:
              print "Cap Sense test Enabled..  Status: '{}'".format(reply_msg.payload.status)
            else:
              print "Cap sense test start failed!.. Status: '{}'".format(reply_msg.payload.status)
        else:
            print "Cap Sense start test failed! No response received"
            return

        #Get touch stream data
        duration = int(args[0])
        start_time = time.time()
        while True:
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cap_sense_test_data_t(), 1)
            if reply_msg != None:
              if reply_msg.payload.command == M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CAP_SENSE_STREAM_DATA:
                print "Received touch position: {}".format(reply_msg.payload.touch_position)
                print "Received touch value: {}".format(reply_msg.payload.touch_value)
            end_time = time.time()
            if end_time-start_time > duration:
                break
            # end of loop

        #End the test
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cap_sense_test_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_REQ
        msg.payload.enable = 0
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cap_sense_test_cmd_t(), 20)
        if reply_msg != None:
            if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK and reply_msg.payload.command == M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_CAP_SENSE_TEST_RESP:
              print "Cap Sense test Disabled..  Status: '{}'".format(reply_msg.payload.status)
            else:
              print "Cap sense test end failed!.. Status: '{}'".format(reply_msg.payload.status)
        else:
            print "Cap Sense end test failed! No response received"
            return

    def do_enterBootLoader(self, arg):
        """
Give the command to enter bootloader
    -----------------------------------------------
Usage:
    #>enterBootLoader
        """
        args = self._parse_args(arg, 0)

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_enter_bloader_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_ENTER_BOOTLOADER_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_enter_bloader_cmd_t(), 10)
        if reply_msg != None:
            status = reply_msg.payload.status
            if status != 0:
              print "Bootloader entry failed"
            else:
              print "Bootloader entry passed! Status: '{}'".format(status)
        else:
            print "Bootloader entry failed!"

    def do_usb_DFU(self, arg):
        """
Give the command to start USB DFU on com port of Watch in bootloader mode.
Pre-requisite command to be executed is: #>enterBootLoader
The following command is executed:
    nrfutil dfu usb-serial -pkg ADI_project.zip -p COM7
based on argument passed for Debug or Release mode package
The argument proj_config is:
    Release: for copying Watch app zip package from Release folder
    Debug: for copying Waych app zip package from Debug folder
    -----------------------------------------------
Usage:
    #>usb_DFU [comport_no} [proj_config]
        """
        args = self._parse_args(arg, 2)
        if args == None:
            self.vrb.write("No arguments specified, check help and retry")
            return
        print "Timestamp of hex package is \n"
        cmd = "dir /T:W  /A:-D ..\\..\\..\\app\\nRF52840_app\\ses\\Output\\"+args[1]
        cmd = cmd + "\\Exe\\watchv4_nrf52840.hex"
        os.system(cmd)
        print "Copying the Watch hex file from {} app directory..".format(args[1])
        cmd = "copy ..\\..\\..\\app\\nRF52840_app\\ses\\Output\\"+args[1]
        cmd = cmd + "\\Exe\\watchv4_nrf52840.hex .\\ADI_project.hex"
        print "Executing command {}".format(cmd)
        os.system(cmd)

        print "\nPreparing the zip package for USB DFU.."
        cmd = "nrfutil pkg generate --hw-version 52 --sd-req 0xAE --application-version 0xff --application ADI_project.hex --key-file ..\\..\\..\\bootloader\\boot_zip\\private.pem ADI_project.zip"
        os.system(cmd)

        print "\nStarting USB DFU.."
        cmd = "nrfutil dfu usb-serial -pkg ADI_project.zip -p "+args[0]
        os.system(cmd)

    def do_fs_get_bad_blocks(self, arg):
        """
Command to get no: of bad blocks from NAND Flash.
#>fs_getBadBlock
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_bad_blocks_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_BAD_BLOCKS_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_bad_blocks_cmd_t(), 10)
        if reply_msg != None:
            if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK: 
              self.vrb.write("No: of bad blocks: {}".format(reply_msg.payload.bad_blocks))
            else:
              self.vrb.write("Error returned in getting Bad blocks from NAND FLash")
        else:
            self.vrb.err("No response from device.")

    def do_delete_config_file(self, arg):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_DELETE_CONFIG_FILE_REQ

        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_Enable_Config_logs(self, args):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_pm_sys_cmd_t())

        if "start" in args:
            msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_ENABLE_USER_CONFIG_LOG_REQ
        elif "stop" in args:
            msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_DISABLE_USER_CONFIG_LOG_REQ

        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_low_touch_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_get_low_touch_status(self, args):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_pm_sys_cmd_t())

        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PS_SYS_COMMAND_GET_LOW_TOUCH_LOGGING_STATUS_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_low_touch_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_pm_activate_touch_sensor(self, args):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_pm_sys_cmd_t())

        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_ACTIVATE_TOUCH_SENSOR_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_low_touch_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_pm_deactivate_touch_sensor(self, args):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_pm_sys_cmd_t())

        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_DEACTIVATE_TOUCH_SENSOR_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_pm_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_low_touch_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_create_gen_blk_dcb(self, arg):
        global lowtouch
        args = self._parse_args(arg, 1)
        if args == None:
            return

        if "start" in args:
            lowtouch.Enable_lowtouch = True
        elif "stop" in args:
            lowtouch.Enable_lowtouch = False
            lowtouch = LowTouch()

    def do_gen_blk_dcb_file_create(self, arg):
        """
Create the file with m2m2 commands that will be written into the gen block DCB.
This command is to be preceded by 'create_gen_blk_dcb start' and succeeded by 'create_gen_blk_dcb stop'.
#>gen_blk_dcb_file_create
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        configmsg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_user_cfg_summary_pkt_t())
        if "write" in args:
            if lowtouch.Enable_lowtouch == True and lowtouch.User_File!='':
                print("\n  ############### GEN_BLK_DCB_CONFIG FILE INFO #############\n")
                self.vrb.write("  Header Info length  : '{}'".format(len(configmsg.pack())))
                self.vrb.write("  Start command length: '{}'".format(lowtouch.Startcmdlen))
                self.vrb.write("  Start command count : '{}'".format( lowtouch.Startcmdcount))
                self.vrb.write("  Stop command length : '{}'".format(lowtouch.Stopcmdlen))
                self.vrb.write("  Stop command count  : '{}'".format(lowtouch.Stopcmdcount))

                lowtouch.Enable_lowtouch = False
                configmsg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP
                configmsg.payload.start_cmd_len = lowtouch.Startcmdlen
                configmsg.payload.start_cmd_cnt = lowtouch.Startcmdcount
                configmsg.payload.stop_cmd_len = lowtouch.Stopcmdlen
                configmsg.payload.stop_cmd_cnt = lowtouch.Stopcmdcount
                configmsg.payload.crc16 = 0 #T0DO
                conmsg = configmsg.pack()
                lowtouch.User_File = conmsg + lowtouch.User_File
                #check if total bytes in User_File is WORD aligned. If not make it. So that DCB contains data which is WORD aligned
                if (len(lowtouch.User_File)) % 4 != 0:
                    for i in range((4 - (len(lowtouch.User_File) % 4) )):
                        lowtouch.User_File += "\0" #putting it as NULL
                #check if total no: of bytes in DCB file will be less than MAXGENBLKDCBSIZE*4*4, if its exceeding don't create the file 
                if ( len(lowtouch.User_File) > (MAXGENBLKDCBSIZE*4*4) ):
                    print("  ##########      GEN_BLK_DCB_CONFIG.LOG can't be created      ##############");
                    print("  ##########   File size exceeding MAXGENBLKDCBSIZE*4*4 bytes  ##############");
                    print("  ##########     Recheck the start-stop commands included      ##############");
                    print("  ########## in gen_blk_dcb_file_create_test and Retry ##############");
                    return
                try:
                    Fileobjw =  open(lowtouch.DCB_File_name, "wb")
                    Fileobjw.write(lowtouch.User_File)
                    Fileobjw.close()
                    print("Copying and replacing file to dcb_cfg folder as gen_blk_dcb.lcfg")
                    print("File size: ")
                    print(len(lowtouch.User_File))
                    cmd = "copy GEN_BLK_DCB_CONFIG.LOG .\\dcb_cfg\gen_blk_dcb.lcfg"
                    os.system(cmd)
                except:
                    print("ERROR: 'GEN_BLK_DCB_CONFIG.LOG' File write failed")
                    return
            else:
                print("Create gen blk DCB config file with 'create_gen_blk_dcb start' command")
        elif "read" in args:
            try:
                Fileobj = open(lowtouch.DCB_File_name, "rb")
            except:
                print("ERROR: 'GEN_BLK_DCB_CONFIG.LOG' File not found")
                return
            chunk_len = 912 #57*4*4
            bytes_read = 0
            if Fileobj != None:
                while True:
                    config_buffer = Fileobj.read(chunk_len)
                    if not config_buffer:
                        break
                    temp_array = list(bytearray(config_buffer))
                    for index in range(len(config_buffer)):
                        print "{}".format(hex(temp_array[index]))
                        if (index+1)%4 == 0:
                          print('\n')
                    bytes_read = bytes_read  + len(config_buffer)
            else:
                print("Create gen blk DCB config file with 'create_gen_blk_dcb start' command")

    def do_loadAd7156Cfg(self, arg):
        """
Load the AD7156 device configuration.
    #>loadAd7156Cfg
        """
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156, m2m2_sensor_ad7156_resp_t())
        msg.payload.command = M2M2_SENSOR_AD7156_COMMAND_ENUM_t.M2M2_SENSOR_AD7156_COMMAND_LOAD_CFG_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_AD7156, m2m2_sensor_ad7156_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("Loaded ad7156 device configuration")
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Loading Ad7156 device configuration failed!")

    def do_fs_config_log(self, arg):
        global lowtouch
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        args = self._parse_args(arg, 1)
        if args == None:
            return

        msg = m2m2_packet(address, m2m2_file_sys_cmd_t())
        if "start" in args:
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_DCFG_START_LOG_REQ
        elif "stop" in args:
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_DCFG_STOP_LOG_REQ
        else:
            self.vrb.err("Incorrect usage!")
            return

        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 120)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, reply_msg.payload.status)
            if status == 'M2M2_FILE_SYS_STATUS_OK' and "start" in args:
                lowtouch.Enable_lowtouch = True
            elif status == 'M2M2_FILE_SYS_STATUS_OK' and "stop" in args:
                lowtouch.Enable_lowtouch = False
                lowtouch = LowTouch()
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_fs_config_log_file(self, arg):
        """
mount file system. Command help to check if file system is available with proper mount.
#>fs_config_log_file write
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_user_config_data())
        configmsg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_user_cfg_summary_pkt_t())
        if "write" in args:
            if lowtouch.Enable_lowtouch == True and lowtouch.User_File!='':
                print("\n  ############### CONFIG FILE INFO #############\n")
                self.vrb.write("  Header Info length  : '{}'".format(len(configmsg.pack())))
                self.vrb.write("  Start command length: '{}'".format(lowtouch.Startcmdlen))
                self.vrb.write("  Start command count : '{}'".format( lowtouch.Startcmdcount))
                self.vrb.write("  Stop command length : '{}'".format(lowtouch.Stopcmdlen))
                self.vrb.write("  Stop command count  : '{}'".format(lowtouch.Stopcmdcount))

                lowtouch.Enable_lowtouch = False
                configmsg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_RESP
                configmsg.payload.start_cmd_len = lowtouch.Startcmdlen
                configmsg.payload.start_cmd_cnt = lowtouch.Startcmdcount
                configmsg.payload.stop_cmd_len = lowtouch.Stopcmdlen
                configmsg.payload.stop_cmd_cnt = lowtouch.Stopcmdcount
                configmsg.payload.crc16 = 0 #T0DO
                conmsg = configmsg.pack()
                lowtouch.User_File = conmsg + lowtouch.User_File
                try:
                    Fileobjw =  open(lowtouch.User_File_name, "wb")
                    Fileobjw.write(lowtouch.User_File)
                    Fileobjw.close()
                except:
                    print("ERROR: 'USER_CONFIG.LOG' File write failed")
                    return
                try:
                    Fileobj = open(lowtouch.User_File_name, "rb")
                except:
                    print("ERROR: 'USER_CONFIG.LOG' File not found")
                    return
                chunk_len = 70
                bytes_read = 0
                if Fileobj != None:
                    while True:
                        config_buffer = Fileobj.read(chunk_len)
                        if not config_buffer:
                            break
                        else:
                            if ((len(config_buffer)) < chunk_len):
                                msg.payload.status = M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_END_OF_FILE
                            else:
                                msg.payload.status = M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK
                        temp_array = list(bytearray(config_buffer))
                        for index in range(len(config_buffer)):
                            msg.payload.byte_configstream[index] = temp_array[index]
                        msg.payload.len_configstream = len(config_buffer)
                        bytes_read = bytes_read  + len(config_buffer)
                        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LOG_USER_CONFIG_DATA_REQ
                        self._send_packet(msg)
                        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 500)
                        if reply_msg!=None:
                            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                            if status != None:
                                if(status != 'M2M2_APP_COMMON_STATUS_OK'):
                                    print("ERROR: Error in writing the config data!")
                                    self.vrb.write("  Status: '{}'".format(status))
                                    return
                                elif((status == 'M2M2_APP_COMMON_STATUS_OK') and (len(config_buffer)) < chunk_len ):
                                    self.vrb.write("\n  Total bytes written: '{}'".format(bytes_read))
                                    self.vrb.write("  Status: '{}'\n".format(status))
                            else:
                                status = self._get_enum_name(M2M2_FILE_SYS_STATUS_ENUM_t, reply_msg.payload.status)
                                self.vrb.write("  Status: '{}'".format(status))
                                break
                        else:
                            self.vrb.write("  Device did not respond")
            else:
                print("Create config file with 'fs_config_log start' command")

    def do_fs_format(self, arg):
        """
format file system. Command to format file system.
#>fs_format
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_FORMAT_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 30)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device.Mount operation failed.")


    def do_fs_req_debug_info(self, arg):
        """
format file system. Command to format file system.
#>fs_req_debug_info
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_impt_debug_info_req_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_IMPT_DEBUG_INFO_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_impt_debug_info_resp_t(), 30)
        if reply_msg != None:
            self.vrb.write("  CIRCULAR BUFFER HEAD POINTER: {} page no".format(int(reply_msg.payload.head_pointer)))
            self.vrb.write("  CIRCULAR BUFFER TAIL POINTER : {} block no".format(int(reply_msg.payload.tail_pointer)))
            self.vrb.write("  USB AVG TRANSMIT TIME : {}us".format(int(reply_msg.payload.usb_avg_tx_time)))
            self.vrb.write("  USB AVG PORT WRITE TIME : {}us".format(int(reply_msg.payload.usb_avg_port_write_time)))
            self.vrb.write("  PAGE READ TIME : {}us".format(int(reply_msg.payload.page_read_time)))
            self.vrb.write("  PAGE WRITE TIME : {}us".format(int(reply_msg.payload.page_write_time)))
            self.vrb.write("  INIT CIRCULAR BUFFER : {}".format(int(reply_msg.payload.init_circular_buffer_flag)))
            self.vrb.write("  MEM FULL FLAG : {}".format(int(reply_msg.payload.mem_full_flag)))
            self.vrb.write("  DATA OFFSET : {}".format(int(reply_msg.payload.data_offset)))
            self.vrb.write("  CONFIG POS OCCUPIED : {}".format(int(reply_msg.payload.config_file_occupied)))
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("debug info req error failed !")

    def do_fs_ls(self, arg):
        """
list files.Command help to view files in the current directory.
#>fs_ls
        """
        fs_ls_list = []
        err_stat = 0
        args = self._parse_args(arg, 0)
        if args == None:
            return err_stat, fs_ls_list
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_req_t(2))
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LS_REQ
        msg.payload.dir_path[0] = ord('\x01')
        msg.payload.dir_path[1] = ord('\x01')
        self._send_packet(msg)

        while True:
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_resp_t(), 20)
            if reply_msg != None:
                if (reply_msg.payload.status != M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                    self._print_file_system_status(reply_msg)
                    break
                else :
                    fs_ls_dict = self._print_file_list(reply_msg)
                    fs_ls_list.append(fs_ls_dict)
            else:
                err_stat = 1
                self.vrb.err("No response from device")
            # end of loop
        return err_stat, fs_ls_list

    def do_fs_vol_info(self, arg):
        """
File system volume info. Command to get file system volume info.
#>fs_vol_info
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_VOL_INFO_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_vol_info_resp_t(), 10)
        if reply_msg != None:
            self.vrb.write("  FS_TOTAL_MEMORY: '{}'bytes".format(int(reply_msg.payload.totalmemory)))
            self.vrb.write("  FS_USED_MEMORY : '{}'bytes".format(int(reply_msg.payload.usedmemory)))
            self.vrb.write("  FS_AVAILABLE_MEMORY : '{}%'".format(int(reply_msg.payload.availmemory)))
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device.Getting volume info failed.")
    def do_fs_mount(self, arg):
        """
mount file system. Command help to check if file system is available with proper mount.
#>fs_mount
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_MOUNT_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 10)
        if reply_msg != None:
            self._print_packet_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_fs_log(self, arg):
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        args = self._parse_args(arg, 1)
        if args == None:
            return
        if "start" in args:
            msg = m2m2_packet(address, m2m2_app_common_sub_op_t())
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_START_LOGGING_REQ
        elif "stop" in args:
            msg = m2m2_packet(address, m2m2_file_sys_stop_log_cmd_t())
            msg.payload.stop_type = FILE_STOP_LOGGING_t.M2M2_FILE_SYS_STOP_LOGGING
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_STOP_LOGGING_REQ
        else:
            self.vrb.err("Incorrect usage!")
            return

        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        if "start" in args:
            reply_msg = self._get_packet(address, m2m2_app_common_sub_op_t(), 20)
        elif "stop" in args:
            reply_msg = self._get_packet(address, m2m2_file_sys_stop_log_cmd_t(), 20)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")

    def do_fs_sub(self, arg):
        fs_address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        address = None
        args = self._parse_args(arg, 2)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        msg = m2m2_packet(fs_address, m2m2_file_sys_log_stream_t())
        if "r" in args or "remove" in args:
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_STOP_STREAM_REQ
            msg.payload.stream = address
        elif "a" in args or "add" in args:
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LOG_STREAM_REQ
        else:
            self.vrb.err("Incorrect usage! You did not provide a valid subscription operation.")
            return
        msg.payload.stream = stream
        self._send_packet(msg)
        reply_msg = self._get_packet(fs_address, m2m2_file_sys_log_stream_t(), 10)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device. Subscription/unSubscription operation failed.")

    def do_fs_log_test(self, arg):
        """
    list files.Command help to view files in the current directory.
    #>fs_log_test
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_TEST_LOG_REQ
       # msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 120)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response received from device!!")

    def do_fs_stream(self, arg):
        """
read contents of file. Command is used to read file by getting data from file streamed as byte array.
    #>fs_stream
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        fobj = open(args[0], "wb")
        #Counter for CRC16 mismatch
        nCRCMisMatchCnt = 0
        #Counter for Sequence number mismatch
        nSeqMisMatchCnt = 0
        #Reference sequence number for comparing received sequence number
        nSequenceNumber = 0
        nComputedCRC = int(0x0FFFF)
        nCRCPolynomial = int(0x1021)
        # Read the size of the file
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_req_t(2))
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LS_REQ
        msg.payload.dir_path[0] = ord('\x01')
        msg.payload.dir_path[1] = ord('\x01')
        self._send_packet(msg)
        while True:
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_resp_t(), 100)
            if reply_msg != None:
                if reply_msg.payload.status != M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK:
                    break
                elif reply_msg.payload.filetype == FILE_TYPE_ENUM_t.M2M2_FILE_SYS_IS_DATA_FILE:
                    if (cast(reply_msg.payload.full_file_name, c_char_p).value == args[0]):
                        total_file_size = reply_msg.payload.filesize
                        #print("file size = {}".format(total_file_size))
                elif reply_msg.payload.filetype == FILE_TYPE_ENUM_t.M2M2_FILE_SYS_IS_CONFIG_FILE:
                    if cast(reply_msg.payload.full_file_name, c_char_p).value == args[0]:
                        total_file_size = reply_msg.payload.filesize        
            else:
                self.vrb.err("No response from device")
                return

        bar = tqdm.tqdm(total=total_file_size, dynamic_ncols=True, ascii=True, unit="B", unit_scale=True)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_req_t(len(args[0])))
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_DOWNLOAD_LOG_REQ

        temp_array = list(bytearray(args[0]))
        for index in range(len(args[0])):
            msg.payload.file_name[index] = temp_array[index]

        start_time = datetime.datetime.now()
        self._send_packet(msg)
        file_size = 0
        while True:
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS_STREAM, m2m2_file_sys_download_log_stream_t(), 1000)
            if reply_msg != None:
                file_size += reply_msg.payload.len_stream
                data_length = reply_msg.payload.len_stream + 12
                #print"Length received t1 = {}".format(data_length)
                crc16_data_array =  buffer(reply_msg.header)[:] + buffer(reply_msg.payload)[:]
                for index in range(0,8,2):
                    crc16_data_array = swap(crc16_data_array,index, index + 1)
                nComputedCRC = int(0xFFFF)
                for nByte in range(data_length):
                	nComputedCRC = ((nComputedCRC >> 8) | (nComputedCRC << 8))&0xFFFF
              		nComputedCRC = nComputedCRC ^ ord(crc16_data_array[nByte])
               		nComputedCRC ^=(nComputedCRC & 0xFF) >> 4
                	nComputedCRC ^= ((nComputedCRC << 8) << 4)&0xFFFF
                	nComputedCRC ^= ((nComputedCRC & 0xFF) << 4) << 1
                #'''print"nComputedCRC = {}".format(nComputedCRC)
               	#print"Length received = {}".format(data_length)
               	#print"nComputedCRC = {}".format(nComputedCRC)
               	#print"received crc16 = {}".format(reply_msg.payload.crc16)'''
            	if((nComputedCRC != reply_msg.payload.crc16)):
                    nCRCMisMatchCnt += 1
                    #print"received crc16 = {}".format(format_hex(crc16_data_array))
                    #print"received crc16 = {}".format((crc16_data_array))
            	    #print"Length received t1 = {}".format(data_length)
            	    #print"CRC mismatch cnt = {}".format(nCRCMisMatchCnt)
                if(nSequenceNumber != reply_msg.header.checksum):
                    nSeqMisMatchCnt += 1
                # Restraining refernce sequence number to 16bit wide
            	if(nSequenceNumber == 65535):
                   nSequenceNumber = 0
	        else:
                   nSequenceNumber += 1
                bar.update(reply_msg.payload.len_stream)
                if (reply_msg.payload.len_stream != len(reply_msg.payload.byte_stream)):
                    fobj.write(bytearray(reply_msg.payload.byte_stream[0:int(reply_msg.payload.len_stream)]))
                else :
                    fobj.write(reply_msg.payload.byte_stream)
            else:
                bar.close()
                self.vrb.err("No response from device.Stream file operation failed.")
                break
            if (reply_msg.payload.status != M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                fobj.close()
                bar.close()
                break
            # end of loop
            end_time = datetime.datetime.now()
            '''self.vrb.write("\n No. of CRC mismatches = {}".format(nCRCMisMatchCnt))
            self.vrb.write("\n No. of Seq mismatches = {}".format(nSeqMisMatchCnt))
            self.vrb.write("\nFile read complete!\n  Start Time: {}\n  End Time: {}\n  Elapsed Time: {}\n  Total bytes read: {}".format(start_time, end_time, end_time - start_time, file_size))'''

    def do_get_file_cnt(self, arg):
        """
    list files.Command help to view files in the current directory.
    #>file_cnt
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_NUMBER_OF_FILE_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_file_count_pkt_t(), 120)
        if reply_msg != None:
            self._print_file_count_status(reply_msg)
        else:
            self.vrb.err("The device did not respond!")


    def do_get_file_info(self, arg):
        """
    This is a test command, used to read file information from TOC.
    It is used for debugging any issues with file information storage page
    #>gets the information about the file present in the given page of TOC
    Usage: get_file_info arg1
    arg1 -> File index whose file Information has to be displayed
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_get_file_info_req_pkt_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        msg.payload.file_index = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_file_info_resp_pkt_t(), 120)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                self._print_file_info_status(reply_msg)
            else:
                self.vrb.err("Error occured while reading the file with given index")
        else:
            self.vrb.err("The device did not respond!")

    page_read_fail_cnt = 0
    def do_page_read_test(self, arg):
        """
    This is a test command, used for testing by reading the given page
    It is used for debugging any issues while reading any page from the NAND flash
    #>checks if there are any errors during reading of a given page
    Usage: page_read_test arg1 arg2 arg3
    arg1 -> page number to be read
    arg2 -> 1 ( enables bytes read from page)/0 (disables bytes read from page)
    arg3 -> Number of bytes to read from page, its between 0-4096
        """
        global page_read_fail_cnt
        args = self._parse_args(arg, 3)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_page_test_req_pkt_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_PAGE_READ_TEST_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        msg.payload.page_num = int(args[0])
        msg.payload.num_bytes = int(args[2])
        if(args[1] != None):
            print_en = int(args[1])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_page_test_resp_pkt_t(), 120)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                self._print_page_read_test_status(reply_msg,print_en)
            else:
                page_read_fail_cnt += 1
                self.vrb.err("Error occured while reading the page with given number")
        else:
            self.vrb.err("The device did not respond!")

    def do_file_read_test(self, arg):
        """
    This is a test command, to display contents of particular file information
    Usage: file_read_test arg1
    arg1 -> Name of file whose contents has to be displayed
        """
        global page_read_fail_cnt
        args = self._parse_args(arg, 1)
        if args == None:
            return
            
        file_found = 0
        file_pos = 0
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_req_t(2))
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_LS_REQ
        msg.payload.dir_path[0] = ord('\x01')
        msg.payload.dir_path[1] = ord('\x01')
        self._send_packet(msg)

        while True:
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_ls_resp_t(), 120)
            if reply_msg != None:
                if (reply_msg.payload.status != M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                    #self._print_file_system_status(reply_msg)
                    if(file_found == 0):
                        self.vrb.write(" File Not Found ")                    
                        return
                    else:
                        break
                else :
                    if(file_found == 0):
                        file_pos += 1
                    if(cast(reply_msg.payload.full_file_name, c_char_p).value == args[0]):
                        file_found = 1                        
            else:
                self.vrb.err("No response from device")
            # end of loop

        
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        msg = m2m2_packet(address, m2m2_file_sys_get_file_info_req_pkt_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_FILE_INFO_REQ
        msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
        msg.payload.file_index = file_pos
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_file_info_resp_pkt_t(), 120)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK):
                self._print_file_info_status(reply_msg)
                start_page = reply_msg.payload.start_page
                end_page = reply_msg.payload.end_page
                page_read_fail_cnt = 0
                #print("\n Testing  by reading the each pages of File \n")
                for page_index in range(start_page,end_page+1,1):
                    print("\n reading page: {}".format(page_index))
                    arg_str = str(page_index) + " 0"
                    self.do_page_read_test(arg_str) 
                print("completed reading all the pages of given file index")
                print("Total No. of pages read = {}".format((end_page-start_page+1)))
                print("No. of pages reported read failures = {}".format(page_read_fail_cnt))
            else:
                self.vrb.err("Error occured while reading the file with given index")
        else:
            self.vrb.err("The device did not respond!")

            
    def do_pattern_write(self, arg):
        """
    multiple pattern write with prescribed file size, scale
    #>pattern_write file_size scale_type scale_factor base num_of_files_to_write
    eg 1: pattern_write 16384 0 2 1 2 ( linear scale)
    eg 2: pattern_write 16384 1 2 2 2 ( log scale)
    eg 3: pattern_write 16384 2 2 2 2( exp scale )
        """
        args = self._parse_args(arg, 5)
        if args == None:
            self.vrb.write("please provide valid argument")
            return
        else:    
            address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
            msg = m2m2_packet(address, m2m2_file_sys_pattern_write_req_pkt_t())
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_PATTERN_WRITE_REQ
            msg.payload.status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
            msg.payload.file_size = int(args[0])
            msg.payload.scale_type = int(args[1])
            msg.payload.scale_factor = int(args[2])
            msg.payload.base = int(args[3])
            msg.payload.num_files_to_write = int(args[4])
            status = M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK
            while msg.payload.num_files_to_write > 0:
                self._send_packet(msg)
                reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_pattern_write_resp_pkt_t(), 120)
                if reply_msg != None:
                    if reply_msg.payload.status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK:
                        if msg.payload.scale_type == 0:#linear
                            msg.payload.file_size *= msg.payload.scale_factor
                        elif msg.payload.scale_type == 1:#log
                            if msg.payload.scale_factor != 1:
                                msg.payload.file_size *= int(math.log(msg.payload.base,msg.payload.scale_factor))
                        elif msg.payload.scale_type == 2:#exp
                            msg.payload.file_size *= int(math.exp(msg.payload.scale_factor))    
                        print "File size computed = {}".format(msg.payload.file_size)
                    else:
                        if reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_ERR_MEMORY_FULL:
                            self.vrb.err("Memory full breaking loop as new files cannot be written!")
                            break  
                        elif reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_ERR_MAX_FILE_COUNT:
                            self.vrb.err("Max file count crossed!")
                            break   
                    status |= reply_msg.payload.status 
                else:
                    self.vrb.err("The device did not respond!")
                    time.sleep(5)
                msg.payload.num_files_to_write -= 1
            if status == M2M2_APP_COMMON_STATUS_ENUM_t.M2M2_APP_COMMON_STATUS_OK:
                self._print_packet_status(reply_msg)
    def do_fs_sub_status(self, arg):
        fs_address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS
        address = None
        args = self._parse_args(arg, 1)
        if args == None:
            return
        for a in args:
            if a in stream_name_map:
                address = stream_name_map[a]["application"]
                stream = stream_name_map[a]["stream"]
        if address == None:
            self.vrb.err("Incorrect usage! You did not provide a valid stream.")
            return
        msg = m2m2_packet(fs_address, m2m2_app_common_sub_op_t())
        msg.payload.stream = stream
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_FS_STREAM_SUB_STATUS_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(fs_address, m2m2_file_sys_get_subs_status_resp_t(), 20)
        if reply_msg != None:
            stream_f = self._get_enum_name(M2M2_ADDR_ENUM_t, reply_msg.payload.stream)
            fs_sub_state = self._get_enum_name(FILE_SYS_STREAM_SUBS_STATE_ENUM_t, reply_msg.payload.subs_state)
            if stream == None:
                stream = hex(reply_msg.payload.stream_f)
            self.vrb.write("Application: {}".format(stream_f))
            self.vrb.write("FS_SUBS_STATUS: '{}'".format(fs_sub_state))
        else:
            self.vrb.err("No response from device. FS stream status getting failed.")

    def do_fs_status(self, arg):
        """
    File system status info. Command to get file system current status.
    #>fs_status
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_GET_FS_STATUS_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 10)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device.Getting file system status info failed.")

    def do_get_max_tx_pkt_comb_cnt(self, arg):
        """
    This is a command, to be used from iOS app to get the "max_tx_pkt_comb_cnt" used
    in the Watch Fw. It is based on this value that no: of pkts for BLE Tx is combined and send out.
    #>get_max_tx_pkt_comb_cnt
        """
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_BLE_GET_MAX_TX_PKT_COMB_CNT_REQ
        msg.payload.status = M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t(), 20)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK):
                print("max_tx_pkt_comb_cnt used in Watch Fw : {} ").format(reply_msg.payload.max_tx_pkt_comb_cnt)
            else:
                self.vrb.err("Error occured while reading max_tx_pkt_comb_cnt")
        else:
            self.vrb.err("The device did not respond!")

    def do_set_max_tx_pkt_comb_cnt(self, arg):
        """
    This is a command, to be used from iOS app to set the "max_tx_pkt_comb_cnt" used
    in the Watch Fw. It is based on this value that no: of pkts for BLE Tx is combined and send out.
    #>set_max_tx_pkt_comb_cnt 1
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_BLE_SET_MAX_TX_PKT_COMB_CNT_REQ
        msg.payload.status = M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK
        msg.payload.max_tx_pkt_comb_cnt = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_ble_max_tx_pkt_comb_cnt_resp_cmd_t(), 20)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK):
                print("max_tx_pkt_comb_cnt changed in Watch Fw to : {} ").format(reply_msg.payload.max_tx_pkt_comb_cnt)
            else:
                self.vrb.err("Error occured while writing max_tx_pkt_comb_cnt")
        else:
            self.vrb.err("The device did not respond!")

    def do_get_hib_mode_status(self, arg):
        """
    This is a command, to be used from iOS app/other tools to get the hibernate mode status- enabled/disabled
    in the Watch Fw. It is based on this value that Hibernate Mode happens.
    #>get_hib_mode_status
        """
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_hibernate_mode_status_resp_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_GET_HIBERNATE_MODE_STATUS_REQ
        msg.payload.status = M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_hibernate_mode_status_resp_cmd_t(), 20)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK):
                print("hib_mode_status used in Watch Fw : {} ").format(reply_msg.payload.hib_mode_status)
            else:
                self.vrb.err("Error occured while reading hib_mode_status")
        else:
            self.vrb.err("The device did not respond!")

    def do_set_hib_mode_status(self, arg):
        """
    This is a command, to be used from iOS app/other tools to control the hibernate mode status- enabled/disabled
    in the Watch Fw. It is based on this value that Hibernate Mode happens.
	#>set_hib_mode_status hib_control
	hib_control:
	  1 -> to enable Hib Mode
	  0 -> to disable Hib Mode
	Eg:
    #>set_hib_mode_status 1
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        address = M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM
        msg = m2m2_packet(address, m2m2_hibernate_mode_status_resp_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_SET_HIBERNATE_MODE_STATUS_REQ
        msg.payload.status = M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK
        msg.payload.hib_mode_status = int(args[0])
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_hibernate_mode_status_resp_cmd_t(), 20)
        if reply_msg != None:
            if (reply_msg.payload.status == M2M2_PM_SYS_STATUS_ENUM_t.M2M2_PM_SYS_STATUS_OK):
                print("hib_mode_status changed in Watch Fw to : {} ").format(reply_msg.payload.hib_mode_status)
            else:
                self.vrb.err("Error occured while writing hib_mode_status")
        else:
            self.vrb.err("The device did not respond!")

    def do_test_252(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        #self.onecmd("quickstart start_log_adxl_252")
        for i in range(cnt):
            print i
            self.onecmd("quickstart start_reg_read_adxl")
            self.onecmd("quickstart start_log_adxl_252")
            # self.onecmd("quickstart adxl")
            # time.sleep(2)
            # self.onecmd("quickstop adxl")
        #self.onecmd("quickstop eda")
        print "test #252 done"

    def do_test_341(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        #self.onecmd("quickstart start_log_adxl_252")
        for i in range(cnt):
            print i
            self.onecmd("quickstart start_log_mv_uc1")
            self.onecmd("quickstop stop_log_mv_uc1")
            # self.onecmd("quickstart adxl")
            # time.sleep(2)
            # self.onecmd("quickstop adxl")
        #self.onecmd("quickstop eda")
        print "test #341 done"
    
    def do_test_330(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            #self.onecmd("quickstart tst_issue_330_1")
            #self.onecmd("quickstart tst_issue_330_2")
            #self.onecmd("quickstart start_log_bcm")
            #self.onecmd("quickstop stop_log_bcm")
            self.onecmd("quickstart start_mv_uc1")
            self.onecmd("quickstop stop_mv_uc1")
        print "test #330 done"

    def do_test_217(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        self.onecmd("quickstart eda")
        for i in range(cnt):
            print i
            self.onecmd("quickstart start_stop_adpd4k")
        self.onecmd("quickstop eda")
        print "test #217 done"
        
    def do_test_230(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart start_stop_230")
            self.onecmd("status adpd4000")
            self.onecmd("status adxl")
            self.onecmd("status eda")
            self.onecmd("status temperature")
        print "test #230 done"

    def do_test_245(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart mv_uc1_streaming_start")
            self.onecmd("status adpd4000")
            self.onecmd("status adxl")
            self.onecmd("status temperature")
            self.onecmd("delay 1")
            self.onecmd("quickstop mv_uc1_streaming_stop")
        print "test #245 done"

    def do_test_245_1(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart mv_uc1_245_issue")
            self.onecmd("delay 1")
        print "test #245_1 done"

    def do_test_245_2(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart mv_uc1_245_issue_wo_dcb")
        print "test #245_2 done"

    def do_test_245_3(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart mv_uc1_245_issue_200Hz")
            self.onecmd("delay 1")
        print "test #245_3 done"

    def do_test_245_4(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart mv_uc1_245_issue_300Hz")
            self.onecmd("delay 1")
        print "test #245_4 done"

    def do_test_dcb(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("write_dcb_config adpd4000 UseCase1.dcfg")
            #self.onecmd("delay 1")
            self.onecmd("read_dcb_config adpd4000")
            #self.onecmd("delay 1")
            self.onecmd("compare_cfg_files UseCase1.dcfg adpd4000_dcb_get.dcfg")
            self.onecmd("delete_dcb_config adpd4000")
            #self.onecmd("delay 1")
        print "test DCB Write/Read/Erase done for count {}".format(cnt)

    def do_test_dcb_gen_blk(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("write_dcb_config low_touch gen_blk_dcb.lcfg")
            #self.onecmd("delay 1")
            self.onecmd("read_dcb_config low_touch")
            #self.onecmd("delay 1")
            self.onecmd("delete_dcb_config low_touch")
            #self.onecmd("delay 1")
        print "test gen blk DCB Write/Read/Erase done for count {}".format(cnt)

    def do_test_dcb_ad7156(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("write_dcb_config ad7156 ad7156_dcb.dcfg")
            #self.onecmd("delay 1")
            self.onecmd("read_dcb_config ad7156")
            #self.onecmd("delay 1")
            self.onecmd("compare_cfg_files ad7156_dcb.dcfg ad7156_dcb_get.dcfg")
            self.onecmd("delete_dcb_config ad7156")
            #self.onecmd("delay 1")
        print "test AD7156 DCB Write/Read/Erase done for count {}".format(cnt)

    def do_test_335(self,arg):
        args = self._parse_args(arg,1)
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("quickstart ppg")
            self.onecmd("delay 1")
            self.onecmd("lcfgPpgCheck 40 ppg_dcb.lcfg")
            self.onecmd("quickstop ppg")
        print "test ppg start, lcfgPpgCheck, ppg stop done for count {}".format(cnt)

    def do_fs_list(self, arg):
        """
    read contents of file. Command is used to read file by getting data from file streamed as byte array.
    #>fs_list
        """
        args = self._parse_args(arg,1)
        if args == None:
            return
        if "start" in args:
            i=0
            for i in range(1,10):
                self.onecmd("quickstart start_log_adxl")
                time.sleep(15)
                self.onecmd("quickstop stop_log_adxl")
        else:
            self.vrb.err("running automation multiple files failed.")
        print "test automate done!"

    def do_test_469(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("lcfgEcgWrite 0:50")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")

            self.onecmd("lcfgEcgWrite 0:100")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")

            self.onecmd("lcfgEcgWrite 0:200")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")

            self.onecmd("lcfgEcgWrite 0:300")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")

            self.onecmd("lcfgEcgWrite 0:400")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")

            self.onecmd("lcfgEcgWrite 0:500")
            self.onecmd("sensor ecg start")
            self.onecmd("sub recg add")
            self.onecmd("delay 10")
            self.onecmd("quickstop ecg")
        print "test #469 done"

    def do_test_502_UC4(self,arg):
        args = self._parse_args(arg,1)
        # for item in args:
            # print item
        cnt = int(args[0])
        print cnt
        if cnt == None:
            return
        for i in range(cnt):
            print i
            self.onecmd("fs_format")
            for j in range(62):
                print i,j
                self.onecmd("quickstart start_stream_mv_uc4_1")
                self.onecmd("delay 1")
                self.onecmd("quickstart start_log_mv_uc4_1")
                self.onecmd("delay 1")
                self.onecmd("quickstop stop_log_mv_uc4_1")
                self.onecmd("delay 1")
                self.onecmd("quickstop stop_stream_mv_uc4_1")
        print "test #502 done"

    def do_get_apps_health_status(self, arg):    
        """
        read health status of all applications
        #>get_apps_health_status
        """
        args = self._parse_args(arg,0)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_get_apps_running_stat_req_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_GET_APPS_HEALTH_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_get_apps_running_stat_resp_cmd_t(), 240)
        if reply_msg != None:
            self.vrb.write("  AD5940 ISR Count: {}".format(int(reply_msg.payload.ad5940_isr_cnt)))
            self.vrb.write("  ADPD4000 ISR Count : {}".format(int(reply_msg.payload.adpd4000_isr_cnt)))
            self.vrb.write("  ADXL ISR Count : {}".format(int(reply_msg.payload.adxl_isr_cnt)))
            self._print_packet_status(reply_msg)
        else:
            self.vrb.err("No response from device.Health status failed.")    

    def do_get_eda_debug_Info(self, arg):    
        """
        read debug info of ad5940
        #>get_eda_debug_Info
        """
        args = self._parse_args(arg,0)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, m2m2_get_eda_debug_info_req_cmd_t())
        msg.payload.command = M2M2_EDA_APP_CMD_ENUM_t.M2M2_EDA_APP_CMD_REQ_DEBUG_INFO_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, m2m2_get_eda_debug_info_resp_cmd_t(), 240)
        if reply_msg != None:
            self.vrb.write("  FIFO Overflow status : {}".format(int(reply_msg.payload.ad5940_fifo_overflow_status)))
            self.vrb.write("  FIFO Level : {}".format(int(reply_msg.payload.ad5940_fifo_level)))
            self.vrb.write("  Time gap between Interrupts : {}us".format(int(reply_msg.payload.Interrupts_time_gap)))
            self.vrb.write("  Time gap between packets : {}us".format(int(reply_msg.payload.packets_time_gap)))
            self.vrb.write("  Time taken for RTIA Calibration : {}s".format(int(reply_msg.payload.rtia_calibration_time)))
            self.vrb.write("  Time taken for first measurement to start : {}s".format(int(reply_msg.payload.delay_in_first_measurements)))
            self.vrb.write("  Time taken for first Voltage measurement: {}us".format(int(reply_msg.payload.first_voltage_measure_time)))
            self.vrb.write("  Time taken for first Current measurement : {}us".format(int(reply_msg.payload.first_current_measure_time)))
            self.vrb.write("  Time gap between successive Voltage Measurements : {}us".format(int(reply_msg.payload.voltage_measure_time_gap)))
            self.vrb.write("  Time gap between successive Current Measurements : {}us".format(int(reply_msg.payload.current_measure_time_gap)))
            self.vrb.write("  Time taken EDA Initilization : {}s".format(int(reply_msg.payload.EDA_Init_Time)))
            self.vrb.write("  Time taken EDA De initialization : {}us".format(int(reply_msg.payload.EDA_DeInit_Time)))
            self._print_packet_status(reply_msg)
        else:
            self.vrb.err("No response from device.Health status failed.")

    def do_getPpgLcfg(self, arg):
        """
Get the PPG LCFG, used from Watch. The argument is the LCFG ID, which is 40 for adpd4000

      Eg: = getPpgLcfg 40 
        """
        args = self._parse_args(arg,1)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return

        if (args[0] != '40'):
            self.vrb.err("Invalid LCFG ID !")
            return

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, m2m2_ppg_lcfg_data_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_GET_LCFG_REQ

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, m2m2_ppg_lcfg_data_t(), 100)
        if reply_msg == None:
            print "Reading PPG LCFG failed!"
            return

        Cnt = 0
        lcfg_array_count = int(reply_msg.payload.size)

        while Cnt < lcfg_array_count:
            self.vrb.write("Reading LCFG: 0x{:08X} {}".format(int(reply_msg.payload.lcfgdata[Cnt]), Cnt))
            Cnt+=1

        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        self.vrb.write("Size  : {:02}".format(lcfg_array_count))
        self.vrb.write("Command return status: {}".format(status))

    def do_fs_KeyValuePair(self, arg):
            """
    Inject KeyValuePair into the log.

    Usage:
    #>fs_KeyValuePair 4562

    Note: The entered Value ID (maximum-16 characters) will retain untill you start and close the file.
          For every new file,you need to inject value ID.

            """
            args = self._parse_args(arg, 1)
            if args == None:
                self.vrb.err("Atleast one argument needed")
                return

            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_set_key_value_pair_req_t())
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_SET_KEY_VALUE_PAIR_REQ
            temp_array = list(bytearray(args[0]))
            if len(args[0]) > 16:
                self.vrb.err("Maximum 16 characters allowed for value ID")
                return
            for index in range(len(args[0])):
                msg.payload.valueID[index] = temp_array[index]
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 10)
            if reply_msg != None:
                self._print_file_system_status(reply_msg)
            else:
                self.vrb.err("No response from device.Injecting keyvaluePair failed")
    def do_fs_stream_chunk(self, arg):
            """
    Get the particular chunk of data from file by specifying the chunk number.
    -----------------------------------------------
    Usage:
        #>fs_stream_chunk rollover chunknumber filename
        #>fs_stream_chunk 0 20 05113CAC.LOG

            """
            args = self._parse_args(arg, 3)
            if args == None:
                return
            Roll_over = int(args[0])
            chunk_number  = int(args[1])
            filename,ext =  args[2].split('.')
            temp_array = list(bytearray(args[2]))
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_pkt_retransmit_req_t(len(args[2])))
            msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_CHUNK_RETRANSMIT_REQ
            msg.payload.Roll_over = Roll_over
            msg.payload.chunk_number = chunk_number
            for index in range(len(args[2])):
                msg.payload.file_name[index] = temp_array[index]
            self._send_packet(msg)
            nComputedCRC = int(0x0FFFF)
            nCRCPolynomial = int(0x1021)
            nCRCMisMatchCnt = 0
            # loop body here
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_get_resp_t(), 1000)
            if reply_msg != None:
                print"Status:{}".format(reply_msg.payload.status)
                if ((reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_STATUS_OK) or (
                    reply_msg.payload.status == M2M2_FILE_SYS_STATUS_ENUM_t.M2M2_FILE_SYS_END_OF_FILE)):
                    fobj = open(filename + '_chunk.LOG', "wb")
                    data_length = reply_msg.payload.len_stream + 12
                    # print"Length received t1 = {}".format(data_length)
                    crc16_data_array = buffer(reply_msg.header)[:] + buffer(reply_msg.payload)[:]
                    for index in range(0, 8, 2):
                        crc16_data_array = swap(crc16_data_array, index, index + 1)
                    nComputedCRC = int(0xFFFF)
                    for nByte in range(data_length):
                        nComputedCRC = ((nComputedCRC >> 8) | (nComputedCRC << 8))&0xFFFF
                        nComputedCRC = nComputedCRC ^ ord(crc16_data_array[nByte])
                        nComputedCRC ^=(nComputedCRC & 0xFF) >> 4
                        nComputedCRC ^= ((nComputedCRC << 8) << 4)&0xFFFF
                        nComputedCRC ^= ((nComputedCRC & 0xFF) << 4) << 1
                       # '''print"nComputedCRC = {}".format(nComputedCRC)
               	       # print"Length received = {}".format(data_length)'''
               	       # print"nComputedCRC = {}".format(nComputedCRC)
               	       # print"received crc16 = {}".format(reply_msg.payload.crc16)
                    if ((nComputedCRC != reply_msg.payload.crc16)):
                        nCRCMisMatchCnt += 1
                        print"CRC mismatch {}".format(nCRCMisMatchCnt)
                        # print"received crc16 = {}".format(format_hex(crc16_data_array))
                        # print"received crc16 = {}".format((crc16_data_array))
                        # print"Length received t1 = {}".format(data_length)
                        print"CRC mismatch cnt = {}".format(nCRCMisMatchCnt)
                        # Restraining refernce sequence number to 16bit wide
                    if (reply_msg.payload.len_stream != len(reply_msg.payload.byte_stream)):
                            fobj.write(bytearray(reply_msg.payload.byte_stream[0:int(reply_msg.payload.len_stream)]))
                    else:
                            fobj.write(reply_msg.payload.byte_stream)
                    print "chunk stream obtained successfully"
                    fobj.close()
                else:
                    self._print_file_system_status(reply_msg)
            else:
                self.vrb.err("No response from device.Stream file operation failed.")

    def do_fs_refhr(self, arg):
        """
Write referenceHr.Command is used to write reference hr and Current PC time.
    #>do_fs_refhr
        """
        args = self._parse_args(arg, 1)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_app_ref_hr_stream_t())
        msg.header.src = M2M2_ADDR_ENUM_t.M2M2_ADDR_APP_CLI_STREAM
        msg.payload.command = M2M2_SENSOR_COMMON_CMD_ENUM_t.M2M2_SENSOR_COMMON_CMD_STREAM_DATA

        now = datetime.datetime.now()
        is_dst = time.daylight and time.localtime().tm_isdst > 0
        utc_offset = - (time.altzone if is_dst else time.timezone)
        msg.payload.refhr = int(args[0])
        msg.payload.year   = now.year
        msg.payload.month  = now.month
        msg.payload.day    = now.day
        msg.payload.hour   = now.hour
        msg.payload.minute = now.minute
        msg.payload.second = now.second
        msg.payload.TZ_sec = utc_offset
        self._send_packet(msg)
        self.vrb.write("date and time: {}".format(now), 2)
        self.vrb.write("timezone: {}".format(utc_offset), 2)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_app_ref_hr_stream_t(), 1)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("response timeout from device.Stream file written successful.")
    
    def do_create_adpd4k_dcfg(self, arg):
        """
        ->creates the dcfg to be loaded into adpd4k
        ->Argument is the slotid and appid

        Ex:create_adpd4k_dcfg 1:0
        creates dcfg for ppg app in slot-A
        
        /***************APP IDs ***************/   /**************************SLOT IDs **********************/
        ----------------------------------------   ------------------------------  ------------------------------
        | App-ID |          Apps               |   | Slot- ID  |  ADPD4k_SLOTS  |  | Slot- ID   |  ADPD4k_SLOTS  |
        ----------------------------------------   ------------------------------   ------------------------
        |   0    |          ECG4K              |   |    1      |       A        |   |    9       |     I    |
         ---------------------------------------   ------------------------------   ------------------------
        |   1    |          PPG                |   |    2      |       B        |   |    10      |     J    |
        ----------------------------------------   ------------------------------   ------------------------
        |   2    |    Temp. (Thermistor)       |   |    3      |       C        |   |    11      |     K    |
        ------------------------- --------------   ------------------------------   ------------------------
        |   3    |Temp. (Calibration Resistor) |   |    4      |       D        |   |    12      |     L    |
        ----------------------------------------   ------------------------------  --------------------------   
        |   4    |          ADPD4K_G           |   |    5      |       E        |   
        ----------------------------------------   ------------------------------
        |   5    |          ADPD4K_R           |   |    6      |       F        |
        ----------------------------------------   ------------------------------
        |   6    |          ADPD4K_IR          |   |    7      |       G        |
        ----------------------------------------   ------------------------------
        |   7    |          ADPD4K_B           |   |    8      |       H        |
        ----------------------------------------  -------------------------------
        
        -> Slot Switching feature not enabled in FW, So Mapping of application and slot is fixed as mentioned in below table. 
        if Created and loaded DCFG for any other combination, apps and static AGC will not work as expected
         -------------------------------------------------------------------
        | App-ID |          Apps               | Slot- ID  |  ADPD4k_SLOT  |  
        -------------------------------------------------------------------- 
        |   0    |          ECG4K              |   1      |       A        | 
         -------------------------------------------------------------------
        |   1    |          PPG                |   6      |       F        | 
        --------------------------------------------------------------------
        |   2    |    Temp. (Thermistor)       |   4      |       D        |
        ------------------------- ------------------------------------------
        |   3    |Temp. (Calibration Resistor) |   5      |       E        | 
        --------------------------------------------------------------------
        |   4    |          ADPD4K_G           |   6      |       F        |   
        --------------------------------------------------------------------
        |   5    |          ADPD4K_R           |   7      |       G        |
        --------------------------------------------------------------------
        |   6    |          ADPD4K_IR          |   8      |       H        |
        --------------------------------------------------------------------
        |   7    |          ADPD4K_B           |   9      |       I        |
        --------------------------------------------------------------------
        """
        args = self._parse_args(arg, None)
        if args == None:
            self.vrb.err("No argument passed, Check Help!")
            return
        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_dcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_CREATE_DCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            slotid = int(tempVal.split(':')[0])
            appid = int(tempVal.split(':')[1])
            if(appid < 0 or appid > 7 or slotid < 1 or slotid >12):
                self.vrb.err("Invalid argument passed, Check Help!")
                return
            msg.payload.ops[i].slotid = slotid
            msg.payload.ops[i].appid = appid

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd_dcfg_op_hdr_t(num_ops), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("DCFG Creation failed!")

    def do_lcfgPpgCheck(self, arg):
        """
Compares the PPG LCFG which is loaded into adpd4000 with dcb_lcfg and f/w_lcfg. There are two arguments to it-

1. LCFG ID, which is 40 for adpd4000
2. PPG DCB LCFG filename  - for ex. ppg_dcb.lcfg

      Eg: = lcfgPpgCheck 40 ppg_dcb.lcfg
        """
        args = self._parse_args(arg,2)
        if args == None:
            self.vrb.err("Incorrect usage! Please check help.")
            return
 
        if (args[0] != '40'):
            self.vrb.err("Invalid LCFG ID !")
            return
        
        dcb_lcfg_filename = 'dcb_cfg/' + args[1]

        fw_lcfg = list()
        dcb_lcfg = list()
        get_lcfg = list()

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, m2m2_ppg_lcfg_data_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_GET_LCFG_REQ

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_PPG, m2m2_ppg_lcfg_data_t(), 100)
        if reply_msg == None:
            print "Reading PPG LCFG failed!"
            return
        Cnt = 0
        
        lcfg_array_count = int(reply_msg.payload.size)
        while Cnt < lcfg_array_count:
            get_lcfg.append(int((reply_msg.payload.lcfgdata[Cnt])))
            Cnt+=1

        status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if status == None:
            status = format(reply_msg.payload.status, '#04x')
        #self.vrb.write("LCFG Size  : {:02}".format(lcfg_array_count))
        self.vrb.write("Command return status: {}".format(status))

        try:
            f = open('dcb_cfg/ppg_fw.lcfg')   # ppg_fw.lcfg contain Default f/w PPG lcfg
        except:
            self.vrb.err("Invalid File Name")
            return
        for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t' or line[0]=='/'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split('/')
                    str = str[0].split(' ')
                    str =str[1].replace(' ','').replace('\t','').replace('\n','')
                    fw_lcfg.append(int(str,16))
        f.close()
        #self.vrb.write("FW LCFG Size  : {:02}".format(len(fw_lcfg)))
        '''self.vrb.write("Fw dcfg")
        for i in range(len(fw_lcfg)):
            self.vrb.write("fw_lcfg {}:{}".format(i,fw_lcfg[i]))'''

        try:
            f = open(dcb_lcfg_filename)   # dcb lcfg file that contains dcb ppg lcfg
        except:
            self.vrb.err("Invalid File Name")
            return
        for line in f.readlines():
                if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t' or line[0]=='/'):
                    continue
                else:
                    str = line.split('#')
                    str = str[0].split('/')
                    str = str[0].split(' ')
                    dcb =str[1].replace(' ','').replace('\t','').replace('\n','')
                    dcb_lcfg.append(int(dcb,16))
        f.close()
        #self.vrb.write("DCB LCFG Size  : {:02}".format(len(dcb_lcfg)))
        '''self.vrb.write("DCB dcfg")
        for i in range(len(dcb_lcfg)):
            self.vrb.write("dcb_lcfg {}:{}".format(i,dcb_lcfg[i]))'''
        if(len(get_lcfg)==len(fw_lcfg)):
            for i in range(len(get_lcfg)):
                if(get_lcfg[i]!=fw_lcfg[i]):
                    self.vrb.write("FW Mismatch found at index{:02} Got: {:04} Actual: {:04}".format(i,get_lcfg[i],fw_lcfg[i]))
                    break
                else:
                    if(i==len(get_lcfg)-1):
                        self.vrb.write("Command return status: {}".format('FW LCFG Present'))
                        return
                    else:
                        continue    
        if(len(get_lcfg)==len(dcb_lcfg)):
            for i in range(len(get_lcfg)):
                if(get_lcfg[i]!=dcb_lcfg[i]):
                    self.vrb.write("DCB Mismatch found at index{:02} Got: {:04} Actual: {:04}".format(i,get_lcfg[i],dcb_lcfg[i]))
                    break
                else:
                    if(i==len(get_lcfg)-1):
                        self.vrb.write("Command return status: {}".format('DCB LCFG Present'))
                        return
                    else:
                        continue
        self.vrb.write("Command return status: {}".format('Invalid LCFG Present'))

    def do_compare_cfg_files(self, arg):
        """
    Compares two dcfg/lcfg files to check if they contain same configurations or not.
    Two arguments-> filename1, filename2
    for ex-
        #>compare_dcb_files adxl_dcb.dcfg adxl_dcb_get.dcfg
        """
        args = self._parse_args(arg, 2)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return
        file1 = 'dcb_cfg/' + args[0]
        file2 = 'dcb_cfg/' + args[1]
        file1_cfg = []
        file2_cfg = []
        try:
            f1 = open(file1)
        except:
            self.vrb.err("Invalid File Name")
            return
        for line in f1.readlines():
            if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                continue
            else:
                str = line.split('#')
                str = str[0].replace(' ','').replace('\t','').replace('\n','')
                file1_cfg.append(int(str,16))
        f1.close()
        try:
            f2 = open(file2)
        except:
            self.vrb.err("Invalid File Name")
            return
        for line in f2.readlines():
            if(line[0] == '#' or line[0]=='\n' or line[0]==' ' or line[0]=='\t'):
                continue
            else:
                str = line.split('#')
                str = str[0].replace(' ','').replace('\t','').replace('\n','')
                file2_cfg.append(int(str,16))
        f2.close()
        if(len(file1_cfg) != len(file2_cfg)):
            self.vrb.write("Command return status: {}".format('Cfg. in both files not matched'))
            return
        else:
            for i in range(len(file1_cfg)):
                if(file1_cfg[i] != file2_cfg[i]):
                    self.vrb.write("Command return status: {}".format('Cfg. in both files not matched'))
                    return
        self.vrb.write("Command return status: {}".format('Cfg. in both files matched'))

    def do_lcfgEcgRead(self, arg):
        """
Read the ECG LCFG. The argument is the LCFG ID to choose from the ecg configuration structure:
    -----------------------------
    |Config Element    |  Index |
    -----------------------------
    |     FS           |    0   |
    |  ADC_PGA_GAIN    |    1   |
    -----------------------------

      Eg: = lcfgEcgRead addr1 addr2 ......
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_lcfg_op_hdr_t(num_ops), 10)
        reg_result_list = []
        if reply_msg == None:
            err_stat = 1
            self.vrb.err("Reading ECG LCFG failed!")
        else:
            self._print_ecg_lcfg_result(reply_msg)
            err_stat = 0
            for i in range(reply_msg.payload.num_ops):
                reg_result_list.append((reg_addr, hex(reply_msg.payload.ops[i].value)))
        return err_stat, reg_result_list

    def do_set_ecg_dcb_lcfg(self,arg):
        """
Writes the ECG LCFG values from DCB if present,otherwise it will write default value. There is no argument.

      Eg: = set_ecg_dcb_lcfg
        """
        args = self._parse_args(arg, None)
        if len(args) != 0:
            self.vrb.err("Invalid No. arguments supplied!")
            return
        
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_dcb_lcfg_t())
        msg.payload.command =M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_SET_LCFG_REQ
        self._send_packet(msg)
        time.sleep(3)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_dcb_lcfg_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Setting ECG Library configuration failed!")   

    def do_set_adpd4k_fs(self,arg):
        """
Set the sampling frequency value in adpd4k dcfg. The argument is ODR value in Hz.

      Eg: = set_adpd4k_fs 100 
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.err("Invalid No. arguments supplied!")
            return
    
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd4000_set_fs_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_SET_FS_REQ
        msg.payload.odr = int(args[0])
        self._send_packet(msg)
        #time.sleep(3)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_sensor_adpd4000_set_fs_t(), 10)
        if reply_msg != None:
            self._print_packet_status(reply_msg)
        else:
            self.vrb.err("Setting ADPD4K sampling frequency in DCFG failed!")

    def do_disable_adpd4k_slots(self, arg):
        """
    Disables all the slots except Slot-A which is enabled by default. There is no argument.

      Eg: = disable_adpd4k_slots
        """
        args = self._parse_args(arg, None)
        if len(args) != 0:
            self.vrb.err("Invalid No. arguments supplied!")
            return
        
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd4k_slot_info_t())
        msg.payload.command = M2M2_SENSOR_ADPD_COMMAND_ENUM_t.M2M2_SENSOR_ADPD_COMMAND_DISABLE_SLOTS_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, m2m2_adpd4k_slot_info_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Disabling adpd4k slots failed!")

    def do_set_eda_dcb_lcfg(self,arg):
        """
Writes the EDA LCFG values from DCB if present,otherwise it will write default value. There is no argument.

      Eg: = set_eda_dcb_lcfg
        """
        args = self._parse_args(arg, None)
        if len(args) != 0:
            self.vrb.err("Invalid No. arguments supplied!")
            return
        
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_dcb_lcfg_t())
        msg.payload.command =M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_SET_LCFG_REQ
        self._send_packet(msg)
        time.sleep(3)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_dcb_lcfg_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Setting EDA Library configuration failed!") 

    def do_read_dcb_info(self,arg):
        """
Reads Important DCB Information

      Eg: = read_dcb_info
        """
        args = self._parse_args(arg, None)
        if len(args) != 0:
            self.vrb.err("Invalid No. arguments supplied!")
            return
        
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, m2m2_dcb_fds_status_info_req_t())
        msg.payload.command = M2M2_BCM_APP_CMD_ENUM_t.M2M2_APP_COMMON_CMD_DCB_TIMING_INFO_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, m2m2_dcb_fds_timing_info_resp_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("  Check entries time: {} us".format(int(reply_msg.payload.adi_dcb_check_entries_time)))
            self.vrb.write("  Clear entries time : {} us".format(int(reply_msg.payload.adi_dcb_clear_entries_time)))
            self.vrb.write("  Delete record time : {} us".format(int(reply_msg.payload.adi_dcb_delete_record_time)))
            self.vrb.write("  Read Entry time : {} us".format(int(reply_msg.payload.adi_dcb_read_entry_time)))
            self.vrb.write("  Update Entry time : {} us".format(int(reply_msg.payload.adi_dcb_update_entry_time)))
        else:
            self.vrb.err("Setting DCB time info failed!") 
                    
    def do_lcfgEdaRead(self, arg):
        """
Read the EDA LCFG. The argument is the LCFG ID to choose from the eda configuration structure:
    -----------------------------
    |Config Element    |  Index |
    -----------------------------
    |     FS           |    0   |
    |  ADC_PGA_GAIN    |    1   |
    -----------------------------

      Eg: = lcfgEdaRead addr1 addr2 ......
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_lcfg_op_hdr_t(num_ops), 10)
        reg_result_list = []
        if reply_msg == None:
            err_stat = 1
            self.vrb.err("Reading EDA LCFG failed!")
        else:
            self._print_eda_lcfg_result(reply_msg)
            err_stat = 0
            for i in range(reply_msg.payload.num_ops):
                reg_result_list.append((reg_addr, hex(int(reply_msg.payload.ops[i].value))))
        return err_stat, reg_result_list

    def do_lcfgBcmRead(self, arg):
        """
        Read the BCM LCFG. The argument is the LCFG ID to choose from the bcm configuration structure:
    -----------------------------
    |Config Element    |  Index |
    -----------------------------
    |     FS           |    0   |
    |  ADC_PGA_GAIN    |    1   |
    -----------------------------

      Eg: = lcfgBcmRead addr1 addr2 ......
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_lcfg_op_hdr_t(num_ops), 10)
        reg_result_list = []
        if reply_msg == None:
            err_stat = 1
            self.vrb.err("Reading BCM LCFG failed!")
        else:
            self._print_bcm_lcfg_result(reply_msg)
            err_stat = 0
            for i in range(reply_msg.payload.num_ops):
                reg_result_list.append((reg_addr, hex(int(reply_msg.payload.ops[i].value))))
        return err_stat, reg_result_list

    def do_set_bcm_dcb_lcfg(self, arg):
        """
        Writes the BCM LCFG values from DCB if present,otherwise it will write default value. There is no argument.
        Eg: = set_bcm_dcb_lcfg
        """
        args = self._parse_args(arg, None)
        if len(args) != 0:
            self.vrb.err("Invalid No. arguments supplied!")
            return

        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_dcb_lcfg_t())
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_SET_LCFG_REQ
        self._send_packet(msg)
        time.sleep(3)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_dcb_lcfg_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
        if reply_msg != None:
            self.vrb.write("  Status: '{}'".format(status))
        else:
            self.vrb.err("Setting BCM Library configuration failed!")


    def do_lcfgEcgWrite(self, arg):
        """
Set the ECG LCFG. The argument is the ECG LCFG ID:VALUE pair to modify the ecg lcfg value

       FS             address -->0
        Values
        100
        200
        300
        400
        500

       ADC_PGA_GAIN   address -->1
        Values
        0     /**< ADC PGA Gain of 1 */
        1     /**< ADC PGA Gain of 1.5 */
        2     /**< ADC PGA Gain of 2 */
        3     /**< ADC PGA Gain of 4 */
        4     /**< ADC PGA Gain of 9 */

      Eg: = lcfgEcgWrite addr1:value1 addr2:value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if (':' not in tempVal):
                self.vrb.err("Invalid Argument Format, check help!")
                return
            elif ("0x") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            else:
                reg_addr = int(tempVal.split(':')[0])
                reg_val = int(tempVal.split(':')[1])
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG, ecg_app_lcfg_op_hdr_t(num_ops), 10)
        if reply_msg == None:
            print "Writing ECG LCFG failed!"
            return
        self._print_ecg_lcfg_result(reply_msg)

    def do_SetEcg4kLcfg(self, arg):
        """
Set the ECG LCFG values for ADPD4000. The argument is the FIELD:VALUE pair to modify the ecg lcfg value
For ODR, field value = 0

Currently ecg lcfg for adpd4k has only one entry i.e. ODR

       Sampling Freq.             address -->0
        Values
        100
        200
        300
        400
        500

      Eg: = SetEcg4kLcfg addr1:value1 addr2:value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, ecg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if (':' not in tempVal):
                self.vrb.err("Invalid Argument Format, check help!")
                return
            elif ("0x") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            else:
                reg_addr = int(tempVal.split(':')[0])
                reg_val = int(tempVal.split(':')[1])
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, ecg_app_lcfg_op_hdr_t(num_ops), 10)
        if reply_msg == None:
            print "Setting ECG LCFG values for ADPD4K failed!"
            return
        self._print_ecg_lcfg_result(reply_msg)

    def do_GetEcg4kLcfg(self, arg):
        """
Get the ECG LCFG values for ADPD4000. The argument is the FIELD value for the entry to be read from the ecg lcfg
For ODR, field value = 0

Currently ecg lcfg for adpd4k has only one entry i.e. ODR

      Eg: = GetEcg4kLcfg addr1 addr2
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, ecg_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_READ_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal, 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal, 16)
            else:
                reg_addr = int(tempVal)
            msg.payload.ops[i].field = reg_addr

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SENSOR_ADPD4000, ecg_app_lcfg_op_hdr_t(num_ops), 10)
        if reply_msg == None:
            print "Reading ECG LCFG values for ADPD4K failed!"
            return
        self._print_ecg_lcfg_result(reply_msg)

    def do_lcfgEdaWrite(self, arg):
        """
Set the EDA LCFG. The argument is the ECG LCFG ID:VALUE pair to modify the eda lcfg value

       FS             address -->0
        Values
        100
        200
        300
        400
        500

       ADC_PGA_GAIN   address -->1
        Values
        0     /**< ADC PGA Gain of 1 */
        1     /**< ADC PGA Gain of 1.5 */
        2     /**< ADC PGA Gain of 2 */
        3     /**< ADC PGA Gain of 4 */
        4     /**< ADC PGA Gain of 9 */

      Eg: = lcfgEdaWrite addr1:value1 addr2:value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            else:
                reg_addr = int(tempVal.split(':')[0])
                reg_val = int(tempVal.split(':')[1])
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_lcfg_op_hdr_t(num_ops), 10)
        if reply_msg == None:
            print "Writing EDA LCFG failed!"
            return
        self._print_eda_lcfg_result(reply_msg)

    def do_lcfgBcmWrite(self, arg):
        """
    Set the BCM LCFG. The argument is the ECG LCFG ID:VALUE pair to modify the bcm lcfg value

       FS             address -->0
        Values
        100
        200
        300
        400
        500

       ADC_PGA_GAIN   address -->1
        Values
        0     /**< ADC PGA Gain of 1 */
        1     /**< ADC PGA Gain of 1.5 */
        2     /**< ADC PGA Gain of 2 */
        3     /**< ADC PGA Gain of 4 */
        4     /**< ADC PGA Gain of 9 */

      Eg: = lcfgBcmWrite addr1:value1 addr2:value2 ...
        """

        args = self._parse_args(arg, None)
        if len(args) == 0:
            self.vrb.err("No arguments supplied!")
            return

        num_ops = len(args)
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_lcfg_op_hdr_t(num_ops))
        msg.payload.command = M2M2_APP_COMMON_CMD_ENUM_t.M2M2_APP_COMMON_CMD_WRITE_LCFG_REQ
        msg.payload.num_ops = num_ops
        for i in range(num_ops):
            tempVal = args[i]
            if ("0x") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            elif ("0X") in tempVal:
                reg_addr = int(tempVal.split(':')[0], 16)
                reg_val = int(tempVal.split(':')[1], 16)
            else:
                reg_addr = int(tempVal.split(':')[0])
                reg_val = int(tempVal.split(':')[1])
            msg.payload.ops[i].field = reg_addr
            msg.payload.ops[i].value = reg_val

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_lcfg_op_hdr_t(num_ops), 10)
        if reply_msg == None:
            print "Writing BCM LCFG failed!"
            return
        self._print_bcm_lcfg_result(reply_msg)    

    def _print_rtia_cal_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of calibrated values: '{}'".format(int(packet.payload.num_calibrated_values)))
        t = table(["Actual resistance in Ohms", "Calibrated resistance in Ohms"])
        for i in range(packet.payload.num_calibrated_values):
            t.add_row([int(packet.payload.rtia_cal_table_val[i].actual_res), int(packet.payload.rtia_cal_table_val[i].calibrated_res)])
        t.display()    
   
    def _print_ecg_lcfg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))
        t = table(["Field", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].field), hex(packet.payload.ops[i].value)])
        t.display()

    def _print_eda_lcfg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))
        t = table(["Field", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].field), hex(packet.payload.ops[i].value)])
        t.display()
        
    def _get_ecgalgo_version(self, address):
        msg = m2m2_packet(address, m2m2_app_common_ver_req_t())
        msg.payload.command = M2M2_ECG_APP_CMD_ENUM_t.M2M2_ECG_APP_CMD_GET_ALGO_VENDOR_VERSION_REQ
        self._send_packet(msg)
        return self._get_packet(address, m2m2_app_common_version_t())    

    def _print_bcm_lcfg_result(self, packet):
        self._print_packet_status(packet)
        self.vrb.write("  Num of registers: '{}'".format(int(packet.payload.num_ops)))
        t = table(["Field", "Value"])
        for i in range(packet.payload.num_ops):
            t.add_row([hex(packet.payload.ops[i].field), hex(packet.payload.ops[i].value)])
        t.display()

    def do_flash_reset(self, arg):
        """
format file system. Command to format file system.
#>flash_reset
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_file_reset_cmd_t())
        msg.payload.command = M2M2_PM_SYS_COMMAND_ENUM_t.M2M2_PM_SYS_COMMAND_FLASH_RESET_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_PM, m2m2_file_reset_cmd_t(), 240)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device.Flash Reset operation failed.")

    def do_get_fds_status(self, arg):
        """
get fs dcb status
#>get_fds_status
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_BCM_APP_CMD_ENUM_t.M2M2_DCB_COMMAND_FDS_STATUS_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, m2m2_dcb_fds_status_info_resp_t(), 10)
        if reply_msg != None:
            self.vrb.write("  Dirty records: {}".format(int(reply_msg.payload.dirty_records)))
            self.vrb.write("  Open records : {}".format(int(reply_msg.payload.open_records)))
            self.vrb.write("  Valid records : {}".format(int(reply_msg.payload.valid_records)))
            self.vrb.write("  Pages available : {}".format(int(reply_msg.payload.pages_available)))
            self.vrb.write("  Memory Number of blocks : {}".format(int(reply_msg.payload.num_blocks)))
            self.vrb.write("  Blocks free : {}".format(int(reply_msg.payload.blocks_free)))
            self._print_packet_status(reply_msg)
        else:
            self.vrb.err("No response from device.Getting FDS status info failed.")

    def do_run_dcb_test(self, arg):
        """
run dcb tests from robot script
#>run_dcb_test 
        """
        args = self._parse_args(arg, 1)
        if args == None:
            self.vrb.write("please provide valid argument")
            return
        else:
            count = 0
            max_iter = int(args[0])
            while (count < max_iter):
                print 'The count is:', count
                count = count + 1
                self.onecmd("sensor ppg start")
                self.onecmd("status ppg")
                self.onecmd("sensor ppg stop")    
                #self.onecmd("quickstart combined_dcb_ecg_test")
                #self.onecmd("quickstart combined_dcb_eda_test")
                #self.onecmd("quickstart combined_dcb_adpd_test")
                #self.onecmd("quickstart combined_dcb_adxl_test")
                #self.onecmd("quickstart adxl_dcb_test_2")
                #self.onecmd("quickstart ppg_status_check_robot_test")
                #self.onecmd("quickstart temp_dcb_test")

    def do_EdaDynamicScaling(self, arg):
        """
Set the device into a desired power state. The argument is the state:
    'disable' for disabling dynamic scaling
    'enable' for enabling dynamic scaling
    -----------------------------------------------
Usage:
    #>EdaDynamicScaling [scale]
    #>EdaDynamicScaling enable/disable minscale maxscale lprtiasel
    #>minscale [20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>maxscale [20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>lprtiasel[20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>minscale<maxscale, lprtiasel >= minscale
    #>EdaDynamicScaling enable 20 26 20
        """
        arg_len = len(arg.split(' ')) 
        if  arg_len == 1: 
            args = self._parse_args(arg, 1)
            if args == None:
                self.vrb.write("please provide valid argument")
                return
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_dynamic_scale_t())     
            if "d" in arg[0]:
                msg.payload.dscale = 0
                msg.payload.minscale = 0
                msg.payload.maxscale = 0
                msg.payload.lprtia = 0
        elif  arg_len == 4:
            args = self._parse_args(arg, 4)
            if args == None:
                self.vrb.write("please provide valid argument")
                return
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_dynamic_scale_t())   
            if "e" in args[0]:
                msg.payload.dscale = 1
                msg.payload.minscale = int(args[1])
                msg.payload.maxscale = int(args[2])
                msg.payload.lprtia=int(args[3])
        
        msg.payload.command = M2M2_EDA_APP_CMD_ENUM_t.M2M2_EDA_APP_CMD_DYNAMIC_SCALE_REQ
        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_dynamic_scale_t(), 10)
        if reply_msg != None:
            status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
            self.vrb.write(" Status: '{}'".format(status))
        else:
            self.vrb.err("Enabling dynamic scaling failed!")

    def do_DoRTIACal(self, arg):
        """
    -----------------------------------------------
Usage:
    #>DoRTIACal minscale maxscale lprtiasel
    #>minscale [20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>maxscale [20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>lprtiasel[20-100k,21-120k,22-128k,23-160k,24-196k,25-256k,26-512k]
    #>minscale<maxscale, lprtiasel >= minscale
    #>DoRTIACal 20 26 20
        """
        args = self._parse_args(arg, 3)
        if args == None:
            self.vrb.write("please provide valid argument")
            return
        length = int(args[1]) - int(args[0]) + 1
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_perform_rtia_cal_t(length))
        msg.payload.command = M2M2_EDA_APP_CMD_ENUM_t.M2M2_EDA_APP_CMD_RTIA_CAL_REQ
        msg.payload.minscale = int(args[0])
        msg.payload.maxscale = int(args[1])
        msg.payload.lowpowerrtia = int(args[2])
        msg.payload.num_calibrated_values = msg.payload.maxscale - msg.payload.minscale + 1

        self._send_packet(msg)
        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_perform_rtia_cal_t(msg.payload.num_calibrated_values), 240)
        if reply_msg == None:
            self.vrb.err("RTIA Calibration failed!!")
            return
        self._print_rtia_cal_result(reply_msg)   
        self.vrb.write("RTIA Calibration success!!")
                
    def do_SetBCMDFTnum(self, arg):
            """
    Set the BCM app DFT number..
        '0' for 4
        '1' for 8
        '2' for 16
        '3' for 32
        '4' for 64
        '5' for 128
        '6' for 256
        '7' for 512
        '8' for 1024
        '9' for 2048
        '10' for 4096
        '11' for 8192
        '12' for 16384
        -----------------------------------------------
    Usage:
        #>SetBCMDFTnum [value]
        #>SetBCMDFTnum 11
            """
            args = self._parse_args(arg, 1)
            if args == None:
                self.vrb.write("please provide valid argument")
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_set_dft_num_t())
            msg.payload.command = M2M2_BCM_APP_CMD_ENUM_t.M2M2_BCM_APP_CMD_SET_DFT_NUM_REQ
            if args == None:
                msg.payload.dftnum = 11
            else:
                msg.payload.dftnum = int(args[0])
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM,
                                         bcm_app_set_dft_num_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                self.vrb.write("  Status: '{}'".format(status))
                self.vrb.write("  Value: '{}'".format(int(reply_msg.payload.dftnum)))
            else:
                self.vrb.err("Setting BCM DFT number failed!")

    def do_SetEdaDFTnum(self, arg):
            """
    Set the EDA app DFT number..
        '0' for 4
        '1' for 8
        '2' for 16
        '3' for 32
        -----------------------------------------------
    Usage:
        #>SetEdaDFTnum [value]
        #>SetEdaDFTnum 4
            """
            args = self._parse_args(arg, 1)
            if args == None:
                self.vrb.write("please provide valid argument")
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA, eda_app_set_dft_num_t())
            msg.payload.command = M2M2_EDA_APP_CMD_ENUM_t.M2M2_EDA_APP_CMD_SET_DFT_NUM_REQ
            if args == None:
                msg.payload.dftnum = 2
            else:
                msg.payload.dftnum = int(args[0])
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA,
                                         eda_app_set_dft_num_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                self.vrb.write("  Status: '{}'".format(status))
                self.vrb.write("  Value: '{}'".format(int(reply_msg.payload.dftnum)))
            else:
                self.vrb.err("Setting EDA DFT number failed!")

    def do_SetHSRTIACal(self, arg):
            """
    Set the BCM app HSRTIA cal..
            HSRTIACAL[0-200,1-1k,2-5k]
        -----------------------------------------------
    Usage:
        #>SetHSRTIACal [value]
        #>SetHSRTIACal 1
            """
            args = self._parse_args(arg, 1)
            if args == None:
                self.vrb.write("please provide valid argument")
            msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM, bcm_app_hs_rtia_sel_t())
            msg.payload.command = M2M2_BCM_APP_CMD_ENUM_t.M2M2_BCM_APP_CMD_SET_HS_RTIA_CAL_REQ
            if args == None:
                msg.payload.hsritasel = 1
            else:
                msg.payload.hsritasel = int(args[0])
            self._send_packet(msg)
            reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_BCM,
                                        bcm_app_hs_rtia_sel_t(), 10)
            if reply_msg != None:
                status = self._get_enum_name(M2M2_APP_COMMON_STATUS_ENUM_t, reply_msg.payload.status)
                self.vrb.write("  Status: '{}'".format(status))
                self.vrb.write("  Value: '{}'".format(int(reply_msg.payload.hsritasel)))
            else:
                self.vrb.err("Setting BCM HSRTIACAL failed!")

    def do_getEcgVersion(self, arg):
        """
Get the Ecg application version information.
    #>getEcgVersion
        """
        version = self._get_version(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_ECG)
        if version != None:
            self._print_version_pkt(version)
        else:
            self.vrb.err("Timed out waiting for the ECG version response.")
        #self.onecmd("getEcgAlgoVendorVersion")
        
    def do_getEdaVersion(self, arg):
        """
Get the EDA application version information.
    #>getEdaVersion
        """
        version = self._get_version(M2M2_ADDR_ENUM_t.M2M2_ADDR_MED_EDA)
        if version != None:
            self._print_version_pkt(version)
        else:
            self.vrb.err("Timed out waiting for the EDA version response.")
    def do_fs_abort(self, arg):
        """
stop logging
#>fs_abort
        """
        args = self._parse_args(arg, 0)
        if args == None:
            return
        msg = m2m2_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t())
        msg.payload.command = M2M2_FILE_SYS_CMD_ENUM_t.M2M2_FILE_SYS_CMD_FORCE_STOP_LOG_REQ
        self._send_packet(msg)

        reply_msg = self._get_packet(M2M2_ADDR_ENUM_t.M2M2_ADDR_SYS_FS, m2m2_file_sys_cmd_t(), 5000)
        if reply_msg != None:
            self._print_file_system_status(reply_msg)
        else:
            self.vrb.err("No response from device.stop log request failed")      
if __name__ == '__main__':
    m2m2_shell().cmdloop()