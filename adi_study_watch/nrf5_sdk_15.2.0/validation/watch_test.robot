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
# >>>>>  Test on watch setup/simulator
AD8233 Noise Test
    [Documentation]    Expected v_noise: < 50e-6
    [Tags]  ECG_TEST
    ad8233_noise_test

ECG Simulator Signal Quality Test
    [Documentation]  ECG@250 simulator signal quality test peformed standalone and along with other apps (usecase3)
    [Tags]  ECG_TEST
    ecg_simulator_signal_quality_test
	
ECG Signal Quality Frequency Sweep Test
    [Documentation]  ECG Signal quality at different frequencies
    [Tags]  ECG_TEST
    ecg_atmultiple_frequencies

ADXL Self Test
    [Tags]  ADXL_Test
    adxl_self_test

PPG Dark Test
    [Tags]  PPG_Test
    ppg_dark_test

#>>>>> BLE TEST
BLE Ping Test
    [Documentation]  Expected Packet loss: 0
    [Tags]  BLE_Test
    ble_ping_test


#>>>>> Dialog Box for wearing watch
Wear Watch On Wrist
    Pause Execution    Place the watch on your wrist and press OK!

# >>>>> Tests on body <<<<<< #
Temperature Test
    [Tags]  TEMP_TEST
    temperature_accuracy_test

PPG AGC Test
    [Documentation]
    [Tags]  PPG_TEST
    ppg_agc_test

PPG AGC Frequency Sweep Test
    [Documentation]  AGC ON and OFF streams
    [Tags]  PPG_TEST
    ppg_at_multiple_frequency_test
	
BioZ Accuracy Test
    [Documentation]
    [Tags]  BIOZ_TEST
    ad5940_accuracy_test

ECG_Use_Case_Test_No_4
    [Documentation]  ECG with PPG, ADXL and Temperature
    [Tags]  UseCase_TEST
    use_case_ecg
	
ECG_Use_Case_Test_No_1
    [Documentation]  PPG with ADXL and Temperature
    [Tags]  UseCase_TEST
    use_case_ppg
	
ECG_Use_Case_Test_No_2
    [Documentation]  PPG with EDA, ADXL and Temperature
    [Tags]  UseCase_TEST
    use_case_eda_ppg
	
ECG_Use_Case_Test_No_3
    [Documentation]  PPG with ECG, ADXL and Temperature
    [Tags]  UseCase_TEST
    use_case_ecg_ppg
