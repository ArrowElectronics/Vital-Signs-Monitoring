import os
import common
import time

# Global Variables
dev_id_dict = {'ADXL362': 'adxl', 'ADPD4K': 'adpd', 'ADP5360': '',
               'AD5940': '', 'NAND_FLASH': '', 'AD7156': '', 'ECG': 'ecg', 'EDA': 'eda', 'BCM': 'bcm'}


def clear_fs_logs(app_name='ADXL'):
    common.watch_shell.do_fs_format('')
    err_stat, fs_ls_list = common.watch_shell.fs_ls()
    for fs_dict in fs_ls_list:
        if 'user_config' not in fs_dict['file'].lower():
            common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
            raise common.ConditionCheckFailure("\n\n" + '{}'.format('FS Format unable to clear all files!'))


def convert_and_verify_logs(log_file_name, csv_file_suffix='ADPDAppStream_Combined_CSV'):
    curr_dir = os.getcwd()
    log_file_path = os.path.join(curr_dir, log_file_name)
    log_converter_path = os.path.join(curr_dir, 'utils', 'LogConverter', 'LogConverter.exe')
    cmd_resp = common.subprocess.call([log_converter_path, log_file_path])
    if cmd_resp == 0:
        log_file_name = os.path.split(log_file_path)[-1]
        log_file_name_wo_ext = os.path.splitext(log_file_name)[0]
        log_file_dir = os.path.split(log_file_path)[0]
        csv_file_dir = os.path.join(log_file_dir, log_file_name_wo_ext, 'CSV')
        csv_file_name = '{}_{}.csv'.format(log_file_name_wo_ext, csv_file_suffix)
        csv_file_path = os.path.join(csv_file_dir, csv_file_name)
        check_status = os.path.exists(csv_file_path) and os.path.isfile(csv_file_path)
    else:
        csv_file_path = None
        check_status = False
    return check_status, csv_file_path


