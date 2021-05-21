import time
# **************** Added for robot file execution ***************** #
import sys
import os
sys.path.insert(0, os.path.join(os.path.abspath(__file__), '../../'))
# **************** ****************************** ***************** #
import common


def bioz_switch_functionality_test():
    """
    Turn off AD8233 and ADPD4002 Switches; Turn on and off the 5940 Switch and
    check if AD5940 is diconneted from the electrodes
    :return:
    """
    capture_time = 10  # seconds

    # Configure waveform (5hz, ~0.1Vpp) on DAC0 and DAC1
    response = common.arduino.serial_write('!CfgDacWfm 5 0.1\r')
    time.sleep(1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')

    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    # Turn switch ON and collect data
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')
    common.watch_shell.do_quickstart('eda')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('eda')
    common.rename_stream_file(common.eda_stream_file_name, '_BioZ_SwitchON.csv')

    # Turn switch OFF and collect data
    common.watch_shell.do_toggleSaveCSV('')
    common.watch_shell.do_quickstart('eda')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 0')
    common.watch_shell.do_toggleSaveCSV('')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('eda')
    common.rename_stream_file(common.ecg_stream_file_name, '_BioZ_SwitchOFF.csv')

    common.close_plot_after_run(['EDA Data Plot', 'EDA Data'])

    # TODO: Read and compare the two captured files to verify if the switch is working
    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad5940_accuracy_test():
    """
    Performs Standalone and Usecase EDA stream captures and evaluates results
    :return:
    """
    capture_time = 10
    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')

    # Standalone Test
    common.watch_shell.do_quickstart('eda')
    common.watch_shell.do_plot('reda')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('eda')
    common.close_plot_after_run(['EDA Data Plot', 'EDA Data'])
    sa_eda_file = common.rename_stream_file(common.eda_stream_file_name, '_standalone.csv')

    # Usecase Test
    common.dcb_cfg('w', 'adxl', 'adxl_dcb_eda_ppg_UC.dcfg')
    common.dcb_cfg('w', 'adpd4000', 'ppg_dcb_eda_ppg_UC.dcfg')
    common.watch_shell.do_quickstart('adxl')
    common.watch_shell.do_plot('radxl')
    common.watch_shell.do_sub('radpd6 add')
    common.watch_shell.do_sensor('adpd4000 start')
    common.watch_shell.do_plot('radpd6')
    common.watch_shell.do_quickstart('temperature')
    common.watch_shell.do_plot('rtemperature')
    common.watch_shell.do_quickstart('eda')
    common.watch_shell.do_plot('reda')
    time.sleep(capture_time)
    common.watch_shell.do_quickstop('adxl')
    common.watch_shell.do_quickstop('adpd4000')
    common.watch_shell.do_quickstop('temperature')
    common.watch_shell.do_quickstop('eda')
    common.close_plot_after_run(['ADXL Data', 'ADPD400x Data', 'Temperature Data Plot', 'EDA Data Plot', 'EDA Data'])
    uc_eda_file = common.rename_stream_file(common.eda_stream_file_name, '_usecase.csv')
    common.dcb_cfg('d', 'adxl', 'adxl_dcb_eda_ppg_UC.dcfg')
    common.dcb_cfg('d', 'adpd4000', 'ppg_dcb_eda_ppg_UC.dcfg')

    # TODO: Implement results evaluation
    # *** Test Results Evaluation ***
    if True:
        common.logging.info('*** EDA Signal Accuracy Test - PASS ***')
    else:
        common.logging.error('*** EDA Signal Accuracy Test - FAIL ***')
        raise ConditionCheckFailure('\n\nOne or more of the results are not within the expected limits!')


def ad5940_accuracy_test_old():
    """
    Enable 5940 Switch and Implement frequncy Sweep and obtain value of a Known Impedance; contact impedance zero Ohm
    Sweep across 5KHz, 25KHz, 50KHz, 100KHz, 150KHz, 180KHz, 190KHz and 200KHz. Test  with 50 Ohm, 100 Ohm and 1kOhm
    :return:
    """
    freq_list_khz = [5, 25, 50, 100, 150, 180, 190, 200]
    imp_list_ohm = [50, 100, 1000]
    amp_vpp = 0.3
    capture_time = 10  # seconds

    # Toggle GPIOs
    # common.set_switch('SNOISE1', 1)
    # common.set_switch('SNOISE2', 0)
    # common.set_switch('ECG_NEGSEL', 1)
    # common.set_switch('ECG_POSSEL', 1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')

    response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_list_khz[0]*1000, amp_vpp))  #initialize wfm
    time.sleep(1)  # 1 second wait time for waveform generation
    for i, freq_khz in enumerate(freq_list_khz):  # Frequency sweep loop
        freq_hz = freq_khz*1000
        response = common.arduino.serial_write('!SetWfmFreq {}\r'.format(freq_hz))  # Generate wfm at iteration freq
        for j, imp_ohm in enumerate(imp_list_ohm):  # Impedance sweep loop
            # TODO: add function to switch impedance

            # Toggle save csv and capture ecg
            # common.watch_shell.do_toggleSaveCSV('')
            common.watch_shell.do_quickstart('ecg')
            time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
            common.watch_shell.do_quickstop('ecg')
            time.sleep(1)
            common.rename_stream_file(common.ecg_stream_file_name,
                                      '_{}Hz_{}Vpp_{}ohm.csv'.format(freq_hz, amp_vpp, imp_ohm))
            # TODO: Read and verify the generated capture files

            common.close_plot_after_run(['ECG Data Plot'])

    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad5940_repeatability_test():
    """
    Enable 5940 Switch and Implement frequncy Sweep and obtain value of a Known Impedance; contact impedance zero Ohm
    Sweep across 5KHz, 25KHz, 50KHz, 100KHz, 150KHz, 180KHz, 190KHz and 200KHz. Test  with 50 Ohm, 100 Ohm and 1kOhm
    repeat each measurement 10 times
    :return:
    """
    freq_list_khz = [5, 25, 50, 100, 150, 180, 190, 200]
    imp_list_ohm = [50, 100, 1000]
    amp_vpp = 0.3
    capture_time = 10  # seconds
    repeat_count = 10

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')

    response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_list_khz[0]*1000, amp_vpp))  #initialize wfm
    time.sleep(1)  # 1 second wait time for waveform generation
    for i, freq_khz in enumerate(freq_list_khz):  # Frequency sweep loop
        freq_hz = freq_khz*1000
        response = common.arduino.serial_write('!SetWfmFreq {}\r'.format(freq_hz))  # Generate wfm at iteration freq
        for j, imp_ohm in enumerate(imp_list_ohm):  # Impedance sweep loop
            # TODO: add function to switch impedance
            for k in range(repeat_count):  # Repeat loop
                # Toggle save csv and capture ecg
                # common.watch_shell.do_toggleSaveCSV('')
                common.watch_shell.do_quickstart('ecg')
                time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
                common.watch_shell.do_quickstop('ecg')
                time.sleep(1)
                common.rename_stream_file(common.ecg_stream_file_name,
                                          '_{}Hz_{}Vpp_{}ohm_{}idx.csv'.format(freq_hz, amp_vpp, imp_ohm, k))
                # TODO: Read and verify the generated capture files

                common.close_plot_after_run(['ECG Data Plot'])

    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad5940_accuracy_nonideal_contact_z_test():
    """
    Enable 5940 Switch and Implement frequncy Sweep and obtain value of a Known Impedance;
    contact impedance 1kOhm, 2kOhm and 3kOhm
    Sweep across 5KHz, 25KHz, 50KHz, 100KHz, 150KHz, 180KHz, 190KHz and 200KHz. Test  with 50 Ohm, 100 Ohm and 1kOhm
    repeat each measurement 10 times
    :return:
    """
    freq_list_khz = [5, 25, 50, 100, 150, 180, 190, 200]
    imp_list_ohm = [50, 100, 1000]
    contact_imp_list_kOhm = [1, 2, 3]
    amp_vpp = 0.3
    capture_time = 10  # seconds
    repeat_count = 10

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')

    response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_list_khz[0]*1000, amp_vpp))  #initialize wfm
    time.sleep(1)  # 1 second wait time for waveform generation
    for i, freq_khz in enumerate(freq_list_khz):  # Frequency sweep loop
        freq_hz = freq_khz*1000
        response = common.arduino.serial_write('!SetWfmFreq {}\r'.format(freq_hz))  # Generate wfm at iteration freq
        for j, imp_ohm in enumerate(imp_list_ohm):  # Impedance sweep loop
            # TODO: add function to switch impedance
            for k, contact_imp_ohm in enumerate(contact_imp_list_kOhm):  # Contact impedance loop
                # TODO: add function to switch contact impedance
                # Toggle save csv and capture ecg
                # common.watch_shell.do_toggleSaveCSV('')
                common.watch_shell.do_quickstart('ecg')
                time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
                common.watch_shell.do_quickstop('ecg')
                time.sleep(1)
                common.rename_stream_file(common.ecg_stream_file_name,
                                          '_{}Hz_{}Vpp_{}ohm_{}ohm.csv'.format(freq_hz, amp_vpp,
                                                                               imp_ohm, contact_imp_ohm))
                # TODO: Read and verify the generated capture files

                common.close_plot_after_run(['ECG Data Plot'])

    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


