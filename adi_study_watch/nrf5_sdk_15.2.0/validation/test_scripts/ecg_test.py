import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def ad8233_switch_functionality_test():
    """
    Turn off AD5940 and ADPD4002 Switches;
    Turn on and off the 8233 Switch and check if AD8233 is disconneted from the electrodes
    :return:
    """
    capture_time = 10  # seconds
    expected_on_vpp = 0.3  # TODO: add execpted ON VPP
    expected_off_vpp = 1e-3  # TODO: add expected OFF VPP

    # Configure waveform (5hz, ~0.1Vpp) on DAC0 and DAC1
    #response = common.arduino.serial_write('!CfgDacWfm 5 0.1\r')
    # TODO: Enable below code to configure function generator
    # common.fg.cfg_waveform(5.0, 0.15, 1.1, 'INV')
    # common.fg.output_enable(1,0)
    # time.sleep(1)
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')

    # common.set_switch('SNOISE1', 1)
    # common.set_switch('SNOISE2', 0)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_SwitchON.csv')
    on_vpp = meas_check.calc_vpp(f_path)
    time.sleep(2)
    # Turn switch OFF and collect data
    common.watch_shell.do_quickstart('ecg')
    #common.watch_shell.do_plot('recg')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_SwitchOFF.csv')
    off_vpp = meas_check.calc_vpp(f_path)

    common.fg.output_enable(0,0)

    common.logging.info('ON_VPP={}, OFF_VPP={}'.format(on_vpp, off_vpp))
    # Test Condition Pass/Fail Check
    if (abs(on_vpp - expected_on_vpp) < 0.1*expected_on_vpp) and off_vpp < expected_off_vpp:
        common.logging.info('*** AD8233 Switch Functionality Test - PASS ***')
    else:
        common.logging.error('*** AD8233 Switch Functionality Test - FAIL ***')
        raise ConditionCheckFailure('\n\nON_VPP={}, OFF_VPP={}'.format(on_vpp, off_vpp))


def ad8233_noise_test():
    """
    Enale 8233 switch, connect the contacts to ECG out of interface board, inject know signal value.
    Collect ADC output data for 61 seconds
    :return:
    """
    capture_time = 5  # seconds
    expected_v_noise = 50e-06

    common.easygui.msgbox('Short the contacts and click OK', 'AD8233 Noise Test')
    # Toggle switches
    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 1)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_ad8233_noise.csv')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    # Test Condition Pass/Fail Check
    v_noise = meas_check.calc_noise(f_path)
    common.logging.info('AD8233 VNoise: {}Vpp'.format(v_noise))
    if v_noise < expected_v_noise:  # if noise is less than 50uVpp
        common.logging.info('*** AD8233 Noise Test - PASS ***')
    else:
        common.logging.error('*** AD8233 Noise Test - FAIL ***')
        raise ConditionCheckFailure('\n\nMeasured v_noise={}'.format(v_noise))


def ad8233_bandwidth_test():
    """
    This function will sweep trough the frequency range and capture data for each frequency
    :return:
    """
    freq_list_hz = [50, 40, 30, 20, 10, 5, 1, 0.5]
    amp_vpp = 0.15
    offset_v = 1.1
    capture_time = 10  # seconds

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    common.fg.cfg_waveform(freq_list_hz[0], amp_vpp, offset_v, 'INV') # added a parameter inv
    common.fg.output_enable(1, 0)
    #response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_list_hz[0], amp_vpp))  # initialize wfm
    time.sleep(1)  # 1 second wait time for waveform generation
    for i, freq_hz in enumerate(freq_list_hz):  # Frequency sweep loop
        #response = common.arduino.serial_write('!SetWfmFreq {}\r'.format(freq_hz))  # Generate wfm at iteration freq
        # Toggle save csv and capture ecg
        # common.watch_shell.do_toggleSaveCSV('')
        common.fg.cfg_waveform(freq_hz, amp_vpp, offset_v, 'INV')
        common.watch_shell.do_quickstart('ecg')
        common.watch_shell.do_plot('recg')
        time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
        common.watch_shell.do_quickstop('ecg')
        time.sleep(1)
        common.rename_stream_file(common.ecg_stream_file_name, '_BW_{}Hz_{}Vpp.csv'.format(freq_hz, amp_vpp))
        # TODO: Read and verify the generated capture files
        common.close_plot_after_run(['ECG Data Plot'])
    common.fg.output_enable(0, 0)


    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad8233_dynamic_range_test():
    """
    This function will sweep trough the amplitude range and capture data for each amplitude
    :return:
    """
    amp_vpp_list = [0.25, 0.15, 0.1, 0.05]
    vin_list = [0.5, 0.3, 0.2, 0.1]
    freq_hz = 5
    offset_v = 1.1
    capture_time = 10  # seconds

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    #response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_hz, amp_vpp_list[0]))  # initialize wfm
    common.fg.cfg_waveform(freq_hz, amp_vpp_list[0], offset_v, 'INV')
    common.fg.output_enable(1, 0)
    time.sleep(1)  # 1 second wait time for waveform generation
    gain_err_list = []
    gain_err_amp_list = []
    for i, amp_vpp in enumerate(amp_vpp_list):  # Amplitude sweep loop
        #response = common.arduino.serial_write('!SetWfmAmp {}\r'.format(amp_vpp))  # Generate wafm at iteration amp
        common.fg.cfg_waveform(freq_hz, amp_vpp, offset_v, 'INV')
        # Toggle save csv and capture ecg
        # common.watch_shell.do_toggleSaveCSV('')
        common.watch_shell.do_quickstart('ecg')
        common.watch_shell.do_plot('recg')
        time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
        common.watch_shell.do_quickstop('ecg')
        time.sleep(1)
        f_path = common.rename_stream_file(common.ecg_stream_file_name, '_DR_{}Hz_{}Vpp.csv'.format(freq_hz, amp_vpp))
        # TODO: Read and verify the generated capture files
        common.close_plot_after_run(['ECG Data Plot'])
        gain, gain_err, gain_err_percent = meas_check.calc_dr(f_path, vin_list[i])
        common.logging.info('[_{}Hz_{}Vpp] Gain:{}, Gain Error:{}, Gain Error Percentage:{}'.format(freq_hz, amp_vpp,
                                                                                                    gain, gain_err,
                                                                                                 gain_err_percent))
        if abs(gain_err_percent) > 10:
            gain_err_list.append('({}vpp: {}%)'.format(amp_vpp, gain_err_percent))

    common.fg.output_enable(0, 0)

    # Test Condition Pass/Fail Check
    if gain_err_list:  # If Failure?
        gain_err_str = '\n'.join(gain_err_list)
        common.logging.error('*** AD8233 Dynamic Range Test - FAIL ***')
        # common.logging.error('Failure Cases:' + gain_err_str)
        raise ConditionCheckFailure("\n\n" + gain_err_str)
    else:
        common.logging.info('*** AD8233 Dynamic Range Test - PASS ***')


