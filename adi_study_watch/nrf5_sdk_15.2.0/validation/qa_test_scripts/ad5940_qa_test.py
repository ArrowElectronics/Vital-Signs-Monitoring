import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils, meas_check

capture_time = 120


def ad5940_dev_id_test():
    """

    :return:
    """
    qa_utils.dev_id_test('AD5940')


def ecg_dcb_test():
    """
    Load a dcb  file and read back to check if the values are loaded correctly
    :return:
    """
    qa_utils.dcb_test(dev='ECG', dcb_file='ecg_dcb.lcfg', dcb_read_file='ecg_dcb_get.lcfg',
                      test_name='ECG DCB Test')


def ecg_dcb_stream_test():
    """
    Load the DCB file with sampling freq of 200Hz and capture the data to check the sampling rate
    :return:
    """
    capture_time = 30
    freq_hz = 200
    ecg_dcb_test()
    qa_utils.enable_ecg_without_electrodes_contact()

    common.quick_start_ecg(samp_freq_hz=freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')

    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_dcb_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz)
    common.test_logger.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ecg_dcb_repeatability_test():
    """
    Repeat the DCB Stream test for 10 iteration
    :return:
    """
    for _ in range(10):
        ecg_dcb_stream_test()


def ecg_stream_test(freq_hz=100):
    """
    Capture ECG data for 30s, check the data for frequency(default 50Hz) frequency mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'ecg')
    qa_utils.enable_ecg_without_electrodes_contact()

    common.quick_start_ecg(samp_freq_hz=freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')
    common.dcb_cfg('d', 'ecg')

    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz)
    common.test_logger.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ecg_fs_stream_test(freq_hz=100):
    """
    Capture ECG data for 30s, check the data for frequency(default 50Hz) frequency mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    common.dcb_cfg('d', 'ecg')
    qa_utils.clear_fs_logs('ecg')
    qa_utils.enable_ecg_without_electrodes_contact()

    common.quick_start_ecg_fs(samp_freq_hz=freq_hz)
    time.sleep(capture_time)
    common.quick_stop_ecg_fs()
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('ecg')
    common.dcb_cfg('d', 'ecg')

    f_path = common.rename_stream_file(csv_file_name_dict["ecg"], '_ecg_stream{}hz_fs_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz, True)
    common.test_logger.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def eda_dcb_test():
    """
    Load a dcb  file and read back to check if the values are loaded correctly
    :return:
    """
    qa_utils.dcb_test(dev='EDA', dcb_file='eda_dcb.lcfg', dcb_read_file='eda_dcb_get.lcfg',
                      test_name='EDA DCB Test')


def eda_dcb_stream_test():
    """
    Load the DCB file with sampling freq of 25Hz and capture the data to check the sampling rate
    :return:
    """
    capture_time = 30
    freq_hz = 25
    eda_dcb_test()

    common.quick_start_eda(freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('eda', 'eda')
    common.dcb_cfg('d', 'eda')
    f_path = common.rename_stream_file(common.eda_stream_file_name, '_eda_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz)
    common.test_logger.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def eda_dcb_repeatability_test():
    """
    Repeat the DCB Stream test for 10 iteration
    :return:
    """
    for _ in range(10):
        eda_dcb_stream_test()


def eda_stream_test(freq_hz=4):
    """
    Capture EDA data for 30s, check the data for frequency(default 4Hz) mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'eda')

    common.quick_start_eda(freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('eda', 'eda')
    common.dcb_cfg('d', 'eda')
    f_path = common.rename_stream_file(common.eda_stream_file_name, '_eda_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz)
    common.test_logger.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def eda_fs_stream_test(freq_hz=4):
    """
    Capture EDA data for 30s, check the data for frequency(default 4Hz) frequency mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    common.dcb_cfg('d', 'eda')
    qa_utils.clear_fs_logs('eda')

    common.quick_start_eda_fs(samp_freq_hz=freq_hz)
    time.sleep(capture_time)
    common.quick_stop_eda_fs()
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('eda')
    common.dcb_cfg('d', 'eda')

    f_path = common.rename_stream_file(csv_file_name_dict["eda"], '_eda_stream{}hz_fs_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz, True)
    common.test_logger.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ad5940_switch_test():
    """
    Capture ECG for 10s turning off all the Switches, no data should be received, the Turn On the 5940 & 8233 switch
    the capture ECG data you should receive the data as expected with the default frequency
    :return:
    """
    capture_time = 10
    freq_hz = 0
    common.dcb_cfg('d', 'ecg')
    common.watch_shell.do_disable_electrode_switch('3')
    common.watch_shell.do_disable_electrode_switch('2')
    common.watch_shell.do_disable_electrode_switch('1')

    common.watch_shell.quick_start('ecg', 'ecg')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')
    f_path_sw_off = common.rename_stream_file(common.ecg_stream_file_name, '_ad5940_switch_off_test.csv')
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path_sw_off, 'ecg', 1, freq_hz)
    common.test_logger.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ECG {}Hz SW OFF Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    freq_hz = 100
    common.dcb_cfg('d', 'ecg')
    common.watch_shell.do_disable_electrode_switch('3')
    common.watch_shell.do_disable_electrode_switch('2')
    common.watch_shell.do_enable_electrode_switch('1')

    common.quick_start_ecg(freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('ecg', 'ecg')
    common.dcb_cfg('d', 'ecg')
    f_path_sw_on = common.rename_stream_file(common.ecg_stream_file_name, '_ad5940_switch_on_test.csv')
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path_sw_on, 'ecg', 1, freq_hz)
    common.test_logger.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ECG {}Hz SW On Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ecg_freq_sweep_test():
    """
    Sweep ECG across Valid Frequencies
    :return:
    """
    freq = [12, 25, 50, 100, 200, 250, 300, 400, 500, 800, 1000]
    for freq_hz in freq:
        ecg_stream_test(freq_hz=freq_hz)


def ecg_repeatability_sweep_test():
    """
    Repeat ECG freq sweep test for 5 iterations
    :return:
    """
    for _ in range(5):
        ecg_freq_sweep_test()


def eda_freq_sweep_test():
    """
    Sweep EDA across Valid Frequencies
    :return:
    """
    freq = [4, 10, 15, 20, 25, 30]
    for freq_hz in freq:
        eda_stream_test(freq_hz=freq_hz)


def eda_repeatability_sweep_test():
    """
    Repeat EDA freq sweep test for 5 iterations
    :return:
    """
    for _ in range(5):
        eda_freq_sweep_test()


def ecg_fs_freq_sweep_test():
    """
    Sweep ECG across Valid Frequencies
    :return:
    """
    freq = [12, 25, 50, 100, 200, 250, 300, 400, 500, 800, 1000]
    for freq_hz in freq:
        ecg_fs_stream_test(freq_hz=freq_hz)


def eda_fs_freq_sweep_test():
    """
    Sweep ECG across Valid Frequencies
    :return:
    """
    freq = [4, 10, 15, 20, 25, 30]
    for freq_hz in freq:
        eda_fs_stream_test(freq_hz=freq_hz)


def bia_fs_stream_test(freq_hz=10):

    qa_utils.clear_fs_logs('eda')

    common.quick_start_bia_fs(freq_hz)
    time.sleep(capture_time)
    common.quick_stop_bia_fs()
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('bia')

    f_path = common.rename_stream_file(csv_file_name_dict["bia"], '_bia_stream_fs_{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'bia', 0, freq_hz, True)
    common.test_logger.info('bia {}Hz FS Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** bia {}Hz FS Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def bia_stream_test(freq_hz=10):

    capture_time = 30
    # common.dcb_cfg('d', 'bia')

    common.quick_start_bia(freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('bia', 'bia')
    # common.dcb_cfg('d', 'bia')

    f_path = common.rename_stream_file(common.bia_stream_file_name, '_bia_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'bia', 1, freq_hz)
    common.test_logger.info('bia {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** bia {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def bia_freq_sweep_test():
    """
    Sweep bia across Valid Frequencies
    :return:
    """
    freq = [4, 10, 15, 20, 22]
    for freq_hz in freq:
        bia_stream_test(freq_hz=freq_hz)


def bia_fs_freq_sweep_test():
    """
    Sweep bia across Valid Frequencies
    :return:
    """
    freq = [4, 10, 15, 20, 22]
    for freq_hz in freq:
        bia_fs_stream_test(freq_hz=freq_hz)


def bia_repeatability_sweep_test():
    """
    Repeat EDA freq sweep test for 5 iterations
    :return:
    """
    for _ in range(5):
        bia_freq_sweep_test()


def bia_dcb_test():
    """
    Load a dcb  file and read back to check if the values are loaded correctly
    :return:
    """
    qa_utils.dcb_test(dev='BIA', dcb_file='bia_dcb.lcfg', dcb_read_file='bia_dcb_get.lcfg',
                      test_name='bia DCB Test')


def bia_dcb_stream_test():

    capture_time = 30
    freq_hz = 10
    bia_dcb_test()

    common.quick_start_bia(freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('bia', 'bia')
    common.dcb_cfg('d', 'bia')

    f_path = common.rename_stream_file(common.bia_stream_file_name, '_bia_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'bia', 1, freq_hz)
    common.test_logger.info('bia {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** bia {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def bia_dcb_repeatability_test():
    """
    Repeat the DCB Stream test for 10 iteration
    :return:
    """
    for _ in range(10):
        bia_dcb_stream_test()


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    ad5940_switch_test()
    common.close_setup()
