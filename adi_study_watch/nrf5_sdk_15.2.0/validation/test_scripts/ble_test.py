import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def exec_ping(usecase_id, ping_count, bytes_per_sec, print_pong=False):
    """
    This is a sub function used by the BLE_Ping test case which executes the ping test
    :param usecase_id:
    :param ping_count:
    :param bytes_per_sec:
    :param print_pong:
    :return:
    """
    pkt_size = int((bytes_per_sec * 20) / 1000)
    common.test_logger.info('Usecase-{}: Ping test for Throughput: {} bytes/sec'.format(usecase_id, bytes_per_sec))
    ping_results_dict = common.watch_shell.do_ping('{} {}'.format(ping_count, pkt_size))
    if ping_results_dict['missed_packets']:
        common.test_logger.error('Usecase-{}: Ping test Failed! {} Packets missed'.format
                             (usecase_id, len(ping_results_dict['missed_packets'])))
        # raise ConditionCheckFailure('Ping test Failed! {} Packets missed'.format(
        #     len(ping_results_dict['missed_pkt_seq_num'])))
        err_stat = 1
    else:
        common.test_logger.info('Usecase-{}: Ping Test Measured Throughput: {} bytes/sec'.format(usecase_id,
                                                                                   ping_results_dict['throughput']))
        err_stat = 0
    return err_stat


def ble_ping_test():
    """
    Test Case for testing various usecases ping test
    :return:
    """
    ping_count = 1000
    err_list = []

    # USECASE 1.	PPG Specific: PPG@500Hz, ADXL@50Hz, Temp@1Hz
    bytes_per_sec = 3713
    err_stat = exec_ping(1, ping_count, bytes_per_sec, False)
    err_list.append(err_stat)

    # USECASE 2.	PPG+ EDA: PPG@100Hz, ADXL@50Hz, EDA@30Hz, Temp@1Hz
    bytes_per_sec = 1504
    err_stat = exec_ping(2, ping_count, bytes_per_sec, False)
    err_list.append(err_stat)

    # USECASE 3.	PPG+ECG: PPG@100Hz, ADXL@50Hz, ECG@250Hz, Temp@1Hz
    bytes_per_sec = 2585
    err_stat = exec_ping(3, ping_count, bytes_per_sec, False)
    err_list.append(err_stat)

    # USECASE 4.	ECG: PPG@50Hz, ADXL@50Hz, ECG@1000Hz, Temp@1Hz
    bytes_per_sec = 6429
    err_stat = exec_ping(4, ping_count, bytes_per_sec, False)
    err_list.append(err_stat)

    if 1 in err_list:
        err_usecase_list = [str(i+1) for i, err in enumerate(err_list) if err]
        raise ConditionCheckFailure('Ping test Failed due to packet loss! Failed Usecases: {}'.format(
            ', '.join(err_usecase_list)))


def ble_reliability_test():
    """
    Perform Ping test at incremental distances with max usecase rate
    :return:
    """
    ping_count = 1000
    bytes_per_sec = 6429
    common.messagebox.showinfo('BLE Reliability Test', 'Increase the BLE - Watch distance and press OK!')
    err_stat = exec_ping(4, ping_count, bytes_per_sec, False)

    if err_stat:
        raise ConditionCheckFailure('Ping test Failed due to packet loss!')


def ble_range_reliability_test():
    """
    This test verifies the ble reliability at different ranges. The streaming is done and packet loss is checked
    from varying distance ranges.
    :return:
    """
    pkt_loss = False
    meas_check.clear_pkt_loss()
    wait_time_for_move = 10
    capture_time = 10
    range_mtr_list = [1, 2, 3, 4, 5, 6, 7, 8]
    for range_mtr in range_mtr_list:
        s_str = 's' if range_mtr > 1 else ''
        common.easygui.msgbox('Press OK and Move back by {} meter{}!'.format(range_mtr, s_str), 'Move Back')
        time.sleep(wait_time_for_move)
        common.watch_shell.quick_start('ecg', 'ecg')
        time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
        common.watch_shell.quick_stop('ecg', 'ecg')
        f_path = common.rename_stream_file(common.ecg_stream_file_name, '_BLE_range_{}m.csv'.format(range_mtr))
        f_path = os.path.abspath(f_path)
        pkt_loss, failure_streams = meas_check.check_pkt_loss()
        sample_loss, sample_loss_idx_list = meas_check.check_sample_loss(f_path, stream='ecg')
        if pkt_loss:
            common.test_logger.error('*** BLE Range Reliability Test - FAIL ***')
            raise ConditionCheckFailure("\n\n" + 'Packet Loss observed at {} meter range'.format(range_mtr))
        if sample_loss:
            common.test_logger.error('*** BLE Range Reliability Test - FAIL ***')
            raise ConditionCheckFailure("\n\n" + '{} Samples lost at {} meter range!\n'.format(len(sample_loss_idx_list), range_mtr))
    if not pkt_loss or not sample_loss:
        common.test_logger.info('*** BLE Range Reliability Test - PASS ***')


def ble_connection_reliability_test():
    """

    :return:
    """
    capture_time = 10
    repeat_count = 10
    #TODO: Figuring out a way to disconnect the watch port programmatically


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
