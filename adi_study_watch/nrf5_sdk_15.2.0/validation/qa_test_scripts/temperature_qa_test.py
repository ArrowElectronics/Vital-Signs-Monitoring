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


def temperature_stream_test(freq_hz=1):
    """
    Capture Temp data for 30s, check the data for frequency(default 1Hz) mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'adpd')

    qa_utils.write_dcb('adpd', 'adpd4000_dcb_temp.dcfg', 'Temperature Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")
    common.quick_start_temperature()
    time.sleep(capture_time)
    common.watch_shell.quick_stop('temp', 'temp')
    common.dcb_cfg('d', 'adpd')
    f_path = common.rename_stream_file(common.temperature_stream_file_name, '_temperature_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'temperature', 1, freq_hz)
    common.test_logger.info('Temperature {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** Temperature {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def temperature_fs_stream_test(freq_hz=1):
    """
    Capture EDA data for 30s, check the data for frequency(default 1Hz) mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    common.dcb_cfg('d', 'adpd')
    common.watch_shell.do_load_adpd_cfg("1")
    qa_utils.clear_fs_logs('ADPD')

    common.watch_shell.do_sensor('temp start')
    common.watch_shell.do_fs_sub('temp add')
    common.watch_shell.do_start_logging("")
    time.sleep(capture_time)
    common.watch_shell.do_fs_sub("temp remove")
    common.watch_shell.do_sensor("temp stop")
    common.watch_shell.do_stop_logging("")
    log_file_name, csv_file_name = qa_utils.get_fs_log('temperature')

    f_path = common.rename_stream_file(csv_file_name["temperature"],
                                       '_temperature_stream{}hz_fs_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'temperature', 1, freq_hz, True)
    common.test_logger.info('Temperature FS {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.test_logger.error('*** Temperature FS {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    temperature_stream_test()
    common.close_setup()