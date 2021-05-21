import os
import math
import numpy as np
import common


def check_stream_status(stream, results_dict, ref_dict):
    """
    :param stream: 'ecg' | 'ppg'
    :param results_dict:
    :param ref_dict:
    :return:
    """
    status = True
    if stream == 'ecg':
        ref_hr = ref_dict['ref_hr']
        ref_qrs_amp = ref_dict['ref_qrs_amp']
        qrs_amp = float(results_dict['qrs_amp_mean'])
        qrs_time = float(results_dict['qrs_time_mean'])
        hr = float(results_dict['hr_mean'])
        if (abs(qrs_amp - ref_qrs_amp) > ref_qrs_amp * 0.1) or \
                (abs(hr - ref_hr) > ref_hr * 0.2) or \
                (0 <= qrs_time <= 0.2):
            status = False
    return status


def check_standalone_vs_usecase_results(stream, sa_results_dict, uc_results_dict, variance=0.1):
    """

    :param stream:
    :param sa_results_dict:
    :param uc_results_dict:
    :param variance:
    :return:
    """
    status = True
    if stream == 'ecg':
        sa_qrs_amp = float(sa_results_dict['qrs_amp_mean'])
        sa_hr = float(sa_results_dict['hr_mean'])
        sa_qrs_time = float(sa_results_dict['qrs_time_mean'])
        uc_qrs_amp = float(uc_results_dict['qrs_amp_mean'])
        uc_hr = float(uc_results_dict['hr_mean'])
        uc_qrs_time = float(uc_results_dict['qrs_time_mean'])
        if (abs(uc_qrs_amp - sa_qrs_amp) > sa_qrs_amp * variance) or \
           (abs(uc_hr - sa_hr) > sa_hr * variance) or \
           (abs(uc_qrs_time - sa_qrs_time) > sa_qrs_time * variance):
            status = False
    elif stream == 'ppg':
        pass
    return status


def clear_pkt_loss():
    """

    :return:
    """
    pkt_loss_file_path = os.path.join(os.getcwd(), 'pkt_loss.yaml')
    if os.path.isfile(pkt_loss_file_path):
        os.remove(pkt_loss_file_path)
    return pkt_loss_file_path


def check_sample_loss(file_path, stream='ecg', data_offset=1, col_idx=-1):
    """

    :param file_path:
    :param stream:
    :param data_offset:
    :param col_idx:
    :return:
    """
    sample_loss_idx_list = []
    with open(file_path, 'r') as f_ref:
        line_list = f_ref.readlines()

    if stream == 'ecg':
        col_idx = 0 if col_idx == -1 else col_idx
        # Extracting ts delta from csv file
        ts_delta_list = [float(line.split(',')[col_idx].strip()) - float(line_list[i - 1].split(',')[col_idx].strip())
                         for i, line in enumerate(line_list) if i > data_offset]
        ref_ts_delta = ts_delta_list[0]
        sample_loss_idx_list = [i+1+data_offset for i, ts_delta in enumerate(ts_delta_list) if abs(ref_ts_delta-ts_delta) > (ref_ts_delta*0.1)]
    elif stream == 'adpd':
        col_idx = 0 if col_idx == -1 else col_idx
        ts_data_list = [float(line.split(',')[col_idx].strip()) for i, line in enumerate(line_list) if i >= data_offset]
        for i, ts_data in enumerate(ts_data_list):
            if ts_data != ts_data_list[0]:
                break
        repeat_count = i
        loop_count = len(ts_data_list)/repeat_count
        ref_delta_ts = None
        for i in range(loop_count):
            if i:
                delta_ts = ts_data_list[i*repeat_count] - ts_data_list[(i*repeat_count)-1]
                if i == 1:
                    ref_delta_ts = delta_ts
                else:
                    if abs(delta_ts - ref_delta_ts) > ref_delta_ts * 0.1:
                        sample_loss_idx_list.append((i+1)*repeat_count)
    else:
        sample_loss_idx_list = [-1]
        common.logging.error("Unsupported Stream data selected for sample loss check!")

    sample_loss = True if sample_loss_idx_list else False
    return sample_loss, sample_loss_idx_list


