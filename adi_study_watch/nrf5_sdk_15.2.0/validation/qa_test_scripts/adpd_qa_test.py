import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import qa_utils, meas_check


# Module Variables **********************
stream_file_name = 'adpd6Stream.csv'
capture_time = 60
data_dict = {"G": [], "R": [], "IR": [], "B": []}


# ***************************************

def adpd_dev_id_test():
    """
    Read Device ID and verify if it is as expected
    :return:
    """
    qa_utils.dev_id_test('ADPD4K')


def adpd_multi_led_stream_test(freq_hz=50, agc_state=0):
    """

    :param freq_hz:
    :param agc_state:
    :return:
    """
    capture_time = 30
    led_list = ['G', 'R', 'IR', 'B']
    mean_dict = {}
    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_multi_led_dcb.dcfg', 'ADPD Multi LED Stream Test')
    common.watch_shell.do_load_adpd_cfg("5")

    # Multi LED Stream Test
    multi_stream_file_list = []
    led_stream_file_dict = qa_utils.quick_start_adpd_multi_led(samp_freq_hz=freq_hz, agc_state=agc_state, skip_load_cfg=True)
    for led in led_list:
        multi_stream_file_list.append(led_stream_file_dict[led])
    time.sleep(capture_time)
    for led in led_list:
        common.quick_stop_adpd(led)
    for i, led in enumerate(led_list):
        f_name = multi_stream_file_list[i]
        # CH1 Data Check
        f_path = common.rename_stream_file(f_name,
                                           led + "_{}Hz_agc{}_adpd_multi_led_stream_test.csv".format(freq_hz, agc_state))
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 1, freq_hz)
        common.test_logger.info('ADPD CH1 MultiStream Test - LED: {} | Freq: {}'.format(led, freq_hz))
        common.test_logger.info('ADPD CH1 MultiStream Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH1 MultiStream Test - FAIL ***'.format(freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
        # CH2 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 2, freq_hz)
        common.test_logger.info('ADPD CH2 MultiStream Test - LED: {} | Freq: {}'.format(led, freq_hz))
        common.test_logger.info('ADPD CH2 MultiStream Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH2 MultiStream Test - FAIL ***'.format(freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
        # Mean Calculation
        ch1_mean = meas_check.calc_mean(f_path, 3, row_offset=5)
        ch2_mean = meas_check.calc_mean(f_path, 5, row_offset=5)
        common.test_logger.info('ADPD MultiStream Mean | LED:{} |AGC:{} | CH1:{} | CH2:{}'.format(led,
                                                                                              agc_state,
                                                                                              ch1_mean,
                                                                                              ch2_mean))
        mean_dict['MultiStream_' + led] = (ch1_mean, ch2_mean)
    return mean_dict


def adpd_stream_test(freq_hz=50, agc_state=0):
    """

    :param freq_hz:
    :return:
    """
    capture_time = 30
    led_list = ['G', 'R', 'IR', 'B']
    mean_dict = {}

    # Single LED Stream Tests
    for led in led_list:
        common.dcb_cfg('d', 'adpd')
        # adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
        adpd_led_dcb = "uc_5_dcb.dcfg"
        qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
        f_name = common.quick_start_adpd(samp_freq_hz=freq_hz, agc_state=agc_state, led=led, skip_load_cfg=False)
        time.sleep(capture_time)
        common.quick_stop_adpd(led)

        f_path = common.rename_stream_file(f_name, led + "_{}Hz_agc{}_adpd_stream_test.csv".format(freq_hz, agc_state))

        # CH1 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 1, freq_hz)
        common.test_logger.info('ADPD CH1 Stream Test - LED: {} | Freq: {}'.format(led, freq_hz))
        common.test_logger.info('ADPD CH1 Stream Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH1 Stream Test - FAIL ***'.format(freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

        # CH2 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 2, freq_hz)
        common.test_logger.info('ADPD CH2 Stream Test - LED: {} | Freq: {}'.format(led, freq_hz))
        common.test_logger.info('ADPD CH2 Stream Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH2 Stream Test - FAIL ***'.format(freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

        # Mean Calculation
        ch1_mean = meas_check.calc_mean(f_path, 3, row_offset=5)
        ch2_mean = meas_check.calc_mean(f_path, 5, row_offset=5)
        common.test_logger.info('ADPD Stream Mean | LED:{} |AGC:{} | CH1:{} | CH2:{}'.format(led,
                                                                                         agc_state,
                                                                                         ch1_mean,
                                                                                         ch2_mean))
        mean_dict[led] = (ch1_mean, ch2_mean)
    return mean_dict


def adpd_fs_stream_test(freq_hz=50, agc_state=0):
    """

    :param freq_hz:
    :return:
    """
    led_list = ['G', 'R', 'IR', 'B']
    mean_dict = {}
    for i, led in enumerate(led_list):
        common.dcb_cfg('d', 'adpd')
        # adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
        adpd_led_dcb = "uc_5_dcb.dcfg"
        qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
        qa_utils.clear_fs_logs('ADPD')

        common.quick_start_adpd_fs(samp_freq_hz=freq_hz, agc_state=agc_state, led=led, skip_load_cfg=False)
        time.sleep(capture_time)
        common.quick_stop_adpd_fs(led)
        log_file_name, csv_file_name = qa_utils.get_fs_log('ADPD')
        adpd_csv_file = common.rename_stream_file(csv_file_name["adpd"], "adpd_fs_stream.csv")
        err_status, err_str, results_dict = qa_utils.check_stream_data(adpd_csv_file, 'adpd_combined', 1,
                                                                       freq_hz, True)
        common.test_logger.info('ADPD FS {}Hz with agc {} Stream Test Results: {}'.format(freq_hz, agc_state, results_dict))
        if err_status:
            common.test_logger.error('*** ADPD FS Stream Test - FAIL ***')
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

        # Mean Calculation
        ch1_mean = meas_check.calc_mean(adpd_csv_file, 3, row_offset=5)
        ch2_mean = meas_check.calc_mean(adpd_csv_file, 5, row_offset=5)
        common.test_logger.info('ADPD Stream Mean | LED:{} |AGC:{} | CH1:{} | CH2:{}'.format(led,
                                                                                             agc_state,
                                                                                             ch1_mean,
                                                                                             ch2_mean))


def adpd_fs_stream_download_repeatiablity_test(freq_hz=50, agc_state=0):
    """

    :param freq_hz:
    :return:
    """
    led_list = ['G', 'R', 'IR', 'B']
    mean_dict = {}
    for index in range(50):
        common.test_logger.info("{} th Iteration".format(str(index)))
        for i, led in enumerate(led_list):
            adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
            qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
            common.watch_shell.do_load_adpd_cfg("1")
            qa_utils.clear_fs_logs('ADPD')

            common.quick_start_adpd_fs(samp_freq_hz=freq_hz, agc_state=agc_state, led=led, skip_load_cfg=True)
            time.sleep(capture_time)
            common.quick_stop_adpd_fs(led)
            log_file_name, csv_file_name = qa_utils.get_fs_log('ADPD')
            adpd_csv_file = common.rename_stream_file(csv_file_name["adpd"], "adpd_fs_stream_download_repeatiability.csv")
            err_status, err_str, results_dict = qa_utils.check_stream_data(adpd_csv_file, 'adpd_combained', 1,
                                                                           freq_hz, True)
            common.test_logger.info(
                'ADPD FS {}Hz with agc {} Stream Test Results: {}'.format(freq_hz, agc_state, results_dict))
            if err_status:
                common.test_logger.error('*** ADPD FS Stream and download repeatiablity Test - FAIL ***')
                raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def adpd_fs_download_repeatiablity_test(freq_hz=50, agc_state=0):
    """

    :param freq_hz:
    :return:
    """
    led_list = ['G', 'R', 'IR', 'B']
    mean_dict = {}
    for i, led in enumerate(led_list):
        adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
        qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
        common.watch_shell.do_load_adpd_cfg("1")
        qa_utils.clear_fs_logs('ADPD')

        common.quick_start_adpd_fs(samp_freq_hz=freq_hz, agc_state=agc_state, led=led, skip_load_cfg=True)
        time.sleep(capture_time)
        common.quick_stop_adpd_fs(led)
        for index in range(50):
            common.test_logger.info("{} th Iteration".format(str(index)))
            log_file_name, csv_file_name = qa_utils.get_fs_log('ADPD')
            adpd_csv_file = common.rename_stream_file(csv_file_name["adpd"], "adpd_fs_download_repeatiability.csv")
            err_status, err_str, results_dict = qa_utils.check_stream_data(adpd_csv_file, 'adpd_combained', 1,
                                                                           freq_hz, True)
            common.test_logger.info(
                'ADPD FS {}Hz with agc {} Stream Test Results: {}'.format(freq_hz, agc_state, results_dict))
            if err_status:
                common.test_logger.error('*** ADPD FS download repeatiability Test - FAIL ***')
                raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


def adpd_agc_test(freq_hz=50):
    """

    :param freq_hz:
    :return:
    """
    agc_values = {
        "G": {"agc_off_min_ch1": 87992, "agc_off_max_ch1": 97255, "agc_off_min_ch2": 27971, "agc_off_max_ch2": 30916,
              "agc_on_min_ch1": 314572, "agc_on_min_ch2": 235929, "agc_on_max": 471859},
        "R": {"agc_off_min_ch1": 51034, "agc_off_max_ch1": 56406, "agc_off_min_ch2": 20434, "agc_off_max_ch2": 22585,
              "agc_on_min_ch1": 314572, "agc_on_min_ch2": 235929, "agc_on_max": 471859},
        "IR": {"agc_off_min_ch1": 40021, "agc_off_max_ch1": 44234, "agc_off_min_ch2": 23962, "agc_off_max_ch2": 26185,
               "agc_on_min_ch1": 314572, "agc_on_min_ch2": 235929, "agc_on_max": 471859},
        "B": {"agc_off_min_ch1": 44759, "agc_off_max_ch1": 54706, "agc_off_min_ch2": 16701, "agc_off_max_ch2": 18459,
              "agc_on_min_ch1": 314572, "agc_on_min_ch2": 235929, "agc_on_max": 471859},
        "ppg": {"agc_off_min": 256324, "agc_off_max": 283305, "agc_on_min": 314572, "agc_on_max": 471859}
    }
    agc_off_results_dict = adpd_stream_test(freq_hz, 0)
    agc_on_results_dict = adpd_stream_test(freq_hz, 1)
    err_led_list = []
    err_stat = False
    for k in agc_on_results_dict.keys():
        if(agc_values[k]["agc_off_min_ch1"] < agc_off_results_dict[k][0] < agc_values[k]["agc_off_max_ch1"] and
                agc_values[k]["agc_off_min_ch2"] < agc_off_results_dict[k][1] < agc_values[k]["agc_off_max_ch2"] and
                agc_values[k]["agc_on_min_ch1"] < agc_on_results_dict[k][0] < agc_values[k]["agc_on_max"] and
                agc_values[k]["agc_on_min_ch2"] < agc_on_results_dict[k][1] < agc_values[k]["agc_on_max"]):
                pass
        else:
            err_led_list.append(k)
            err_stat = True
    if err_stat:
        common.test_logger.error('*** ADPD AGC Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'AGC state change is not reflecting '
                                             'on the streams from LEDs - {}'.format(err_led_list))


def adpd_freq_sweep_stream_test():
    """

    :return:
    """
    freq_hz_list = [50, 100, 500]
    for freq_hz in freq_hz_list:
        adpd_stream_test(freq_hz)


def adpd_freq_sweep_fs_stream_test():
    """

    :return:
    """
    freq_hz_list = [50, 100, 500]
    for freq_hz in freq_hz_list:
        adpd_fs_stream_test(freq_hz)


def adpd_freq_sweep_agc_test():
    """

    :return:
    """
    freq_hz_list = [50, 100, 500]
    for freq_hz in freq_hz_list:
        adpd_agc_test(freq_hz)




def adpd_freq_sweep_repeatability_test():
    """
    Repeat the Freq sweep test for 10 iterations
    :return:
    """
    for _ in range(10):
        adpd_freq_sweep_stream_test()


def adpd_dcb_test():
    """

    :return:
    """
    qa_utils.dcb_test(dev='ADPD4K', dcb_file='adpd_qa_dcb.dcfg', dcb_read_file='adpd_dcb_get.dcfg',
                      test_name='ADPD DCB Test')


def adpd_dcb_stream_test():
    """

    :return:
    """
    capture_time = 10
    common.dcb_cfg('d', 'adpd')  # Deleting any previous DCB
    qa_utils.write_dcb('adpd', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_load_adpd_cfg("1")

    err_stat, dcb_dir = common.dcb_cfg('r', 'adpd')
    if err_stat:
        common.test_logger.error('*** ADPD DCB Stream Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'DCB Read failed!')
    write_dcb_file = os.path.join(dcb_dir, 'adpd_qa_dcb.dcfg')
    read_dcb_file = os.path.join(dcb_dir, 'adpd_dcb_get.dcfg')
    err_stat, err_str = qa_utils.compare_dcb_files(write_dcb_file, read_dcb_file)
    if err_stat:
        common.test_logger.error('*** ADPD DCB Stream Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'Read DCB does not match with the write DCB!\nDetails:{}'.format(err_str))

    f_name = common.quick_start_adpd(samp_freq_hz=None, agc_state=0, led='G', skip_load_cfg=True)
    time.sleep(capture_time)
    common.quick_stop_adpd('G')
    err_stat, dcb_dir = common.dcb_cfg('d', 'adpd')
    if err_stat:
        common.test_logger.error('*** ADPD DCB Stream Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'DCB Delete failed!')
    adpd_stream_file = os.path.abspath(f_name)
    f_path = common.rename_stream_file(adpd_stream_file, "_50Hz_adpd_dcb_stream_test.csv")

    # Results check for Ch1
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 1, 50)
    if err_status:
        common.test_logger.error('*** ADPD DCB CH1 Stream Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
    else:
        common.test_logger.info('ADPD DCB CH1 Stream Test Results: {}'.format(results_dict))
    # Results check for Ch2
    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'adpd', 2, 50)
    if err_status:
        common.test_logger.error('*** ADPD DCB CH2 Stream Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
    else:
        common.test_logger.info('ADPD DCB CH2 Stream Test Results: {}'.format(results_dict))


def adpd_dcb_stream_repeatability_test():
    """
    Perform both dcb sanity test and stream test in a loop
    :return:
    """
    repeat_count = 10
    for i in range(repeat_count):
        adpd_dcb_test()
    for i in range(repeat_count):
        adpd_dcb_stream_test()


def ppg_stream_test(agc_state=0):
    """
    Capture PPG data for 30s, check the data for frequency(default 50Hz) frequency mismatch
    :return:
    """
    capture_time = 30
    freq_hz = 50
    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'ppg')

    common.quick_start_ppg(samp_freq_hz=freq_hz, agc_state=agc_state)
    time.sleep(capture_time)
    common.quick_stop_ppg()
    common.rename_stream_file(common.ppg_stream_file_name, "_ppg_{}hz_agc_{}_stream_test.csv".format(freq_hz, agc_state))
    f_path = common.rename_stream_file(common.syncppg_stream_file_name,
                                       "_syncppg_{}hz_agc_{}_stream_test.csv".format(freq_hz, agc_state))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'ppg', 1, freq_hz)
    common.test_logger.info('PPG {}Hz with agc {} Stream Test Results: {}'.format(freq_hz, agc_state, results_dict))
    if err_status:
        common.test_logger.error('***PPG {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    mean = meas_check.calc_mean(f_path, row_offset=3)
    common.test_logger.info('PPG Stream | AGC:{} | Mean:{}'.format(agc_state, mean))
    return mean


def ppg_agc_test():
    """
    Capture PPG with and without AGC the mean of AGC on capture should higher
    :return:
    """
    # TODO verify this function -- When running PPG stream send time plot throws error but executes
    freq_hz = 50
    agc_values = {
        "ppg": {"agc_off_min": 256324, "agc_off_max": 283305, "agc_on_min": 314572, "agc_on_max": 471859}
    }

    mean_agc_on = ppg_stream_test(1)
    mean_agc_off = ppg_stream_test(0)

    if(agc_values["ppg"]["agc_off_min"] <= mean_agc_off <= agc_values["ppg"]["agc_off_max"] and
            agc_values["ppg"]["agc_on_min"] <= mean_agc_on <= agc_values["ppg"]["agc_on_max"]):
        common.test_logger.info('{}Hz PPG AGC Test Pass'.format(freq_hz))
    else:
        common.test_logger.error('***PPG {}Hz AGC Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + 'AGC state change is not reflecting on the streams')


def ppg_fs_stream_test(agc_state=0):
    """
    Capture PPG data for 30s, check the data for frequency(default 50Hz) frequency mismatch
    :return:
    """
    freq_hz = 50
    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'ppg')
    qa_utils.clear_fs_logs('ADPD')

    common.watch_shell.quick_start("ppg", "ppg", fs=True)
    common.watch_shell.do_start_logging("")
    time.sleep(capture_time)
    common.watch_shell.quick_stop("ppg", "ppg", fs=True)
    common.watch_shell.do_stop_logging("")
    log_file_name, csv_file_name = qa_utils.get_fs_log('ppg')

    common.rename_stream_file(csv_file_name["ppg"], "_ppg_{}hz_agc_{}_stream_test.csv".format(freq_hz, agc_state))
    f_path = common.rename_stream_file(csv_file_name["syncppg"], "_syncppg_{}hz_agc_{}_stream_test.csv".format(freq_hz, agc_state))

    err_status, err_str, results_dict = qa_utils.check_stream_data(f_path, 'syncppg', 1, freq_hz, True)
    common.test_logger.info('PPG FS {}Hz with agc {} Stream Test Results: {}'.format(freq_hz, agc_state, results_dict))
    if err_status:
        common.test_logger.error('***PPG FS {}Hz Stream Test - FAIL ***'.format(freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    # adpd_dev_id_test()
    # adpd_dcb_test()
    common.close_setup()
