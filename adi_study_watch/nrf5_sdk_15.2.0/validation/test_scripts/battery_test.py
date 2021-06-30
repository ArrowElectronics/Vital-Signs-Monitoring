import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from utils import meas_check


def setup_battery_voltage():
    common.sm.setup_battery_voltage()
    common.messagebox.showinfo('Battery Test', 'Remove the USB connector and click OK')


def ble_advertising_stage_current_consumption_test():
    """

    :return:
    """
    local_file_name = 'ble_advertising_curr.csv'
    common.sm.setup_battery_mode()
    common.sm.cfg_setup_buff(10)
    ret = common.sm.measure_current()
    time.sleep(1)
    with open(local_file_name, 'w') as f_ref:
        for i in ret.split(','):
            f_ref.write(i.strip('+')+'\n')
            time.sleep(1)
    f_path = common.rename_stream_file(local_file_name, '_consumption.csv', 0, 0)  # step copies report to shared drive
    # TODO: Update Current Threshold below
    check_pass, avg_curr, failure_curr = meas_check.check_battery_charge(f_path, curr_threshold=17e-3)
    if not check_pass:
        common.test_logger.error('*** BLE Advertising Stage Current Consumption Test - FAIL ***')
        raise common.ConditionCheckFailure('\n\nFailure Current: {} | Average Current: {}'.format(failure_curr,
                                                                                                  avg_curr))
    else:
        common.test_logger.info('Average BLE Advertising Stage Current: {}'.format(avg_curr))


def shipment_mode_current_consumption_test():
    """

    :return:
    """
    local_file_name = 'shipment_mode_curr.csv'
    common.sm.setup_battery_mode()
    common.sm.cfg_setup_buff(10)
    common.watch_shell.do_set_power_mode('3')
    ret = common.sm.measure_current()
    time.sleep(1)
    with open(local_file_name, 'w') as f_ref:
        for i in ret.split(','):
            f_ref.write(i.strip('+')+'\n')
            time.sleep(1)
    f_path = common.rename_stream_file(local_file_name, '_consumption.csv', 0, 0)  # step copies report to shared drive
    # TODO: Update Current Threshold below
    check_pass, avg_curr, failure_curr = meas_check.check_battery_charge(f_path, curr_threshold=17e-3)
    if not check_pass:
        common.test_logger.error('*** Shipment Mode Current Consumption Test - FAIL ***')
        raise common.ConditionCheckFailure('\n\nFailure Current: {} | Average Current: {}'.format(failure_curr,
                                                                                                  avg_curr))
    else:
        common.test_logger.info('Average Shipment Mode Current: {}'.format(avg_curr))


def streaming_stage_current_consumption_test():
    """

    :return:
    """
    local_file_name = 'streaming_stage_curr.csv'
    common.sm.setup_battery_mode()
    # ***** Starting ECG, ADXL and PPG stream ******
    common.watch_shell.quick_start('ecg', 'ecg')
    common.watch_shell.quick_start('adxl', 'adxl')
    # PPG capture - start
    common.watch_shell.quick_start('adpd', 'adpd6')
    # **********************************************
    time.sleep(5)
    common.sm.cfg_setup_buff(10)
    #time.sleep(10)
    #common.sm.cfg_read_buff()
    #common.sm.cfg_output_state(0)
    ret = common.sm.measure_current()
    #time.sleep(1)
    with open(local_file_name, 'w') as f_ref:
        for i in ret.split(','):
    #        err, msg = common.sm.cfg_get_data()
            f_ref.write(i.strip('+')+'\n')
            time.sleep(1)
    f_path = common.rename_stream_file(local_file_name, '_consumption.csv', 0, 0)  # step copies report to shared drive
    #print ret
    #common.messagebox.showinfo('Streaming Current Value', 'Current Value = %f press OK!' % ret)
    common.watch_shell.quick_stop('ecg', 'ecg')
    common.watch_shell.quick_stop('adxl', 'adxl')
    common.watch_shell.quick_stop('adpd', 'adpd6')
    #common.sm.cfg_output_state(0)
    # TODO: Update Current Threshold below
    check_pass, avg_curr, failure_curr = meas_check.check_battery_charge(f_path, curr_threshold=17e-3)
    if not check_pass:
        common.test_logger.error('*** Streaming Stage Current Consumption Test - FAIL ***')
        raise common.ConditionCheckFailure('\n\nFailure Current: {} | Average Current: {}'.format(failure_curr,
                                                                                                  avg_curr))
    else:
        common.test_logger.info('Average Streaming Stage Current: {}'.format(avg_curr))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
