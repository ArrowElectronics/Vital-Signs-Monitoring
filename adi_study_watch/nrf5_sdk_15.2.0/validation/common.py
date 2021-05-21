import os
import sys

try:
    import tkinter
    import time
    import yaml
    from tkinter import messagebox
    from utils.serial_comm import SerialIface
    from datetime import datetime
    from utils.test_utils import util_logger
    from matplotlib import pyplot as plt
    from robot.libraries.BuiltIn import BuiltIn
    import shutil
    import inspect
    import logging
    import easygui
    import subprocess
    from utils import instr_lib
except Exception as e:
    print("Import Exception! Details:", e)


# Adding CLI destination path to sys path in order to import the module
curr_dir = os.path.join(os.path.abspath(__file__), '../')
cli_dir = os.path.join(curr_dir, '../adi_study_watch/cli/m2m2/tools')
sys.path.insert(0, cli_dir)
import CLI


# **********************************************************************
# Initializing TkInter for showing dialog pop ups
root = tkinter.Tk()
root.withdraw()
# **********************************************************************


# ********************** Test Variables ********************************
arduino_port = None  # This variable will be updated from station config file [read_station_Cfg()]
watch_port = None  # This variable will be updated from station config file [read_station_Cfg()]
fg_instr_addr = None  # This variable will be updated from station config file [read_station_Cfg()]
watch_port_type = None  # This variable will be updated from station config file [read_station_Cfg()]
sm_instr_addr = None  # This variable will be updated from station config file [read_station_Cfg()]
arduino = None
watch_shell = None
fg, sm = None, None
matlab_eng = None
pcb_name_default='A1H1'
shared_drive_path = r'\\wilmnas4\Local Programs\FDWatch_TestData\Data_Testsetup\DVT1_Test_Results'
ecg_stream_file_name = 'EcgAppStream.csv'
bcm_stream_file_name = "BcmAppStream.csv"
ppg_stream_file_name = 'ppgStream.csv'
syncppg_stream_file_name = 'syncppgStream.csv'
adxl_stream_file_name = 'adxlStream.csv'
temperature_stream_file_name = 'TempAppStream.csv'
adpd_stream_file_name = 'adpd6Stream.csv'
eda_stream_file_name = 'EdaAppStream.csv'
volt_scale_range = (0, 5)
# The switch map dictionary maps the various switches to the arduino digital pins (24-42)
switch_map = {'SNOISE1': 22, 'SNOISE2': 23, 'ECG_NEGSEL': 24, 'ECG_POSSEL': 25}
close_plot_mode_global = True
test_report_dir = None
copy_results_to_shared_drive = True
save_plots = False
DVT_version = None
# **********************************************************************

# ********************* Configure Logging ******************************
# logging_format = "[%(asctime)s] [%(levelname)s]: %(message)s"
logging_format = "[%(levelname)s] : %(message)s"
date_str = "%m/%d/%Y %I:%M:%S %p"
logging.basicConfig(# filename='output.log',
                    level=logging.INFO,
                    # filemode='w',
                    format=logging_format,
                    datefmt=date_str)
# **********************************************************************


# ********************* Common Functions *******************************
class ConditionCheckFailure(RuntimeError):
    """
    This class is used to raise failures from test cases so that
    robot framework detects them as failures and continues to
    next test case due to the below variable
    """
    ROBOT_CONTINUE_ON_FAILURE = True


def update_robot_suite_doc(doc):
    """

    :param doc:
    :return:
    """
    try:
        BuiltIn().set_suite_documentation(doc)
    except Exception as e:
        logging.warn('Skipping robot documentation update!')
        pass


def write_analysis_report(result_dict, report_file=None, header='Analysis Section', append_report=False):
    """

    :param result_dict:
    :param report_file:
    :param header:
    :param append_report:
    :return:
    """
    report_file = 'analysis_report.txt' if not report_file else report_file
    file_mode = 'a' if append_report else 'w'
    with open(report_file, file_mode) as f_ref:
        f_ref.write('<<< {} >>>\n'.format(header))
        for k, v in result_dict.iteritems():
            f_ref.write('{} = {}\n'.format(k, v))
        f_ref.write('\n'.format(header))
    return os.path.abspath(report_file)


