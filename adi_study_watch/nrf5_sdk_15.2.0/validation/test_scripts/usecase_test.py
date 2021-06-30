import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def use_case_ecg():
    """
    This is a testcase template.
    Create a copy of this function, rename the function name and description to use this
    :return:
    """
    capture_time = 20  # seconds
    ref_dict = {'ref_qrs_amp': 0.1, 'ref_hr': 70}  # TODO: These variables need to be updated

    common.watch_shell.do_disable_electrode_switch('3')
    common.watch_shell.do_disable_electrode_switch('2')
    common.watch_shell.do_enable_electrode_switch('1')
    common.dcb_cfg('w', 'adxl', 'adxl_dcb_ecg_UC.DCFG')
    common.dcb_cfg('w', 'adpd', 'ppg_dcb_ecg_UC.dcfg')
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.easygui.msgbox('Touch the top electrodes with your finger and thumb and click OK', 'ECG_PPG_Usecase_Test')

    common.quick_start_ecg(samp_freq_hz=500)
    common.quick_start_adxl(samp_freq_hz=50)
    common.quick_start_temperature()
    common.quick_start_adpd(samp_freq_hz=None, agc_state=1, led="G", skip_load_cfg=True)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.watch_shell.quick_stop('temp', 'temp')
    common.rename_stream_file(common.adxl_stream_file_name, '_ecg_UC.csv')
    common.rename_stream_file(common.adpd_stream_file_name, '_ecg_UC.csv')
    common.rename_stream_file(common.temperature_stream_file_name, '_ecg_UC.csv')
    ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_UC.csv')
    ecg_file = os.path.abspath(ecg_file)
    usecase_ecg_results_dict = common.analyze_wfm(ecg_file, 'ecg')
    common.test_logger.info('ecg_results = {}'.format(usecase_ecg_results_dict))

    common.watch_shell.do_disable_electrode_switch('1')
    common.set_ecg_stream_freq(samp_freq_hz=250)
    common.dcb_cfg('d', 'adxl', 'adxl_dcb_ecg_UC.dcfg')
    common.dcb_cfg('d', 'adpd', 'adxl_dcb_ecg_UC.dcfg')

    # *** Test Results Evaluation ***
    status = meas_check.check_stream_status('ecg', usecase_ecg_results_dict, ref_dict)
    if status:
        common.test_logger.info('*** ECG With PPG ADXL Temperature Test - PASS ***')
    else:
        common.test_logger.error('*** ECG With PPG ADXL Temperature Test - FAIL ***')
        raise ConditionCheckFailure('\n\n One or more parameters of the results exceeded the limits!')


def use_case_ppg():
    """
    This is a testcase template.
    Create a copy of this function, rename the function name and description to use this
    :return:
    """
    capture_time = 20  # seconds
    mi_range = (0.1, 2)  # TODO: These variables need to be updated

    common.dcb_cfg('w', 'adxl', 'adxl_dcb_ppg_UC.dcfg')
    common.dcb_cfg('w', 'adpd', 'ppg_dcb_ppg_UC.dcfg')
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.watch_shell.do_enable_agc('1')  # Turning AGC ON

    common.quick_start_adxl(samp_freq_hz=50)
    common.quick_start_temperature()
    common.quick_start_adpd(samp_freq_hz=None, agc_state=1, led="G", skip_load_cfg=True)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.watch_shell.quick_stop('temp', 'temp')
    common.rename_stream_file(common.adxl_stream_file_name, '_ppg_UC.csv')
    common.rename_stream_file(common.temperature_stream_file_name, '_ppg_UC.csv')
    ppg_file = common.rename_stream_file(common.adpd_stream_file_name, '_ppg_UC.csv')
    ppg_file = os.path.abspath(ppg_file)
    ppg_results_dict = common.analyze_wfm(ppg_file, 'ppg')
    common.test_logger.info('usecase_ppg_results = {}'.format(ppg_results_dict))

    common.dcb_cfg('d', 'adxl', 'adxl_dcb_ppg_UC.dcfg')
    common.dcb_cfg('d', 'adpd', 'ppg_dcb_ppg_UC.dcfg')

    # *** Test Results Evaluation ***
    if (mi_range[0] <= float(ppg_results_dict['mi_ch0']) <= mi_range[1]) and \
       (mi_range[0] <= float(ppg_results_dict['mi_ch1']) <= mi_range[1]):
        common.test_logger.info('*** PPG AGC With ADXL TEMP Test - PASS ***')
    else:
        common.test_logger.error('*** PPG AGC With ADXL TEMP Test - FAIL ***')
        raise ConditionCheckFailure('\n\nCh1 or Ch2 Modulation Index is not within the expected limits!')


