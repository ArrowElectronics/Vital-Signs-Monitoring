import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common
from common import ConditionCheckFailure
from utils import meas_check


def ecg_switch_functionality_test():
    """
    Turn off AD5940 and ADPD4002 Switches;
    Turn on and off the 8233 Switch and check if AD8233 is disconneted from the electrodes
    :return:
    """
    capture_time = 10  # seconds

    # Configure waveform (5hz, ~0.1Vpp) on DAC0 and DAC1
    #response = common.arduino.serial_write('!CfgDacWfm 5 0.1\r')
    # TODO: Enable below code to configure function generator
    common.fg.cfg_waveform(5.0, 0.15, 1.1)
    common.fg.output_enable(state=1)
    time.sleep(1)
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')

    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.rename_stream_file(common.ecg_stream_file_name, '_SwitchON.csv')

    # Turn switch OFF and collect data
    common.watch_shell.do_quickstart('ecg')
    common.watch_shell.do_plot('recg')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.rename_stream_file(common.ecg_stream_file_name, '_SwitchOFF.csv')

    common.close_plot_after_run(['ECG Data Plot'])

    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ecg_simulator_signal_quality_test():
    """
    Turn off AD5940 and ADPD4002 Switches; Turn on the 8233 Switch and collect AD5940 output data
    Standalone: ECG @ 250hz
    MultiApp: UC3 - ECG@250hz PPG@100hz ADXL@50hz TEMP@1hz
    :return:
    """
    # *** Testcase Variables *** #
    capture_time = 60
    ref_dict = {'ref_qrs_amp': 0.1, 'ref_hr': 70}
    # ************************** #
    common.easygui.msgbox('Connect the Watch to the ECG Simulator leads and press OK!', 'Connect Leads')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 0)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    # *** Standalone ECG Stream *** #
    common.quick_start_ecg(samp_freq_hz=250)
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    sim_ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_simulator_standalone.csv')
    sim_ecg_file = os.path.abspath(sim_ecg_file)
    standalone_ecg_results_dict = common.analyze_wfm(sim_ecg_file, 'ecg')
    common.logging.info('standalone_ecg_results = {}'.format(standalone_ecg_results_dict))

    # *** Usecase ECG Stream *** #
    common.quick_start_adpd(100, 1)
    common.watch_shell.do_quickstart('adxl')
    common.watch_shell.do_plot('radxl')
    common.watch_shell.do_quickstart('temperature')
    common.watch_shell.do_plot('rtemperature')
    common.quick_start_ecg(samp_freq_hz=250)
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.watch_shell.do_quickstop('adxl')
    common.watch_shell.do_quickstop('adpd4000')
    common.watch_shell.do_quickstop('temperature')
    common.close_plot_after_run(['ECG Data Plot', 'ADXL Data', 'ADPD400x Data', 'Temperature Data Plot'])
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    sim_ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_simulator_usecase.csv')
    sim_ecg_file = os.path.abspath(sim_ecg_file)
    usecase_ecg_results_dict = common.analyze_wfm(sim_ecg_file, 'ecg')
    common.logging.info('usecase_ecg_results = {}'.format(usecase_ecg_results_dict))

    # *** Test Results Evaluation ***
    sa_status = meas_check.check_stream_status('ecg', standalone_ecg_results_dict, ref_dict)
    uc_status = meas_check.check_stream_status('ecg', usecase_ecg_results_dict, ref_dict)
    sa_uc_status = meas_check.check_standalone_vs_usecase_results('ecg', standalone_ecg_results_dict,
                                                                  usecase_ecg_results_dict, 0.1)
    if sa_status and uc_status and sa_uc_status:
        common.logging.info('*** ECG Simulator Signal Quality Test - PASS ***')
    else:
        common.logging.error('*** ECG Simulator Signal Quality Test - FAIL ***')
        raise ConditionCheckFailure('\n\nTest Pass Status:\nStandAlone_Test={}, '
                                    'Usecase_Test={}, '
                                    'SandAlone_Vs_Usecase_Test={}'.format(sa_status, uc_status, sa_uc_status))


def ecg_body_signal_quality_test():
    """
    Turn off AD5940 and ADPD4002 Switches; Turn on the 8233 Switch and collect AD5940 output data
    :return:
    """
    capture_time = 40
    ref_dict = {'ref_qrs_amp': 0.1, 'ref_hr': 70}

    common.easygui.msgbox('Hold the top electrodes with you first finger and thumb and press OK!', 'Connect Leads')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 0)

    # Capturing ECG data
    common.quick_start_ecg(samp_freq_hz=250)
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('ecg')
    common.close_plot_after_run(['ECG Data Plot'])
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')

    ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_body.csv')
    ecg_file = os.path.abspath(ecg_file)
    ecg_results_dict = common.analyze_wfm(ecg_file, 'ecg')

    # *** Test Results Evaluation ***
    status = meas_check.check_stream_status('ecg', ecg_results_dict, ref_dict)
    if status:
        common.logging.info('body_ecg_results = {}'.format(ecg_results_dict))
        common.logging.info('*** ECG Body Signal Quality Test - PASS ***')
    else:
        common.logging.error('body_ecg_results = {}'.format(ecg_results_dict))
        common.logging.error('*** ECG Body Signal Quality Test - FAIL ***')
        raise ConditionCheckFailure('\n\nOne or more parameters of ECG results did not match the reference results')


