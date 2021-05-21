/**
    ***************************************************************************
    * @file    PpgLcfg.h
    * @author  ADI Team
    * @version V0.0.1
    * @date    05-Feb-2016
    * @brief   Default PPG library configuration file
    ***************************************************************************
     * @attention
    ***************************************************************************
*/
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
#ifndef __PPGLCFG_H
#define __PPGLCFG_H

//#include "adpd_lib.h"
#include "adpd400x_lib.h"

#if 0
const AdpdLibConfig_t AdpdLibCfg105 = {
    105,            /* 16-bit. Device ID: 107 etc. */
    0xCC,           /* 8-bit.  Channel select. bit[7:0]=ch7,ch6...ch1,ch0. */
    0x0014,         /* 16-bit. Device Operation Mode. bit2=32bits, bit4=sum */
    0x02C4,         /* 16-bit. Skip some preprocess states */
    30000,          /* 16-bit. Interval to check data rate. in Seconds */
    0x12345605,     /* 32-bit. Reserved */
    0x0032,         /* 16-bit. Reserved. 1-6 here */
    0x3,            /* 8-bit.  Sync Mode, Default to H/W sync */

    0x000A0001,     /* 32-bit. Proximity fs and decimation 10/1=10hz */
    10000,          /* 16-bit. Proximity detection timeout */
    0x600,          /* 16-bit. Proximity Detect On level */

    0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
    (-1),           /* 16-bit. Detect on timeout: infinite second */
    3,              /* 8-bit.  Settling occurrence number for detect on: 3 counts */
    30,             /* 16-bit. Detect ON level */
    1000,           /* 16-bit. Detect ON AIR Level */
    20000,          /* 32-bit. Detect ON Stable Threshold (variance*16)*/
    0x1218,         /* 16-bit. Reserved. 1-18 here */

    10,             /* 8-bit.  Settling occurrence number for detect off: 10 counts */
    70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
    20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
    0x1222,         /* 16-bit. Reserved. 1-22 here */

    80,             /* 8-bit.  Detect OFF percentage during calibration */

    15,             /* 8-bit.  Allowing % of signal level to drift before adjustment */
    30,             /* 8-bit.  Drifting ajustment interval in Seconds */

    200,            /* 16-bit. Max sampling rate after optimization. 1-26*/
    50,             /* 8-bit.  LED Calibration: Target percentage */
    100,            /* 16-bit. LED Calibration: Maximum LED current */
    16,             /* 8-bit.  Max amount of pulses for the midium skin tone subject */
    70,             /* 8-bit.  Threshold to switch to Float Mode */
    50,             /* 8-bit.  Green LED voltage = 5v */
    50,             /* 8-bit.  Percentage of signal before Analgo Saturation  */
    10000,          /* 16-bit. 1000/MI. ie. Mi=0.2, this value = 5000 */

    0x19999,        /* 32-bit. motion detection threshold */
    0x000A0002,     /* 32-bit. upper = check period. lower = rested time */
    0x29999,        /* 32-bit. High motion detection threshold. x/2^20=MI in matlab */
    0x000A,         /* 16-bit. time interval to check high motion */

    0x05,           /* 8-bit.  CTR threshold */
    0x5BD4,         /* 16-bit. Mt2 threshold */
    0x53CC,         /* 16-bit. Mt3 threshold */

    0x1401,         /* Ambient check interval. Upper for NM, lower for FM */
    0xA800,         /* ambient threshold for 100K mode */

    /***ADI_algo parameters**/
    50,     /*spotalgosamplerate*/
    6,      /*spotalgodecimation*/
    4,      /*mindifftrackSpot*/
    716,    /*initialconfidencethreshold*/
    3200,   /*ppgscale*/
    4194,   /*accelscale*/
    5,      /*spotstabilitycount*/
    15,     /*spothrtimeoutsecs*/
    1,      /*zeroorderholdnumsamples. Not used*/
    50,     /*trackalgosamplerate*/
    15,     /*trackhrtimeoutsecs*/
    5000,   /*Spotwindowlength*/
    30,     /*TrackerMinHeartrateBpm*/
     0      /*hrvEnable*/
};

