/*
  Arduino DUE Watch Test Controller

*/
 
#include "data_samples.h"
#include "CommandHandler.h"
// #include "Array.h"


int       fast_mode  =    0;
int       freq_inhz  = 1000; // Default Hz
int       freq_intc  =    0; 
int       user_intf  =    0;
volatile   uint16_t   sptr       =    0;
uint32_t  active_wfm[NWAVE];
uint8_t   amp_per = 100;
float     amp_volts_fs = 2.3;


CommandHandler<> SerialCommandHandler;

void Generate_Waveform()
{
  int temp = 0;
  fast_mode = 0;
  if( freq_inhz >  10000 ) fast_mode = 1;
  if( freq_inhz > 100000 ) fast_mode = 2;

  if (((freq_inhz > 0) && (freq_inhz < 200000)) && ((amp_per > 0) && (amp_per <= 100)))
  {
    freq_intc = freqToTc(freq_inhz);
    TC_setup();        
    Serial.print("Fast Mode = ");
    Serial.println(fast_mode, DEC);      
    Serial.print("freq_inhz = ");
    Serial.println(freq_inhz, DEC);      
    Serial.print("freq_intc = ");
    Serial.println(freq_intc, DEC);      
    Serial.print("approximation = ");
    temp = tcToFreq(freq_intc);
    Serial.println(temp, DEC);
  }
  else
  {
    Serial.println("ERR: Invalid Inputs!");
  }
}

void Cmd_CfgDacWfm(CommandParameter &Parameters)
{  
  freq_inhz = Parameters.NextParameterAsInteger();
  String amp_volts_str = Parameters.NextParameter();
  float amp_volts = amp_volts_str.toFloat();
  amp_per = ampVoltsToAmpPer(amp_volts);
  Generate_Waveform();  
}

void Cmd_SetWfmFreq(CommandParameter &Parameters)
{
  freq_inhz = Parameters.NextParameterAsInteger();
  Generate_Waveform();
}

void Cmd_SetWfmAmp(CommandParameter &Parameters)
{
  String amp_volts_str = Parameters.NextParameter();
  float amp_volts = amp_volts_str.toFloat();
  amp_per = ampVoltsToAmpPer(amp_volts);
  Generate_Waveform();
}

void Cmd_CfgIOMap(CommandParameter &Parameters)
{
  int arr_size;
  // Configuring output IO pins
  arr_size = sizeof(gpo_arr)/sizeof(gpo_arr[0]);
  for (int i=0; i<arr_size; i++)
  {
    pinMode(gpo_arr[i], OUTPUT);
    Serial.print("Init GPIO OUT: ");
    Serial.println(gpo_arr[i]);
  }

  // Configuring Input IO pins
  arr_size = sizeof(gpi_arr)/sizeof(gpi_arr[0]);
  for (int i=0; i<arr_size; i++)
  {
    pinMode(gpi_arr[i], INPUT);
    Serial.print("Init GPIO IN: ");
    Serial.println(gpi_arr[i]);
  }
}

void Cmd_SetIO(CommandParameter &Parameters)
{
  uint8_t io_id = Parameters.NextParameterAsInteger();
  uint8_t io_state = Parameters.NextParameterAsInteger();
  if (io_state > 0)
  {
    digitalWrite(io_id, HIGH);
    Serial.print("Setting GPIO OUT: ");
    Serial.println(io_id);
  }
  else
  {
    digitalWrite(io_id, LOW);
  }
}

void Cmd_GetIO(CommandParameter &Parameters)
{
  uint8_t io_id = Parameters.NextParameterAsInteger();
  Serial.println("TBD");
}

void Cmd_GetAnalogIn(CommandParameter &Parameters)
{
  int ain_pin = A0;
  int ain_val = 0;
  uint8_t ain_pin_idx = Parameters.NextParameterAsInteger();
  uint8_t capt_time_secs = Parameters.NextParameterAsInteger();
  uint8_t delay_ms = Parameters.NextParameterAsInteger();
  uint16_t starttime = millis();
  uint16_t endtime = starttime;

  switch (ain_pin_idx) 
  {
    case 0:
      ain_pin = A0;
      break;
    case 1:
      ain_pin = A1;
      break;
    case 2:
      ain_pin = A2;
      break;
    case 3:
      ain_pin = A3;
      break;
    case 4:
      ain_pin = A4;
      break;
    default:
      ain_pin = A0;
      break;
   }
  
  while ((endtime - starttime) <= capt_time_secs*1000) // do this loop for up to n secs
  {
    ain_val = analogRead(ain_pin);
    Serial.println(ain_val);
    endtime = millis();
    delay(delay_ms);
  }
  
}

void Cmd_TestCmd(CommandParameter &Parameters)
{
  String str_var = Parameters.NextParameter();
  float flt_var = str_var.toFloat();
  uint8_t amp_per = ampVoltsToAmpPer(flt_var);
  Serial.println(amp_per);
}

void setup()
{
  Serial.begin (115200); 
  dac_setup();        
  // freq_intc = freqToTc(freq_inhz);
  // TC_setup();        
  setup_pio_TIOA0();

  SerialCommandHandler.AddCommand(F("CfgDacWfm"), Cmd_CfgDacWfm);
  SerialCommandHandler.AddCommand(F("SetWfmFreq"), Cmd_SetWfmFreq);
  SerialCommandHandler.AddCommand(F("SetWfmAmp"), Cmd_SetWfmAmp);
  SerialCommandHandler.AddCommand(F("CfgIOMap"), Cmd_CfgIOMap);
  SerialCommandHandler.AddCommand(F("SetIO"), Cmd_SetIO);
  SerialCommandHandler.AddCommand(F("GetIO"), Cmd_GetIO);
  SerialCommandHandler.AddCommand(F("TestCmd"), Cmd_TestCmd);
  SerialCommandHandler.AddCommand(F("GetAnalogIn"), Cmd_GetAnalogIn);
  

  // pinMode(22, OUTPUT);
  // digitalWrite(22, HIGH);
}