def temperature_accuracy_test():
    """
    Temperature accuracy test
    :return:
    """
    capture_time = 10
    temp_range = (28, 39)  # TODO: These variables need to be updated
    common.easygui.msgbox('Make sure that the watch surface is in contact with your skin for temperature measurements!',
                          'Temperature Test')

    common.watch_shell.do_quickstart('temperature')
    common.watch_shell.do_plot('rtemperature')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('temperature')
    common.close_plot_after_run(['Temperature Data Plot'])
    temp_file = os.path.abspath(common.temperature_stream_file_name)
    temp_in_range, out_of_range_count, out_of_range_percent = meas_check.check_temp_range(temp_file, temp_range)

    # *** Test Results Evaluation ***
    if temp_in_range:
        common.logging.info('*** Temperature Accuracy Test - PASS ***')
    else:
        common.logging.error('*** Temperature Accuracy Test - FAIL ***')
        raise ConditionCheckFailure('\n\n{}% of the temperature values are out of range!'.format(out_of_range_percent))


def ecg_atmultiple_frequencies_iter():
    """
    Enale 8233 switch, connect the contacts to ECG out of interface board, inject know signal value.
    Collect ADC output data for 61 seconds
    :return:
    """
    capture_time = 40  # seconds
    iterations = 3
    freq_list_hz = [500, 250, 100]
    ref_dict = {'ref_qrs_amp': 0.1, 'ref_hr': 70}
    fail_stat_list = []

    # Toggle switches
    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 1)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    init_report = False
    for iter_num in range(iterations):
        for i, freq_hz in enumerate(freq_list_hz):  # Frequency sweep loop
            common.easygui.msgbox('Connect the contacts to ECG out of interface board and click OK', 'ECG Test')
            common.quick_start_ecg(samp_freq_hz= freq_hz)
            common.watch_shell.do_plot('recg')
            time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
            common.watch_shell.do_quickstop('ecg')
            common.close_plot_after_run(['ECG Data Plot'])
            time.sleep(1)
            ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_{}Hz_iter-{}.csv'.format(freq_hz, iter_num))
            ecg_file = os.path.abspath(ecg_file)
            ecg_results_dict = common.analyze_wfm(ecg_file, 'ecg')
            common.write_analysis_report(ecg_results_dict, 'ecg_test_result.txt',
                                         'Idx_{}_Freq_{}hz'.format(iter_num, freq_hz),
                                         True) if init_report else common.write_analysis_report(ecg_results_dict,
                                                                                                 'ecg_test_result.txt',
                                                                                                 'Idx_{}_Freq_{}hz'.format(iter_num, freq_hz),
                                                                                                 False)
            status = meas_check.check_stream_status('ecg', ecg_results_dict, ref_dict)
            if not status:
                fail_stat_list.append('[Iteration-{} | Freq-{}hz]'.format(iter_num, freq_hz))
                common.logging.error('ECG_results_iter{}_freq{}hz = {}'.format(iter_num, freq_hz, ecg_results_dict))
            else:
                common.logging.info('ECG_results_iter{}_freq{}hz = {}'.format(iter_num, freq_hz, ecg_results_dict))
            init_report = True
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')

    # Test Condition Pass/Fail Check
    if not fail_stat_list:
        common.logging.info('*** ECG Simulator Signal Quality Frequency Sweep Test - PASS ***')
    else:
        common.logging.error('*** ECG Simulator Signal Quality Frequency Sweep Test - FAIL ***')
        raise ConditionCheckFailure('\n\nFailed Iterations:\n{}'.format(', '.join(fail_stat_list)))