const AdpdLibConfig_t AdpdLibCfg107 = {
    107,            /* 16-bit. Device ID: 105 etc. */
    0x33,           /* 8-bit.  Channel select. bit[7:0]=ch7,ch6...ch1,ch0. */
    0x0014,         /* 16-bit. Device Operation Mode. bit2=32bits, bit4=sum */
    0x0A77,         /* 16-bit. Skip some preprocess states */
    30000,          /* 16-bit. Interval to check data rate. in Miliseconds */
    0x12345605,     /* 32-bit. Reserved */
    0x0032,         /* 16-bit. Reserved */
    0x3,            /* 8-bit.  Reserved. Sync Mode, Default to H/W sync */

    0x000A0001,     /* 32-bit. Reserved. Proximity fs and decimation 10/1=10hz */
    10000,          /* 16-bit. Reserved. Proximity detection timeout */
    0x600,          /* 16-bit. Reserved. Proximity Detect On level */

    0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
    (-1),           /* 16-bit. Detect on timeout: infinite second */
    3,              /* 8-bit.  Settling occurrence number for detect on: 5 counts */
    20,             /* 16-bit. Detect ON level */
    10,             /* 16-bit. Detect ON AIR Level */
    500000,         /* 32-bit. Detect ON Stable Threshold (variance*16)*/
    0x1217,         /* 16-bit. Reserved */

    10,             /* 8-bit.  Settling occurrence number for detect off: 10 counts */
    70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
    20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
    0x1221,         /* 16-bit. Reserved */

    80,             /* 8-bit.  Reserved, no longer use */

    15,             /* 8-bit.  Reserved. Allowing % of signal level to drift before adjustment */
    00,             /* 8-bit.  Reserved. Drifting ajustment interval in Seconds */

    200,            /* 16-bit. Reserved. Max sampling rate after optimization.*/
    70,             /* 8-bit.  Saturation: Percentage of the signal reduced to */
    380,            /* 16-bit. LED Calibration: Maximum LED current */
    32,             /* 8-bit.  Max amount of pulses for 200Hz */
    70,             /* 8-bit.  Threshold to switch to Float Mode */
    50,             /* 8-bit.  Green LED voltage * 10. 5v*10=50 */
    50,             /* 8-bit.  Percentage of signal before Analgo Saturation  */
    10000,          /* 16-bit. Reserved. 1000/MI. ie. Mi=0.2, this value = 5000 */

    0x19999,        /* 32-bit. Reserved. motion detection threshold */
    0x00780003,     /* 32-bit. upper = check period. lower = rested time */
    0x29999,        /* 32-bit. Reserved. High motion detection threshold. x/2^20=MI in matlab */
    0x000A,         /* 16-bit. time interval to check high motion */

    0x05,           /* 8-bit.  Reserved. CTR threshold */
    0x5BD4,         /* 16-bit. Reserved. Mt2 threshold */
    0x53CC,         /* 16-bit. Reserved. Mt3 threshold */

    0x0001,         /* float mode, ambient check interval */
    0xA800,         /* ambient threshold for 100K mode */

    /***ADI_algo parameters**/
    50,     /*spotalgosamplerate*/
    6,      /*spotalgodecimation*/
    4,      /*mindifftrackSpot*/
    716,    /*initialconfidencethreshold*/
    3200,   /*ppgscale*/
    4194,   /*accelscale*/
    5,      /*spotstabilitycount*/
    15,     /*spothrtimeoutsecs*/
    1,      /*zeroorderholdnumsamples. Not used*/
    50,     /*trackalgosamplerate*/
    15,     /*trackhrtimeoutsecs*/
    5000,   /*Spotwindowlength*/
    30,     /*TrackerMinHeartrateBpm*/
     1      /*hrvEnable*/
};

