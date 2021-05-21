/**
***************************************************************************
* @file         HipAlgorithm.c
* @author       ADI
* @version      V1.1.0
* @date         06-July-2011
* @brief        Realize Pedometer based on the new Hip Algorithm.
*
***************************************************************************
* @attention
***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2017 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the Pedometer library                *
*                                                                             *
*                                                                             *
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include "hip_algorithm.h"

//-------------------------------------- ALGORITHM CONSTANTS-------------------------------------------------------//

#define		THRESHOLD_ORDER		    4  /* NEW CONSTANT */
#define		AF_ORDER_HIP		    4
#define		WINDOW_SIZE_HIP             9
#define		_1_SECOND_HIP		    25
#define		REGULATION_OFF_TIME_HIP     50
#define		LIMIT_HIP                   50   //158 /* 0.632 gees*/
#define		INIT_OFFSET_VALUE_HIP       256  /* 1 gees*/
#define		INIT_VALUE_MIN_HIP          0xFFFF  // 0x7FFFFFFF
#define		REGULATION_MODE_HIP         0x0080
#define		OFF_REGULATION_MODE_HIP     0xFF7F


/* DEFINES and function prototypes in Algorithm.h */

//-------------------------ALGORITHM VARIABLES-------------------------//

// CIRCULAR BUFFERS
// buffer_RawData[]--> Buffer to store the not filtering module data
// buffer_filtered_window_hip[]--> Buffer to store the filtering module data

// buffer_dynamic_threshold[]-->Buffer to store the Threshold data  /*NEW*/

int32_t buffer_RawData[AF_ORDER_HIP];
int32_t buffer_filtered_window_hip[WINDOW_SIZE_HIP];
int32_t buffer_dynamic_threshold[THRESHOLD_ORDER];  /* NEW */
int32_t count_steps_hip;

// IndexWindowMin_hip--> Shows where is the window minimum
// IndexWindowMax_hip--> Shows where is the window max
// IndexThreshold--> Shows the current index in the Threshold Buffer
// IndexBuffer_hip--> Shows the current index in the window
// flag_max_min_samplecounter_hip--> Count the samples between max and min
//  to discard step if it would be very long.
// Regulation_mode-->Sets if the pedometer is in Regulation mode or not.
// StepToStepSamples_hip--> This number is incremented every algorithm iteration
//    to count the time. If it is greater than an specific value the pedometer 
//   leaves from the regulation mode. It is reset every detected step.
// possible_steps_hip--> They are the counted steps in Regulation mode,
// if the pedometer counts 4 consecutive possible steps this steps will be 
// real steps.

int8_t IndexWindowMin_hip, IndexWindowMax_hip;
int8_t IndexThreshold, IndexBuffer_hip, IndexAverage_hip;
int8_t StepToStepSamples_hip, Regulation_mode;
int8_t flag_max_min_samplecounter_hip, possible_steps_hip;

// flag_max_hip--> Maximum detected
// flag_threshold--> it indicates that the Threshold has been overcome
// flag_threshold_counter--> it indicates the number of times that the 
//  Threshold has been overcome

uint8_t flag_max_hip, flag_threshold,flag_threshold_counter;

// LastMax_hip--> Last max value
// LastMin_hip--> Last min value
// WindowMin_hip--> window minimum value
// WindowMax_hip--> window max value
// FilterMeanBuffer-->filtered mean buffer
// FilterModuleData_hip--> filtered module data
// ModuleData--> Not filtered module data
// Difference--> Difference between the max and min
// BufferDinamicThreshold--> Threshold Buffer
// NewThreshold--> New Threshold
// old_threshold--> New Threshold

uint16_t LastMax_hip, LastMin_hip, WindowMin_hip, WindowMax_hip;
uint16_t FilterMeanBuffer, FilterModuleData_hip, ModuleData;
uint16_t Difference, BufferDinamicThreshold, NewThreshold, old_threshold;

void InitAlgorithmParameters_hip() {
   int8_t i;
   flag_max_hip = 0;
   flag_max_min_samplecounter_hip = 0;
   count_steps_hip = 0;
   IndexBuffer_hip = 0;
   IndexAverage_hip = 0;
   IndexWindowMin_hip = 0;
   IndexWindowMax_hip = 0;
   IndexThreshold = 0;
   FilterMeanBuffer = 0;
   FilterModuleData_hip = 0;
   StepToStepSamples_hip = 0;
   Regulation_mode = 0;
   possible_steps_hip = 0;
   NewThreshold = 0;
   LastMax_hip = 0;
   LastMin_hip = 32000;
   Difference = 0;
   ModuleData = 0;
   // 65536-->1gee value (acceleration offset value)
   old_threshold = INIT_OFFSET_VALUE_HIP;
   BufferDinamicThreshold = (INIT_OFFSET_VALUE_HIP)<<2; // 65536 * 4 (Buffer)
   flag_threshold = 0;
   flag_threshold_counter = 0;

   for (i = 0; i < THRESHOLD_ORDER; i++) {
     buffer_dynamic_threshold[i] = INIT_OFFSET_VALUE_HIP;
   }
   for (i = 0; i < AF_ORDER_HIP; i++) {
     buffer_RawData[i] = 0;
   }
   for (i = 0; i < WINDOW_SIZE_HIP; i++) {
     buffer_filtered_window_hip[i] = 0;
   }
}


