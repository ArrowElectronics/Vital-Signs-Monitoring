/**
    ***************************************************************************
    * @addtogroup User
    * @{
    * @file         variables.c
    * @author       ADI
    * @version      V1.0.0
    * @date         18-March-2016
    * @brief        Variables used in the library.
    *
    ***************************************************************************
    * @attention
    ***************************************************************************
*/
/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2019 Analog Devices Inc.                                      *
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
* This software is intended for use with the ADPD and derivative parts     *
* only                                                                        *
*                                                                             *
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "Variables.h"
#include "AdpdLib.h"


/* ------------------------- Defines  --------------------------------------- */


/* ------------------------- Public Function Prototypes -------------------- */


/* ------------------------- Public Variables ------------------------------ */
uint32_t gnTempData[4*SD_BUFFER_SIZE]; // Temporary buffer
