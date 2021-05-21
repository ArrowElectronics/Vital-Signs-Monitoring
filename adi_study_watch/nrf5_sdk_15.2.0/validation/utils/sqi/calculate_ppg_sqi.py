# Example Application of SQI Algorithm
# Author: Miles Bennett
# Analog Devices

# import libraries
import os
import re
import numpy as np
import pandas as pd
from ppg_sqi.config import data_dir
from ppg_sqi.config import logistic_params_dir
import matplotlib.pyplot as plt
import joblib
from ppg_sqi.utils.ppg_preprocessor import PPG_Preprocessor

def calc_sampling_frequency(df):
    for i, ts in enumerate(df['TimeStamp_ch1']):
        if ts != df['TimeStamp_ch1'][0]:
            break
    repeat_count = i
    t0 = df["TimeStamp_ch1"][i-1]
    t1 = df["TimeStamp_ch1"][i]
    fs = int(repeat_count/((t1-t0)/32000.0))
    ref_fs_list = [50,100,250,500]
    delta_fs_list = [abs(i-fs) for i in ref_fs_list]
    fs_idx = delta_fs_list.index(min(delta_fs_list))
    fs = ref_fs_list[fs_idx]
    return fs

def calc_sqi_channel(X, clf, fs_in, fs_out, time_window):

    # calc window length in samples
    w_len = int(fs_in*time_window)

    # calculate number of time windows
    w_num = int(np.floor(len(X)/w_len))

    # reshape X with respect to front alignment
    X_mat_0 = np.reshape(X[0:w_num*w_len], (w_num, w_len))

    # reshape X with respect to back alignment
    X_mat_1 = np.reshape(X[-w_num*w_len:], (w_num, w_len))

    # combine results
    X_mat = np.vstack([X_mat_0, X_mat_1])

    # preprocess data
    ppg_preprocessor = PPG_Preprocessor(fs_in=fs_in, fs_out=fs_out)
    X_mat = ppg_preprocessor(X_mat)

    # classifier each data segment
    y_pred = clf.predict(X_mat)/2

    # return mean SQI for all windows
    return np.mean(y_pred)

def calc_sqi(filename, clf=None, fs_out=25, time_window=5.12):

    # load data
    df = pd.read_csv(filename)

    # get sample frequency
    fs = calc_sampling_frequency(df)

    if clf is None:
        # load default classifier
        clf_dir = logistic_params_dir / "default"
        clf = joblib.load(clf_dir/"clf.joblib")

    # calculate sqi for each channel
    sqi_ch0 = calc_sqi_channel(df[" adpd_data_SlotF_ch1"].values, clf, fs, fs_out, time_window)
    sqi_ch1 = calc_sqi_channel(df[" adpd_data_SlotF_ch2"].values, clf, fs, fs_out, time_window)

    # return dictionary with keys for each channel
    return {
        "SQI_Ch0": sqi_ch0,
        "SQI_Ch1": sqi_ch1
    }

def plot_data(filename, title):
    
    # load data for plotting
    df = pd.read_csv(filename)

    # plot channels
    fig1, ax1 = plt.subplots()
    ax1.plot(df["TimeStamp_ch1"], df[" adpd_data_SlotF_ch1"], label="Ch0")
    ax1.plot(df[" TimeStamp_ch2"], df[" adpd_data_SlotF_ch2"], label="Ch1")
    ax1.set_title(title)
    ax1.set_xlabel("Time [s]")
    ax1.set_ylabel("Amplitude")
    ax1.legend()


