import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


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


def adxl_stream_test():
    """
    Run the ADXL362 self test
    :return:
    """
    capture_time = 10
    common.easygui.msgbox('Press OK and move the accelerometer for 10 seconds!', 'Move Accelerometer')
    common.watch_shell.quick_start('adxl', 'adxl')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('adxl', 'adxl')

    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
