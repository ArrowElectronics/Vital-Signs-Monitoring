*** Settings ***
Documentation    This scripts tests the various capabilities of the study watch.
Library          Dialogs
Library          common.py
Library          qa_test_scripts/ad5940_qa_test.py
Library          qa_test_scripts/ad8233_qa_test.py
Library          qa_test_scripts/adpd_qa_test.py
Library          qa_test_scripts/adxl_qa_test.py
Library          qa_test_scripts/temperature_qa_test.py
Library          qa_test_scripts/general_qa_test.py
Library          qa_test_scripts/usecase_qa_all_combination_test.py
Library          qa_test_scripts/inter_op_test.py

Suite Setup      initialize_setup   ${TS_Tolerance}     ${DUT_COMPORT}
Suite Teardown   close_setup

*** Variables ***
${TS_Tolerance}     0
${DUT_COMPORT}      NA


*** Test Cases ***
# >>>>> Generic Tests
Firmware Version Test
    [Documentation]
    [Tags]  GEN_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    fw_version_test

Date And Time Test
    [Documentation]
    [Tags]  GEN_TEST   SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    date_time_test


# >>>>> ADPD Tests
ADPD DEV ID Test
    [Documentation]
    [Tags]  ADPD_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_dev_id_test

ADPD DCB Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_dcb_test

ADPD Dcb Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_dcb_stream_test

ADPD DCB Repeatability Test
    [Documentation]
    [Tags]  ADPD_TEST    FULL_REGRESSION
    adpd_dcb_stream_repeatability_test

ADPD Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_stream_test

ADPD MultiLED Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_multi_led_stream_test

ADPD FS Stream Test
    [Documentation]
    [Tags]  ADPD_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    adpd_fs_stream_test

ADPD Frequency Sweep Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_freq_sweep_stream_test

ADPD Frequency Sweep FS Stream Test
    [Documentation]
    [Tags]  ADPD_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    adpd_freq_sweep_fs_stream_test

ADPD AGC Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_agc_test

ADPD Frequency Sweep AGC Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    adpd_freq_sweep_agc_test

ADPD Frequency Sweep Repeatability Test
    [Documentation]
    [Tags]  ADPD_TEST    FULL_REGRESSION
    adpd_freq_sweep_repeatability_test

PPG Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    ppg_stream_test

PPG AGC Test
    [Documentation]
    [Tags]  ADPD_TEST    ROUTINE_TEST    FULL_REGRESSION
    ppg_agc_test

PPG FS Stream Test
    [Documentation]
    [Tags]  ADPD_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    ppg_fs_stream_test


# >>>>> ADXL Tests
ADXL DEV ID Test
    [Documentation]
    [Tags]  ADXL_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl362_dev_id_test

ADXL DCB Test
    [Documentation]
    [Tags]  ADXL_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl_dcb_test

ADXL DCB Repeatability Test
    [Documentation]
    [Tags]  ADXL_TEST    FULL_REGRESSION
    adxl_dcb_repeatability_test

ADXL DCB Stream Test
    [Documentation]
    [Tags]  ADXL_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl_dcb_stream_test

ADXL Stream Test
    [Documentation]
    [Tags]  ADXL_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl_stream_test

ADXL FS Stream Test
    [Documentation]
    [Tags]  ADXL_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    adxl_fs_stream_test

ADXL Frequency Sweep Test
    [Documentation]
    [Tags]  ADXL_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl_freq_sweep_test

ADXL FS Frequency Sweep Test
    [Documentation]
    [Tags]  ADXL_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    adxl_fs_freq_sweep_test

ADXL Repeatability Test
    [Documentation]
    [Tags]  ADXL_TEST    ROUTINE_TEST    FULL_REGRESSION
    adxl_repeatability_test


# >>>>> AD5940 Tests
AD5940 DEV ID Test
    [Documentation]
    [Tags]  AD5940_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    ad5940_dev_id_test

EDA DCB Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    eda_dcb_test

EDA DCB Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    eda_dcb_stream_test

EDA DCB Repeatability Test
    [Documentation]
    [Tags]  AD5940_TEST   FULL_REGRESSION
    eda_dcb_repeatability_test

EDA Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    eda_stream_test

EDA FS Stream Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    eda_fs_stream_test

EDA Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    eda_freq_sweep_test

EDA FS Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST    ROUTINE_TEST    FULL_REGRESSION
    eda_fs_freq_sweep_test

EDA Repeatability Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    FULL_REGRESSION
    eda_repeatability_sweep_test

ECG DCB Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    ecg_dcb_test

ECG DCB Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    ecg_dcb_stream_test

ECG DCB Repeatability Test
    [Documentation]
    [Tags]  AD5940_TEST   FULL_REGRESSION
    ecg_dcb_repeatability_test

ECG Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    ecg_stream_test

ECG FS Stream Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    ecg_fs_stream_test

#AD5940 Switch Test
#    [Documentation]
#    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
#    ad5940_switch_test

ECG Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    ecg_freq_sweep_test

ECG FS Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST    ROUTINE_TEST    FULL_REGRESSION
    ecg_fs_freq_sweep_test

ECG Repeatability Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    FULL_REGRESSION
    ecg_repeatability_sweep_test

BIA DCB Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    bia_dcb_test

BIA DCB Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    bia_dcb_stream_test

BIA DCB Repeatability Test
    [Documentation]
    [Tags]  AD5940_TEST   FULL_REGRESSION
    bia_dcb_repeatability_test

BIA Stream Test
    [Documentation]
    [Tags]  AD5940_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    bia_stream_test

BIA FS Stream Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    bia_fs_stream_test