def get_fs_log(app_name='ADXL'):
    err_stat, fs_ls_list = common.watch_shell.fs_ls()
    log_file_name = None
    csv_file_name = None
    if len(fs_ls_list) < 1:
        common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
        raise common.ConditionCheckFailure("\n\n" + '{}'.format('File not found in FS!'))
    elif len(fs_ls_list) == 1 and 'user_config' in fs_ls_list[0]['file'].lower():
        common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
        raise common.ConditionCheckFailure("\n\n" + '{}'.format('File not found in FS!'))
    elif len(fs_ls_list) == 2 and not ('user_config' in fs_ls_list[0]['file'].lower() or
                                       'user_config' in fs_ls_list[1]['file'].lower()):
        common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
        raise common.ConditionCheckFailure("\n\n" + '{}'.format('Multiple files found for a single stream!'))
    elif len(fs_ls_list) > 2:
        common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
        raise common.ConditionCheckFailure("\n\n" + '{}'.format('Multiple files found for a single stream!'))
    else:
        log_file_name = None
        for fs_dict in fs_ls_list:
            if 'user_config' not in fs_dict['file'].lower():
                log_file_name = fs_dict['file']
                break
        if not log_file_name:
            common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
            raise common.ConditionCheckFailure("\n\n" + '{}'.format('File not found in FS!'))
        else:
            common.test_logger.info("*** Log File Name - {} ***".format(log_file_name))
            common.watch_shell.do_download_file(log_file_name)
            csv_file_name = '{}.csv'.format(log_file_name.split('.')[0])
            if not (os.path.exists(log_file_name) and os.path.isfile(log_file_name)):
                common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
                raise common.ConditionCheckFailure("\n\n" + '{}'.format('FS log file was not found after download!'))
            elif os.stat(log_file_name).st_size == 0:
                common.test_logger.error('*** {} FS Stream Test - FAIL ***'.format(app_name))
                raise common.ConditionCheckFailure("\n\n" + '{}'.format('Empty FS Log file downloaded!'))
            os.system(r"LogConverter\LogConverter.exe .\{}".format(log_file_name))
            csv_file_name = {}
            csv_files_folder = os.path.join(os.getcwd(), log_file_name.split('.')[0], "CSV")
            files = os.listdir(csv_files_folder)
            for file in files:
                file_name_lower = file.lower()
                if "adpd" in file_name_lower and "slotfchannel2" in file_name_lower:
                    csv_file_name["adpd_g_ch_2"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "slotf" in file_name_lower:
                    csv_file_name["adpd_g"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "slotgchannel2" in file_name_lower:
                    csv_file_name["adpd_r_ch_2"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "slotg" in file_name_lower:
                    csv_file_name["adpd_r"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "slothchannel2" in file_name_lower:
                    csv_file_name["adpd_ir_ch_2"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "sloth" in file_name_lower:
                    csv_file_name["adpd_ir"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "slotichannel2" in file_name_lower:
                    csv_file_name["adpd_b_ch_2"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower and "sloti" in file_name_lower:
                    csv_file_name["adpd_b"] = os.path.join(csv_files_folder, file)
                elif "adpd" in file_name_lower:
                    csv_file_name["adpd"] = os.path.join(csv_files_folder, file)
                elif "adxl" in file_name_lower:
                    csv_file_name["adxl"] = os.path.join(csv_files_folder, file)
                elif "temperature" in file_name_lower:
                    csv_file_name["temperature"] = os.path.join(csv_files_folder, file)
                elif "syncppg" in file_name_lower:
                    csv_file_name["syncppg"] = os.path.join(csv_files_folder, file)
                elif "ppg" in file_name_lower:
                    csv_file_name["ppg"] = os.path.join(csv_files_folder, file)
                elif "ecg" in file_name_lower:
                    csv_file_name["ecg"] = os.path.join(csv_files_folder, file)
                elif "eda" in file_name_lower:
                    csv_file_name["eda"] = os.path.join(csv_files_folder, file)

    return log_file_name, csv_file_name


def write_reg_from_dcb(dev, dcb_name):
    curr_dir = os.getcwd()
    dcb_cfg_dir = os.path.join(curr_dir, 'dcb_cfg')
    dcb_file = os.path.join(dcb_cfg_dir, dcb_name)
    addr_list, val_list = get_dcb_data_list(dcb_file)
    for i, addr in enumerate(addr_list):
        common.watch_shell.do_reg('w {} 0x{}:0x{}'.format(dev, addr, val_list[i]))


def read_reg_and_verify_dcb(dev, dcb_name):
    curr_dir = os.getcwd()
    dcb_cfg_dir = os.path.join(curr_dir, 'dcb_cfg')
    dcb_file = os.path.join(dcb_cfg_dir, dcb_name)
    addr_list, val_list = get_dcb_data_list(dcb_file)
    mis_match_list = []
    for i, addr in enumerate(addr_list):
        err_stat, reg_val_list = common.watch_shell.reg_read('{} r 0x{}'.format(dev, addr))
        if int(val_list[i], 16) != int(reg_val_list[0][1], 16):
            mis_match_list.append(addr)
    return mis_match_list


def write_dcb(dev, dcb_file, test_name):
    err_stat, dcb_dir = common.dcb_cfg('w', dev, dcb_file)  # fs = 50Hz
    if err_stat:
        common.test_logger.error('*** {} - FAIL ***'.format(test_name))
        raise common.ConditionCheckFailure("\n\n" + 'DCB Write failed!')
    return err_stat, dcb_dir


def get_dcb_data_list(f_path):
    """

    :param f_path:
    :return:
    """
    with open(f_path, 'r') as f_ref:
        line_list = f_ref.readlines()
    addr_list, val_list = [], []
    for line in line_list:
        if line.strip() and line.strip()[0] != '#':
            addr_val_list = line.split(' ')
            addr_list.append(addr_val_list[0].strip())
            val_list.append(addr_val_list[1].strip())
    return addr_list, val_list


def compare_dcb_files(f1, f2):
    """

    :param f1:
    :param f2:
    :return:
    """
    f1_addr_list, f1_val_list = get_dcb_data_list(f1)
    f2_addr_list, f2_val_list = get_dcb_data_list(f2)
    f1_addr_list = [hex(int(f1_addr, 16)) for f1_addr in f1_addr_list]
    f1_val_list = [hex(int(f1_val, 16)) for f1_val in f1_val_list]
    f2_addr_list = [hex(int(f2_addr, 16)) for f2_addr in f2_addr_list]
    f2_val_list = [hex(int(f2_val, 16)) for f2_val in f2_val_list]
    err_stat = 0
    err_str = ''
    if len(f1_addr_list) != len(f2_addr_list):
        err_stat = 1
        err_str = 'DCB data size does not match!'
    else:
        for f1_idx, f1_addr in enumerate(f1_addr_list):
            if f1_addr in f2_addr_list:
                f2_idx = f2_addr_list.index(f1_addr)
                if f1_val_list[f1_idx] != f2_val_list[f2_idx]:
                    err_stat = 1
                    err_str = 'DCB values mismatch!'
                    break
            else:
                err_stat = 1
                err_str = 'DCB address mismatch!'
                break
    return err_stat, err_str


def quick_start_adpd_multi_led(samp_freq_hz=50, agc_state=0, skip_load_cfg=False):
    led_stream_file_dict = {}
    led_list = ['G', 'R', 'IR', 'B']
    cfg_dict = {'G': {'adpd_cfg': '1', 'clk_calib': common.adpd_clk_calib, 'sub': '6', 'agc_ctrl_id': '1'},
                'R': {'adpd_cfg': '2', 'clk_calib': common.adpd_clk_calib, 'sub': '7', 'agc_ctrl_id': '2'},
                'IR': {'adpd_cfg': '3', 'clk_calib': common.adpd_clk_calib, 'sub': '8', 'agc_ctrl_id': '3'},
                'B': {'adpd_cfg': '4', 'clk_calib': common.adpd_clk_calib, 'sub': '9', 'agc_ctrl_id': '4'},
                'MWL': {'adpd_cfg': '5', 'clk_calib':common.adpd_clk_calib, 'sub': '10', 'agc_ctrl_id': '5'}}

    for led in led_list:
        if agc_state:
            common.watch_shell.do_enable_agc('{}'.format(cfg_dict[led]['agc_ctrl_id']))
        else:
            common.watch_shell.do_disable_agc('{}'.format(cfg_dict[led]['agc_ctrl_id']))

    if not skip_load_cfg:
        for led in led_list:
            adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
            write_reg_from_dcb('adpd', adpd_led_dcb)
            time.sleep(.5)
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
    common.watch_shell.do_sensor("adpd start")
    for led in led_list:
        common.watch_shell.do_csv_log("adpd{} start".format(cfg_dict[led]['sub']))
        led_stream_file_dict[led] = 'adpd{}.csv'.format(cfg_dict[led]['sub'])
        time.sleep(2)
    for led in led_list:
        common.watch_shell.do_sub("adpd{} add".format(cfg_dict[led]['sub']))
    return led_stream_file_dict


def quick_start_ppg_multi_led(samp_freq_hz=50, agc_state=0):
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


    if samp_freq_hz == 50:
        common.watch_shell.do_reg("w adpd 0xD:0x4e20")
    elif samp_freq_hz == 100:
        common.watch_shell.do_reg("w adpd 0xD:0x2710")
    elif samp_freq_hz == 500:
        common.watch_shell.do_reg("w adpd 0xD:0x07D0")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")

    # for led in led_list:
    #     adpd_led_dcb = 'adpd_{}_dcb.dcfg'.format(led.lower())
    #     write_reg_from_dcb('adpd4000', adpd_led_dcb)
    #     time.sleep(.5)
    common.watch_shell.do_sensor("ppg start")
    common.watch_shell.do_csv_log("ppg start")
    common.watch_shell.do_csv_log("sync_ppg start")
    common.watch_shell.do_sub("ppg add")

def get_delta_ts_and_fs(ts_data_list, repeat_count, fs_div):
    if len(ts_data_list) > 100:
        ts_data_list = ts_data_list[100:]  # Removing first 100 samples to avoid initial junk value related issues
        ts_data_list.sort()
        median_start_idx = int(len(ts_data_list)/2)
        for i in range(50):
            if ts_data_list[median_start_idx+i] != ts_data_list[median_start_idx+i-1]:
                break
        if i == 49:
            raise Exception('Unable to calculate Delta TS. Invalid TS data list!')
        else:
            median_start_idx += i

        delta_ts = ts_data_list[median_start_idx + repeat_count] - ts_data_list[(median_start_idx + repeat_count) - 1]
        fs = repeat_count/(delta_ts/fs_div)
    else:
        delta_ts, fs = None, None
    return delta_ts, fs


def check_stream_data(file_path, stream='ecg', ch=1, exp_fs_hz=50, fs_log=False):
    """
    This function performs frequency calculations on stream data csv files
    :param file_path:
    :param stream:
    :param ch:
    :param exp_fs_hz:
    :return:
    """
    if fs_log:
        if stream == 'ecg' or stream == "eda" or stream == "temperature":
            ch0_col_idx = 0
            ch1_col_idx = 0
            row_offset = 3
        elif stream == 'syncppg' or stream == 'ppg':
            ch0_col_idx = 0
            ch1_col_idx = 2
            row_offset = 3
        elif stream == 'adpd':
            ch0_col_idx = 0
            ch1_col_idx = 0
            row_offset = 5
        elif stream == 'adpd_combined':
            ch0_col_idx = 1
            ch1_col_idx = 1
            row_offset = 5
        elif stream == 'adxl':
            ch0_col_idx = 0
            ch1_col_idx = 2
            row_offset = 3
        else:
            ch0_col_idx = 0
            ch1_col_idx = 0
            row_offset = 3
        div = 1000
    else:
        if stream == 'ecg':
            ch0_col_idx = 0
            ch1_col_idx = 0
            row_offset = 3
        elif stream == 'syncppg':
            ch0_col_idx = 0
            ch1_col_idx = 2
            row_offset = 1
        elif stream == 'adpd':
            ch0_col_idx = 1
            ch1_col_idx = 1
            row_offset = 5
        elif stream == 'adxl':
            ch0_col_idx = 0
            ch1_col_idx = 2
            row_offset = 3
        else:
            ch0_col_idx = 0
            ch1_col_idx = 2
            row_offset = 3
        div = 1000
    col_idx = ch0_col_idx if ch == 1 else ch1_col_idx
    time_data_list = common.read_csv_col(file_path, col_idx, row_offset)
    i = 1
    for i, ts in enumerate(time_data_list):  # calculate repeat count
        if ts != time_data_list[0]:
            break
    repeat_count = i

    loop_iter = int(len(time_data_list)/repeat_count) - 1
    delta_ts_sum = 0
    ts_mismatch_count = 0
    ref_delta_ts, ref_fs = get_delta_ts_and_fs(time_data_list, repeat_count, div)
    for i in range(1, loop_iter+1):
        delta_ts = time_data_list[i*repeat_count] - time_data_list[(i*repeat_count)-1]

        if not ref_delta_ts and i == 1:
            ref_delta_ts = delta_ts
            common.test_logger.warning('Median Ref Delta TS calculation Failed. Switching to start index calculation!')
        else:
            if not(0.9 * ref_delta_ts <= delta_ts <= 1.1 * ref_delta_ts):
                ts_mismatch_count += 1
            else:
                delta_ts_sum += delta_ts
    avg_delta_ts = delta_ts_sum/(i-ts_mismatch_count)
    if avg_delta_ts > 0:
        avg_fs = repeat_count/(avg_delta_ts/div)
    else:
        avg_fs = 0
    if time_data_list:
        total_time = (time_data_list[-1] - time_data_list[0]) / div
        expected_num_samples = total_time * avg_fs
        actual_num_samples = len(time_data_list)
        sample_loss = False
        excess_samples = False
        num_samples_lost = 0
        num_excess_samples = 0
        if actual_num_samples > expected_num_samples * 1.02:
            excess_samples = True
            num_excess_samples = actual_num_samples - expected_num_samples
        elif 0.98 * expected_num_samples > actual_num_samples:
            sample_loss = True
            num_samples_lost = expected_num_samples-actual_num_samples
        freq_check_dict = {'avg_fs': avg_fs, 'ts_mismatch_count': ts_mismatch_count,
                           'sample_loss': sample_loss, 'num_samples_lost': num_samples_lost,
                           'excess_samples': excess_samples, 'num_excess_samples': num_excess_samples}
        if freq_check_dict["ts_mismatch_count"] <= common.ts_mismatch_tolerance and \
                freq_check_dict["ts_mismatch_count"] != 0:
            common.test_logger.warning("***TS Mismatch {}***".format(freq_check_dict["ts_mismatch_count"]))

        if exp_fs_hz <= 10:
            freq_value = (exp_fs_hz * 0.9 >= freq_check_dict['avg_fs']) or \
                         (freq_check_dict['avg_fs'] >= exp_fs_hz * 1.1)
        elif exp_fs_hz <= 50:
            freq_value = (exp_fs_hz * 0.95 >= freq_check_dict['avg_fs']) or \
                         (freq_check_dict['avg_fs'] >= exp_fs_hz * 1.05)
        else:
            freq_value = (exp_fs_hz * 0.98 >= freq_check_dict['avg_fs']) or \
                         (freq_check_dict['avg_fs'] >= exp_fs_hz * 1.02)

        if freq_value or \
           freq_check_dict["ts_mismatch_count"] > common.ts_mismatch_tolerance or \
           freq_check_dict['sample_loss'] or \
           freq_check_dict['excess_samples']:
            err_str = 'AvgFs(Hz)= {} | SamplesLost={} | ExcessSamples={} | TimeStampMismatch={}'.format(freq_check_dict['avg_fs'],
                                                                                                        freq_check_dict['num_samples_lost'],
                                                                                                        freq_check_dict['num_excess_samples'],
                                                                                                        freq_check_dict['ts_mismatch_count'])
            err_stat = True
        else:
            err_stat = False
            err_str = ''
    elif exp_fs_hz is None:
        freq_check_dict = {'avg_fs': None, 'ts_mismatch_count': None,
                           'sample_loss': None, 'num_samples_lost': None,
                           'excess_samples': None, 'num_excess_samples': None}
        err_stat = False
        err_str = ''
    else:
        freq_check_dict = {'avg_fs': None, 'ts_mismatch_count': None,
                           'sample_loss': None, 'num_samples_lost': None,
                           'excess_samples': None, 'num_excess_samples': None}
        err_stat, err_str = True, 'Empty or invalid stream file!'
    return err_stat, err_str, freq_check_dict


def dev_id_test(dev):
    """

    :return:
    """
    dev_idx_dict = {'ADXL362': (1, 0xf2), 'ADPD4K': (2, [0xc0, 0x1c2]), 'ADP5360': (3, 0x10),
                    'AD5940': (4, 0x5502), 'NAND_FLASH': (5, 0x35), 'AD7156': (6, 0x88)}
    chip_id_exp = 0xff
    if dev.upper() in dev_idx_dict:
        dev_idx = dev_idx_dict[dev.upper()][0]
        if dev.upper() == "ADPD4K":
            chip_id_exp = dev_idx_dict[dev.upper()][1][common.DVT_version]
        else:
            chip_id_exp = dev_idx_dict[dev.upper()][1]
        err_stat, chip_id = common.watch_shell.get_chip_id('{}'.format(dev_idx))
        print(chip_id_exp)
    else:
        err_stat = 1
        chip_id = None
    if err_stat:
        common.test_logger.error('*** {} Chip ID Test - FAIL ***'.format(dev.upper()))
        raise common.ConditionCheckFailure("\n\n" + 'Unable to retrieve ADPD Chip ID!')
    elif chip_id != chip_id_exp:
        common.test_logger.error('*** {} Chip ID Test - FAIL ***'.format(dev.upper()))
        raise common.ConditionCheckFailure("\n\n" + 'Read Chip ID {} does not match with {} chip ID {}!'.format(hex(chip_id), dev.upper(), hex(chip_id_exp)))
    else:
        common.test_logger.info('{} Chip ID: {}'.format(dev.upper(), hex(chip_id)))


def dcb_test(dev='ADPD4K', dcb_file='adpd_qa_dcb.dcfg',
             dcb_read_file='adpd4000_dcb_get.dcfg', test_name='ADPD DCB Test'):
    """

    :return:
    """
    dev_id = dev_id_dict[dev]
    common.dcb_cfg('d', dev_id)  # Deleting any previous DCB
    write_dcb(dev_id, dcb_file, test_name)
    if dev_id == 'adpd':
        common.watch_shell.do_load_adpd_cfg("1")
    elif dev_id == 'adxl':
        common.watch_shell.do_load_adxl_cfg("1")
    elif dev_id == "ecg":
        common.watch_shell.do_write_dcb_to_lcfg("ecg")
    elif dev_id == "eda":
        common.watch_shell.do_write_dcb_to_lcfg("eda")
    elif dev_id == "bcm":
        common.watch_shell.do_write_dcb_to_lcfg("bcm")

    err_stat, dcb_dir = common.dcb_cfg('r', dev_id)
    if err_stat:
        common.test_logger.error('*** {} - FAIL ***'.format(test_name))
        raise common.ConditionCheckFailure("\n\n" + 'DCB Read failed!')

    write_dcb_file = os.path.join(dcb_dir, dcb_file)
    read_dcb_file = os.path.join(dcb_dir, dcb_read_file)
    err_stat, err_str = compare_dcb_files(write_dcb_file, read_dcb_file)
    if err_stat:
        common.test_logger.error('*** {} - FAIL ***'.format(test_name))
        raise common.ConditionCheckFailure("\n\n" + 'Read DCB does not match with '
                                                    'the write DCB!\nDetails:{}'.format(err_str))

    err_stat, mismatch = check_dcb(dev, dcb_file)
    if err_stat or mismatch:
        common.test_logger.error('*** {} - FAIL ***'.format(test_name))
        raise common.ConditionCheckFailure("\n\n" + 'DCB Write failed or Register '
                                                    'values does not match with DCB values!')

    err_stat, dcb_dir = common.dcb_cfg('d', dev_id)
    if err_stat:
        common.test_logger.error('*** {} - FAIL ***'.format(test_name))
        raise common.ConditionCheckFailure("\n\n" + 'DCB Delete failed!')


def check_dcb(dev, dcb_name):
    err_stat = 0
    dcb_val_mismatch = False
    if dev in dev_id_dict.keys():
        dev_id = dev_id_dict[dev]
        dcb_path = os.path.join('.', 'dcb_cfg', dcb_name)
        with open(dcb_path, 'r') as f_ref:
            line_list = f_ref.readlines()
        for i, data in enumerate(line_list):
            if data.strip():
                if data.strip()[0] != '#':
                    addr = data.split(' ')[0]
                    val = int(data.split(' ')[1].strip(), 16)
                    if dev_id.lower() == "ecg" or dev_id.lower() == "eda" or dev_id.lower() == "bcm":
                        packet = common.watch_shell.do_lcfg("r {} 0x{}".format(dev_id, addr))
                        reg_val_list = packet["payload"]["data"]
                    # elif dev_id.lower() == "eda":
                    #     status, reg_val_list = common.watch_shell.do_lcfgEdaRead(addr)
                    else:
                        packet = common.watch_shell.do_reg('r {} 0x{}'.format(dev_id, addr))
                        reg_val_list = packet["payload"]["data"]
                    if reg_val_list:
                        reg_val = int(reg_val_list[0][1], 16)
                        if reg_val != val:
                            dcb_val_mismatch = True
                            common.test_logger.error('Expected: Addr={} Val={} | Actual: Addr={} Val={}'.format(addr, val,
                                                                                                            addr,
                                                                                                            reg_val))
    else:
        err_stat = 1
    return err_stat, dcb_val_mismatch


def enable_ecg_without_electrodes_contact():
    common.watch_shell.do_lcfg("w ecg 0x3:0x0")
