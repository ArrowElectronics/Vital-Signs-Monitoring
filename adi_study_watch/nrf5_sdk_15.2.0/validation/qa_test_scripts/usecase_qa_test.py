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


capture_time = 600


def use_case_qa_ppg(start_stream=("adpd", "adxl", "temp"), stop_stream=("temp", "adxl", "adpd")):
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

    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_500hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'UC-1 Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'UC-1 Test')

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.dcb_cfg('d', 'adpd')
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

    common.test_logger.info(
        'ADPD CH-1 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-1 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.test_logger.info('Temperature {}Hz UseCase-1  Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-1 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-1 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_fs(start_stream=("adpd", "adxl", "temp"), stop_stream=("temp", "adxl", "adpd")):
    """
    USE CASE - 1
    :param start_stream:
    :param stop_stream:
    :return:
    """

    adpd_freq_hz = 500
    adxl_freq_hz = 50
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_500hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')
    qa_utils.clear_fs_logs('')

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_start_logging("")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_stop_logging("")

    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'adxl')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-1')
    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase1_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase1_{}Hz.csv'.format(adxl_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase1_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combined', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                        adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.test_logger.info('ADPD CH-1 {}-LED {}Hz UseCase-1 FS Test Results: {}'.format(led, adpd_freq_hz,
                                                                                         results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}-LED {}Hz UseCase-1 Test Results: {}'.format(led, adpd_freq_hz, results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-1 FS Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.test_logger.info('Temperature {}Hz UseCase-1 FS Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}-LED {}Hz UseCase-1 FS Test - FAIL ***'.format(led, adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}-LED {}Hz UseCase-1 Test - FAIL ***'.format(led, adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-1 FS Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz FS UseCase-1 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_eda(start_stream=("eda", "adxl", "adpd", "temp"),
                        stop_stream=("temp", "adpd", "adxl", "eda")):
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

    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'eda')
    common.set_eda_stream_freq(samp_freq_hz=eda_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.dcb_cfg('d', 'adpd')
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

    common.test_logger.info('ADPD CH-1 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
                                                                                      results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
    #                                                                               results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-2 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.test_logger.info('EDA {}Hz UseCase-2 Test Results: {}'.format(eda_freq_hz, results_dict_eda))
    common.test_logger.info('Temperature {}Hz UseCase-2  Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-2 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_eda:
        common.test_logger.error('*** EDA {}Hz UseCase-2 Test - FAIL ***'.format(eda_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_eda))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-2 Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_eda_fs(start_stream=("eda", "adxl", "adpd", "temp"),
                           stop_stream=("temp", "adpd", "adxl", "eda")):
    """
    Use Case - 2
    :param start_stream:
    :param stop_stream:
    :return:
    """

    adpd_freq_hz = 100
    adxl_freq_hz = 50
    eda_freq_hz = 30
    temp_freq_hz = 1
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    qa_utils.clear_fs_logs('')
    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'eda')

    common.set_eda_stream_freq(samp_freq_hz=eda_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_start_logging("")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_stop_logging("")

    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'eda')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-2')

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase2_{}Hz.csv".format(adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase2_{}Hz.csv'.format(adxl_freq_hz))
    f_path_eda = common.rename_stream_file(csv_file_name_dict["eda"], 'usecase2_{}hz.csv'.format(eda_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase2_{}Hz.csv'.format(temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combined', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_eda, err_str_eda, results_dict_eda = qa_utils.check_stream_data(f_path_eda, 'eda', 1, eda_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.test_logger.info('ADPD CH-1 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
                                                                                      results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}-LED {}Hz UseCase-2 Test Results: {}'.format(led, adpd_freq_hz,
    #                                                                               results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-2 FS Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))
    common.test_logger.info('EDA {}Hz UseCase-2 FS Test Results: {}'.format(eda_freq_hz, results_dict_eda))
    common.test_logger.info('Temperature {}Hz UseCase-2 FS Test Results: {}'.format(temp_freq_hz, results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}Hz UseCase-2 FS Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}Hz UseCase-2 Test - FAIL ***'.format(adpd_freq_hz))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-2 FS Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_eda:
        common.test_logger.error('*** EDA {}Hz UseCase-2 FS Test - FAIL ***'.format(eda_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_eda))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-2 FS Test - FAIL ***'.format(temp_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_ecg(start_stream=("ecg", "adpd", "adxl", "temp"),
                        stop_stream=("temp", "adxl", "adpd", "ecg")):
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
    ecg_freq_hz = 300
    usecase = "3"
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')
    qa_utils.enable_ecg_without_electrodes_contact()

    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.dcb_cfg('d', 'adpd')
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
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'ecg', 1, ecg_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.test_logger.info(
        'ADPD CH-1 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-{} Test Results: {}'.format(adxl_freq_hz, usecase, results_dict_adxl))
    common.test_logger.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.test_logger.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                                   results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-{} Test - FAIL ***'.format(adxl_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_ecg:
        common.test_logger.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ppg_ecg_fs(start_stream=("ecg", "adpd", "adxl", "temp"),
                            stop_stream=("temp", "adxl", "adpd", "ecg")):

    """
    Use Case - 3
    :param start_stream:
    :param stop_stream:
    :return:
    """

    temp_freq_hz = 1
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    ecg_freq_hz = 300
    usecase = "3 FS"
    # led = random.choice(["G", "R", "IR", "B"])
    led = "G"

    qa_utils.clear_fs_logs('')
    common.dcb_cfg('d', 'adpd')
    adpd_led_dcb = 'adpd_{}_dcb_100hz.dcfg'.format(led.lower())
    qa_utils.write_dcb('adpd', adpd_led_dcb, 'ADPD Stream Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    qa_utils.enable_ecg_without_electrodes_contact()
    common.watch_shell.do_start_logging("")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_stop_logging("")

    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'adxl')
    common.dcb_cfg('d', 'ecg')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-3')

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], "usecase{}_{}Hz.csv".format(usecase,
                                                                                                    adpd_freq_hz))
    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase{}_{}Hz.csv'.format(usecase,
                                                                                                    adxl_freq_hz))
    f_path_ecg = common.rename_stream_file(csv_file_name_dict["ecg"], 'usecase{}_{}hz.csv'.format(usecase,
                                                                                                  ecg_freq_hz))
    f_path_temperature = common.rename_stream_file(csv_file_name_dict["temperature"],
                                                   'usecase{}_{}Hz.csv'.format(usecase,
                                                                               temp_freq_hz))

    err_status_adpd_ch1, err_str_adpd_ch1, results_dict_adpd_ch1 = qa_utils.check_stream_data(f_path_adpd,
                                                                                              'adpd_combined', 1,
                                                                                              adpd_freq_hz, True)
    # err_status_adpd_ch2, err_str_adpd_ch2, results_dict_adpd_ch2 = qa_utils.check_stream_data(f_path_adpd, 'adpd', 2,
    #                                                                                           adpd_freq_hz)
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'ecg', 1, ecg_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.test_logger.info(
        'ADPD CH-1 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch1))
    # common.test_logger.info('ADPD CH-2 {}Hz UseCase-{} Test Results: {}'.format(adpd_freq_hz, usecase, results_dict_adpd_ch2))
    common.test_logger.info('ADXL {}Hz UseCase-{} Test Results: {}'.format(adxl_freq_hz, usecase, results_dict_adxl))
    common.test_logger.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.test_logger.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                                   results_dict_temp))

    if err_status_adpd_ch1:
        common.test_logger.error('*** ADPD CH-1 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch1))
    # if err_status_adpd_ch2:
    #     common.test_logger.error('*** ADPD CH-2 {}Hz UseCase-{} Test - FAIL ***'.format(adpd_freq_hz, usecase))
    #     raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd_ch2))
    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-{} Test - FAIL ***'.format(adxl_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))
    if err_status_ecg:
        common.test_logger.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ecg(start_stream=("ecg", "ppg", "temp"), stop_stream=("temp", "ppg", "ecg")):

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

    common.dcb_cfg('d', 'ppg')
    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')
    qa_utils.enable_ecg_without_electrodes_contact()

    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_stream_start(start_stream)

    time.sleep(capture_time)
    rand_utils.randomized_stream_stop(stop_stream)

    common.dcb_cfg('d', 'adpd')
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
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'ecg', 1, ecg_freq_hz)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz)

    common.test_logger.info('PPG {}Hz UseCase-{} Test Results: {}'.format(ppg_freq_hz, usecase, results_dict_ppg))
    common.test_logger.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.test_logger.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                                   results_dict_temp))

    if err_status_ppg:
        common.test_logger.error('***PPG {}Hz UseCase-{} Test - FAIL ***'.format(ppg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ppg))
    if err_status_ecg:
        common.test_logger.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_qa_ecg_fs(start_stream=("ecg", "ppg", "temp"), stop_stream=("temp", "ppg", "ecg")):
    """
    Use Case - 4
    :param start_stream:
    :param stop_stream:
    :return:
    """

    temp_freq_hz = 1
    ppg_freq_hz = 50
    ecg_freq_hz = 1000
    usecase = "4 FS"

    qa_utils.clear_fs_logs('')
    common.dcb_cfg('d', 'ppg')
    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_qa_dcb.dcfg', 'ADPD DCB Test')
    common.watch_shell.do_load_adpd_cfg("1")

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'ecg')

    common.set_ecg_stream_freq(samp_freq_hz=ecg_freq_hz)

    rand_utils.randomized_fs_stream_start(start_stream)
    qa_utils.enable_ecg_without_electrodes_contact()
    common.watch_shell.do_start_logging("")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_stop_logging("")

    common.dcb_cfg('d', 'adpd')
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
    err_status_ecg, err_str_ecg, results_dict_ecg = qa_utils.check_stream_data(f_path_ecg, 'ecg', 1, ecg_freq_hz, True)
    err_status_temp, err_str_temp, results_dict_temp = qa_utils.check_stream_data(f_path_temperature, 'temperature', 1,
                                                                                  temp_freq_hz, True)

    common.test_logger.info('PPG {}Hz UseCase-{} Test Results: {}'.format(ppg_freq_hz, usecase, results_dict_ppg))
    common.test_logger.info('ECG {}Hz UseCase-{} Test Results: {}'.format(ecg_freq_hz, usecase, results_dict_ecg))
    common.test_logger.info('Temperature {}Hz UseCase-{}  Test Results: {}'.format(temp_freq_hz, usecase,
                                                                                   results_dict_temp))

    if err_status_ppg:
        common.test_logger.error('***PPG {}Hz UseCase-{} Test - FAIL ***'.format(ppg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ppg))
    if err_status_ecg:
        common.test_logger.error('*** ECG {}Hz UseCase-{} Test - FAIL ***'.format(ecg_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_ecg))
    if err_status_temp:
        common.test_logger.error('*** Temperature {}Hz UseCase-{} Test - FAIL ***'.format(temp_freq_hz, usecase))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_temp))


