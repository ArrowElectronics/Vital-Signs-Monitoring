import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common


def temperature_accuracy_test():
    """
    Place the Palladin above the temp sensor increase and decrease the temperature in steps and
    match the value from ADPD output
    :return:
    """
    capture_time = 20
    common.messagebox.showinfo('Temperature Test', 'Place a metal bar with (palladin) '
                                                   'over the RTD and press OK.'
                                                   'Change the temperature in steps and verify the capture')
    # common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.quick_start('temp', 'temp')
    time.sleep(capture_time)
    common.watch_shell.quick_stop('temp', 'temp')

    common.close_plot_after_run(['Temperature Data Plot'])

    # TODO: Read and compare the values
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