const AdpdLibConfig_t AdpdLibCfg185 = {
    185,            /* 16-bit. Device ID: 105 etc. */
    0xC0,           /* 8-bit.  Channel select. bit[7:0]=ch7,ch6...ch1,ch0. */
    0x0014,         /* 16-bit. Device Operation Mode. bit2=32bits, bit4=sum */
    0x0277,         /* 16-bit. Skip some preprocess states */
    0,              /* 16-bit. Interval to check data rate. in Seconds */
    0x12345605,     /* 32-bit. Reserved */
    0x0032,         /* 16-bit. Reserved */
    0x3,            /* 8-bit.  Sync Mode, Default to H/W sync */

    0x000A0001,     /* 32-bit. Proximity fs and decimation 10/1=10hz */
    10000,          /* 16-bit. Proximity detection timeout */
    0x600,          /* 16-bit. Proximity Detect On level */

    0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
    (-1),           /* 16-bit. Detect on timeout: infinite second */
    3,              /* 8-bit.  Settling occurrence number for detect on: 5 counts */
    20,             /* 16-bit. Detect ON level */
    10,             /* 16-bit. Detect ON AIR Level */
    500000,         /* 32-bit. Detect ON Stable Threshold (variance*16)*/
    0x1218,         /* 16-bit. Reserved */

    10,             /* 8-bit.  Settling occurrence number for detect off: 10 counts */
    70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
    20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
    0x1222,         /* 16-bit. Reserved */

    80,             /* 8-bit.  Detect OFF percentage during calibration */

    15,             /* 8-bit.  Allowing % of signal level to drift before adjustment */
    30,             /* 8-bit.  Drifting ajustment interval in Seconds */

    200,            /* 16-bit. Max sampling rate after optimization. 1-26*/
    50,             /* 8-bit.  LED Calibration: Target percentage */
    300,            /* 16-bit. LED Calibration: Maximum LED current */
    16,             /* 8-bit.  Max amount of pulses for the midium skin tone subject */
    10,             /* 8-bit.  Ctr threshold value for Float Mode */
    50,             /* 8-bit.  Green LED voltage = 5v */
    50,             /* 8-bit.  Percentage of signal before Analgo Saturation  */
    10000,          /* 16-bit.  1000/MI. ie. Mi=0.2, this value = 5000 */

    0x19999,        /* 32-bit.  motion detection threshold */
    0x000A0002,     /* 32-bit. upper = check period. lower = rested time */
    0x29999,        /* 32-bit. High motion detection threshold. x/2^20=MI in matlab */
    0x000A,         /* 16-bit. time interval to check high motion */

    0x05,           /* 8-bit.  CTR threshold */
    0x5BD4,         /* 16-bit. Mt2 threshold */
    0x53CC,         /* 16-bit. Mt3 threshold */

    0x0001,         /* float mode, ambient check interval */
    0xA800,         /* ambient threshold for 100K mode */

    /***ADI_algo parameters**/
    50,     /*spotalgosamplerate*/
    6,      /*spotalgodecimation*/
    4,      /*mindifftrackSpot*/
    716,    /*initialconfidencethreshold*/
    3200,   /*ppgscale*/
    4194,   /*accelscale*/
    5,      /*spotstabilitycount*/
    15,     /*spothrtimeoutsecs*/
    2,      /*zeroorderholdnumsamples. Not used*/
    100,    /*trackalgosamplerate*/
    15,     /*trackhrtimeoutsecs*/
    5000,   /*Spotwindowlength*/
    30,     /*TrackerMinHeartrateBpm*/
     1      /*hrvEnable*/
};

const AdpdLibConfig_t AdpdLibCfg108 = {
    108,            /* 16-bit. Device ID: 105 etc. */
    0x10,           /* 8-bit.  Channel select. bit[7:0]=ch7,ch6...ch1,ch0. */
    0x0014,         /* 16-bit. Device Operation Mode. bit2=32bits, bit4=sum */
    0x0E77,         /* 16-bit. Skip some preprocess states */
    0,              /* 16-bit. Interval to check data rate. in Seconds */
    0x12345605,     /* 32-bit. Reserved */
    0x0032,         /* 16-bit. Reserved */
    0x3,            /* 8-bit.  Reserved. Sync Mode, Default to H/W sync */

    0x000A0001,     /* 32-bit. Reserved. Proximity fs and decimation 10/1=10hz */
    10000,          /* 16-bit. Reserved. Proximity detection timeout */
    0x600,          /* 16-bit. Reserved. Proximity Detect On level */

    0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
    (-1),           /* 16-bit. Detect on timeout: infinite second */
    3,              /* 8-bit.  Settling occurrence number for detect on: 5 counts */
    400,            /* 16-bit. Detect ON level */
    10,             /* 16-bit. Detect ON AIR Level */
    500000,         /* 32-bit. Detect ON Stable Threshold (variance*16)*/
    0x1217,         /* 16-bit. Reserved */

    10,             /* 8-bit.  Settling occurrence number for detect off: 10 counts */
    70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
    20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
    0x1221,         /* 16-bit. Reserved */

    80,             /* 8-bit.  Reserved, no longer use */

    15,             /* 8-bit.  Reserved. Allowing % of signal level to drift before adjustment */
    00,             /* 8-bit.  Reserved. Drifting ajustment interval in Seconds */

    200,            /* 16-bit. Reserved. Max sampling rate after optimization.*/
    70,             /* 8-bit.  Saturation: Percentage of the signal reduced to */
    380,            /* 16-bit. LED Calibration: Maximum LED current */
    254,            /* 8-bit.  Max amount of pulses for 200Hz */
    70,             /* 8-bit.  Threshold to switch to Float Mode */
    50,             /* 8-bit.  Green LED voltage * 10. 5v*10=50 */
    50,             /* 8-bit.  Percentage of signal before Analgo Saturation  */
    10000,          /* 16-bit. Reserved. 1000/MI. ie. Mi=0.2, this value = 5000 */

    0x19999,        /* 32-bit. Reserved. motion detection threshold */
    0x00780003,     /* 32-bit. upper = check period. lower = rested time */
    0x29999,        /* 32-bit. Reserved. High motion detection threshold. x/2^20=MI in matlab */
    0x0006,         /* 16-bit. time interval to check high motion */

    0x00,           /* 8-bit.  Reserved. CTR threshold */
    0xDC99,         /* 16-bit. Reserved. Mt2 threshold */
    0x53CC,         /* 16-bit. Reserved. Mt3 threshold */

    0x0001,         /* float mode, ambient check interval */
    0xA800,         /* ambient threshold for 100K mode */

    /***ADI_algo parameters**/
    50,     /*spotalgosamplerate*/
    6,      /*spotalgodecimation*/
    4,      /*mindifftrackSpot*/
    716,    /*initialconfidencethreshold*/
    3200,   /*ppgscale*/
    4194,   /*accelscale*/
    5,      /*spotstabilitycount*/
    15,     /*spothrtimeoutsecs*/
    2,      /*zeroorderholdnumsamples. Not used*/
    100,    /*trackalgosamplerate*/
    15,     /*trackhrtimeoutsecs*/
    5000,   /*Spotwindowlength*/
    30,     /*TrackerMinHeartrateBpm*/
     1      /*hrvEnable*/
};

