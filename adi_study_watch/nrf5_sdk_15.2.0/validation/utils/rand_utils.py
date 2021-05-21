import common
import random
import itertools
from utils import qa_utils


def adpd_stream_and_plot(freq=None):
    if freq:
        common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_sensor('adpd4000 start')
    common.watch_shell.do_sub('radpd6 add')
    if freq:
        common.set_adpd_stream_freq(freq)
        common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_plot('radpd6')


def adpd_fs_stream_and_plot(freq=None):
    if freq:
        common.set_adpd_stream_freq(freq)
    common.watch_shell.do_sensor('adpd4000 start')
    common.watch_shell.do_fs_sub('radpd6 add')


def eda_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('eda start')
    common.watch_shell.do_sub('reda add')
    common.watch_shell.do_plot('reda')


def eda_fs_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('eda start')
    common.watch_shell.do_fs_sub('reda add')


def adxl_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('adxl start')
    common.watch_shell.do_sub('radxl add')
    common.watch_shell.do_plot('radxl')


def adxl_fs_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('adxl start')
    common.watch_shell.do_fs_sub('radxl add')


def ecg_stream_and_plot(freq=None):
    if freq:
        common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_sensor('ecg start')
    common.watch_shell.do_sub('recg add')
    if freq:
        common.set_ecg_stream_freq(freq)
        common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_plot('recg')


def ecg_fs_stream_and_plot(freq=None):
    if freq:
        common.set_ecg_stream_freq(freq)
    common.watch_shell.do_sensor('ecg start')
    common.watch_shell.do_fs_sub('recg add')


def temperature_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('temperature start')
    common.watch_shell.do_sub('rtemperature add')
    common.watch_shell.do_plot('rtemperature')


def temperature_fs_stream_and_plot(freq=None):
    common.watch_shell.do_sensor('temperature start')
    common.watch_shell.do_fs_sub('rtemperature add')


def ppg_stream_and_plot(freq=None):
    common.watch_shell.do_loadAdpdCfg("40")
    common.watch_shell.do_clockCalibration("6")
    common.watch_shell.do_setPpgLcfg("40")
    common.watch_shell.do_sensor("ppg start")
    common.watch_shell.do_sub("rppg add")
    common.watch_shell.do_plot("rppg")
    common.watch_shell.do_plot("rsyncppg")


def ppg_fs_stream_and_plot(freq=None):
    common.watch_shell.do_loadAdpdCfg("40")
    common.watch_shell.do_clockCalibration("6")
    common.watch_shell.do_setPpgLcfg("40")
    common.watch_shell.do_sensor("ppg start")
    common.watch_shell.do_fs_sub("rppg add")


def multi_led_adpd_stream_and_plot(freq=None):
    qa_utils.quick_start_adpd_multi_led(samp_freq_hz=100, agc_state=1, skip_load_cfg=True)


def multi_led_adpd_fs_stream_and_plot(freq=None):
    samp_freq_hz = 100
    agc_state = 1
    led_stream_file_dict = {}
    led_list = ['G', 'R', 'IR', 'B']
    cfg_dict = {'G': {'adpd_cfg': '40', 'clk_calib': '6', 'sub': '6', 'agc_ctrl_id': '1'},
                'R': {'adpd_cfg': '41', 'clk_calib': '6', 'sub': '7', 'agc_ctrl_id': '2'},
                'IR': {'adpd_cfg': '42', 'clk_calib': '6', 'sub': '8', 'agc_ctrl_id': '3'},
                'B': {'adpd_cfg': '43', 'clk_calib': '6', 'sub': '9', 'agc_ctrl_id': '4'}}

    for led in led_list:
        if agc_state:
            common.watch_shell.do_adpdAGCControl('{}:1'.format(cfg_dict[led]['agc_ctrl_id']))
        else:
            common.watch_shell.do_adpdAGCControl('{}:0'.format(cfg_dict[led]['agc_ctrl_id']))

    if samp_freq_hz == 50:
        common.watch_shell.do_reg("w adpd4000 0xD:0x4e20")
    elif samp_freq_hz == 100:
        common.watch_shell.do_reg("w adpd4000 0xD:0x2710")
    elif samp_freq_hz == 500:
        common.watch_shell.do_reg("w adpd4000 0xD:0x07D0")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")

    common.watch_shell.do_sensor("adpd4000 start")
    for led in led_list:
        common.watch_shell.do_fs_sub("radpd{} add".format(cfg_dict[led]['sub']))


