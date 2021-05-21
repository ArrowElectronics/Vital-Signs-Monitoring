import time
import random
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check, qa_utils, rand_utils


def adxl_self_test():
    """

    :return:
    """
    err_stat = common.watch_shell.do_adxl_self_test('')

    if err_stat:
        common.logging.error('*** ADXL Self Test - FAIL ***')
        raise ConditionCheckFailure("\n\n" + 'ADXL Self Test returned failure!')
    else:
        common.logging.info('*** ADXL Self Test - PASS ***')


def use_case_qa_ppg(start_stream=("adxl", "temperature", "adpd"), stop_stream=("adxl", "temperature", "adpd")):
    """
    USE CASE - 1
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    adpd_freq_hz = 500
    adxl_freq_hz = 50
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_500hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    # common.preset_adpd_quick_start_values(samp_freq_hz=adpd_freq_hz, agc_state=1, led=led)
    # common.preset_adxl_quick_start_values(samp_freq_hz=adxl_freq_hz)

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.close_plot_after_run(['ADPD400x Data', 'ADXL Data', 'Temperature Data Plot'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')

    f_path_adpd = common.rename_stream_file(common.adpd_stream_file_name, "usecase1_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name, 'usecase1_{}Hz.csv'.format(adxl_freq_hz))
    f_path_temperature = common.rename_stream_file(common.temperature_stream_file_name,
                                                   'usecase1_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 1,
                                                                                           adpd_freq_hz)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                        adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.logging.info('ADPD CH-1 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-1 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.logging.info('Temperature {}Hz UseCase-1  Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-1 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-1 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_fs(start_stream=("adxl", "temperature", "adpd"), stop_stream=("adxl", "temperature", "adpd")):
    """
    USE CASE - 1
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    adpd_freq_hz = 500
    adxl_freq_hz = 50
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_500hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')
    qa_utils.clear_fs_logs('')

    # common.preset_adpd_quick_start_values(samp_freq_hz=adpd_freq_hz, agc_state=1, led=led)
    # common.preset_adxl_quick_start_values(samp_freq_hz=adxl_freq_hz)

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_fs_log("start")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_fs_log("stop")

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-1')
    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase1_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase1_{}Hz.csv'.format(adxl_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase1_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combained', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                        adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.logging.info('ADPD CH-1 {}-LED {}Hz UseCase-1 FS Test Results: {}'.format(led, adpd_freq_hz,
                                                                                     results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-1 FS Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.logging.info('Temperature {}Hz UseCase-1 FS Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}-LED {}Hz UseCase-1 FS Test - FAIL ***'.format(led, adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-1 FS Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz FS UseCase-1 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_eda(start_stream=("adxl", "adpd", "temperature", "eda"),
                        stop_stream=("adxl", "adpd", "temperature", "eda")):
    """
    Use Case - 2
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    eda_freq_hz = 30
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'eda')

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)
    common.set_eda_stream_freq(samp_freq_hz=eda_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.close_plot_after_run(['ADPD400x Data','ADXL Data', 'EDA Data Plot', 'Temperature Data Plot'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'eda')

    f_path_adpd = common.rename_stream_file(common.adpd_stream_file_name, "usecase2_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name, 'usecase2_{}Hz.csv'.format(adxl_freq_hz))
    f_path_eda = common.rename_stream_file(common.eda_stream_file_name, 'usecase2_{}hz.csv'.format(eda_freq_hz))
    f_path_temperature = common.rename_stream_file(common.temperature_stream_file_name,
                                                   'usecase2_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 1,
                                                                                              adpd_freq_hz)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    err_status_eda, err_str_eda, results_dict_eda = qa_utils.check_stream_data(f_path_eda, 'eda', 1, eda_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.logging.info('ADPD CH-1 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
                                                                                  results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
    #                                                                               results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-2 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.logging.info('EDA {}Hz UseCase-2 Test Results: {}'.format(eda_freq_hz, results_dict_eda))
    common.logging.info('Temperature {}Hz UseCase-2  Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-2 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_eda:
        common.logging.error('*** EDA {}Hz UseCase-2 Test - FAIL ***'.format(eda_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_eda))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-2 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_eda_fs(start_stream=("adxl", "adpd", "temperature", "eda"),
                           stop_stream=("adxl", "adpd", "temperature", "eda")):
    """
    Use Case - 2
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    eda_freq_hz = 30
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")
    qa_utils.clear_fs_logs('')

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'eda')

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)
    common.set_eda_stream_freq(samp_freq_hz=eda_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_fs_log("start")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_fs_log("stop")

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'eda')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-2')

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase2_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase2_{}Hz.csv'.format(adxl_freq_hz))
    f_path_eda = common.rename_stream_file(csv_file_name_dict["eda"], 'usecase2_{}hz.csv'.format(eda_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase2_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combained', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_eda, err_str_eda, results_dict_eda = qa_utils.check_stream_data(f_path_eda, 'eda', 1, eda_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.logging.info('ADPD CH-1 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
                                                                                  results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
    #                                                                               results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-2 FS Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.logging.info('EDA {}Hz UseCase-2 FS Test Results: {}'.format(eda_freq_hz, results_dict_eda))
    common.logging.info('Temperature {}Hz UseCase-2 FS Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}Hz UseCase-2 FS Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-2 FS Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_eda:
        common.logging.error('*** EDA {}Hz UseCase-2 FS Test - FAIL ***'.format(eda_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_eda))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-2 FS Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_ecg(start_stream=("ecg", "adxl", "adpd", "temperature"),
                        stop_stream=("ecg", "adxl", "adpd", "temperature")):
    """
    Use Case - 3
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    temp_freq_hz = 1
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    ecg_freq_hz = 250
    usecase = "3"
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)
    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)
    common.close_plot_after_run(['ADPD400x Data', 'ADXL Data', 'ECG Data Plot', 'Temperature Data Plot'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'ecg')

    f_path_adpd = common.rename_stream_file(common.adpd_stream_file_name, "usecase{}_{}Hz.csv".format(usecase,
                                                                                                      adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name, 'usecase{}_{}Hz.csv'.format(usecase,
                                                                                                      adxl_freq_hz))
    f_path_ecg = common.rename_stream_file(common.ecg_stream_file_name, 'usecase{}_{}hz.csv'.format(usecase,
                                                                                                    ecg_freq_hz))
    f_path_temperature = common.rename_stream_file(common.temperature_stream_file_name,
                                                   'usecase{}_{}Hz.csv'.format(usecase, temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 1,
                                                                                              adpd_freq_hz)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'eda', 1, ecg_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.logging.info('ADPD CH-1 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-{} Test Results: {}'.format(adxl_freq_hz, usecase, results_dict_adxl))
    common.logging.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.logging.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                               results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-{} Test - FAIL ***'.format(adxl_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_ecg:
        common.logging.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_ecg_fs(start_stream=("ecg", "adxl", "adpd", "temperature"),
                           stop_stream=("ecg", "adxl", "adpd", "temperature")):
    """
    Use Case - 3
    :param start_stream:
    :param stop_stream:
    :return:
    """

    capture_time = 30
    temp_freq_hz = 1
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    ecg_freq_hz = 250
    usecase = "3 FS"
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd4000')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd4000', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")
    qa_utils.clear_fs_logs('')

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    # common.set_adpd_stream_freq(samp_freq_hz=adpd_freq_hz)
    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_fs_log("start")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_fs_log("stop")

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'ecg')
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-3')

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase{}_{}Hz.csv".format(usecase,
                                                                                                    adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase{}_{}Hz.csv'.format(usecase,
                                                                                                    adxl_freq_hz))
    f_path_ecg = common.rename_stream_file(csv_file_name_dict["ecg"], 'usecase{}_{}hz.csv'.format(usecase,
                                                                                                    ecg_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"], 'usecase{}_{}Hz.csv'.format(usecase,
                                                                                                     temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combained', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'eda', 1, ecg_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.logging.info('ADPD CH-1 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch1))
    # common.logging.info('ADPD CH-2 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch2))
    common.logging.info('ADXL {}Hz UseCase-{} Test Results: {}'.format(adxl_freq_hz, usecase, results_dict_adxl))
    common.logging.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.logging.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                               results_dict_temp))

    if err_status_adpd_ch1:
        common.logging.error('*** ADPD CH-1 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.logging.error('*** ADPD CH-2 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-{} Test - FAIL ***'.format(adxl_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_ecg:
        common.logging.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ecg(start_stream=("ecg", "ppg", "temperature"), stop_stream=("ecg", "temperature", "ppg")):
    """
    Use Case - 4
    :param start_stream:
    :param stop_stream:
    :return:
    """
    capture_time = 30
    temp_freq_hz = 1
    ppg_freq_hz = 50
    ecg_freq_hz = 1000
    usecase = "4"

    common.dcb_cfg('d', 'adpd4000')
    qa_utils.write_dcb('adpd4000', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_loadAdpdCfg("40")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    # common.set_adpd_stream_freq(samp_freq_hz=ppg_freq_hz)
    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.close_plot_after_run(['Sync PPG Data', 'PPG Data', 'ECG Data Plot', 'Temperature Data Plot'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'ecg')

    common.rename_stream_file(common.ppg_stream_file_name, "usecase{}_{}Hz.csv".format(usecase, ppg_freq_hz))
    f_path_ppg = common.rename_stream_file(common.syncppg_stream_file_name, "usecase{}_{}Hz.csv".format(usecase,
                                                                                                        ppg_freq_hz))
    f_path_ecg = common.rename_stream_file(common.ecg_stream_file_name, 'usecase{}_{}hz.csv'.format(usecase,
                                                                                                    ecg_freq_hz))
    f_path_temperature = common.rename_stream_file(common.temperature_stream_file_name,
                                                   'usecase{}_{}Hz.csv'.format(usecase, temp_freq_hz))

    err_status_ppg, err_str_ppg, results_dict_ppg = qa_utils.check_stream_data(f_path_ppg, 'ppg', 1, ppg_freq_hz)
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'eda', 1, ecg_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.logging.info('PPG {}Hz UseCase-{} Test Results: {}'.format(ppg_freq_hz, usecase, results_dict_ppg))
    common.logging.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.logging.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz,usecase,
                                                                               results_dict_temp))

    if err_status_ppg:
        common.logging.error('***PPG {}Hz UseCase-{} Test - FAIL ***'.format(ppg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ppg))
    if err_status_ecg:
        common.logging.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ecg_fs(start_stream=("ecg", "ppg", "temperature"), stop_stream=("ecg", "temperature", "ppg")):
    """
    Use Case - 4
    :param start_stream:
    :param stop_stream:
    :return:
    """
    capture_time = 30
    temp_freq_hz = 1
    ppg_freq_hz = 50
    ecg_freq_hz = 1000
    usecase = "4 FS"

    common.dcb_cfg('d', 'adpd4000')
    qa_utils.write_dcb('adpd4000', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_loadAdpdCfg("40")
    qa_utils.clear_fs_logs('')

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    # common.set_adpd_stream_freq(samp_freq_hz=ppg_freq_hz)
    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_fs_log("start")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_fs_log("stop")

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'ecg')
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-4')

    common.rename_stream_file(csv_file_name_dict["ppg"], "usecase{}_{}Hz.csv".format(usecase, ppg_freq_hz))
    f_path_ppg = common.rename_stream_file(csv_file_name_dict["syncppg"], "usecase{}_{}Hz.csv".format(usecase,
                                                                                                      ppg_freq_hz))
    f_path_ecg = common.rename_stream_file(csv_file_name_dict["ecg"], 'usecase{}_{}hz.csv'.format(usecase, ecg_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase{}_{}Hz.csv'.format(usecase, temp_freq_hz))

    err_status_ppg, err_str_ppg, results_dict_ppg = qa_utils.check_stream_data(f_path_ppg, 'ppg', 1, ppg_freq_hz, True)
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'eda', 1, ecg_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.logging.info('PPG {}Hz UseCase-{} Test Results: {}'.format(ppg_freq_hz, usecase, results_dict_ppg))
    common.logging.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.logging.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz,usecase,
                                                                               results_dict_temp))

    if err_status_ppg:
        common.logging.error('***PPG {}Hz UseCase-{} Test - FAIL ***'.format(ppg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ppg))
    if err_status_ecg:
        common.logging.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.logging.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_5(start_stream=("multi_led_adpd", "adxl"), stop_stream=("multi_led_adpd", "adxl")):
    """
    Use Case - 5
    :param start_stream:
    :param stop_stream:
    :return:
    """
    capture_time = 30
    agc_state = 1
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    led_list = ['G', 'R', 'IR', 'B']
    led_stream_file_dict = {}
    for i, led in zip(range(6, 10), led_list):
        led_stream_file_dict[led] = 'adpd{}Stream.csv'.format(str(i))

    # Multi LED Stream Test
    multi_stream_file_list = []

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'adpd4000')
    qa_utils.write_dcb('adpd4000', 'adpd_multi_led_dcb.dcfg', 'ADPD Multi LED Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    rand_utils.randomized_stream_start(start_stream)
    time.sleep(capture_time)

    rand_utils.randomized_stream_stop(stop_stream)
    common.close_plot_after_run(['ADPD400x Data', 'ADXL Data'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')

    for led in led_list:
        multi_stream_file_list.append(common.rename_stream_file(led_stream_file_dict[led],
                                                                '_{}_usecase5_{}Hz.csv'.format(led, adpd_freq_hz)))

    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name, 'usecase5_{}Hz.csv'.format(adxl_freq_hz))
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    common.logging.info('ADXL {}Hz UseCase-5 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))

    for i, led in enumerate(led_list):
        f_name = multi_stream_file_list[i]
        # CH1 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 1, adpd_freq_hz)
        common.logging.info('ADPD CH1 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
        common.logging.info('ADPD CH1 UseCase-5 Test Results: {}'.format(results_dict))
        if err_status:
            common.logging.error('*** ADPD {}Hz {}_LED CH1 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
        # CH2 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 2, adpd_freq_hz)
        common.logging.info('ADPD CH2 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
        common.logging.info('ADPD CH2 UseCase-5 Test Results: {}'.format(results_dict))
        if err_status:
            common.logging.error('*** ADPD {}Hz {}_LED CH2 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-5 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))


def use_case_5_fs(start_stream=("multi_led_adpd", "adxl"), stop_stream=("multi_led_adpd", "adxl")):
    """
    Use Case - 5
    :param start_stream:
    :param stop_stream:
    :return:
    """
    capture_time = 30
    agc_state = 1
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    led_list = ['G', 'R', 'IR', 'B']
    led_stream_file_dict = {}
    for i, led in zip(range(6, 10), led_list):
        led_stream_file_dict[led] = 'adpd{}Stream.csv'.format(str(i))
    qa_utils.clear_fs_logs('')

    # Multi LED Stream Test
    multi_stream_file_list = []

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'adpd4000')
    qa_utils.write_dcb('adpd4000', 'adpd_multi_led_dcb.dcfg', 'ADPD Multi LED Stream Test')
    common.watch_shell.do_loadAdpdCfg("40")

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_fs_log("start")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_fs_log("stop")
    common.close_plot_after_run(['ADPD400x Data', 'ADXL Data'])

    common.dcb_cfg('d', 'adpd4000')
    common.dcb_cfg('d', 'adxl')
    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-5')

    # for led in led_list:
    #     multi_stream_file_list.append(common.rename_stream_file(led_stream_file_dict[led],
    #                                                             '_{}_usecase5_{}Hz.csv'.format(led, adpd_freq_hz)))

    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase5_{}Hz.csv'.format(adxl_freq_hz))
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    common.logging.info('ADXL {}Hz UseCase-5 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], 'usecase5_{}Hz.csv'.format(adpd_freq_hz))
    err_status_adpd, err_str_adpd, results_dict_adpd = qa_utils.check_stream_data(f_path_adpd, 'adpd_combination', 1,
                                                                                  adpd_freq_hz, True)
    common.logging.info('ADPD FS {}Hz UseCase-5 Test Results: {}'.format(adpd_freq_hz, results_dict_adpd))
    if err_status_adpd:
        common.logging.error('*** ADPD FS {}Hz UseCase-5 Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd))

    # for i, led in enumerate(led_list):
    #     f_name = multi_stream_file_list[i]
    #     # CH1 Data Check
    #     err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 1, adpd_freq_hz)
    #     common.logging.info('ADPD CH1 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
    #     common.logging.info('ADPD CH1 UseCase-5 Test Results: {}'.format(results_dict))
    #     if err_status:
    #         common.logging.error('*** ADPD {}Hz {}_LED CH1 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
    #         raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
    #     # CH2 Data Check
    #     err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 2, adpd_freq_hz)
    #     common.logging.info('ADPD CH2 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
    #     common.logging.info('ADPD CH2 UseCase-5 Test Results: {}'.format(results_dict))
    #     if err_status:
    #         common.logging.error('*** ADPD {}Hz {}_LED CH2 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
    #         raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    if err_status_adxl:
        common.logging.error('*** ADXL {}Hz UseCase-5 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
