/*
  Arduino DUE sine / cosine generator

  Dac-0 outputs sine wave, and dac-1 consequently cosine.
  Frequency may be changed via serial monitor, format of the command line:
  "nnnnnnf", where nnnnn is any number in 1 - 199 999 range (Hz)
  and "f" is a letter for frequency.
 
 */

#define    NWAVE               80

uint32_t  Sinewave[3][2][NWAVE] = {
  {
    { // 0 - 10 kHz
0x1FFF0800,0x1FF908A1,0x1FE60940,0x1FC609DE,0x1F9B0A79,0x1F630B0F,0x1F200BA1,0x1ED10C2E,0x1E780CB3,0x1E150D31,0x1DA70DA7,0x1D310E15,0x1CB30E78,0x1C2E0ED1,0x1BA10F20,0x1B0F0F63,
0x1A790F9B,0x19DE0FC6,0x19400FE6,0x18A10FF9,0x18000FFF,0x175F0FF9,0x16C00FE6,0x16220FC6,0x15870F9B,0x14F10F63,0x145F0F20,0x13D20ED1,0x134D0E78,0x12CF0E15,0x12590DA7,0x11EB0D31,
0x11880CB3,0x112F0C2E,0x10E00BA1,0x109D0B0F,0x10650A79,0x103A09DE,0x101A0940,0x100708A1,0x10010800,0x1007075F,0x101A06C0,0x103A0622,0x10650587,0x109D04F1,0x10E0045F,0x112F03D2,
0x1188034D,0x11EB02CF,0x12590259,0x12CF01EB,0x134D0188,0x13D2012F,0x145F00E0,0x14F1009D,0x15870065,0x1622003A,0x16C0001A,0x175F0007,0x18000001,0x18A10007,0x1940001A,0x19DE003A,
0x1A790065,0x1B0F009D,0x1BA100E0,0x1C2E012F,0x1CB30188,0x1D3101EB,0x1DA70259,0x1E1502CF,0x1E78034D,0x1ED103D2,0x1F20045F,0x1F6304F1,0x1F9B0587,0x1FC60622,0x1FE606C0,0x1FF9075F
    },
    {
0x1FFF0800,0x1FF908A1,0x1FE60940,0x1FC609DE,0x1F9B0A79,0x1F630B0F,0x1F200BA1,0x1ED10C2E,0x1E780CB3,0x1E150D31,0x1DA70DA7,0x1D310E15,0x1CB30E78,0x1C2E0ED1,0x1BA10F20,0x1B0F0F63,
0x1A790F9B,0x19DE0FC6,0x19400FE6,0x18A10FF9,0x18000FFF,0x175F0FF9,0x16C00FE6,0x16220FC6,0x15870F9B,0x14F10F63,0x145F0F20,0x13D20ED1,0x134D0E78,0x12CF0E15,0x12590DA7,0x11EB0D31,
0x11880CB3,0x112F0C2E,0x10E00BA1,0x109D0B0F,0x10650A79,0x103A09DE,0x101A0940,0x100708A1,0x10010800,0x1007075F,0x101A06C0,0x103A0622,0x10650587,0x109D04F1,0x10E0045F,0x112F03D2,
0x1188034D,0x11EB02CF,0x12590259,0x12CF01EB,0x134D0188,0x13D2012F,0x145F00E0,0x14F1009D,0x15870065,0x1622003A,0x16C0001A,0x175F0007,0x18000001,0x18A10007,0x1940001A,0x19DE003A,
0x1A790065,0x1B0F009D,0x1BA100E0,0x1C2E012F,0x1CB30188,0x1D3101EB,0x1DA70259,0x1E1502CF,0x1E78034D,0x1ED103D2,0x1F20045F,0x1F6304F1,0x1F9B0587,0x1FC60622,0x1FE606C0,0x1FF9075F
    }
  },
  {
    {// 10 - 100 kHz
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259
    },
    {
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,
0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259,0x1FFF0800,0x1DA70DA7,0x18000FFF,0x12590DA7,0x10010800,0x12590259,0x18000001,0x1DA70259
    }
  },
  {
    {// 100 - 200 kHz
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001
    },
    {
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,
0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001,0x1FFF0800,0x18000FFF,0x10010800,0x18000001
    }
  }
};

            int       fast_mode  =    0;
            int       freq_inhz  = 1000; // Default Hz
            int       freq_intc  =    0; 
            int       user_intf  =    0;

volatile   uint16_t   sptr       =    0;
uint32_t active_wfm[NWAVE];
uint8_t amp_per = 100;

void setup()
{
  Serial.begin (115200); 
  dac_setup();        
  freq_intc = freqToTc(freq_inhz);
  TC_setup();        
  setup_pio_TIOA0(); 
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
          
  if (Serial.available() > 0) {
    in_Byte = Serial.read();
    // Message Format To Set  Frequency:  1000f + "send".
    if((in_Byte >= '0') && (in_Byte <= '9')) 
    {
      user_intf = (user_intf * 10) + (in_Byte - '0'); 
    }
    else
    { 
      if (in_Byte == 'f') // end delimiter 
      {
        if ((user_intf > 0) && (user_intf < 200000)) 
        {
          freq_inhz = user_intf;
            fast_mode = 0;
            if( freq_inhz >  10000 ) fast_mode = 1;
            if( freq_inhz > 100000 ) fast_mode = 2;
          
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
      user_intf = 0; // reset to 0 ready for the next sequence of digits
      }
      else if (in_Byte == 'a') // amplitude change end delimiter
      {
        if ((user_intf > 1) && (user_intf <= 100)) 
        {
          amp_per=user_intf;
        }
        else
        {
          amp_per=100;
        }
        TC_setup();
        Serial.print("Amplitude Change (%) = ");
        Serial.println(amp_per);
        Serial.print("Fast Mode = ");
        Serial.println(fast_mode, DEC);      
        Serial.print("freq_inhz = ");
        Serial.println(freq_inhz, DEC);      
        Serial.print("freq_intc = ");
        Serial.println(freq_intc, DEC);     
        user_intf = 0; // reset to 0 ready for the next sequence of digits
      }
          
    }
  }
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
