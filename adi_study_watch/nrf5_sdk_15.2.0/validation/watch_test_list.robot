*** Settings ***
Documentation    This scripts tests the various capabilities of the study watch.
Library          Dialogs
Library          common.py
Library          test_scripts/adxl_test.py
Library          test_scripts/performance_test.py
Library          test_scripts/ecg_test.py
Library          test_scripts/bioz_test.py
Library          test_scripts/battery_test.py
Library          test_scripts/ble_test.py
Library          test_scripts/usecase_test.py
Library          test_scripts/functional_test.py
Library          test_scripts/ppg_test.py

Suite Setup      initialize_setup
Suite Teardown   close_setup

*** Test Cases ***
# >>>>> ECG Tests
ECG Simulator Signal Quality Test
    [Documentation]  ECG@250 simulator signal quality test peformed standalone and along with other apps (usecase3)
    [Tags]  ECG_TEST
    ecg_simulator_signal_quality_test
ECG Signal Quality Frequency Sweep Test
    [Documentation]  ECG Signal quality at different frequencies
    [Tags]  ECG_TEST
    ecg_atmultiple_frequencies
ECG Body Signal Quality Test
    [Documentation]  ECG@250 Signal quality on body
    [Tags]  ECG_TEST
    ecg_body_signal_quality_test
#AD8233 Switch Test
#    [Documentation]  Test ECG with Switch ON and OFF
#    [Tags]  ECG_TEST
#    ad8233_switch_functionality_test
AD8233 Noise Test
    [Documentation]    Expected v_noise: < 50e-6
    [Tags]  ECG_TEST
    ad8233_noise_test
#ADPD4000 Input Noise Test
#    [Documentation]
#    [Tags]  ECG_TEST
#    adpd400_input_noise_test
#ADPD4000 Frequency Response Test
#    [Documentation]
#    [Tags]  ECG_TEST
#    adpd400_frequency_response_test
#Output Accuracy Test
#    [Documentation]
#    [Tags]  ECG_TEST
#    ecg_output_accuracy_test


# >>>>> ADXL Tests <<<<<< #
ADXL Self Test
    [Documentation]
    [Tags]  ADXL_TEST
    adxl_self_test


# >>>>> PPG Tests <<<<<< #
PPG Dark Test
    [Documentation]
    [Tags]  PPG_TEST
    ppg_dark_test
PPG AGC Test
    [Documentation]
    [Tags]  PPG_TEST
    ppg_agc_test
#PPG AGC Frequency Sweep Test
#    [Documentation] AGC ON and OFF streams
#    [Tags]  PPG_TEST
#    ppg_agc_test
PPG AGC With ADXL TEMP Test
    [Documentation]
    [Tags]  PPG_TEST
    use_case_ppg
PPG AGC With ADXL EDA TEMP Test
    [Documentation]
    [Tags]  PPG_TEST
    use_case_eda_ppg
PPG AGC With ECG ADXL TEMP Test
    [Documentation]
    [Tags]  PPG_TEST
    use_case_ecg_ppg


# >>>>> Temperature Tests <<<<<< #
Temperature Test
    [Tags]  TEMP_TEST
    temperature_accuracy_test


# >>>>> Bluetooth Tests <<<<<< #
BLE Functionality Test
    [Documentation]  Expected Packet loss: 0
    [Tags]  BLE_TEST
    ble_ping_test
#BLE Range Reliability Test
#    [Documentation]  Expected Packet loss: 0
#    [Tags]  BLE_TEST
#    ble_range_reliability_test


# >>>>> BioZ Tests <<<<<< #
#BioZ Switch Functionality Test
#    [Documentation]
#    [Tags]  BIOZ_TEST
#    bioz_switch_functionality_test
BioZ Accuracy Test
    [Documentation]
    [Tags]  BIOZ_TEST
    ad5940_accuracy_test
#BioZ Repeatability Test
#    [Documentation]
#    [Tags]  BIOZ_TEST
#    ad5940_repeatability_test
#BioZ NonIdeal Contact Impedance Test
#    [Documentation]
#    [Tags]  BIOZ_TEST
#    ad5940_accuracy_nonideal_contact_z_test


# >>>>> Functional Tests <<<<<< #
#Memory Access Test
#    [Documentation]
#    [Tags]  FUNC_TEST
#    memory_access_test
#Communication Test
#    [Documentation]  Read device id of each device and check
#    [Tags]  FUNC_TEST
#    communication_test


# >>>>> Cap Touch Tests <<<<<< #
#AD7156 Cap Touch Test
#    [Documentation]
#    [Tags]  CAPTOUCH_TEST
#    ad7156_repeatability_test


*** Keywords ***
Dialog_Wear_Watch_On_Wrist
    Pause Execution    Place the watch on your wrist and press OK!