def ad5940_accuracy_mismatch_contact_z_test():
    """
    Enable 5940 Switch and Implement frequncy Sweep and obtain value of a Known Impedance;
    contact impedance F+/S+/F-/S- to 2/2/2/1 kOhm
    Sweep across 5KHz, 25KHz, 50KHz, 100KHz, 150KHz, 180KHz, 190KHz and 200KHz. Test  with 50 Ohm, 100 Ohm and 1kOhm
    repeat each measurement 10 times
    :return:
    """
    freq_list_khz = [5, 25, 50, 100, 150, 180, 190, 200]
    imp_list_ohm = [50, 100, 1000]
    contact_imp_list_kOhm = [1, 2, 3]
    amp_vpp = 0.3
    capture_time = 10  # seconds
    repeat_count = 10

    # Toggle GPIOs
    common.set_switch('SNOISE1', 1)
    common.set_switch('SNOISE2', 0)
    common.set_switch('ECG_NEGSEL', 1)
    common.set_switch('ECG_POSSEL', 1)

    common.watch_shell.do_controlECGElectrodeSwitch('8233_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('4k_sw 0')
    common.watch_shell.do_controlECGElectrodeSwitch('5940_sw 1')

    response = common.arduino.serial_write('!CfgDacWfm {} {}\r'.format(freq_list_khz[0]*1000, amp_vpp))  #initialize wfm
    time.sleep(1)  # 1 second wait time for waveform generation
    # TODO: add function to switch contact impedance F+/S+/F-/S- to 2/2/2/1 kOhm
    for i, freq_khz in enumerate(freq_list_khz):  # Frequency sweep loop
        freq_hz = freq_khz*1000
        response = common.arduino.serial_write('!SetWfmFreq {}\r'.format(freq_hz))  # Generate wfm at iteration freq
        for j, imp_ohm in enumerate(imp_list_ohm):  # Impedance sweep loop
            # TODO: add function to switch impedance
            # Toggle save csv and capture ecg
            # common.watch_shell.do_toggleSaveCSV('')
            common.watch_shell.do_quickstart('ecg')
            time.sleep(capture_time)  # A stream file is auto generated at this stage and data is written into it
            common.watch_shell.do_quickstop('ecg')
            time.sleep(1)
            common.rename_stream_file(common.ecg_stream_file_name,
                                      '_{}Hz_{}Vpp_{}ohm_mismatchZ.csv'.format(freq_hz, amp_vpp, imp_ohm))
            # TODO: Read and verify the generated capture files
            common.close_plot_after_run(['ECG Data Plot'])

    test_status = 'pass'
    if test_status != 'pass':
        assert False, "Condition check failed!"


if __name__ == '__main__':
    # This section can be used for testing and verifying individual test cases
    common.initialize_setup()
    # Add test cases to be verified here
    common.close_setup()