def multi_led_adpd_stop():
    for led in range(6, 10):
        common.watch_shell.do_sub("radpd{} remove".format(str(led)))
    common.watch_shell.do_sensor("adpd4000 stop")


def multi_led_adpd_fs_stop():
    for led in range(6, 10):
        common.watch_shell.do_fs_sub("radpd{} remove".format(str(led)))
    common.watch_shell.do_sensor("adpd4000 stop")


def ppg_stream_stop():
    common.quick_stop_ppg()


def ppg_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_ppg')


def temperature_stream_stop():
    common.watch_shell.do_quickstop('temperature')


def temperature_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_temperature')


def adxl_stream_stop():
    common.watch_shell.do_quickstop('adxl')


def adxl_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_adxl')


def ecg_stream_stop():
    common.watch_shell.do_quickstop('ecg')


def ecg_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_ecg')


def adpd_stream_stop():
    common.watch_shell.do_quickstop('adpd4000')


def adpd_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_adpd4000')


def eda_stream_stop():
    common.watch_shell.do_quickstop('eda')


def eda_fs_stream_stop():
    common.watch_shell.do_quickstop('stop_log_eda')


# ***************************************************************************** #
stream_fn_dict = {'adpd': {'start': adpd_stream_and_plot, 'stop': adpd_stream_stop},
                  'adxl': {'start': adxl_stream_and_plot, 'stop': adxl_stream_stop},
                  'ecg': {'start': ecg_stream_and_plot, 'stop': ecg_stream_stop},
                  'eda': {'start': eda_stream_and_plot, 'stop': eda_stream_stop},
                  'temperature': {'start': temperature_stream_and_plot, 'stop': temperature_stream_stop},
                  'ppg': {'start': ppg_stream_and_plot, 'stop': ppg_stream_stop},
                  'multi_led_adpd': {'start': multi_led_adpd_stream_and_plot, 'stop': multi_led_adpd_stop},
                  }
stream_fs_fn_dict = {'adpd': {'start': adpd_fs_stream_and_plot, 'stop': adpd_fs_stream_stop},
                     'adxl': {'start': adxl_fs_stream_and_plot, 'stop': adxl_fs_stream_stop},
                     'ecg': {'start': ecg_fs_stream_and_plot, 'stop': ecg_fs_stream_stop},
                     'eda': {'start': eda_fs_stream_and_plot, 'stop': eda_fs_stream_stop},
                     'temperature': {'start': temperature_fs_stream_and_plot, 'stop': temperature_fs_stream_stop},
                     'ppg': {'start': ppg_fs_stream_and_plot, 'stop': ppg_fs_stream_stop},
                     'multi_led_adpd': {'start': multi_led_adpd_fs_stream_and_plot, 'stop': multi_led_adpd_fs_stop}
                     }

def randomizer(stream_list=['adpd', 'adxl', 'ecg', 'temperature', 'eda']):
    num_streams = len(stream_list)
    stream_idx_list = []
    rand_stream_list = []
    for i in range(num_streams):
        while True:
            rand_idx = random.randint(0, num_streams-1)
            if rand_idx not in stream_idx_list:
                stream_idx_list.append(rand_idx)
                rand_stream_list.append(stream_list[rand_idx])
                break
    return rand_stream_list


def randomize_list(stream_list=['adpd', 'adxl', 'ecg', 'temperature', 'eda'], include_non_randomized=False,
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
                # rand_stream_list.append(stream_list[rand_stream_list])
                break
    return rand_stream_list


def randomized_stream_start(rand_stream_list, stream_args=None):
    if not stream_args:
        stream_args = [None for i in rand_stream_list]
    common.logging.info("Randomized Start Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fn_dict[stream]['start'](stream_args[i])


def randomized_fs_stream_start(rand_stream_list):
    common.logging.info("Randomized Start Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['start']()


def randomized_stream_stop(rand_stream_list):
    common.logging.info("Randomized Stop Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fn_dict[stream]['stop']()


def randomized_fs_stream_stop(rand_stream_list):
    common.logging.info("Randomized Stop Streaming Order is {}".format("--".join(rand_stream_list)))
    for i, stream in enumerate(rand_stream_list):
        stream_fs_fn_dict[stream]['stop']()
