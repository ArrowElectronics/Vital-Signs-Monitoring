import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils


def temperature_stream_test(freq_hz=1):
    """
    Capture EDA data for 30s, check the data for frequency(default 1Hz) mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'adpd4000')
    common.watch_shell.do_loadAdpdCfg("40")

    # qa_utils.write_dcb('adpd4000', 'temp_dcb.dcfg', 'EDA Stream Test')
    common.watch_shell.do_quickstart("temperature")
    common.watch_shell.do_plot("rtemperature")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('temperature')
    common.close_plot_after_run(['Temperature Data Plot'])
    # common.dcb_cfg('d', 'adpd4000')
    f_path = common.rename_stream_file(common.temperature_stream_file_name, '_temperature_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'temperature', 1, freq_hz)
    common.logging.info('Temperature {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** Temperature {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def temperature_fs_stream_test(freq_hz=1):
    """
    Capture EDA data for 30s, check the data for frequency(default 1Hz) mismatch
    :param freq_hz: Frequency in which the Sampling should happen(HZ)
    :return:
    """
    capture_time = 30
    common.dcb_cfg('d', 'adpd4000')
    common.watch_shell.do_loadAdpdCfg("40")
    qa_utils.clear_fs_logs('ADPD')

    common.watch_shell.do_quickstart("start_log_temperature")
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('stop_log_temperature')
    log_file_name, csv_file_name = qa_utils.get_fs_log('temperature')

    f_path = common.rename_stream_file(csv_file_name["temperature"], '_temperature_stream{}hz_test.csv'.format(freq_hz))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'temperature', 1, freq_hz, True)
    common.logging.info('Temperature FS {}Hz Stream Test Results: {}'.format(freq_hz, results_dict))
    if err_status:
        common.logging.error('*** Temperature FS {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    temperature_stream_test()
    common.close_setup()