const AdpdLibConfig_t AdpdLibCfg188 = {
    188,            /* 16-bit. Device ID: 105 etc. */
    0x10,           /* 8-bit.  Channel select. bit[7:0]=ch7,ch6...ch1,ch0. */
    0x0014,         /* 16-bit. Device Operation Mode. bit2=32bits, bit4=sum */
    0x0E37,         /* 16-bit. Skip some preprocess states */
    0,              /* 16-bit. Interval to check data rate. in Seconds */
    0x12345605,     /* 32-bit. Reserved */
    0x0032,         /* 16-bit. Reserved */
    0x3,            /* 8-bit.  Reserved. Sync Mode, Default to H/W sync */

    0x000A0001,     /* 32-bit. Reserved. Proximity fs and decimation 10/1=10hz */
    10000,          /* 16-bit. Reserved. Proximity detection timeout */
    0x600,          /* 16-bit. Reserved. Proximity Detect On level */

    0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
    (-1),           /* 16-bit. Detect on timeout: infinite second */
    5,              /* 8-bit.  Settling occurrence number for detect on: 5 counts */
    500,            /* 16-bit. Detect ON level */
    400,            /* 16-bit. Detect ON AIR Level */
    500000,         /* 32-bit. Detect ON Stable Threshold (variance*16)*/
    0x1217,         /* 16-bit. Reserved */

    06,             /* 8-bit.  Settling occurrence number for detect off: 6s */
    70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
    20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
    0x1221,         /* 16-bit. Reserved */

    80,             /* 8-bit.  Reserved, no longer use */

    15,             /* 8-bit.  Reserved. Allowing % of signal level to drift before adjustment */
    00,             /* 8-bit.  Reserved. Drifting ajustment interval in Seconds */

    200,            /* 16-bit. Reserved. Max sampling rate after optimization.*/
    70,             /* 8-bit.  Saturation: Percentage of the signal reduced to */
    380,            /* 16-bit. LED Calibration: Maximum LED current */
    254,            /* 8-bit.  Max amount of pulses for 200Hz */
    70,             /* 8-bit.  Threshold to switch to Float Mode */
    50,             /* 8-bit.  Green LED voltage * 10. 5v*10=50 */
    50,             /* 8-bit.  Percentage of signal before Analgo Saturation  */
    10000,          /* 16-bit. Reserved. 1000/MI. ie. Mi=0.2, this value = 5000 */

    0x19999,        /* 32-bit. Reserved. motion detection threshold */
    0x001E0003,     /* 32-bit. upper = check period. lower = rested time */
    0x29999,        /* 32-bit. Reserved. High motion detection threshold. x/2^20=MI in matlab */
    0x000A,         /* 16-bit. time interval to check high motion */

    0x05,           /* 8-bit.  Reserved. CTR threshold */
    0xDC99,         /* 16-bit. Reserved. Mt2 threshold */
    0x53CC,         /* 16-bit. Reserved. Mt3 threshold */

    0x0001,         /* float mode, ambient check interval */
    0xA800,         /* ambient threshold for 100K mode */

    /***ADI_algo parameters**/
    50,     /*spotalgosamplerate*/
    6,      /*spotalgodecimation*/
    4,      /*mindifftrackSpot*/
    716,    /*initialconfidencethreshold*/
    3200,   /*ppgscale*/
    4194,   /*accelscale*/
    5,      /*spotstabilitycount*/
    15,     /*spothrtimeoutsecs*/
    1,      /*zeroorderholdnumsamples. Not used*/
    50,     /*trackalgosamplerate*/
    15,     /*trackhrtimeoutsecs*/
    5000,   /*Spotwindowlength*/
    30,     /*TrackerMinHeartrateBpm*/
     1      /*hrvEnable*/
};
#endif