int32_t StepAlgorithm_hip(int16_t *Status, int16_t X, int16_t Y, int16_t Z) {

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++   SIGNAL FILTERING   ++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      uint16_t Abs_X, Abs_Y, Abs_Z;

      Abs_X = abs(X);
      Abs_Y = abs(Y);
      Abs_Z = abs(Z);
      ModuleData = Abs_X + Abs_Y + Abs_Z;
#if 1
      // Eliminate the last value from the mean and add the new value
      FilterMeanBuffer = FilterMeanBuffer - buffer_RawData[IndexAverage_hip] + ModuleData; 
      FilterModuleData_hip = (FilterMeanBuffer>>2);
      // stores the module in the not filtered buffer
      buffer_RawData[IndexAverage_hip] = ModuleData; 

      buffer_filtered_window_hip[IndexBuffer_hip] = FilterModuleData_hip;

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++  MAX AND MIN DETECTION  ++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

       /* The algorithm updates every iteration the max and min. With this method
       we ensure that we don't lose any max o min. The if structure takes less 
      time than a for loop. Around 2 ms lesser (working at 32kHz) */

       WindowMax_hip = buffer_filtered_window_hip[0];
       IndexWindowMax_hip = 0;

       if (buffer_filtered_window_hip[1] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[1];
         IndexWindowMax_hip = 1;
       }
       if (buffer_filtered_window_hip[2] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[2];
         IndexWindowMax_hip = 2;
       }
       if (buffer_filtered_window_hip[3] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[3];
         IndexWindowMax_hip = 3;
       }
       if (buffer_filtered_window_hip[4] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[4];
         IndexWindowMax_hip = 4;
       }
       if (buffer_filtered_window_hip[5] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[5];
         IndexWindowMax_hip = 5;
       }
       if (buffer_filtered_window_hip[6] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[6];
         IndexWindowMax_hip = 6;
       }
       if (buffer_filtered_window_hip[7] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[7];
         IndexWindowMax_hip = 7;
       }
       if (buffer_filtered_window_hip[8] > WindowMax_hip) {
         WindowMax_hip = buffer_filtered_window_hip[8];
         IndexWindowMax_hip = 8;
       }

       WindowMin_hip = buffer_filtered_window_hip[0];
       IndexWindowMin_hip = 0;

       if (buffer_filtered_window_hip[1] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[1];
         IndexWindowMin_hip = 1;
       }

       if (buffer_filtered_window_hip[2] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[2];
         IndexWindowMin_hip = 2;
       }

        if (buffer_filtered_window_hip[3] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[3];
         IndexWindowMin_hip = 3;
       }

        if (buffer_filtered_window_hip[4] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[4];
         IndexWindowMin_hip = 4;
       }

        if (buffer_filtered_window_hip[5] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[5];
         IndexWindowMin_hip = 5;
       }

        if (buffer_filtered_window_hip[6] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[6];
         IndexWindowMin_hip = 6;
       }

        if (buffer_filtered_window_hip[6] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[6];
         IndexWindowMin_hip = 6;
       }

        if (buffer_filtered_window_hip[7] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[7];
         IndexWindowMin_hip = 7;
       }

       if (buffer_filtered_window_hip[8] < WindowMin_hip) {
         WindowMin_hip = buffer_filtered_window_hip[8];
         IndexWindowMin_hip = 8;
       }

        //======================================================================
        //MAX SEARCHING
 if (flag_max_hip == 0) { 
   // Now, we try to know if the Window max is in the midle of the window,
   //  in this case I will mark it as a max
   if (IndexWindowMax_hip == ((IndexBuffer_hip + (WINDOW_SIZE_HIP>>1)) % WINDOW_SIZE_HIP)) { 
     flag_max_hip = 1;
     LastMax_hip = WindowMax_hip;
     flag_max_min_samplecounter_hip = 0;
   }
 } else { 
   // Now, we try to know if the Window min is in the midle of the window,
   //  in this case I will mark it as a min
   if (IndexWindowMin_hip == ((IndexBuffer_hip + (WINDOW_SIZE_HIP>>1)) % WINDOW_SIZE_HIP)) {
   LastMin_hip = WindowMin_hip;  // Updates the MIN
   Difference = LastMax_hip - LastMin_hip;  // Updates the Difference
   flag_max_hip = 0;
   flag_max_min_samplecounter_hip = 0;  // Reset the max-min sample counter

   /* The algorithm detects if the limits are in of the Threshold level */

   if ((LastMax_hip > old_threshold)&&(LastMin_hip < old_threshold)) {
     flag_threshold = 1;  // Possible step (it will be analize later)
     flag_threshold_counter = 0;
   } else {
     flag_threshold_counter++;  // Number of times the limits are out of the Threshold Level consecutively
   }
   /* Limit detection */
   /* Each time the difference is higher than the fixed limit the algorithm updates the Threshold */

   //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   //+++++++++++++++++++++++ THRESHOLD LEVEL UPDATE ++++++++++++++++++++++++++++
   //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   if (Difference > LIMIT_HIP) {
     /* Threshold level is calculated with the 4 previous good differences */
     // The same method as the Filtering method
     NewThreshold = (LastMax_hip + LastMin_hip) >> 1;  // Calculates the New Threshold
     BufferDinamicThreshold = BufferDinamicThreshold - buffer_dynamic_threshold[IndexThreshold] + NewThreshold;
     old_threshold =(BufferDinamicThreshold) >> 2;
     buffer_dynamic_threshold[IndexThreshold] = NewThreshold;
     IndexThreshold++;
     if (IndexThreshold > THRESHOLD_ORDER - 1) {
       IndexThreshold = 0;
     }

     // Once the Threshold Level is calculated and if the limits was in of the
     // Threshold Level we certificate the step.

     //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     //++++++++++++++++++++++++ STEP CERTIFICATION   +++++++++++++++++++++++++
     //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

     if (flag_threshold) {
       // DETECTED STEP
       flag_threshold = 0;
       StepToStepSamples_hip = 0; // Reset the time.
       flag_max_min_samplecounter_hip = 0;

       if (Regulation_mode) {
         //=====Regulation mode======
         // the step is counted
         count_steps_hip++;
       } else {
         //=====Non Regulation mode======
         possible_steps_hip++;

         // if the possible steps number is 8,
         // the algorithm concludes that the person is walking.
         if (possible_steps_hip == 8) {
           count_steps_hip = count_steps_hip + possible_steps_hip; // Possible steps are added to counted steps
           possible_steps_hip = 0;
           Regulation_mode = 1;  // The pedometer enters the Regulation mode
           *Status |= REGULATION_MODE_HIP;
         }
       }
     }
   }
   WindowMin_hip = INIT_VALUE_MIN_HIP;
   // If the limits are out of the Threshold Level 2 times consecutively with a good difference,
   // Is very probably that the acceleration profile isn't a step sequence
   // (Reset the possible steps to discard false steps)
   if (flag_threshold_counter > 1) {
     flag_threshold_counter = 0;
     flag_max_min_samplecounter_hip = 0;
     WindowMax_hip = 0;  // maybe it is not necessary  reset them.
     WindowMin_hip = 0;
     Regulation_mode = 0;
     *Status &= OFF_REGULATION_MODE_HIP;
     possible_steps_hip = 0;
     flag_threshold_counter = 0;
   }
   } else {
     // While the pedometer is searching a min if between MAX and MIN
     //  takes more than 25 samples I reset the step.
     flag_max_min_samplecounter_hip++;
     if (flag_max_min_samplecounter_hip == _1_SECOND_HIP) {
       // Reset Step
       flag_max_min_samplecounter_hip = 0;
       WindowMax_hip = 0;
       WindowMin_hip = 0;
       flag_max_hip = 0;
       possible_steps_hip = 0;
     }
   }
 }
 // IndexBuffer_hip is used as a circular buffer index
 IndexBuffer_hip++;
 if (IndexBuffer_hip > WINDOW_SIZE_HIP-1) {
   IndexBuffer_hip = 0;
 }

  // IndexAverage_hip is used as a circular buffer index to average
  IndexAverage_hip++;
  if (IndexAverage_hip > AF_ORDER_HIP-1) {
    IndexAverage_hip = 0;
  }

  StepToStepSamples_hip++;
  if (StepToStepSamples_hip >= REGULATION_OFF_TIME_HIP) 
  {
    // If the pedometer takes 2 seconds without counting a step it
    //  returns to the Non Regulation mode.
    StepToStepSamples_hip = 0;
    possible_steps_hip = 0;
    Regulation_mode = 0;
    flag_max_min_samplecounter_hip = 0;
    *Status &= OFF_REGULATION_MODE_HIP;
    if (Regulation_mode == 1) {
      Regulation_mode = 0;
      old_threshold = INIT_OFFSET_VALUE_HIP;
      BufferDinamicThreshold = (INIT_OFFSET_VALUE_HIP)<<2;
      IndexThreshold = 0;
      flag_threshold_counter = 0;
      for (char i = 0; i < THRESHOLD_ORDER; i++) {
        buffer_dynamic_threshold[i] = INIT_OFFSET_VALUE_HIP;
      }
    }
  }
#endif
  return count_steps_hip;
}
