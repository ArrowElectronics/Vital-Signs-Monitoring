import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def form_reg_write_str(reg_list):
    """

    :param reg_list:
    :return:
    """
    write_reg_str_list = []
    read_reg_str_list = []
    for reg_addr, reg_val in reg_list:
        write_reg_str_list.append('{}:{}'.format(reg_addr, reg_val))
        read_reg_str_list.append(str(reg_addr))
    return ', '.join(write_reg_str_list), ' '.join(read_reg_str_list)


def check_reg_val(write_reg_list, read_reg_list):
    reg_val_mismatch = False
    if len(read_reg_list) == len(write_reg_list):
        i = 0
        for reg_addr, reg_val in write_reg_list:
            if reg_addr.lower() != read_reg_list[i][0].lower() or reg_val.lower() != read_reg_list[i][1].lower():
                reg_val_mismatch = True
                break
            i += 1
    else:
        reg_val_mismatch = True
    return reg_val_mismatch


def check_memory_access(dev_name, write_reg_list):
    """

    :param dev_name:
    :param write_reg_list:
    :return:
    """
    write_reg_str, read_reg_str = form_reg_write_str(write_reg_list)
    common.watch_shell.do_reg('w {} {}'.format(dev_name, write_reg_str))
    err_stat, read_reg_list = common.watch_shell.reg_read(dev_name, read_reg_str)
    if not err_stat:
        reg_mismatch = check_reg_val(write_reg_list, read_reg_list)
    else:
        reg_mismatch = True
    return reg_mismatch


def memory_access_test():
    """
    Perform Ping test at incremental distances with max usecase rate
    :return:
    """
    mismatch_dict = {'adpd4000': False, 'adxl': False}

    # ADPD memory access
    write_reg_list = [('0x105', '0x2525')]
    mismatch_dict['adpd4000'] = check_memory_access('adpd', write_reg_list)

    # ADXL memory access
    write_reg_list = [('0x2D', '0x10')]
    mismatch_dict['adxl'] = check_memory_access('adxl', write_reg_list)

    if True in mismatch_dict.itervalues():
        failure_list = [k for k, v in mismatch_dict.items() if v]
        raise ConditionCheckFailure('Memory Access Test Failed! Failure Cases: {}'.format(', '.join(failure_list)))


def data_loss_test():
    """

    :return:
    """
    pkt_loss_file_path = os.path.join(os.getcwd(), 'pkt_loss.yaml')
    if os.path.isfile(pkt_loss_file_path):
        os.remove(pkt_loss_file_path)
    common.watch_shell.quick_start('ecg', 'ecg')
    common.watch_shell.quick_start('adxl', 'adxl')
    common.watch_shell.quick_start('ppg', 'ppg')
    common.watch_shell.quick_start('sync_ppg', 'sync_ppg')
    common.watch_shell.quick_start('temp', 'temp')

    time.sleep(10)

    common.watch_shell.quick_stop('ecg', 'ecg')
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('ppg', 'ppg')
    common.watch_shell.quick_stop('sync_ppg', 'sync_ppg')
    common.watch_shell.quick_stop('temp', 'temp')

    if os.path.isfile(pkt_loss_file_path):  # This file will be generated with results by CLI if pkt loss occurs
        with open(pkt_loss_file_path, 'r') as f_ref:
            pkt_loss_dict = common.yaml.load(f_ref, Loader=common.yaml.FullLoader)
        if True in pkt_loss_dict.itervalues():
            failure_list = [k[1:] for k, v in pkt_loss_dict.items() if v]
            raise ConditionCheckFailure('Data Loss Test Failed! Failure Streams: {}'.format(', '.join(failure_list)))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
