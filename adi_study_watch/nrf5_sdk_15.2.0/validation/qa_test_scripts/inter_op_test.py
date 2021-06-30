import time
import random
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check, qa_utils


def inter_op_dcb_test():
    """

    :return:
    """
    qa_utils.dcb_test(dev='ADPD4K', dcb_file='adpd_qa_dcb.dcfg', dcb_read_file='adpd_dcb_get.dcfg',
                      test_name='Inter-Op ADPD DCB Test')
    qa_utils.dcb_test(dev='ADXL362', dcb_file='adxl_dcb.dcfg', dcb_read_file='adxl_dcb_get.dcfg',
                      test_name='Inter-Op ADXL DCB Test')
    qa_utils.dcb_test(dev='ECG', dcb_file='ecg_dcb.lcfg', dcb_read_file='ecg_dcb_get.lcfg',
                      test_name='Inter-Op ECG DCB Test')
    qa_utils.dcb_test(dev='EDA', dcb_file='eda_dcb.lcfg', dcb_read_file='eda_dcb_get.lcfg',
                      test_name='Inter-Op EDA DCB Test')


def inter_op_dcb_repeatability_test():
    """

    :return:
    """
    for _ in range(10):
        inter_op_dcb_test()


def inter_op_ecg_switch_independent_test():
    """

    :return:
    """
    capture_time = 30
    adpd_freq_hz = 500
    adxl_freq_hz = 50
    temp_freq_hz = 1
    eda_freq_hz = 4

    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'eda')

    common.watch_shell.do_disable_electrode_switch('3')
    common.watch_shell.do_disable_electrode_switch('2')
    common.watch_shell.do_disable_electrode_switch('1')

    adpd_f_name = common.quick_start_adpd(samp_freq_hz=adpd_freq_hz, agc_state=1)
    common.quick_start_adxl(samp_freq_hz=adxl_freq_hz)
    common.quick_start_temperature()
    common.quick_start_eda(eda_freq_hz)

    time.sleep(capture_time)
    common.quick_stop_adpd('G')
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('temp', 'temp')
    common.watch_shell.quick_stop('eda', 'eda')

    f_path_adpd = common.rename_stream_file(adpd_f_name, "Ecg_SW_independent_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name,
                                            'Ecg_SW_independent_{}Hz.csv'.format(adxl_freq_hz))
    f_path_temperature = common.rename_stream_file(common.temperature_stream_file_name,
                                                   'Ecg_SW_independent_{}Hz.csv'.format(temp_freq_hz))
    f_path_eda = common.rename_stream_file(common.eda_stream_file_name,
                                           'Ecg_SW_independent_{}hz_test.csv'.format(eda_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd, 'adpd',
                                                                                              1, adpd_freq_hz)
    err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd',
                                                                                              2, adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)
    err_status_eda, err_str_eda, results_dict_eda = qa_utils.check_stream_data(f_path_eda, 'eda', 1, eda_freq_hz)

    common.test_logger.info('ADPD {}Hz Ecg SW Independent Test Results: {}'.format(adpd_freq_hz, results_dict_adpd_ch1))
    common.test_logger.info('ADPD {}Hz Ecg SW Independent Test Results: {}'.format(adpd_freq_hz, results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz Ecg SW Independent Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.test_logger.info('Temperature {}Hz Ecg SW Independent Test Results: {}'.format(temp_freq_hz, results_dict_temp))
    common.test_logger.info('EDA {}Hz Ecg SW Independent Test Results: {}'.format(eda_freq_hz, results_dict_eda))

    if err_status_adpd_ch1:
        common.test_logger.error('***PPG {}Hz Ecg SW Independent Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    if err_status_adpd_ch2:
        common.test_logger.error('***PPG {}Hz Ecg SW Independent Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz Ecg SW Independent Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz Ecg SW Independent Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))
    if err_status_eda:
        common.test_logger.error('*** EDA {}Hz Ecg SW Independent Test - FAIL ***'.format(eda_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_eda))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    inter_op_ecg_switch_independent_test()
    common.close_setup()