def generate_ppg_sqi_report(log_dir, sorted_report=True):
    """
    
    :param log_dir: 
    :param sorted_report: 
    :return: 
    """
    fenda_data = data_dir / "fenda_watch_sqi"
    results_dict = {}
    agc_off_list = []
    for log_file in os.listdir(log_dir):
        print (os.path.join(log_dir, log_file))
        if 'adpd6Stream' in log_file and '.csv' in log_file:
            if 'agc_off' in log_file:
                agc_off_list.append(log_file)
            log_file_path = os.path.join(log_dir, log_file)
            full_log_file_path = fenda_data / log_file_path
            # calc SQI
            sqi_data = calc_sqi(full_log_file_path)
            results_dict[log_file] = sqi_data
            # print(log_file+':')
            # for key, val in sqi_data.items():
            #     print("{0}: {1:0.3f}".format(key, val))
    if sorted_report:
        report_path = os.path.join(log_dir, 'ppg_sqi_analysis_report.txt')
        log_folder_name = os.path.split(log_dir)[-1]
        with open(report_path, 'w') as f_ref:
            f_ref.write('{}:\n'.format(log_folder_name))
            # f_ref.write('Iteration Idx,CH0 - AGC OFF,CH1 - AGC OFF,CH0 - AGC ON,CH1 - AGC ON\n')
            for agc_off_file in agc_off_list:
                src_obj = re.search(r'adpd6Stream_agc_off_iteration-(\d*)?.csv', agc_off_file)
                if src_obj:
                    idx = src_obj.group(1)
                    agc_on_file = 'adpd6Stream_agc_on_iteration-{}.csv'.format(idx)
                    if agc_on_file in results_dict:
                        '''
                        f_ref.write('{0},{1:0.3f},{1:0.3f},{1:0.3f},{1:0.3f}\n'.format(idx,
                                                                                 results_dict[agc_off_file]['SQI_Ch0'],
                                                                                 results_dict[agc_off_file]['SQI_Ch1'],
                                                                                 results_dict[agc_on_file]['SQI_Ch0'],
                                                                                 results_dict[agc_on_file]['SQI_Ch1']))
                        '''
                        f_ref.write('\nIteration {} - AGC OFF:\n'.format(idx))
                        for key, val in results_dict[agc_off_file].items():
                            f_ref.write("{0}: {1:0.3f}\n".format(key, val))
                        f_ref.write('Iteration {} - AGC ON:\n'.format(idx))
                        for key, val in results_dict[agc_on_file].items():
                            f_ref.write("{0}: {1:0.3f}\n".format(key, val))


def calc_ppg_sqi(log_file_path, print_enabled=True):
    """

    :param log_file_path:
    :param print_enabled:
    :return:
    """
    fenda_data = data_dir / "fenda_watch_sqi"
    full_log_file_path = fenda_data / log_file_path
    # calc SQI
    sqi_data = calc_sqi(full_log_file_path)
    if print_enabled:
        print("SQI Results:")
        for key, val in sqi_data.items():
            print("{0} : {1:0.3f}".format(key, val))
    return sqi_data

if __name__ == "__main__":
    log_dir_list = [r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\55-8C-DA-D8-15-E7 - 50Hz',
                    r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\55-8C-DA-D8-15-E7 - 100Hz',
                    r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\DD-10-51-93-CC-C5 - 500Hz']
    file_path = r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\55-8C-DA-D8-15-E7 - 100Hz\adpd6Stream_agc_off_iteration-1.csv'
    # result = calc_ppg_sqi(file_path)
    for log_dir in log_dir_list:
        generate_ppg_sqi_report(log_dir)
    r"""
    # set data directories
    # Note: Paths are being processed implicitly using Pathlib
    fenda_data = data_dir / "fenda_watch_sqi"
    
    # get data
    file_agc_off = r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\55-8C-DA-D8-15-E7 - 100Hz\adpd6Stream_agc_off_iteration-2.csv' #"adpd6Stream_agc_off_Watch_No_1_updated.csv"
    file_agc_on = r'C:\Nitin\adi_projects_offline\study_watch\ppg_data\55-8C-DA-D8-15-E7 - 100Hz\adpd6Stream_agc_on_iteration_On-2.csv' #"adpd6Stream_agc_on_Watch_No_1_updated.csv"

    full_file_agc_off = fenda_data / file_agc_off
    full_file_agc_on = fenda_data / file_agc_on

    # calc SQI
    agc_off_sqi_data=calc_sqi(full_file_agc_off)
    agc_on_sqi_data=calc_sqi(full_file_agc_on)

    # print results for AGC off
    print("Results for AGC Off")
    for key, val in agc_off_sqi_data.items():
        print("{0} : {1:0.3f}".format(key, val))

    # print results for AGC On
    print("")
    print("Results for AGC On")
    for key, val in agc_on_sqi_data.items():
        print("{0} : {1:0.3f}".format(key, val))

    # plot datafiles (optional)
    plot_data(full_file_agc_off, "AGC Off")
    plot_data(full_file_agc_on, "AGC On")
    plt.show()
    """