def use_case_5(start_stream=("multi_led_adpd", "adxl"), stop_stream=("adxl", "multi_led_adpd")):
    """
    Use Case - 5
    :param start_stream:
    :param stop_stream:
    :return:
    """
    capture_time = 30
    agc_state = 0
    adpd_freq_hz = 100
    adxl_freq_hz = 50
    led_list = ['G', 'R', 'IR', 'B']
    led_stream_file_dict = {}
    for i, led in zip(range(6, 10), led_list):
        led_stream_file_dict[led] = 'adpd{}.csv'.format(str(i))

    # Multi LED Stream Test
    multi_stream_file_list = []

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_multi_led_dcb.dcfg', 'ADPD Multi LED Stream Test')
    common.watch_shell.do_load_adpd_cfg("5")

    rand_utils.randomized_stream_start(start_stream)
    time.sleep(capture_time)

    rand_utils.randomized_stream_stop(stop_stream)

    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'adxl')

    for led in led_list:
        multi_stream_file_list.append(common.rename_stream_file(led_stream_file_dict[led],
                                                                '_{}_usecase5_{}Hz.csv'.format(led, adpd_freq_hz)))

    f_path_adxl = common.rename_stream_file(common.adxl_stream_file_name, 'usecase5_{}Hz.csv'.format(adxl_freq_hz))
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz)
    common.test_logger.info('ADXL {}Hz UseCase-5 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))

    for i, led in enumerate(led_list):
        f_name = multi_stream_file_list[i]
        # CH1 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 1, adpd_freq_hz)
        common.test_logger.info('ADPD CH1 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
        common.test_logger.info('ADPD CH1 UseCase-5 Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH1 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))
        # CH2 Data Check
        err_status, err_str, results_dict = qa_utils.check_stream_data(f_name, 'adpd', 2, adpd_freq_hz)
        common.test_logger.info('ADPD CH2 UseCase-5 Test - LED: {} | Freq: {}'.format(led, adpd_freq_hz))
        common.test_logger.info('ADPD CH2 UseCase-5 Test Results: {}'.format(results_dict))
        if err_status:
            common.test_logger.error('*** ADPD {}Hz {}_LED CH2 UseCase-5 Test - FAIL ***'.format(adpd_freq_hz, led))
            raise ConditionCheckFailure("\n\n" + '{}'.format(err_str))

    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-5 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))


def use_case_5_fs(start_stream=("multi_led_adpd", "adxl"), stop_stream=("adxl", "multi_led_adpd")):
    """
    Use Case - 5
    :param start_stream:
    :param stop_stream:
    :return:
    """

    adpd_freq_hz = 100
    adxl_freq_hz = 50
    qa_utils.clear_fs_logs('')

    common.dcb_cfg('d', 'adxl')
    qa_utils.write_dcb('adxl', 'adxl_dcb.dcfg', 'ADXL Stream Test')

    common.dcb_cfg('d', 'adpd')
    qa_utils.write_dcb('adpd', 'adpd_multi_led_dcb.dcfg', 'ADPD Multi LED Stream Test')
    common.watch_shell.do_load_adpd_cfg("5")

    rand_utils.randomized_fs_stream_start(start_stream)
    common.watch_shell.do_start_logging("")

    time.sleep(capture_time)
    rand_utils.randomized_fs_stream_stop(stop_stream)
    common.watch_shell.do_stop_logging("")

    common.dcb_cfg('d', 'adpd')
    common.dcb_cfg('d', 'adxl')

    log_file_name, csv_file_name_dict = qa_utils.get_fs_log('UC-5')

    f_path_adxl = common.rename_stream_file(csv_file_name_dict["adxl"], 'usecase5_{}Hz.csv'.format(adxl_freq_hz))
    err_status_adxl, err_str_adxl, results_dict_adxl = qa_utils.check_stream_data(f_path_adxl, 'adxl', 1, adxl_freq_hz,
                                                                                  True)
    common.test_logger.info('ADXL {}Hz UseCase-5 Test Results: {}'.format(adxl_freq_hz, results_dict_adxl))

    f_path_adpd = common.rename_stream_file(csv_file_name_dict["adpd"], 'usecase5_{}Hz.csv'.format(adpd_freq_hz))
    err_status_adpd, err_str_adpd, results_dict_adpd = qa_utils.check_stream_data(f_path_adpd, 'adpd_combined', 1,
                                                                                  adpd_freq_hz, True)
    common.test_logger.info('ADPD FS {}Hz UseCase-5 Test Results: {}'.format(adpd_freq_hz, results_dict_adpd))
    if err_status_adpd:
        common.test_logger.error('*** ADPD FS {}Hz UseCase-5 Test - FAIL ***'.format(adpd_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adpd))

    if err_status_adxl:
        common.test_logger.error('*** ADXL {}Hz UseCase-5 Test - FAIL ***'.format(adxl_freq_hz))
        raise ConditionCheckFailure("\n\n" + '{}'.format(err_str_adxl))


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
