import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common


def adpd_functionality_test():
    """
    Close DG2502 switch and collect data for 10 sec
    Open DG2502 switch and collect data for 10 sec
    :return:
    """
    capture_time = 10  # seconds

    # Configure waveform (5hz, ~0.1Vpp) on DAC0 and DAC1
    # response = common.arduino.serial_write('!CfgDacWfm 5 0.1\r')
    time.sleep(1)
    common.watch_shell.do_disable_electrode_switch('2')  # AD5940 Switch

    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 0)
    # common.set_switch('ECG_NEGSEL', 0)
    # common.set_switch('ECG_POSSEL', 0)

    # Turn switch ON and collect data
    common.watch_shell.do_enable_electrode_switch('3')  # ADPD4K Switch
    common.watch_shell.quick_start('adpd', 'adpd6')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.rename_stream_file(common.adpd_stream_file_name, '_SwitchON.csv')

    # Turn switch OFF and collect data
    common.watch_shell.do_disable_electrode_switch('3')  # ADPD4K Switch
    common.watch_shell.quick_start('adpd', 'adpd6')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.rename_stream_file(common.adpd_stream_file_name, '_SwitchOFF.csv')

    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    adpd_functionality_test()
    common.close_setup()
