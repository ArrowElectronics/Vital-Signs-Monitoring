import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils

capture_time = 120


def adxl362_dev_id_test():
    """

    :return:
    """
    qa_utils.dev_id_test('ADXL362')


def adxl_self_test():
    """

    :return:
    """
    packet = common.watch_shell.do_adxl_self_test('')

    if packet["payload"]["status"].value[0] != 0:
        common.test_logger.error('*** ADXL Self Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'ADXL Self Test returned failure!')
    else:
        common.test_logger.info('*** ADXL Self Test - PASS ***')


def adxl_dcb_test():
    qa_utils.dcb_test(dev='ADXL362', dcb_file='adxl_dcb.dcfg', dcb_read_file='adxl_dcb_get.dcfg',
                      test_name='ADXL DCB Test')


def adxl_dcb_stream_test(freq_hz=50):
    """

    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'adxl')

    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')
    common.watch_shell.quick_start('adxl', 'adxl')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.dcb_cfg('d', 'adxl')
    f_path = common.rename_stream_file(common.adxl_stream_file_name, '{}Hz.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adxl', 1, freq_hz)
    common.test_logger.info('ADXL {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ADXL {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def adxl_dcb_repeatability_test():
    loop_count = 10
    for i in range(loop_count):
        adxl_dcb_test()
        adxl_dcb_stream_test()


def adxl_stream_test(freq_hz=100):
    """

    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'adxl')

    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')
    common.quick_start_adxl(samp_freq_hz=freq_hz)
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.dcb_cfg('d', 'adxl')
    f_path = common.rename_stream_file(common.adxl_stream_file_name, '{}Hz.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adxl', 1, freq_hz)
    common.test_logger.info('ADXL {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ADXL {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def adxl_freq_sweep_test():
    """
    Sweep ADXL across all valid frequencies
    :return:
    """
    freq = [12.5, 25, 50, 100, 200, 400]
    for freq_hz in freq:
        adxl_stream_test(freq_hz=freq_hz)


def adxl_repeatability_test():
    """
    Repeat the adxl_freq_sweep_test for 5 times
    :return:
    """

    for _ in range(5):
        adxl_freq_sweep_test()


def adxl_fs_stream_test(freq_hz=50):
    """

    :return:
    """
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')
    qa_utils.clear_fs_logs('ADXL')

    common.watch_shell.quick_start("adxl", "adxl", fs=True)
    common.set_adxl_stream_freq(freq_hz)
    common.watch_shell.do_start_logging("")
    time.sleep(capture_time)
    common.watch_shell.quick_stop("adxl", "adxl", fs=True)
    common.watch_shell.do_stop_logging("")
    log_file_name, csv_file_name = qa_utils.get_fs_log('ADXL')
    f_path = common.rename_stream_file(csv_file_name["adxl"], "_adxl_{}hz_fs_stream_test.csv".format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adxl', 1, freq_hz, True)
    common.test_logger.info('ADXL FS {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** ADXL FS {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def adxl_fs_freq_sweep_test():
    """
    Sweep ADXL across all valid frequencies
    :return:
    """
    freq = [12.5, 25, 50, 100, 200, 400]
    for freq_hz in freq:
        adxl_fs_stream_test(freq_hz=freq_hz)


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    adxl362_dev_id_test()
    common.close_setup()