def analyze_wfm(file_path, mode='ecg', file_mode='cli', gen_filtered_ppg='1'):
    """
    This function calls the BioSigProcess app built from LV vi and extracts results stored in yaml file
    :param file_path: waveform *.csv file path
    :param mode: 'ecg' | 'ppg'
    :param file_mode: 'cli' | 'awt'
    :return:
    """
    results_dict = {}
    if os.path.isfile(file_path):
        subprocess.call(['utils/lv/builds/bio_sig_process/BioSigProcess.exe', mode, file_path, file_mode, gen_filtered_ppg])
        time.sleep(2)
        result_file_path = os.path.join(file_path, '../{}_extracted_results.txt'.format(mode))
        with open(result_file_path, 'r') as f_ref:
            line_list = f_ref.readlines()
        for line in line_list:
            result_list = line.split(' - ')
            results_dict[result_list[0]] = result_list[1].strip()
    else:
        logging.warn('Input File not found! {}'.format(file_path))
        results_dict = None
    return results_dict


def quick_start_ecg(samp_freq_hz=100):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgEcgWrite('0:{}'.format(samp_freq_hz))
    watch_shell.do_sensor('ecg start')
    watch_shell.do_sub('recg add')


def quick_start_bcm(samp_freq_hz=100):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgBcmWrite('0:{}'.format(samp_freq_hz))
    watch_shell.do_sensor('bcm start')
    watch_shell.do_sub('rbcm add')



def set_ecg_stream_freq(samp_freq_hz=100):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgEcgWrite('0:{}'.format(samp_freq_hz))


def set_eda_stream_freq(samp_freq_hz=4):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgEdaWrite('0:{}'.format(samp_freq_hz))


def quick_start_eda(samp_freq_hz=4):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgEdaWrite('0:{}'.format(samp_freq_hz))
    watch_shell.do_sensor('eda start')
    watch_shell.do_sub('reda add')


def config_adpd_stream(samp_freq_hz=50, agc_state=0, led='G', skip_load_cfg=False):
    """

    :param samp_freq_hz:
    :param agc_state:
    :param led:
    :param skip_load_cfg:
    :return:
    """
    cfg_dict = {'G': {'adpd_cfg': '40', 'clk_calib': '6', 'sub': '6', 'agc_ctrl_id': '1'},
                'R': {'adpd_cfg': '41', 'clk_calib': '6', 'sub': '7', 'agc_ctrl_id': '2'},
                'IR': {'adpd_cfg': '42', 'clk_calib': '6', 'sub': '8', 'agc_ctrl_id': '3'},
                'B': {'adpd_cfg': '43', 'clk_calib': '6', 'sub': '9', 'agc_ctrl_id': '4'}}
    led = led.upper()
    if not skip_load_cfg:
        watch_shell.do_loadAdpdCfg(cfg_dict[led]['adpd_cfg'])
        watch_shell.do_clockCalibration(cfg_dict[led]['clk_calib'])
    if agc_state:
        watch_shell.do_adpdAGCControl('{}:1'.format(cfg_dict[led]['agc_ctrl_id']))
    else:
        watch_shell.do_adpdAGCControl('{}:0'.format(cfg_dict[led]['agc_ctrl_id']))

    if samp_freq_hz == 50:
        watch_shell.do_reg("w adpd4000 0xD:0x4e20")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adpd4000 0xD:0x2710")
    elif samp_freq_hz == 500:
        watch_shell.do_reg("w adpd4000 0xD:0x07D0")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")


