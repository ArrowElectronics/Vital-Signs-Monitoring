import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from datetime import datetime
from common import ConditionCheckFailure
from utils import meas_check, qa_utils


def fw_version_test():
    """

    :return:
    """
    err_stat, fw_ver_info_dict = common.watch_shell.do_getVersion('')
    # date_str = datetime.now().strftime("%Y-%m-%d")
    if not err_stat:
        ver_info_str = 'Firmware Version: V{}.{}.{}  |  Build Date: {}'.format(fw_ver_info_dict['major'],
                                                                               fw_ver_info_dict['minor'],
                                                                               fw_ver_info_dict['patch'],
                                                                               fw_ver_info_dict['date'])
        common.update_robot_suite_doc(ver_info_str)
        common.logging.info(ver_info_str)
    else:
        common.logging.error('*** Firmware Version Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'Error in reading firmware version!')


def date_time_test():
    """

    :return:
    """
    common.watch_shell.do_setDateTime('')
    err_stat, date_time_dict = common.watch_shell.do_getDateTime('')
    dt_str = '{}-{}-{} {}:{}:{}'.format(date_time_dict['year'], date_time_dict['month'], date_time_dict['day'],
                                        date_time_dict['hour'], date_time_dict['min'], date_time_dict['sec'])
    common.logging.info('Watch Time: {}'.format(dt_str))
    dt_watch = datetime.strptime(dt_str, "%Y-%m-%d %H:%M:%S")
    dt_now = datetime.now()
    delta_dt = dt_now - dt_watch
    if not delta_dt.seconds < 10:
        common.logging.error('*** Date & Time Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'Set date and time does not match with the current date and time!')


def flash_dev_id_test():
    """
    Read Device ID and verify if it is as expected
    :return:
    """
    qa_utils.dev_id_test('NAND_FLASH')


def fs_download_repeatability_test():
    capture_time = 10
    repeate_count = 30
    adpd_led_dcb = 'adpd_g_dcb.dcfg'
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'FS Download Test')
    common.watch_shell.do_loadAdpdCfg("40")
    common.config_adpd_stream(samp_freq_hz=50, agc_state=1, led='G', skip_load_cfg=True)
    qa_utils.clear_fs_logs('ADPD')
    common.watch_shell.do_quickstart('start_log_adpd4000_g')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('stop_log_adpd4000_g')
    common.dcb_cfg('d', 'adpd4000')
    curr_dir = os.getcwd()
    for i in range(repeate_count):
        log_file_name, csv_file_name = qa_utils.get_fs_log('ADPD')
        log_file_path = os.path.join(curr_dir, log_file_name)

    # check_status, csv_file_path = qa_utils.convert_and_verify_logs(log_file_name)
    # TODO: Convert LOG file to CSV and parse data


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    fw_version_test()
    date_time_test()
    common.close_setup()
