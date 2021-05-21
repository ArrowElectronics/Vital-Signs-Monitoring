/*!
 *  \copyright Analog Devices
 * ****************************************************************************
 *
 * License Agreement
 *
 * Copyright (c) 2019 Analog Devices Inc.
 * All rights reserved.
 *
 * This source code is intended for the recipient only under the guidelines of
 * the non-disclosure agreement with Analog Devices Inc.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER  LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING  FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER  DEALINGS IN THE SOFTWARE.
 * ****************************************************************************
 */
#ifndef LOGGING_H
#define LOGGING_H


#define CYCLES_API_COUNT (8)

#ifndef ALGO_ET
typedef struct {
    uint32_t CYCLE_T1;
    uint32_t CYCLE_T2;
    uint32_t CYCLE_S1;
    uint32_t CYCLE_S2;
    uint32_t MIPS_T;
    uint32_t MIPS_S;
    uint32_t DEV_RATIO;
    uint32_t HR_TRACK;
    uint32_t HR_SPOT;
    uint32_t HR_SELECTED;
    uint32_t SUM_SLOTB;  // g_g_resultlog->SUM_IR  = slotBCh[0]+slotBCh[1]+slotBCh[2]+slotBCh[3];
    uint32_t SUM_SLOTA;  // g_g_resultlog->SUM_RED = slotACh[0]+slotACh[1]+slotACh[2]+slotACh[3];
    int32_t PPG_SCALED;
    int32_t ACCEL_SCALED[3];
    int16_t CONFIDENCE;
    int16_t HR_TYPE;
    int16_t CONFIDENCE_T;
    int16_t CONFIDENCE_S;
    int16_t Activity;
    int16_t StepCount;
    int16_t RRinterval;
    uint16_t IsHrvValid;
} HR_Log_Result_t;
#else
typedef struct {
    char INDICATOR;
    uint32_t ALG_TYPE_IND;
    float HR_M;
    float SNR_M;
    float ACC_MOTION_M;
    float PERFUSION_M;
    int8_t FLAGS_M;
    float HR_T;
    float SNR_T;
    float HR_S;
    float SNR_S;
    int8_t FLAGS_S;
    float DATARATE_RATIO;
    float B2B_INTERVAL_T;
    float B2B_INTERVAL_M;
    float HR_SELECTED;
    uint32_t HR_FLAGS;

    uint32_t SUM_SLOTB;  // g_g_resultlog->SUM_IR  = slotBCh[0]+slotBCh[1]+slotBCh[2]+slotBCh[3];
    uint32_t SUM_SLOTA;  // g_g_resultlog->SUM_RED = slotACh[0]+slotACh[1]+slotACh[2]+slotACh[3];

    uint32_t IRS_Value;
    uint32_t RS_Value;
    float IRO_Value;
    float RO_Value;
    float B2B_INTERVAL_ARR[15];
    float B2B_TIME_STAMP[15];
    float B2B_P2P[15];
    float MotionValue;
    float HRV_RESULT[2];
    uint32_t cycles[CYCLES_API_COUNT];
    float FD_DISPLAY;
    float PN_VALUE;
    uint8_t PN_FLAG;
    float stress_snr_result;
    float  hz10_accelerometer_log[3];
    float  hz10_accelerometer_scaled_log[3];
    float  hz10_LED1;
    float  hz10_LED2;
    float B2B_TS_M;
    float B2B_COMBINED;
    uint8_t time_to_call_spot;
    uint32_t nPedoSteps;
    uint16_t nPedoStatus;
} HR_Log_Result_t;
#endif //ADUCM3029

typedef void (*debugLog_t)(const char *M, ...);
typedef void (*debugLogC_t)(const char *M, ...);
typedef void (*logData_t)(uint32_t stage,
                        TimeStamps_t timeStamp,
                        uint32_t *s,
                        uint32_t *t,
                        int16_t  *a,
                        void *data);
typedef void (*logHeader_t)();

extern uint32_t g_logtype;

void LogHeader();
void LogData(uint32_t stage,
             TimeStamps_t timeStamp,
             uint32_t *s,
             uint32_t *t,
             int16_t  *a,
             void *data);

void InitLog(int32_t state);

void SetConsoleFullLog();
void SetConsoleAndroidLog();
void SetConsoleLabViewLog();
void SetConsoleNoLog();
void SetSDCardLog();
void SetSDCardNoLog();
uint32_t GetSDCardLog();
uint32_t GetConsoleLog();
void PushLogType();
void PopLogType();

#endif  // LOGGING_H
