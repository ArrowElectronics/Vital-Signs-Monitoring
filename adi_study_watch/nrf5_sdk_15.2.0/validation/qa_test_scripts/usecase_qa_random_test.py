import random
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
from usecase_qa_test import *
from utils import rand_utils


def use_case_qa_ppg_randomized_test(iterate=1):
    """
    Use Case - 1 Randomized
    :param iterate: Maximum iterate value is 6
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ppg_fs_randomized_test(iterate=1):
    """
    Use Case - 1 Randomized
    :param iterate: Maximum iterate value is 6
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "temperature", "adpd"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg_fs(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ppg_eda_randomized_test(iterate=1):  # 2,0,1,3 -sequence not working
    """
    Use Case - 2 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg_eda(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ppg_eda_fs_randomized_test(iterate=1):  # 2,0,1,3 -sequence not working
    """
    Use Case - 2 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["adxl", "adpd", "temperature", "eda"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg_eda_fs(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ppg_ecg_randomized_test(iterate=1):  # 1,3,2,0
    """
    Use Case - 3 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg_ecg(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ppg_ecg_fs_randomized_test(iterate=1):  # 1,3,2,0
    """
    Use Case - 3 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "adxl", "adpd", "temperature"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ppg_ecg_fs(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ecg_randomized_test(iterate=1):
    """
    Use Case - 4 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "ppg", "temperature"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "temperature", "ppg"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ecg(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_qa_ecg_fs_randomized_test(iterate=1):
    """
    Use Case - 4 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["ecg", "ppg", "temperature"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["ecg", "temperature", "ppg"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_qa_ecg_fs(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_5_randomized_test(iterate=1):
    """
    Use Case - 5 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_5(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_5_fs_randomized_test(iterate=1):
    """
    Use Case - 5 Randomized
    :param iterate: Maximum possible iteration is 24
    :return:
    """
    start_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                  include_non_randomized=False, all_possible_combination=False,
                                                  iteration=iterate)
    stop_stream_list = rand_utils.randomize_list(stream_list=["multi_led_adpd", "adxl"],
                                                 include_non_randomized=False, all_possible_combination=False,
                                                 iteration=iterate)
    for index in range(iterate):
        use_case_5_fs(start_stream=start_stream_list[index], stop_stream=stop_stream_list[index])


def use_case_randomized_test(iterate=1):
    indices = range(5)
    indices_list = [tuple(indices)]
    for _ in range(iterate):
        while 1:
            random.shuffle(indices)
            if indices not in indices_list:
                indices_list.append(tuple(indices))
                break
        common.logging.info("Order of index is {}".format(indices))
        for index in indices:
            if index == 0:
                use_case_qa_ppg()
            elif index == 1:
                use_case_qa_ppg_eda()
            elif index == 2:
                use_case_qa_ppg_ecg()
            elif index == 3:
                use_case_qa_ecg()
            elif index == 4:
                use_case_5()


def use_case_fs_randomized_test(iterate=1):
    indices = range(5)
    indices_list = [tuple(indices)]
    for _ in range(iterate):
        while 1:
            random.shuffle(indices)
            if indices not in indices_list:
                indices_list.append(tuple(indices))
                break
        common.logging.info("Order of index is {}".format(indices))
        for index in indices:
            if index == 0:
                use_case_qa_ppg_fs()
            elif index == 1:
                use_case_qa_ppg_eda_fs()
            elif index == 2:
                use_case_qa_ppg_ecg_fs()
            elif index == 3:
                use_case_qa_ecg_fs()
            elif index == 4:
                use_case_5_fs()


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    # use_case_qa_ecg()
    common.close_setup()