def ad8233_repeatability_test():
    """
    This function will configure frequency and amplitude and capture data for 'n' iterations
    :return:
    """
    amp_vpp = 0.15
    freq_hz = 5
    offset_v = 1.1
    repeat_count = 10
    capture_time = 10  # seconds

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    #response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_hz, amp_vpp))  # initialize wfm
    common.fg.cfg_waveform(freq_hz, amp_vpp, offset_v, 'INV')
    common.fg.output_enable(1, 0)
    time.sleep(1)  # 1 second wait time for waveform generation
    for i in range(repeat_count):  # Repeat sweep loop
        # Toggle save csv and capture ecg
        # common.watch_shell.do_toggleSaveCSV('')
        common.watch_shell.do_quickstart('ecg')
        common.watch_shell.do_plot('recg')
        time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
        common.watch_shell.do_quickstop('ecg')
        time.sleep(1)
        common.rename_stream_file(common.ecg_stream_file_name, '_idx{}.csv'.format(i))
        # TODO: Read and verify the generated capture files
        common.close_plot_after_run(['ECG Data Plot'])

    common.fg.output_enable(0, 0)
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad8233_cmrr_test():
    """
    Enable the 8233 Switch and connect the contacts to ECG out of the interface board and measure how good is the
    AD8233 eliminating the 50-60 Hz noise.

    Check 20LOG( (amplitude_ecg_codes/gain)/ amplitude_input signal ).
    Test with no electrode imbalance . Target >80dB
    - Repeat with 51k||47nF imbalance in one electrode. Target > 40dB

    :return:
    """
    amp_vpp = 0.15
    freq_hz = 50
    offset_v = 1.1
    capture_time = 10  # seconds
    cmrr_err_list = []

    # Test A - Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    common.fg.cfg_waveform(freq_hz, amp_vpp, offset_v, 'ON')
    common.fg.output_enable(1, 0)
    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    time.sleep(1)
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_cmrr_a.csv')
    common.close_plot_after_run(['ECG Data Plot'])
    cmrr = meas_check.calc_cmrr(f_path)
    common.logging.info('Test A CMRR: {}dB)'.format(cmrr))
    if cmrr < 80:
        cmrr_err_list.append('(Test A: {}dB)'.format(cmrr))



    # Test B - Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 0)
    common.set_switch('ECG_POSSEL', 1)

    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    time.sleep(1)
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_cmrr_b.csv')
    common.close_plot_after_run(['ECG Data Plot'])
    cmrr = meas_check.calc_cmrr(f_path)
    common.logging.info('Test B CMRR: {}dB)'.format(cmrr))
    if cmrr < 80:
        cmrr_err_list.append('(Test B: {}dB)'.format(cmrr))


    # Test C - Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 0)

    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    time.sleep(1)
    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_cmrr_c.csv')
    cmrr = meas_check.calc_cmrr(f_path)
    common.close_plot_after_run(['ECG Data Plot'])
    common.logging.info('Test C CMRR: {}dB)'.format(cmrr))
    if cmrr < 80:
        cmrr_err_list.append('(Test C: {}dB)'.format(cmrr))

    common.fg.output_enable(0, 0)

    # Test Condition Pass/Fail Check
    if cmrr_err_list:  # If Failure?
        cmrr_err_str = '\n'.join(cmrr_err_list)
        common.logging.error('*** AD8233 CMRR Test - FAIL ***')
        # common.logging.error('Failure Cases:' + cmrr_err_str)
        raise ConditionCheckFailure("\n\n" + cmrr_err_str)
    else:
        common.logging.info('*** AD8233 CMRR Test - PASS ***')


def ad8233_input_impedance_test():
    """

    :return:
    """
    amp_vpp = 0.3
    freq_hz = 5

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    common.messagebox.showinfo('Input Impedance Test', 'Replace R25 and R26 by 1M resistor. '
                                                       'Check output amplitude. '
                                                       'Add 1M in series with each input and '
                                                       'check change in output amplitude. Calculate Zin'
                                                       ''
                                                       'Press OK after measurements are completed!')
    common.watch_shell.do_quickstop('ecg')
    time.sleep(1)
    common.rename_stream_file(common.ecg_stream_file_name, '_inZ.csv')

    common.close_plot_after_run(['ECG Data Plot'])

    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"



if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
