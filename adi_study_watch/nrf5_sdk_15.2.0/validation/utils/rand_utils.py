import common
import random
import itertools
from utils import qa_utils


def adpd_stream_and_plot(freq=None):
    if freq:
        common.set_adpd_stream_freq(freq)
    common.watch_shell.quick_start("adpd", "adpd6")


def adpd_fs_sub_add(freq=None):
    if freq:
        common.set_adpd_stream_freq(freq)
    common.watch_shell.do_fs_sub("adpd6 add")


def adpd_sensor_start():
    common.watch_shell.do_sensor("adpd start")


def eda_stream_and_plot(freq=None):
    common.watch_shell.quick_start("eda", "eda")


def eda_fs_sub_add(freq=None):
    common.watch_shell.do_fs_sub("eda add")


def eda_sensor_start():
    common.watch_shell.do_sensor("eda start")


def adxl_stream_and_plot(freq=None):
    common.watch_shell.quick_start("adxl", "adxl")


def adxl_fs_sub_add(freq=None):
    common.watch_shell.do_fs_sub("adxl add")


def adxl_sensor_start():
    common.watch_shell.do_sensor("adxl start")


def ecg_stream_and_plot(freq=None):
    if freq:
        common.set_ecg_stream_freq(freq)
    common.watch_shell.quick_start("ecg", "ecg")


def ecg_fs_sub_add(freq=None):
    if freq:
        common.set_ecg_stream_freq(freq)
    common.watch_shell.do_fs_sub("ecg add")


def ecg_sensor_start():
    common.watch_shell.do_sensor("ecg start")


def temp_stream_and_plot(freq=None):
    common.watch_shell.quick_start("temp", "temp")


def temp_fs_sub_add(freq=None):
    common.watch_shell.do_fs_sub("temp add")


def temp_sensor_start():
    common.watch_shell.do_sensor("temp start")


def ppg_stream_and_plot(freq=None):
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.watch_shell.do_set_ppg_lcfg("5")
    common.watch_shell.quick_start("ppg", "ppg")


def ppg_fs_sub_add(freq=None):
    common.watch_shell.do_load_adpd_cfg("1")
    common.watch_shell.do_calibrate_clock(common.adpd_clk_calib)
    common.watch_shell.do_set_ppg_lcfg("5")
    common.watch_shell.do_fs_sub("ppg add")
    # common.watch_shell.do_fs_sub("sync_ppg add")


def ppg_sensor_start():
    common.watch_shell.do_sensor("ppg start")


def multi_led_adpd_stream_and_plot(freq=None):
    qa_utils.quick_start_adpd_multi_led(samp_freq_hz=100, agc_state=0, skip_load_cfg=True)


def multi_led_adpd_fs_sub_add(freq=None):
    samp_freq_hz = 100
    agc_state = 0
    led_list = ['G', 'R', 'IR', 'B']
    cfg_dict = {'G': {'adpd_cfg': '1', 'clk_calib': common.adpd_clk_calib, 'sub': '6', 'agc_ctrl_id': '1'},
                'R': {'adpd_cfg': '2', 'clk_calib': common.adpd_clk_calib, 'sub': '7', 'agc_ctrl_id': '2'},
                'IR': {'adpd_cfg': '3', 'clk_calib': common.adpd_clk_calib, 'sub': '8', 'agc_ctrl_id': '3'},
                'B': {'adpd_cfg': '4', 'clk_calib': common.adpd_clk_calib, 'sub': '9', 'agc_ctrl_id': '4'},
                'MWL': {'adpd_cfg': '5', 'clk_calib': common.adpd_clk_calib, 'sub': '10', 'agc_ctrl_id': '5'}}

    for led in led_list:
        if agc_state:
            common.watch_shell.do_enable_agc('{}'.format(cfg_dict[led]['agc_ctrl_id']))
        else:
            common.watch_shell.do_disable_agc('{}'.format(cfg_dict[led]['agc_ctrl_id']))

    if samp_freq_hz is None:
        pass
    elif samp_freq_hz == 50:
        common.watch_shell.do_reg("w adpd 0xD:0x4e20")
    elif samp_freq_hz == 100:
        common.watch_shell.do_reg("w adpd 0xD:0x2710")
    elif samp_freq_hz == 500:
        common.watch_shell.do_reg("w adpd 0xD:0x07D0")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")

    for led in led_list:
        common.watch_shell.do_fs_sub("adpd{} add".format(cfg_dict[led]['sub']))


def multi_led_adpd_stop():
    for led in range(6, 10):
        common.watch_shell.do_sub("adpd{} remove".format(str(led)))
        common.watch_shell.do_csv_log('adpd{} stop'.format(str(led)))
    common.watch_shell.do_sensor("adpd stop")


def multi_led_adpd_fs_sub_remove():
    for led in range(6, 10):
        common.watch_shell.do_fs_sub("adpd{} remove".format(str(led)))


def adpd_sensor_stop():
    common.watch_shell.do_sensor("adpd stop")


def ppg_stream_stop():
    common.quick_stop_ppg()


