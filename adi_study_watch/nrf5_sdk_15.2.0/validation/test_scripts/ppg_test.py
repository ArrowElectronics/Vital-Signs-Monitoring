import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def ppg_dark_test():
    """
    Place a reflective surface over the LEDs and measure the output at 1 pulse 0mA current
    :return:
    """
    capture_time = 20
    common.easygui.msgbox('Place the wrist/finger on the sensor and press OK', 'PPG')
    common.dcb_cfg('w', 'adpd', 'ppg_dark_test.dcfg')
    stream_file_name = common.quick_start_adpd(100, 0)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.dcb_cfg('d', 'adpd')
    ppg_file = common.rename_stream_file(common.adpd_stream_file_name, '_dark_test.csv')
    ch1_std_dev = meas_check.calc_std_dev(ppg_file, 1)
    ch2_std_dev = meas_check.calc_std_dev(ppg_file, 3)

    # *** Test Results Evaluation ***
    if ch1_std_dev < 2 and ch2_std_dev < 2:
        common.test_logger.info('Ch0 StdDev = {} | Ch1 StdDev = {}'.format(ch1_std_dev, ch2_std_dev))
        common.test_logger.info('*** PPG Dark Test - PASS ***')
    else:
        common.test_logger.error('Ch0 StdDev = {} | Ch1 StdDev = {}'.format(ch1_std_dev, ch2_std_dev))
        common.test_logger.error('*** PPG Dark Test - FAIL ***')
        raise ConditionCheckFailure('\n\nCh1 or Ch2 standard deviation exceeded the limits!')


def ppg_agc_test():
    """
    Place the finger/wrist on the optical window
    :return:
    """
    capture_time = 40
    common.easygui.msgbox('Make sure watch is on the wrist and press OK', 'PPG')
    # TODO: Add PPG AGC Test DCFG
    # common.dcb_cfg('w', 'adpd', 'ppg_dark_test.dcfg')

    # ADPD Stream with AGC ON
    stream_file_name = common.quick_start_adpd(100, 1)
    time.sleep(capture_time)
    common.quick_stop_adpd()
    ppg_agc_on_file = common.rename_stream_file(stream_file_name, '_agc_on.csv')
    ppg_agc_on_file = os.path.abspath(ppg_agc_on_file)
    ppg_agc_on_results_dict = common.analyze_wfm(ppg_agc_on_file, 'ppg')
    common.write_analysis_report(ppg_agc_on_results_dict,'ppg_test_result.txt', 'AGC-ON', True)
    time.sleep(5)

    # ADPD Stream with AGC OFF
    status, reg_val_list = common.watch_shell.reg_read('adpd', '0x01A5')
    common.watch_shell.do_load_adpd_cfg('1')
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    if reg_val_list:
        reg_val = int(reg_val_list[0][1], 16) >> 2
        common.watch_shell.do_reg('w adpd 0x01A5:{}'.format(reg_val))
    stream_file_name = common.quick_start_adpd(100, 0, 'G', True)
    time.sleep(capture_time)
    common.quick_stop_adpd()
    ppg_agc_off_file = common.rename_stream_file(stream_file_name, '_agc_off.csv')
    ppg_agc_off_file = os.path.abspath(ppg_agc_off_file)
    ppg_agc_off_results_dict = common.analyze_wfm(ppg_agc_off_file, 'ppg')
    common.write_analysis_report(ppg_agc_off_results_dict, 'ppg_test_result.txt', 'AGC-OFF', True)
    time.sleep(5)

    # *** Test Results Evaluation ***
    dc_amp_ch0_agc_off = float(ppg_agc_off_results_dict['dc_amp_ch0'])
    dc_amp_ch1_agc_off = float(ppg_agc_off_results_dict['dc_amp_ch1'])
    dc_amp_ch0_agc_on = float(ppg_agc_on_results_dict['dc_amp_ch0'])
    dc_amp_ch1_agc_on = float(ppg_agc_on_results_dict['dc_amp_ch1'])
    common.test_logger.info('CH0: agc_on_dc_amp = {} | agc_on_dc_amp = {}'.format(dc_amp_ch0_agc_on,
                                                                              dc_amp_ch0_agc_off))
    common.test_logger.info('CH1: agc_on_dc_amp = {} | agc_on_dc_amp = {}'.format(dc_amp_ch0_agc_on,
                                                                              dc_amp_ch0_agc_off))
    if (dc_amp_ch0_agc_off < (0.09 * dc_amp_ch0_agc_on)) and \
       (dc_amp_ch1_agc_off < (0.09 * dc_amp_ch1_agc_on)):
        common.test_logger.info('*** PPG AGC ON/OFF Test - PASS ***')
    else:
        common.test_logger.error('*** PPG AGC ON/OFF Test - FAIL ***')
        raise ConditionCheckFailure('\n\nCh1 or Ch2 DC amplitude is not showing expected changes during AGC ON/OFF!')


