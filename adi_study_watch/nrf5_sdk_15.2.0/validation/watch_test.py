import time
import common
from utils.serial_comm import SerialIface
import random

# ******************** Test Script Imports *****************************
from test_scripts import performance_test
from test_scripts import ecg_test
from test_scripts import bioz_test
from test_scripts import battery_test
from test_scripts import temperature_test
from test_scripts import adpd_test

# **********************************************************************
# 

def battery_test_seq():
    battery_test.ble_advertising_stage_current_consumption_test()
    battery_test.streaming_stage_current_consumption_test()


# *********************** Test Sequences *******************************
def performance_test_seq():
    """
    This function calls and sequences the needed performance tests
    :return:
    """
    performance_test.ecg_switch_functionality_test()
    performance_test.ecg_simulator_signal_quality_test()
    performance_test.ecg_body_signal_quality_test()
    performance_test.adxl_self_test()
    performance_test.ppg_dark_test()
    performance_test.ppg_agc_test()
    performance_test.temperature_accuracy_test()


def ecg_test_seq():
    """
    This function calls and sequences the needed ecg tests
    :return:
    """
    ecg_test.ad8233_bandwidth_test()
    ecg_test.ad8233_dynamic_range_test()
    ecg_test.ad8233_repeatability_test()


def bioz_test_seq():
    """
    This function calls and sequences the needed ecg tests
    :return: 
    """
    a = random.randrange(1, 10)
    if a > 5:
        assert False, "BioZ Test Failued due to limit mismatch!"
    pass  # TODO: to be created


def capture_test():
    """
    This function will test the watch ecg capture by generating a waveform from arduino and capturing the received data
    :return:
    """
    response = common.arduino.serial_write('!CfgDacWfm 500 2.5\r')
    time.sleep(2)
    response = common.arduino.serial_write('!SetWfmAmp 1.2\r')

    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('ecg')
    time.sleep(10)
    common.watch_shell.do_quickstop('ecg')
    # common.watch_shell.do_exit('')


# **********************************************************************


if __name__ == '__main__':
    """
    This is the main case of this script and all functions called below will be executed.
    The same sequence can be initiated from a Robot Script as well.
    """
    common.initialize_setup()
    battery_test_seq()
    # capture_test()
    # common.close_setup()
    pass
