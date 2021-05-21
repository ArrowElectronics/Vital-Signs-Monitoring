import random
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
from usecase_qa_random_test import *
from utils import rand_utils


def use_case_qa_ppg_all_combination_test():
    """
    Use Case - 1 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"], 
                                                 all_possible_combination=True)
    print(start_stream_list)
    print(stop_stream_list)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ppg_fs_all_combination_test():
    """
    Use Case - 1 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg_fs(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ppg_eda_all_combination_test():
    """
    Use Case - 2 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg_eda(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ppg_eda_fs_all_combination_test():
    """
    Use Case - 2 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg_eda_fs(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ppg_ecg_all_combination_test():
    """
    Use Case - 3 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg_ecg(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ppg_ecg_fs_all_combination_test():
    """
    Use Case - 3 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ppg_ecg_fs(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ecg_all_combination_test():
    """
    Use Case - 4 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "temperature", "ppg"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "temperature", "ppg"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ecg(start_stream=start_seq, stop_stream=stop_seq)


def use_case_qa_ecg_fs_all_combination_test():
    """
    Use Case - 4 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "temperature", "ppg"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "temperature", "ppg"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_qa_ecg_fs(start_stream=start_seq, stop_stream=stop_seq)


def use_case_5_all_combination_test():
    """
    Use Case - 5 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_5(start_stream=start_seq, stop_stream=stop_seq)


def use_case_5_fs_all_combination_test():
    """
    Use Case - 5 all_combination
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                  all_possible_combination=True)
    stop_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"], 
                                                 all_possible_combination=True)
    for start_seq, stop_seq in zip(start_stream_list, stop_stream_list):
        use_case_5_fs(start_stream=start_seq, stop_stream=stop_seq)


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    # use_case_5_all_combination_test()
    common.close_setup()