def use_case_ecg_ppg():
    """
    This is a testcase template.
    Create a copy of this function, rename the function name and description to use this
    :return:
    """
    capture_time = 20  # seconds
    mi_range = (0.1, 2)  # TODO: These variables need to be updated

    common.watch_shell.do_disable_electrode_switch('3')
    common.watch_shell.do_disable_electrode_switch('2')
    common.watch_shell.do_enable_electrode_switch('1')
    common.dcb_cfg('w', 'adxl', 'adxl_dcb_ecg_ppg_UC.dcfg')
    common.dcb_cfg('w', 'adpd', 'ppg_dcb_ecg_ppg_UC.dcfg')
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.easygui.msgbox('Touch the top electrodes with your finger and thumb and click OK', 'ECG_PPG_Usecase_Test')
    common.watch_shell.do_enable_agc('1')  # Turning AGC ON

    common.quick_start_ecg(samp_freq_hz=250)
    common.quick_start_adxl(samp_freq_hz=50)
    common.quick_start_adpd(samp_freq_hz=None, agc_state=1, led="G", skip_load_cfg=True)
    common.quick_start_temperature()
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.watch_shell.quick_stop('temp', 'temp')
    common.rename_stream_file(common.adxl_stream_file_name, '_ecg_ppg_UC.csv')
    common.rename_stream_file(common.temperature_stream_file_name, '_ecg_ppg_UC.csv')
    ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_ppg_UC.csv')
    ppg_file = common.rename_stream_file(common.adpd_stream_file_name, '_ecg_ppg_UC.csv')
    ppg_file = os.path.abspath(ppg_file)
    ppg_results_dict = common.analyze_wfm(ppg_file, 'ppg')
    common.test_logger.info('usecase_ppg_results = {}'.format(ppg_results_dict))

    common.watch_shell.do_disable_electrode_switch('1')
    common.set_ecg_stream_freq(samp_freq_hz=250)
    common.dcb_cfg('d', 'adxl', 'adxl_dcb_ecg_ppg_UC.dcfg')
    common.dcb_cfg('d', 'adpd', 'ppg_dcb_ecg_ppg_UC.dcfg')

    # *** Test Results Evaluation ***
    if (mi_range[0] <= float(ppg_results_dict['mi_ch0']) <= mi_range[1]) and \
            (mi_range[0] <= float(ppg_results_dict['mi_ch1']) <= mi_range[1]):
        common.test_logger.info('*** PPG AGC With ECG ADXL TEMP Test - PASS ***')
    else:
        common.test_logger.error('*** PPG AGC With ECG ADXL TEMP Test - FAIL ***')
        raise ConditionCheckFailure('\n\nCh1 or Ch2 Modulation Index is not within the expected limits!')


def use_case_eda_ppg():
    """
    This is a testcase template.
    Create a copy of this function, rename the function name and description to use this
    :return:
    """
    capture_time = 20  # seconds
    mi_range = (0.1, 2)  # TODO: These variables need to be updated

    common.dcb_cfg('w', 'adxl', 'adxl_dcb_eda_ppg_UC.dcfg')
    common.dcb_cfg('w', 'adpd', 'ppg_dcb_eda_ppg_UC.dcfg')
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.watch_shell.do_enable_agc('1')  # Turning AGC ON

    common.quick_start_adxl(samp_freq_hz=50)
    common.quick_start_adpd(samp_freq_hz=None, agc_state=1, led="G", skip_load_cfg=True)
    common.quick_start_temperature()
    common.quick_start_eda(samp_freq_hz=30)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('adpd', 'adpd6')
    common.watch_shell.quick_stop('temp', 'temp')
    common.watch_shell.quick_stop('eda', 'eda')
    common.close_plot_after_run(['ADXL Data'])
    common.rename_stream_file(common.eda_stream_file_name, '_eda_ppg_UC.csv')
    common.rename_stream_file(common.adxl_stream_file_name, '_eda_ppg_UC.csv')
    common.rename_stream_file(common.temperature_stream_file_name, '_eda_ppg_UC.csv')
    ppg_file = common.rename_stream_file(common.adpd_stream_file_name, '_eda_ppg_UC.csv')
    ppg_file = os.path.abspath(ppg_file)
    ppg_results_dict = common.analyze_wfm(ppg_file, 'ppg')
    common.test_logger.info('usecase_ppg_results = {}'.format(ppg_results_dict))

    common.dcb_cfg('d', 'adxl', 'adxl_dcb_eda_ppg_UC.dcfg')
    common.dcb_cfg('d', 'adpd', 'ppg_dcb_eda_ppg_UC.dcfg')

    # *** Test Results Evaluation ***
    if (mi_range[0] <= float(ppg_results_dict['mi_ch0']) <= mi_range[1]) and \
       (mi_range[0] <= float(ppg_results_dict['mi_ch1']) <= mi_range[1]):
        common.test_logger.info('*** PPG AGC With ADXL EDA TEMP Test - PASS ***')
    else:
        common.test_logger.error('*** PPG AGC With ADXL EDA TEMP Test - FAIL ***')
        raise ConditionCheckFailure('\n\nCh1 or Ch2 Modulation Index is not within the expected limits!')


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    use_case_ecg()
    common.close_setup()