const Adpd400xLibConfig_t AdpdLibCfg4000 = {
  0x00C0,         /* 16-bit. Device ID: Reg08 value . */
  0x0020,         /* 16-bit. Bit[11:0]=selected slot. 1=slotA, 2-slotB, 4=slotC */
  0x01,           /* 8-bit.  bit[1:0]=Selected Channels. 1=channel 1, 2=channel 2 */
  0x0004,         /* 16-bit. FIFO Data size in bytes. */
  0x1210,         /* 16-bit. Enable some preprocess features */ //Static AGC enabled by default //0x1010--> to disable STATIC AGC
  0,              /* 16-bit. Interval to check data rate. in Seconds */
  0x00000000,     /* 32-bit. DutyCycle Ton[0:15] , Toff[16:31], By Default Ton=Toff=0->continous PPG, if Ton>0 & Toff>0 ->Periodic PPG */
  0x0032,         /* 16-bit. Reserved */  //0x32->50Hz, 0x64->100Hz, 0x1F4->500Hz
  0x3,            /* 8-bit.  Sync Mode, Default to H/W sync */

  0x000A0001,     /* 32-bit. Proximity fs and decimation 10/1=10hz */
  10000,          /* 16-bit. Proximity detection timeout */
  0x600,          /* 16-bit. Proximity Detect On level */

  0x01900008,     /* 32-bit. Detect ON Sampling rate of 400/8=50 Hz */
  (-1),           /* 16-bit. Detect on timeout: infinite second */
  3,              /* 8-bit.  Settling occurrence number for detect on: 5 counts */
  400,            /* 16-bit. Detect ON level */
  10,             /* 16-bit. Detect ON AIR Level */
  500000,         /* 32-bit. Detect ON Stable Threshold (variance*16)*/
  0x1217,         /* 16-bit. Reserved */

  10,             /* 8-bit.  Settling occurrence number for detect off: 10 counts */
  70,             /* 8-bit.  Detect OFF percentage, trigger at: 70% */
  20000000,       /* 32-bit. Detect OFF Stable Threshold (variance*16) */
  0x1221,         /* 16-bit. Reserved */

  80,             /* 8-bit.  Detect OFF percentage during calibration */

  200,            /* 16-bit. Max sampling rate after optimization. 1-26*/
  70,             /* 8-bit.  Target DC level in percentage */
  380,            /* 16-bit. LED Calibration: Maximum LED current */
  254,            /* 8-bit.  Max amount of pulses for each repeat cycle */
  70,             /* 8-bit.  Threshold to switch to Float Mode */
  50,             /* 8-bit.  Green LED voltage = 5v */
  10000,          /* 16-bit.  1000/MI. ie. Mi=0.2, this value = 5000 */

  0x19999,        /* 32-bit. motion detection threshold */
  0x00780003,     /* 32-bit. upper = check period. lower = rested time */
  0x29999,        /* 32-bit  Reserved. High motion detection threshold. x/2^20=MI in matlab */
  0x0006,         /* 16-bit  time interval to check high motion */

  0xDC99,         /* 16-bit. Reserved Mt2 threshold */
  0x53CC,         /* 16-bit. Reserved Mt3 threshold */

  0x0001,         /* float mode, ambient check interval */
  0xA800,         /* ambient threshold for 100K mode */

  /***ADI_algo parameters**/
  50,     /*spotalgosamplerate*/
  6,      /*spotalgodecimation*/
  4,      /*mindifftrackSpot*/
  716,    /*initialconfidencethreshold*/
  3200,   /*ppgscale*/
  4194,   /*accelscale*/
  5,      /*spotstabilitycount*/
  15,     /*spothrtimeoutsecs*/
  2,      /*zeroorderholdnumsamples. Not used*/
  100,    /*trackalgosamplerate*/
  15,     /*trackhrtimeoutsecs*/
  5000,   /*Spotwindowlength*/
  30,     /*TrackerMinHeartrateBpm*/
   1      /*hrvEnable*/
};
//extern AdpdLibConfig_t gAdpdLibCfg;

#endif /* __ADPDCFG_H */