def ecg_atmultiple_frequencies():
    """
    Enale 8233 switch, connect the contacts to ECG out of interface board, inject know signal value.
    Collect ADC output data for 61 seconds
    :return:
    """
    capture_time = 40  # seconds
    freq_list_hz = [500, 250, 100]
    ref_dict = {'ref_qrs_amp': 0.1, 'ref_hr': 70}
    fail_stat_list = []

    # Toggle switches
    # common.set_switch('SNOISE1', 0)
    # common.set_switch('SNOISE2', 1)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')
    init_report = False
    for i, freq_hz in enumerate(freq_list_hz):  # Frequency sweep loop
        common.easygui.msgbox('Connect the contacts to ECG out of interface board and click OK', 'ECG Test')
        common.quick_start_ecg(samp_freq_hz= freq_hz)
        common.watch_shell.do_plot('recg')
        time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
        common.watch_shell.do_quickstop('ecg')
        common.close_plot_after_run(['ECG Data Plot'])
        time.sleep(1)
        ecg_file = common.rename_stream_file(common.ecg_stream_file_name, '_{}Hz.csv'.format(freq_hz))
        ecg_file = os.path.abspath(ecg_file)
        ecg_results_dict = common.analyze_wfm(ecg_file, 'ecg')
        common.write_analysis_report(ecg_results_dict, 'ecg_test_result.txt',
                                     'Freq_{}hz'.format(freq_hz),
                                     True) if init_report else common.write_analysis_report(ecg_results_dict,
                                                                                             'ecg_test_result.txt',
                                                                                             'Freq_{}hz'.format(freq_hz),
                                                                                             False)
        status = meas_check.check_stream_status('ecg', ecg_results_dict, ref_dict)
        if not status:
            fail_stat_list.append('Freq-{}hz'.format(freq_hz))
            common.logging.error('ECG_results_freq{}hz = {}'.format(freq_hz, ecg_results_dict))
        else:
            common.logging.info('ECG_results_freq{}hz = {}'.format(freq_hz, ecg_results_dict))
        init_report = True
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')

    # Test Condition Pass/Fail Check
    if not fail_stat_list:
        common.logging.info('*** ECG Simulator Signal Quality Frequency Sweep Test - PASS ***')
    else:
        common.logging.error('*** ECG Simulator Signal Quality Frequency Sweep Test - FAIL ***')
        raise ConditionCheckFailure('\n\nFailed Iterations:\n{}'.format(', '.join(fail_stat_list)))


def ecg_ppg_adxl_temp():
    """
    This is a testcase template.
    Create a copy of this function, rename the function name and description to use this
    :return:
    """
    capture_time = 40  # seconds
    exp_val = 0  # TODO: These variables need to be updated
    act_val = 0  # TODO: These variables need to be updated

    # TODO: Add your test case code here
    #Turn on AD8233 Electrode switch and turn off the other two
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 1')

    #load appropriate dcb for adxl and ppg
    common.dcb_cfg('w', 'adxl', 'adxl_dcb_ecg_UC.DCFG')
    common.dcb_cfg('w', 'adpd4000', 'ppg_dcb_ecg_UC.dcfg')

    #turn on ECG @500Hz and take some reading
    common.messagebox.showinfo('ECG_PPG_Usecase_Test', 'Make sure Simulator is connected and click OK')
    common.quick_start_ecg(samp_freq_hz=500)
    common.watch_shell.do_plot('recg')
    time.sleep(capture_time)

    common.watch_shell.do_quickstop('ecg')

    common.rename_stream_file(common.ecg_stream_file_name, '_ecg_standalone.csv')
    time.sleep(2)

    #Start ECG application along with ADXL, PPG and Temp
    common.messagebox.showinfo('ECG_PPG_Usecase_Test', 'Make sure Simulator is connected and click OK')
    common.quick_start_ecg(samp_freq_hz=500)
    common.watch_shell.do_plot('recg')
    time.sleep(2)
    common.watch_shell.do_quickstart('adxl')
    common.watch_shell.do_plot('radxl')
    time.sleep(2)
    common.watch_shell.do_quickstart('temperature')
    common.watch_shell.do_plot('rtemperature')
    time.sleep(2)
    common.watch_shell.do_sub('radpd6 add')
    common.watch_shell.do_sensor('adpd4000 start')
    common.watch_shell.do_plot('radpd6')
    time.sleep(capture_time)

    common.watch_shell.do_quickstop('ecg')
    common.watch_shell.do_quickstop('adxl')
    common.watch_shell.do_quickstop('adpd4000')
    common.watch_shell.do_quickstop('temperature')

    common.close_plot_after_run(['ECG Data Plot'])
    common.close_plot_after_run(['ADXL Data'])
    common.close_plot_after_run(['ADPD400x Data'])
    common.close_plot_after_run(['Temperature Data Plot'])

    common.rename_stream_file(common.ecg_stream_file_name, '_ecg_with_ppg_adxl_temp.csv')
    common.rename_stream_file(common.adxl_stream_file_name, '_adxl_with_ecg_ppg_temp.csv')
    common.rename_stream_file(common.adpd_stream_file_name, '_ppg_ecg_adxl_temp.csv')
    common.rename_stream_file(common.temperature_stream_file_name, '_temp_ecg_ppg_adxl.csv')

    #Reset everything back to default condition
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.set_ecg_samp_freq(samp_freq_hz=250)
    common.dcb_cfg('d', 'adxl', 'adxl_dcb_ecg_UC.dcfg')
    common.dcb_cfg('d', 'adpd4000', 'adxl_dcb_ecg_UC.dcfg')
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    status = 'PASS'

    # TODO: Read and compare the values
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"



if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here

    # common.close_setup()