def check_pkt_loss():
    """

    :param file_path:
    :param col_idx:
    :return:
    """
    '''
    with open(file_path, 'r') as f_ref:
        line_list = f_ref.readlines()
    # Extracting ts delta from csv file
    ts_delta_list = [int(line.split(',')[col_idx].strip())-int(line_list[i-1].split(',')[col_idx].strip())
                     for i, line in enumerate(line_list) if i > 1]
    avg_ts_delta = sum(ts_delta_list)/len(ts_delta_list)
    # Checking for packet loss
    if ((max(ts_delta_list) - avg_ts_delta) < 0.05*max(ts_delta_list) and
       (avg_ts_delta - min(ts_delta_list)) < 0.05*min(ts_delta_list)):
        pkt_loss = False
    else:
        pkt_loss = True
    '''
    pkt_loss_file_path = os.path.join(os.getcwd(), 'pkt_loss.yaml')
    failure_streams_str = ''
    pkt_loss = False
    if os.path.isfile(pkt_loss_file_path):  # This file will be generated with results by CLI if pkt loss occurs
        with open(pkt_loss_file_path, 'r') as f_ref:
            pkt_loss_dict = common.yaml.load(f_ref, Loader=common.yaml.FullLoader)
        if True in pkt_loss_dict.itervalues():
            pkt_loss = True
            failure_list = [k[1:] for k, v in pkt_loss_dict.items() if v]
            failure_streams_str = ', '.join(failure_list)
    return pkt_loss, failure_streams_str


def calc_mean(f_path, col_idx=1):
    """

    :param f_path:
    :param col_idx:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    data_list = [float(data) for data in data_list]
    data_len = len(data_list)
    data_mean = sum(data_list)/data_len
    return data_mean


def calc_std_dev(f_path, col_idx=1):
    """

    :param f_path:
    :param col_idx:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    data_list = [float(data) for data in data_list]
    '''
    data_len = len(data_list)
    data_mean = sum(data_list)/data_len
    std_dev2 = np.sqrt(sum([(data-data_mean)**2 for data in data_list])/data_len)
    '''
    std_dev = np.std(data_list)
    return std_dev


def check_temp_range(f_path, temp_range=(35, 39), col_idx=1):
    """

    :param f_path:
    :param temp_range:
    :param col_idx:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    data_list = [float(data) for data in data_list]
    temp_in_range = True
    out_of_range_count = 0
    out_of_range_percent = 0
    for temp_data in data_list:
        if not (temp_range[0] <= temp_data <= temp_range[0]):
            out_of_range_count += 1
            temp_in_range = False
    if not temp_in_range:
        out_of_range_percent = (out_of_range_count/len(data_list))*100
    return temp_in_range, out_of_range_count, out_of_range_percent


def calc_cmrr(f_path, col_idx=2, vin=0.3):
    """
    This function calculates and returns the cmrr value of the input data_list
    :param f_path:
    :param vin:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)

    vdata = [1.835*((data/2**15)-1)+1.11 for data in data_list]
    vdata = np.array(vdata)
    vrms = np.sqrt(np.mean(vdata ** 2))
    vin_rms = (vin / 99) * 0.353
    cm = vrms / vin_rms
    cmrr = 20 * math.log(cm, 10)
    return cmrr


def calc_dr(f_path, vin=0.5, col_idx=2, gain_8233=150, data_range={'start': 199, 'end': 1528}):
    """
    This function calculates and returns the gain, gain error and gain error percentage value of the input data_list
    :param f_path:
    :param vin:
    :param col_idx:
    :param gain_8233:
    :param data_range:
    :return:
    gain, gain_err, gain_err_percent
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    vdata = [(((float(data)/2**15) - 1) * 1.835) + 1.11
             for data in data_list[data_range['start']:data_range['end']]]
    vpp = max(vdata) - min(vdata)
    gain = vpp / (float(vin) / 99.0)
    gain_err = gain_8233 - gain
    gain_err_percent = (gain_err / gain) * 100
    return gain, gain_err, gain_err_percent


def calc_noise(f_path, col_idx=2, data_range={'start': 299, 'end': 999}):
    """
    This function calculates and returns the v_noise
    :param f_path:
    :param data_range:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    vdata = [(((float(data) / 2 ** 15) - 1) * 1.835) + 1.11
             for data in data_list[data_range['start']:data_range['end']]]
    vpp = max(vdata) - min(vdata)
    v_noise = vpp / 150
    return v_noise