def quick_start_adxl(samp_freq_hz=100):
    """
    Set ADXL sampling frequency and start capturing the data
    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_quickstart("adxl")

    if samp_freq_hz == 12.5:
        watch_shell.do_reg("w adxl 0x2C:0x98")
    elif samp_freq_hz == 25:
        watch_shell.do_reg("w adxl 0x2C:0x99")
    elif samp_freq_hz == 50:
        watch_shell.do_reg("w adxl 0x2C:0x9A")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adxl 0x2C:0x9B")
    elif samp_freq_hz == 200:
        watch_shell.do_reg("w adxl 0x2C:0x9C")
    elif samp_freq_hz == 400:
        watch_shell.do_reg("w adxl 0x2C:0x9F")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")

    watch_shell.do_plot("radxl")


def set_adxl_stream_freq(samp_freq_hz=100):
    """

    :param samp_freq_hz:
    :return:
    """
    if samp_freq_hz == 12.5:
        watch_shell.do_reg("w adxl 0x2C:0x98")
    elif samp_freq_hz == 25:
        watch_shell.do_reg("w adxl 0x2C:0x99")
    elif samp_freq_hz == 50:
        watch_shell.do_reg("w adxl 0x2C:0x9A")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adxl 0x2C:0x9B")
    elif samp_freq_hz == 200:
        watch_shell.do_reg("w adxl 0x2C:0x9C")
    elif samp_freq_hz == 400:
        watch_shell.do_reg("w adxl 0x2C:0x9F")
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")


def quick_start_adpd(samp_freq_hz=50, agc_state=0, led='G', skip_load_cfg=False):
    """

    :param samp_freq_hz:
    :param agc_state: [0 | 1]
    :param led: ['G' | 'R' | 'IR' | 'B']
    :return: stream_file_name
    """
    cfg_dict = {'G': {'adpd_cfg': '40', 'clk_calib': '6', 'sub': '6', 'agc_ctrl_id': '1'},
                'R': {'adpd_cfg': '41', 'clk_calib': '6', 'sub': '7', 'agc_ctrl_id': '2'},
                'IR': {'adpd_cfg': '42', 'clk_calib': '6', 'sub': '8', 'agc_ctrl_id': '3'},
                'B': {'adpd_cfg': '43', 'clk_calib': '6', 'sub': '9', 'agc_ctrl_id': '4'}}
    led = led.upper()
    if not skip_load_cfg:
        watch_shell.do_loadAdpdCfg(cfg_dict[led]['adpd_cfg'])
        watch_shell.do_clockCalibration(cfg_dict[led]['clk_calib'])
    if agc_state:
        watch_shell.do_adpdAGCControl('{}:1'.format(cfg_dict[led]['agc_ctrl_id']))
    else:
        watch_shell.do_adpdAGCControl('{}:0'.format(cfg_dict[led]['agc_ctrl_id']))

    if samp_freq_hz == 50:
        watch_shell.do_reg("w adpd4000 0xD:0x4e20")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adpd4000 0xD:0x2710")
    elif samp_freq_hz == 500:
        watch_shell.do_reg("w adpd4000 0xD:0x07D0")
    elif samp_freq_hz is None:
        pass
    else:
        raise RuntimeError("Sampling Frequency Not Supported!")

    watch_shell.do_sensor("adpd4000 start")
    watch_shell.do_sub("radpd{} add".format(cfg_dict[led]['sub']))

    watch_shell.do_plot("radpd{}".format(cfg_dict[led]['sub']))
    stream_file_name = 'adpd{}Stream.csv'.format(cfg_dict[led]['sub'])
    return stream_file_name


def quick_start_temperature():
    watch_shell.do_sensor('temperature start')
    watch_shell.do_sub('rtemperature add')
    watch_shell.do_plot('rtemperature')


def quick_stop_adpd(led='G'):
    """

    :param led: ['G' | 'R' | 'IR' | 'B']
    :return:
    """
    cfg_dict = {'G': {'qs_cmd': 'adpd4000_g'},
                'R': {'qs_cmd': 'adpd4000_r'},
                'IR': {'qs_cmd': 'adpd4000_ir'},
                'B': {'qs_cmd': 'adpd4000_b'}}
    led = led.upper()
    watch_shell.do_quickstop(cfg_dict[led]['qs_cmd'])


def quick_start_ppg(samp_freq_hz=50, agc_state=0):
    """

    :param samp_freq_hz:
    :param agc_state:
    :return:
    """
    watch_shell.do_loadAdpdCfg("40")
    watch_shell.do_clockCalibration("6")
    watch_shell.do_setPpgLcfg("40")
    if samp_freq_hz == 50:
        watch_shell.do_reg("w adpd4000 0xD:0x4e20")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adpd4000 0xD:0x2710")
    elif samp_freq_hz == 500:
        watch_shell.do_reg("w adpd4000 0xD:0x07D0")
    if agc_state:
        watch_shell.do_lcfgPpgWrite("0x4 0x1210")
    else:
        watch_shell.do_lcfgPpgWrite("0x4 0x1010")
    watch_shell.do_sensor("ppg start")
    watch_shell.do_sub("rppg add")
    watch_shell.do_plot("rppg")
    watch_shell.do_plot("rsyncppg")


def set_adpd_stream_freq(samp_freq_hz=50):
    """

    :param samp_freq_hz:
    :return:
    """
    if samp_freq_hz == 50:
        watch_shell.do_reg("w adpd4000 0xD:0x4e20")
    elif samp_freq_hz == 100:
        watch_shell.do_reg("w adpd4000 0xD:0x2710")
    elif samp_freq_hz == 500:
        watch_shell.do_reg("w adpd4000 0xD:0x07D0")


def quick_stop_ppg(samp_freq_hz=50):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_quickstop('ppg')


def set_ecg_samp_freq(samp_freq_hz=100):
    """

    :param samp_freq_hz:
    :return:
    """
    watch_shell.do_lcfgEcgWrite('0:{}'.format(samp_freq_hz))


def dcb_cfg(mode='w', dev='adxl', file_name=''):
    """

    :param mode: 'w'| 'r' | 'd'
    :param dev: 'adpd4000' | 'adxl' | 'ecg' | 'eda'
    :param file_name: '*_dcb_config.cfg'
    :return:
    """
    curr_dir = os.getcwd()
    dcb_cfg_dir = os.path.join(curr_dir, 'dcb_cfg')
    if not os.path.exists(dcb_cfg_dir):
        os.mkdir(dcb_cfg_dir)
        logging.warn("DCG Config Dir Not Found! Creating empty directory 'dcb_cfg'")
    if mode == 'w':
        if os.path.exists(os.path.join(dcb_cfg_dir, file_name)) and os.path.isfile(os.path.join(dcb_cfg_dir, file_name)):
            err_stat = watch_shell.do_write_dcb_config('{} {}'.format(dev, file_name))
        else:
            err_stat = 1
            logging.error("DCB Config file not found!")
            raise RuntimeError("DCB Config file not found!\n{}".format(os.path.join(dcb_cfg_dir, file_name)))
    elif mode == 'r':
        err_stat = watch_shell.do_read_dcb_config('{}'.format(dev))
        logging.info('DCB File Name: dcb_cfg\{}_dcb_get.dcfg'.format(dev))
    elif mode == 'd':
        err_stat = watch_shell.do_delete_dcb_config('{}'.format(dev))
    else:
        err_stat = 1
    return err_stat, dcb_cfg_dir


def plot_and_save_png(f_path, col_idx=1, row_offset=1):
    """
    This function reads a csv file and plots the data and saves the plot into a png file
    :param f_path:
    :param col_idx:
    :return: plot_path
    """
    data_list = read_csv_col(f_path, col_idx, row_offset)
    f_name = os.path.splitext(os.path.split(f_path)[-1])[0]
    plot_path = os.path.join(os.path.split(f_path)[0], 'plots', f_name+'.png')
    plt.plot(data_list)
    plt.xlabel('time')
    plt.ylabel('amplitude')
    plt.savefig(plot_path)
    plt.close()
    return plot_path


def update_arduino(in_obj):
    """
    This function updates the arduino global variable usually from an initialize function call
    :param in_obj:
    :return:
    """
    global arduino
    arduino = in_obj


def update_watch_shell(in_obj):
    """
    This function updates the watch_shell global variable usually from an initialize function call
    :param in_obj:
    :return:
    """
    global watch_shell
    watch_shell = in_obj


def update_dvt_version():
    """
    This function updates the DVT_version global variable usually from an initialize function call
    :return:
    """

    global DVT_version
    err_stat, chip_id = watch_shell.do_getChipID("2")  # ADPD chip ID index is 2
    if chip_id == 0xc0:
        logging.info("DVT1 Watch Connected")
        DVT_version = 0
    elif chip_id == 0x1c2:
        logging.info("DVT2 Watch Connected")
        DVT_version = 1
    else:
        raise RuntimeError("Unknown DVT Watch version ADPD Chip ID-{}".format(str(chip_id)))


def read_station_cfg():
    """
    This function reads the station config yaml file and updates the global variables. If a file is not found, it will
    create a file with the default values in it. The file location is <user>/AppData/Roaming/
    :return:
    """
    # Default values
    cfg_dict = {'arduino_port': 'COM7', 'watch_port': 'COM13',
                'fg_instr_addr': 'USB0::0x0957::0x2C07::MY52802639::0::INSTR',
                'sm_instr_addr': 'GPIB0::23::INSTR', 'watch_port_type': 'USB'}
    station_cfg_path = os.path.join(os.getenv('APPDATA'), 'watch_test.yaml')
    if os.path.exists(station_cfg_path) and os.path.isfile(station_cfg_path):
        with open(station_cfg_path, 'r') as f_ref:
            cfg_dict = yaml.load(f_ref, Loader=yaml.FullLoader)
    else:
        with open(station_cfg_path, 'w') as f_ref:
            yaml.dump(cfg_dict, f_ref)

    global arduino_port, watch_port, fg_instr_addr, sm_instr_addr, watch_port_type
    arduino_port = cfg_dict['arduino_port']
    watch_port = cfg_dict['watch_port']
    fg_instr_addr = cfg_dict['fg_instr_addr']
    sm_instr_addr = cfg_dict['sm_instr_addr']
    if 'watch_port_type' in cfg_dict.keys():
        watch_port_type = cfg_dict['watch_port_type']
    else:
        watch_port_type = 'USB'


def close_plot_after_run(plot_name_list, close_plot_mode=False):
    """
    This function closes all open plot and cmd windows opened by the test.
    This checks for a global mode variable or the local mode variable. Local variable defaults to False
    :param plot_name_list: This is a list of string values of the plot window names
    :param close_plot_mode: This is a boolean arg
    :return:
    """
    if close_plot_mode or close_plot_mode_global:
        for plot_name in plot_name_list:
            os.system('taskkill /fi "WINDOWTITLE eq {}"'.format(plot_name))
        time.sleep(0.25)
        os.system('taskkill /fi "WINDOWTITLE eq C:\WINDOWS\system32\cmd.exe"')


def init_matlab_engine():
    """
    This function imports and initializes matlab engine
    MATLAB package needs to be installed from <matlab_root>/extern/engine/python directory
    Use the command "python setup.py install"
    :return:
    """
    global matlab_eng
    try:
        import matlab.engine
        matlab_eng = matlab.engine.start_matlab()
    except:
        print("Error loading MATLAB Engine!")
    if matlab_eng:
        matlab_dir = os.path.join(os.path.abspath('.'), 'utils', 'matlab_utils')
        matlab_eng.addpath(matlab_dir, nargout=0)
    return matlab_eng


def initialize_setup():
    """
    This function runs necessary steps to initialize the test setup
    - Connects to Arduino and initializes arduino global variable
    :return:
    """
    global fg, sm
    global test_report_dir
    read_station_cfg()
    # Instantiating watch shell
    watch_shell_obj = CLI.m2m2_shell()
    if watch_port_type == 'USB':
        watch_shell_obj.do_connect_usb(watch_port)
    else:
        watch_shell_obj.do_connect_dongle(watch_port)
    # Creating Test Rport Directory
    err_stat, sys_info_dict = watch_shell_obj.do_getSystemInfo('')
    if err_stat:
        raise RuntimeError('Unable to communicate with the watch!')
    pcb_name = str(sys_info_dict['mac_addr'])
    if not pcb_name:
        pcb_name = easygui.enterbox('PCB Number:', 'Enter PCB Number')
    test_report_dir = init_test_report_dir(pcb_name)
    logging.info('Test Results Directory: {}'.format(test_report_dir))
    err_stat, fw_ver_info_dict = watch_shell_obj.do_getVersion('')
    if not err_stat:
        ver_info_str = 'Firmware Version: V{}.{}.{}  |  Build Date: {}'.format(fw_ver_info_dict['major'],
                                                                               fw_ver_info_dict['minor'],
                                                                               fw_ver_info_dict['patch'],
                                                                               fw_ver_info_dict['date'])
        update_robot_suite_doc(ver_info_str)
    # Instantiating Arduino
    #arduino_obj = SerialIface(port=arduino_port)
    #arduino_obj.serial_write('!CfgIOMap\r')

    watch_shell_obj.do_toggleSaveCSV('')
    #update_arduino(arduino_obj)
    update_watch_shell(watch_shell_obj)
    # TODO: Enable below code to configure instruments
    #fg = instr_lib.KeysightFG33522B()
    #fg.instr_connect(fg_instr_addr)
    #sm = instr_lib.KeithleySM2400()
    #sm.instr_connect(sm_instr_addr)
    update_dvt_version()


def init_test_report_dir(pcb_name):
    """
    This function creates a directory for pcb test reports if not already present and
    creates a folder inside it with the current date and time string for storing the test results
    :param pcb_name:
    :return:
    """
    if copy_results_to_shared_drive:
        if not pcb_name:
            pcb_name = pcb_name_default
        pcb_name = pcb_name.upper()
        pcb_test_dir_path = os.path.join(shared_drive_path, pcb_name+'_test_result')
        if not os.path.exists(shared_drive_path):
            raise Exception('Unable to access shared drive path!')
        if not os.path.exists(pcb_test_dir_path):
            # Creating PCB test directory
            os.mkdir(pcb_test_dir_path)
        now = datetime.now()
        dt_str = now.strftime("%m_%d_%y_%H_%M_%S")
        # Creating time-stamped test directory
        test_report_dir = os.path.join(pcb_test_dir_path, dt_str)
        os.mkdir(test_report_dir)
        with open('output_dir.tmp', 'w') as f_ref:
            f_ref.write(test_report_dir)
    else:
        test_report_dir = ''
    # Clearing plots folder
    if os.path.isdir('plots'):
        shutil.rmtree('plots')
        time.sleep(1)
    os.mkdir('plots')
    return test_report_dir

@util_logger
def close_setup():
    """
    This function runs necessary steps to close the test setup
    :return:
    """
    # common.watch_shell.do_exit('')  # TODO: Need to enable this after exit command is fixed
    update_arduino(None)
    update_watch_shell(None)
    close_plot_after_run(['ECG Data Plot'], True)
    # TODO: Enable below code to configure function generator
    # fg.close()

@util_logger
def set_switch(name, state):
    """
    This function extracts the io_id from the switch_map based on the input naem and sets the state
    :param name:
    :param state:
    :return:
    """
    if name in switch_map:
        io_id = switch_map[name]
        arduino.serial_write('!SetIO {} {}\r'.format(io_id, state))
    else:
        raise Exception('Invalid switch name! Unable to find the provided switch name in the switch map')


def rename_stream_file(old_file_name, suffix='', row_offset=1, col_idx=1,
                       copy_to_shared_drive=copy_results_to_shared_drive, plot=save_plots):
    """
    This function renames the old_file_name of stream file by appending a suffix to it
    :param old_file_name:
    :param suffix:
    :param row_offset: If there is header on row 0 of csv data, row_offset can be 1
    :param col_idx: If the data is on column 2, col_idx will be 1
    :param copy_to_shared_drive:
    :param plot: True/False
    :return:
    """
    if os.path.exists(old_file_name):  # Renaming stream file for each iteration
        new_name = os.path.splitext(old_file_name)[0] + suffix
        if os.path.exists(new_name) and os.path.isfile(new_name):
            os.remove(new_name)
        time.sleep(0.5)
        os.rename(old_file_name, new_name)
    else:
        new_name = ''
    if plot:
        plot_path = plot_and_save_png(new_name, col_idx, row_offset)
    if copy_to_shared_drive:
        test_group_name = inspect.getmodule(inspect.stack()[1][0]).__name__.split('.')[-1]
        test_group_dir = os.path.join(test_report_dir, test_group_name)
        if not os.path.exists(test_group_dir):
            os.mkdir(test_group_dir)
        file_name = os.path.split(new_name)[-1]
        shutil.copyfile(new_name, os.path.join(test_group_dir, file_name))
        if plot:
            plot_dir = os.path.join(test_group_dir, 'plots')
            if not os.path.exists(plot_dir):
                os.mkdir(plot_dir)
            plot_name = os.path.split(plot_path)[-1]
            new_plot_path = os.path.join(plot_dir, plot_name)
            shutil.copyfile(plot_path, new_plot_path)
    return new_name


def amp_volts_to_percentage(in_volt):
    """
    This function takes in amplitude in volts and returns amplitude percentage for arduino
    :param in_volt:
    :return:
    """
    full_scale_amp = float(volt_scale_range[1])/2
    amp_percentage = (in_volt * 100) / full_scale_amp
    if amp_percentage > 100:
        amp_percentage = 100.0
    return amp_percentage


def read_csv_file(file_path, num_cols=2):
    """
    This function can be used for reading and returning data from csv files
    :param file_path:
    :param num_cols:
    :return:
    """
    with open(file_path, 'r') as f_ref:
        lines = f_ref.readlines()
    lines.pop(0)
    rows = [list(map(float, line.split(','))) for line in lines]
    cols = zip(*rows)
    return cols[:num_cols]

def read_csv_col(file_path, col_idx=0, row_offset=1):
    """
    This function reads a csv file and returns the column data specified by the col_idx.
    :param file_path:
    :param col_idx:
    :return:
    """
    with open(file_path, 'r') as f_ref:
        line_list = f_ref.readlines()

    # col_data_list = [float(line.split(',')[col_idx].strip())
    #                  for i, line in enumerate(line_list)
    #                  if (i >= row_offset) and line.strip()]
    col_data_list = []
    last_line = len(line_list) - 1
    for i, line in enumerate(line_list):
        if i >= row_offset and any(line):
            line.strip()
            if last_line == i and not any(line.split(",")[col_idx]):
                continue
            col_data_list.append(float(line.split(",")[col_idx]))

    return col_data_list



# **********************************************************************
def test_func():
    logging.info('Logging has started!')