BIA Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    ROUTINE_TEST    FULL_REGRESSION
    bia_freq_sweep_test

BIA FS Frequency Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST   FS_TEST    ROUTINE_TEST    FULL_REGRESSION
    bia_fs_freq_sweep_test

BIA Repeatability Sweep Test
    [Documentation]
    [Tags]  AD5940_TEST    FULL_REGRESSION
    bia_repeatability_sweep_test




# >>>>> Temperature Tests
Temperature Stream Test
    [Documentation]
    [Tags]  ADPD_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    temperature_stream_test

Temperature FS Stream Test
    [Documentation]
    [Tags]  ADPD_TEST   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    temperature_fs_stream_test


# >>>>> Use Case Tests
Use Case QA PPG Test
    [Documentation]
    [Tags]  USE_CASE_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg

Use Case QA PPG FS Test
    [Documentation]
    [Tags]  USE_CASE_TEST   FS_TEST   UC_FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg_fs

Use Case QA PPG EDA Test
    [Documentation]
    [Tags]  USE_CASE_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg_eda

Use Case QA PPG EDA FS Test
    [Documentation]
    [Tags]  USE_CASE_TEST   FS_TEST   UC_FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg_eda_fs

Use Case QA PPG ECG Test
    [Documentation]
    [Tags]  USE_CASE_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg_ecg

Use Case QA PPG ECG FS Test
    [Documentation]
    [Tags]  USE_CASE_TEST   FS_TEST   UC_FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ppg_ecg_fs

Use Case QA ECG Test
    [Documentation]
    [Tags]  USE_CASE_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ecg

Use Case QA ECG FS Test
    [Documentation]
    [Tags]  USE_CASE_TEST   FS_TEST   UC_FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    use_case_qa_ecg_fs

Use Case 5 QA Test
    [Documentation]
    [Tags]  USE_CASE_TEST    SANITY_TEST    ROUTINE_TEST    FULL_REGRESSION
    use_case_5

Use Case 5 FS QA Test
    [Documentation]
    [Tags]  USE_CASE_TEST   FS_TEST   UC_FS_TEST   ROUTINE_TEST    FULL_REGRESSION
    use_case_5_fs


## >>>>> Use Case Tests Randomized
#Use Case QA PPG Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_randomized_test
#
#Use Case QA PPG FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_fs_randomized_test
#
#Use Case QA PPG EDA Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_eda_randomized_test
#
#Use Case QA PPG EDA FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_eda_fs_randomized_test
#
#Use Case QA PPG ECG Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_ecg_randomized_test
#
#Use Case QA PPG ECG FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ppg_ecg_fs_randomized_test
#
#Use Case QA ECG Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ecg_randomized_test
#
#Use Case QA ECG FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_qa_ecg_fs_randomized_test
#
#Use Case 5 QA Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_5_randomized_test
#
#Use Case 5 QA FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_5_fs_randomized_test
#
#Use Case Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED    ROUTINE_TEST    FULL_REGRESSION
#    use_case_randomized_test
#
#Use Case FS Randomized Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_RANDOMIZED   FS_TEST   ROUTINE_TEST    FULL_REGRESSION
#    use_case_fs_randomized_test
#
#
#
#
## >>>>> Use Case Tests All Combination
#Use Case QA PPG All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination    FULL_REGRESSION
#    use_case_qa_ppg_all_combination_test
#
#Use Case QA PPG FS All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination   FS_TEST   FULL_REGRESSION
#    use_case_qa_ppg_fs_all_combination_test
#
#Use Case QA PPG EDA All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination    FULL_REGRESSION
#    use_case_qa_ppg_eda_all_combination_test
#
#Use Case QA PPG EDA FS All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination   FS_TEST   FULL_REGRESSION
#    use_case_qa_ppg_eda_fs_all_combination_test
#
#Use Case QA PPG ECG All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination    FULL_REGRESSION
#    use_case_qa_ppg_ecg_all_combination_test
#
#Use Case QA PPG ECG FS All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination   FS_TEST   FULL_REGRESSION
#    use_case_qa_ppg_ecg_fs_all_combination_test
#
#Use Case QA ECG All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination    FULL_REGRESSION
#    use_case_qa_ecg_all_combination_test
#
#Use Case QA ECG FS All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination   FS_TEST   FULL_REGRESSION
#    use_case_qa_ecg_fs_all_combination_test
#
#Use Case 5 QA All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination    FULL_REGRESSION
#    use_case_5_all_combination_test
#
#Use Case 5 QA FS All Combination Test
#    [Documentation]
#    [Tags]  USE_CASE_TEST_All_Combination   FS_TEST   FULL_REGRESSION
#    use_case_5_fs_all_combination_test


# >>>>> Inter-Op Tests
Inter-Op DCB Test
    [Documentation]
    [Tags]  INTER_OP    ROUTINE_TEST    FULL_REGRESSION
    inter_op_dcb_test

Inter-Op DCB Repeatability Test
    [Documentation]
    [Tags]  INTER_OP    FULL_REGRESSION
    inter_op_dcb_repeatability_test

#Inter-Op ECG Switch Independent Test
#    [Documentation]
#    [Tags]  INTER_OP    ROUTINE_TEST    FULL_REGRESSION
#    inter_op_ecg_switch_independent_test



# >>>>> AD8233 Tests
#AD8233 DEV ID Test
#    [Documentation]
#    [Tags]  AD8233_TEST
#    ad8233_dev_id_test



*** Keywords ***
Dialog_Wear_Watch_On_Wrist
    Pause Execution    Place the watch on your wrist and press OK!