def calc_vpp(f_path,  col_idx=2, data_range={'start': 299, 'end': 999}):
    """

    :param f_path:
    :param data_range:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=col_idx)
    vdata = [(((float(data) / 2 ** 15) - 1) * 1.835) + 1.11
                for data in data_list[data_range['start']:data_range['end']]]
    vpp = max(vdata) - min(vdata)
    return vpp


def calc_swith_and_noise(sw_on_f_path, sw_off_f_path, noise_f_path,
                         data_range={'start': 299, 'end': 999},
                         noise_data_range={'start': 199, 'end': 899}):
    """

    :param sw_on_f_path:
    :param sw_off_f_path:
    :param noise_f_path:
    :param data_range:
    :param noise_data_range:
    :return:
    """
    on_data_list = common.read_csv_col(sw_on_f_path, col_idx=1)
    on_vdata = [(((float(data)/2**15) - 1) * 1.835) + 1.11
                for data in on_data_list[data_range['start']:data_range['end']]]
    vpp_swith_on = (max(on_vdata) - min(on_vdata)) * 99/150

    off_data_list = common.read_csv_col(sw_off_f_path, col_idx=1)
    off_vdata = [(((float(data)/2**15) - 1) * 1.835) + 1.11
                 for data in off_data_list[data_range['start']:data_range['end']]]
    vpp_swith_off = (max(off_vdata) - min(off_vdata)) * 99/150

    noise_data_list = common.read_csv_col(noise_f_path, col_idx=1)
    noise_vdata = [(((float(data)/2**15) - 1) * 1.835) + 1.11
                   for data in noise_data_list[noise_data_range['start']:noise_data_range['end']]]
    vpp_noise = max(noise_vdata) - min(noise_vdata)
    v_noise = vpp_noise/150
    return vpp_swith_on, vpp_swith_off, vpp_noise, v_noise


def check_battery_charge(f_path, curr_threshold=17e-3):
    """

    :param f_path:
    :param curr_threshold:
    :return:
    """
    data_list = common.read_csv_col(f_path, col_idx=0)
    check_pass = True
    failure_curr = None
    avg_curr = sum(data_list)/len(data_list)
    for data in data_list:
        if abs(data) > curr_threshold:
            check_pass = False
            failure_curr = data
            break
    return check_pass, avg_curr, failure_curr



if __name__ == '__main__':
    f_path = r'C:\Users\nphilip\OneDrive - Analog Devices, Inc\Documents\MATLAB\A11H11_ECG_test_result\EcgAppStream_SwitchON.csv'
    print calc_cmrr(f_path)

    f_path = r'C:\Users\nphilip\OneDrive - Analog Devices, Inc\Documents\MATLAB\A3H3_ECG_test_result\EcgAppStream_DR_5Hz_0.5Vpp.csv'
    print(calc_dr(f_path))

    sw_on_f_path = r'C:\Users\nphilip\OneDrive - Analog Devices, Inc\Documents\MATLAB\A11H11_ECG_test_result\EcgAppStream_SwitchON.csv'
    sw_off_f_path = r'C:\Users\nphilip\OneDrive - Analog Devices, Inc\Documents\MATLAB\A11H11_ECG_test_result\EcgAppStream_SwitchOFF.csv'
    noise_f_path = r'C:\Users\nphilip\OneDrive - Analog Devices, Inc\Documents\MATLAB\EcgAppStream_ad8233_noise.csv'
    print(calc_swith_and_noise(sw_on_f_path, sw_off_f_path, noise_f_path))