def ppg_fs_sub_remove():
    common.watch_shell.do_fs_sub("ppg remove")
    # common.watch_shell.do_fs_sub("sync_ppg remove")


def ppg_sensor_stop():
    common.watch_shell.do_sensor("ppg stop")


def temp_stream_stop():
    common.watch_shell.quick_stop('temp', 'temp')


def temp_fs_sub_remove():
    common.watch_shell.do_fs_sub("temp remove")


def temp_sensor_stop():
    common.watch_shell.do_sensor("temp stop")


def adxl_stream_stop():
    common.watch_shell.quick_stop('adxl', 'adxl')


def adxl_fs_sub_remove():
    common.watch_shell.do_fs_sub("adxl remove")


def adxl_sensor_stop():
    common.watch_shell.do_sensor("adxl stop")


def ecg_stream_stop():
    common.watch_shell.quick_stop('ecg', 'ecg')


def ecg_fs_sub_remove():
    common.watch_shell.do_fs_sub("ecg remove")


def ecg_sensor_stop():
    common.watch_shell.do_sensor("ecg stop")


def adpd_stream_stop():
    common.quick_stop_adpd("G")


def adpd_fs_sub_remove():
    common.watch_shell.do_fs_sub("adpd6 remove")


def eda_stream_stop():
    common.watch_shell.quick_stop('eda', 'eda')


def eda_fs_sub_remove():
    common.watch_shell.do_fs_sub("eda remove")


def eda_sensor_stop():
    common.watch_shell.do_sensor("eda stop")


# ***************************************************************************** #
stream_fn_dict = {'adpd': {'start': adpd_stream_and_plot, 'stop': adpd_stream_stop},
                  'adxl': {'start': adxl_stream_and_plot, 'stop': adxl_stream_stop},
                  'ecg': {'start': ecg_stream_and_plot, 'stop': ecg_stream_stop},
                  'eda': {'start': eda_stream_and_plot, 'stop': eda_stream_stop},
                  'temp': {'start': temp_stream_and_plot, 'stop': temp_stream_stop},
                  'ppg': {'start': ppg_stream_and_plot, 'stop': ppg_stream_stop},
                  'multi_led_adpd': {'start': multi_led_adpd_stream_and_plot, 'stop': multi_led_adpd_stop},
                  }
stream_fs_fn_dict = {
    'adpd': {'start': [adpd_fs_sub_add, adpd_sensor_start], 'stop': [adpd_fs_sub_remove, adpd_sensor_stop]},
    'adxl': {'start': [adxl_fs_sub_add, adxl_sensor_start], 'stop': [adxl_fs_sub_remove, adxl_sensor_stop]},
    'ecg': {'start': [ecg_fs_sub_add, ecg_sensor_start], 'stop': [ecg_fs_sub_remove, ecg_sensor_stop]},
    'eda': {'start': [eda_fs_sub_add, eda_sensor_start], 'stop': [eda_fs_sub_remove, eda_sensor_stop]},
    'temp': {'start': [temp_fs_sub_add, temp_sensor_start], 'stop': [temp_fs_sub_remove, temp_sensor_stop]},
    'ppg': {'start': [ppg_fs_sub_add, ppg_sensor_start], 'stop': [ppg_fs_sub_remove, ppg_sensor_stop]},
    'multi_led_adpd': {'start': [multi_led_adpd_fs_sub_add, adpd_sensor_start],
                       'stop': [multi_led_adpd_fs_sub_remove, adpd_sensor_stop]}
    }


def randomizer(stream_list=['adpd', 'adxl', 'ecg', 'temp', 'eda']):
    num_streams = len(stream_list)
    stream_idx_list = []
    rand_stream_list = []
    for i in range(num_streams):
        while True:
            rand_idx = random.randint(0, num_streams - 1)
            if rand_idx not in stream_idx_list:
                stream_idx_list.append(rand_idx)
                rand_stream_list.append(stream_list[rand_idx])
                break
    return rand_stream_list


def randomize_list(stream_list=['adpd', 'adxl', 'ecg', 'temp', 'eda'], include_non_randomized=False,
                   all_possible_combination=False, iteration=1):
    possibility = list(itertools.permutations(stream_list))
    if all_possible_combination:
        return possibility
    if include_non_randomized:
        rand_stream_list = [stream_list]
        iteration -= 1
    else:
        rand_stream_list = []
    for i in range(iteration):
        while True:
            random_stream = random.choice(possibility)
            if rand_stream_list not in random_stream:
                rand_stream_list.append(random_stream)
                break
    return rand_stream_list


def randomized_stream_start(rand_stream_list, stream_args=None):
    if not stream_args:
        stream_args = [None for i in rand_stream_list]
    common.test_logger.info("Randomized Start Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fn_dict[stream]['start'](stream_args[i])


def randomized_fs_stream_start(rand_stream_list):
    common.test_logger.info("Randomized Start Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['start'][0]()

    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['start'][1]()


def randomized_stream_stop(rand_stream_list):
    common.test_logger.info("Randomized Stop Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fn_dict[stream]['stop']()


def randomized_fs_stream_stop(rand_stream_list):
    common.test_logger.info("Randomized Stop Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['stop'][1]()

    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['stop'][0]()