def ppg_at_multiple_frequency_test():
    """
    Place the finger/wrist on the optical window
    :return:
    """
    capture_time = 40
    freq_list_hz = [500, 100, 50]
    common.easygui.msgbox('Make sure watch is on the wrist and press OK', 'PPG')

    # ADPD Stream with AGC ON
    append_report = False
    for i, freq_hz in enumerate(freq_list_hz):
        stream_file_name = common.quick_start_adpd(freq_hz, 1)
        time.sleep(capture_time)
        common.quick_stop_adpd()
        ppg_agc_on_file = common.rename_stream_file(stream_file_name, '_{}Hz.csv'.format(freq_hz))
        ppg_agc_on_file = os.path.abspath(ppg_agc_on_file)
        ppg_agc_on_results_dict = common.analyze_wfm(ppg_agc_on_file, 'ppg')
        common.write_analysis_report(ppg_agc_on_results_dict,'ppg_test_result.txt', 
        'Sampling Frequency-{}Hz'.format(freq_hz), True) if append_report else common.write_analysis_report(ppg_agc_on_results_dict,'ppg_test_result.txt', 
        'Sampling Frequency-{}Hz'.format(freq_hz), False)
        time.sleep(5)
        append_report = True
    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ppg_agc_test_watches():
    """
    Place the finger/wrist on the optical window
    :return:
    """
    watch_num_list = [1, 2, 3, 4, 5]  # type:
    # TODO: This is a place holder for AGC test. The logic has to be implemented
    capture_time = 40
    for watch_num in watch_num_list:
        common.easygui.msgbox('Make sure watch is on the wrist and press OK', 'PPG')
    # ADPD Stream with AGC ON
        stream_file_name = common.quick_start_adpd(500, 1)
        time.sleep(capture_time)
        common.quick_stop_adpd()
        ppg_agc_on_file = common.rename_stream_file(stream_file_name, '_agc_on_iteration-{}.csv'.format(watch_num))
        ppg_agc_on_file = os.path.abspath(ppg_agc_on_file)
        ppg_agc_on_results_dict = common.analyze_wfm(ppg_agc_on_file, 'ppg')
        common.write_analysis_report(ppg_agc_on_results_dict,'ppg_test_result.txt', 'Iteration_On-{}'.format(watch_num),
                                     True) if watch_num > 1 else common.write_analysis_report(ppg_agc_on_results_dict,
                                                                                              'ppg_test_result.txt',
                                                                                              'Iteration_On-{}'.format
                                                                                                      (watch_num), False)
        time.sleep(5)
    #print('AGC ON PPG Results:')
    #print(ppg_agc_on_results_dict)

    # ADPD Stream with AGC OFF
        status, reg_val_list = common.watch_shell.reg_read('adpd', '0x01A5')
        if reg_val_list:
            reg_val = int(reg_val_list[0][1], 16) >> 2
            common.watch_shell.do_reg('adpd w 0x01A5:{}'.format(reg_val))
        stream_file_name = common.quick_start_adpd(500, 0)
        time.sleep(capture_time)
        common.quick_stop_adpd()
        ppg_agc_off_file = common.rename_stream_file(stream_file_name, '_agc_off_iteration-{}.csv'.format(watch_num))
        ppg_agc_off_file = os.path.abspath(ppg_agc_off_file)
        ppg_agc_off_results_dict = common.analyze_wfm(ppg_agc_off_file, 'ppg')
        common.write_analysis_report(ppg_agc_off_results_dict, 'ppg_test_result.txt', 'Iteration_Off-{}'.format(watch_num),
                                     True)
        time.sleep(5)
    # print('AGC OFF PPG Results:')
    # print(ppg_agc_off_results_dict)

    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