int tcToFreq( int tc_cntr)
{
  int freq_hz;     

  if( tc_cntr == 0 ) return 1000;

  if( fast_mode ) freq_hz = (420000000UL / tc_cntr) / (2 * NWAVE); 
  else            freq_hz = ( 42000000UL / tc_cntr) / (2 * NWAVE);

  if( fast_mode == 2 ) freq_hz *= 2;

  return freq_hz;   
}

int freqToTc( int freq_hz)
{
  int tc_cntr = 0;

  if( freq_hz == 0 ) return 25;

  if( fast_mode == 0 )  tc_cntr = (  42000000UL / freq_hz) / (2 * NWAVE);
  if( fast_mode == 1 )  tc_cntr = ( 420000000UL / freq_hz) / (2 * NWAVE); 
  if( fast_mode == 2 )  tc_cntr = ( 840000000UL / freq_hz) / (2 * NWAVE); 

  return tc_cntr;
}

void loop() 
{
  char in_Byte;
  int     temp;

  SerialCommandHandler.Process();
  
}

uint32_t ampVoltsToAmpPer(float amp_volts)
{
  uint8_t amp_per;
  if (amp_volts <= amp_volts_fs)
  {
    amp_per = round((amp_volts * 100)/amp_volts_fs);
  }
  else
  {
    amp_per = 100;
  }
  return amp_per;
}

uint32_t findAmplitudeCode()
{
  uint32_t dac0_data=0;
  uint32_t max_val=0;
  uint32_t min_val=0;
  uint32_t amplitude=0;
  for (int i=0; i<NWAVE; i++) 
  {
    dac0_data = ((Sinewave[0][0][i] & 4095) * ((float)amp_per/100));
    if (i == 0) 
    {
      max_val = dac0_data;
      min_val = dac0_data;
    }
    else
    {
      if (dac0_data > max_val)
      {
        max_val = dac0_data;
      }
      if (dac0_data < min_val)
      {
        min_val = dac0_data;
      }
    }
  }
  amplitude = round((max_val-min_val)/2);
  
  return amplitude;
}

void DACC_Handler(void)
{
  if((dacc_get_interrupt_status(DACC) & DACC_ISR_ENDTX) == DACC_ISR_ENDTX) {
    ++sptr; 
    sptr &=  0x01;
      // ***********************************************************************************
      // Changing DAC0 and DAC1 amplitude according to amp_per variable value
      // amp_per is the percentage of full scale value of DAC
      uint32_t dac_data=0;
      uint32_t dac0_data=0;
      uint32_t dac1_data=0;
      for (int i=0; i<NWAVE; i++) {
        dac_data = Sinewave[0][0][i];
        dac0_data = ((dac_data & 4095) * ((float)amp_per/100));
        dac1_data = (((dac_data >> 16) & 4095) * ((float)amp_per/100));
        dac_data =  (0x10000000 + ((dac1_data << 16) + dac0_data));
        active_wfm[i] = dac_data;  
      }
      // **********************************************************************************
      DACC->DACC_TNPR =  (uint32_t)  active_wfm;      // next DMA buffer
      DACC->DACC_TNCR =  NWAVE;
  }
}

void setup_pio_TIOA0()  
{
  PIOB->PIO_PDR = PIO_PB25B_TIOA0;  
  PIOB->PIO_IDR = PIO_PB25B_TIOA0;  
  PIOB->PIO_ABSR |= PIO_PB25B_TIOA0;
}


void TC_setup ()
{
  pmc_enable_periph_clk(TC_INTERFACE_ID + 0 *3 + 0); 

  TcChannel * t = &(TC0->TC_CHANNEL)[0];            
  t->TC_CCR = TC_CCR_CLKDIS;                        
  t->TC_IDR = 0xFFFFFFFF;                           
  t->TC_SR;                                         
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |          
              TC_CMR_WAVE |                         
              TC_CMR_WAVSEL_UP_RC |                 
              TC_CMR_EEVT_XC0 |     
              TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
              TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR;
  
  t->TC_RC = freq_intc;
  t->TC_RA = freq_intc /2;       
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET; 
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;    
}

void dac_setup ()
{
  pmc_enable_periph_clk (DACC_INTERFACE_ID) ; // start clocking DAC
  dacc_reset(DACC);
//  dacc_set_transfer_mode(DACC, 0);
  dacc_set_transfer_mode(DACC, 1); // word trasfer mode
  
  dacc_set_power_save(DACC, 0, 1);            // sleep = 0, fastwkup = 1
  dacc_set_analog_control(DACC, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));
  dacc_set_trigger(DACC, 1);
  
  dacc_enable_channel(DACC, 1);
//  dacc_set_channel_selection(DACC, 0);
  dacc_enable_channel(DACC, 0);
  dacc_enable_flexible_selection(DACC);

  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);

  DACC->DACC_TPR  =  (uint32_t)  Sinewave[fast_mode][0];      // DMA buffer
  DACC->DACC_TCR  =  NWAVE;
  DACC->DACC_TNPR =  (uint32_t)  Sinewave[fast_mode][1];      // next DMA buffer
  DACC->DACC_TNCR =  NWAVE;
  DACC->DACC_PTCR =  0x00000100;  //TXTEN - 8, RXTEN - 1.
}
