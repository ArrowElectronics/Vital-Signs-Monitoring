import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils, meas_check


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

    common.watch_shell.do_sub('recg add')
    common.watch_shell.do_sensor('ecg start')
    common.watch_shell.do_plot("recg")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])

    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_dcb_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz)
    common.logging.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
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

    common.quick_start_ecg(freq_hz)
    common.watch_shell.do_plot("recg")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    common.dcb_cfg('d', 'ecg')

    f_path = common.rename_stream_file(common.ecg_stream_file_name, '_ecg_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz)
    common.logging.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ecg_fs_stream_test(freq_hz=100):
    """
    Capture ECG data for 30s, check the data for frequency(default 50Hz) frequency mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'ecg')
    qa_utils.clear_fs_logs('ecg')

    common.watch_shell.do_quickstart("start_log_ecg")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('stop_log_ecg')
    common.dcb_cfg('d', 'ecg')
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('ecg')

    f_path = common.rename_stream_file(csv_file_name_dict["ecg"], '_ecg_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ecg', 1, freq_hz, True)
    common.logging.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** ECG {}Hz Stream Test - FAIL ***'.format(freq_hz))
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

    common.watch_shell.do_sub('reda add')
    common.watch_shell.do_sensor('eda start')
    common.watch_shell.do_plot("reda")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('eda')
    common.close_plot_after_run(['EDA Data Plot'])
    f_path = common.rename_stream_file(common.eda_stream_file_name, '_eda_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz)
    common.logging.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
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
    common.watch_shell.do_plot("reda")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('eda')
    common.close_plot_after_run(['EDA Data Plot'])
    common.dcb_cfg('d', 'eda')
    f_path = common.rename_stream_file(common.eda_stream_file_name, '_eda_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz)
    common.logging.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def eda_fs_stream_test(freq_hz=4):
    """
    Capture EDA data for 30s, check the data for frequency(default 4Hz) frequency mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'eda')
    qa_utils.clear_fs_logs('eda')

    common.set_eda_stream_freq(samp_freq_hz=freq_hz)
    common.watch_shell.do_quickstart("start_log_eda")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('stop_log_eda')
    common.dcb_cfg('d', 'eda')
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('eda')

    f_path = common.rename_stream_file(csv_file_name_dict["eda"], '_eda_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'eda', 1, freq_hz, True)
    common.logging.info('EDA {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** EDA {}Hz Stream Test - FAIL ***'.format(freq_hz))
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
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')

    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    f_path_sw_off = common.rename_stream_file(common.ecg_stream_file_name, '_ad5940_switch_off_test.csv')
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path_sw_off, 'ecg', 1, freq_hz)
    common.logging.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** ECG {}Hz SW OFF Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    freq_hz = 100
    common.dcb_cfg('d', 'ecg')
    common.watch_shell.do_set_ecg_dcb_lcfg("")
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    f_path_sw_on = common.rename_stream_file(common.ecg_stream_file_name, '_ad5940_switch_on_test.csv')
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path_sw_on, 'ecg', 1, freq_hz)
    common.logging.info('ECG {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** ECG {}Hz SW On Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def ecg_freq_sweep_test():
    """
    Sweep ECG across Valid Frequencies
    :return:
    """
    freq = [12, 25, 50, 100, 200, 250, 300, 400, 500, 600, 700, 800, 900, 1000]
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


def bcm_stream_test(freq_hz=10):

    capture_time = 30
    common.dcb_cfg('d', 'ecg')

    common.quick_start_bcm(freq_hz)
    common.watch_shell.do_plot("rbcm")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('BCM')
    common.close_plot_after_run(['BCM Data Plot'])

    f_path = common.rename_stream_file(common.bcm_stream_file_name, '_BCM_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'BCM', 1, freq_hz)
    common.logging.info('BCM {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    common.watch_shell.do_lcfgBcmRead("0")
    if err_status:
        common.logging.error('*** BCM {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def bcm_dcb_test():
    """
    Load a dcb  file and read back to check if the values are loaded correctly
    :return:
    """
    qa_utils.dcb_test(dev='BCM', dcb_file='bcm_dcb.lcfg', dcb_read_file='bcm_dcb_get.lcfg',
                      test_name='BCM DCB Test')


def bcm_dcb_stream_test():

    capture_time = 30
    freq_hz = 10
    bcm_dcb_test()

    common.quick_start_bcm(freq_hz)
    common.watch_shell.do_plot("rbcm")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('BCM')
    common.close_plot_after_run(['BCM Data Plot'])

    f_path = common.rename_stream_file(common.bcm_stream_file_name, '_BCM_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'BCM', 1, freq_hz)
    common.logging.info('BCM {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    common.watch_shell.do_lcfgBcmRead("0")
    if err_status:
        common.logging.error('*** BCM {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def bcm_dcb_repeatability_test():
    """
    Repeat the DCB Stream test for 10 iteration
    :return:
    """
    for _ in range(10):
        bcm_dcb_stream_test()


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    ad5940_switch_test()
    common.close_setup()
