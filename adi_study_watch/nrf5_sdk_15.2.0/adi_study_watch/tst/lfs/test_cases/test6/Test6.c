#include "Test6.h"
#include "hw_if_config.h"
#include "light_fs.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MAX_NO_FILE_NAME_LEN            30

//uint8_t dcfg_org_4000_g[10] = {1,2,3,4,5,6,7,8,9};
#if 1
uint32_t dcfg_org_4000_g[] = {
    //0x000F8000,
    0x00090097,
    0x00078FFF,
    0x000B02F6,
    0x000C0012,
    0x000D4E20,//0x2710
    0x000E0000,
    0x000F0006,
    0x00148000,
    0x00202222,
    0x00210000,
    0x00220003,
    0x00230302,
    0x00240001,
/*Configuration for Time slot F*/
    0x01A00000,
    0x01A141DA,
    0x01A20003,
    0x01A35002,
    //0x01A4E2C1,
#ifdef PCBA
    0x01A50200,
#else
    0x01A50002,//0040
#endif
    0x01A60000,


    0x01A70140, // 64 pulse
    //0x01A90000,//added
    0x01AB0E10,
    0x01AE1F00,
    0x01AF0000,
    //0x01100004,
    //0x01200010,
    0x00100500, /* Enable slot F for green LED data*/

/*Moving thermistor sensor to slot D*/
#if 1
    0x01600000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x016141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot A */
    0x01620010,   /*IN3 connected to CH1 of slot A*/
    0x01635A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state, 
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x0164E281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */
    0x01650000,
    0x01660000,
    0x01670101,   /*single pulse and single repition is used*/
    0x01680000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01690419,   /*LED pulse width and offset*/
    0x016A0003,   /*Integrator clock width set to 3us*/
    0x016B130F,   /*Integrator offset set to 15.595us*/
    0x016C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x016E0000,   /*ADC CH1 offset set to 0*/
    0x016F0000,   /*ADC CH2 offset set to 0*/
    0x01700004,   /* signal size Data format set to 4 bytes;*/
    //0x01720310,   /* Decimation set to 50 */
#endif //slot D

/*Moving calibration resistor to slot E*/
#if 1
    0x01800000,   /*INPUT_RESISTOR set ot 500 ohm*/
    0x018141DA,   /*pre conditioning period of 8us, TIA+INT+BPF+ADC signal path selected for Slot B */
    0x01820030,   /*IN4 connected to CH1 of slot B*/
    0x01835A40,   /*Pre condition with TIA V_ref,VC2_pulse changes between active state and alternate state, 
                    VC2_Active = V-ref and VC2_alternate = V_ref + 250mV*/
    0x0184E281,   /* R_int = 400K, R_tia_c h1 = 100k, TIA_Vref = 0.8855  V, Vref_pulse = 0.8855 V; No v-ref pulsing */
    0x01850000,
    0x01860000,
    0x01870101,   /*single pulse and single repition is used*/
    0x01880000,   /*MOD_TYPE set to mode 0; normal mode*/
    //0x01890419,   /*LED pulse width and offset*/
    0x018A0003,   /*Integrator clock width set to 3us*/
    0x018B130F,   /*Integrator offset set to 15.595us*/
    0x018C0210,   /*Modulation pulse width set to 2us, with offset of 16us*/
    0x018E0000,   /*ADC CH1 offset set to 0*/
    0x018F0000,   /*ADC CH2 offset set to 0*/
    0x01900004,   /*signal size data format set to 4 bytes; */
    //0x01920310,   /* Decimation set to 50 */
#endif //slot E

/* Set the signal and dark sample data size of the slots from A to E to zero*/
    0x01100000, 
    0x01300000,
    0x01500000,
    //0x01700000,
    //0x01900000,
    0x01B00004,
    0xFFFFFFFF,
};
#endif

#define PAGES_PER_BLOCK 64

//config file test
void Test6(_file_handler *file_handler, _table_file_handler *table_file_handler, struct _memory_buffer *light_fs_mem)
{
    NRF_LOG_INFO("Start of Test 6");


    initialize_circular_buffer();

     //Create config file
    uint8_t i;
    char file_name[MAX_NO_FILE_NAME_LEN];
    uint8_t number_of_files=1;
    uint32_t total_file_size = 0;
    uint32_t file_size = sizeof(dcfg_org_4000_g);
    uint32_t error_counter = 0;

    NRF_LOG_INFO("******* File size to write = %d ***********",file_size);
    
    //Erase cfg block
        // test for knowing format time
    uint32_t t1 = MCU_HAL_GetTick();

    //erase config block 
    if(nand_flash_block_erase(CFGBLOCK*PAGES_PER_BLOCK) != NAND_FUNC_SUCCESS)
      {
        NRF_LOG_INFO("Error in erasing CFG block");
      }
    
    uint32_t t2 = MCU_HAL_GetTick();
    uint32_t format_time = abs(t2 - t1);

    NRF_LOG_INFO("Format time for single block = %d", format_time);


     // get remaining space
    uint32_t remaining_bytes=0;
    uint32_t used_mem_bytes = 0;
   
   if(lfs_get_remaining_space(&remaining_bytes,&used_mem_bytes,LFS_CONFIG_FILE,table_file_handler) != LFS_SUCCESS){
      NRF_LOG_INFO("Error in getting remanining bytes");
   }
   else{
      NRF_LOG_INFO("Remaining bytes = %d,Used memory bytes = %d",
        remaining_bytes,
        used_mem_bytes);
   }

    for(i=1;i<=number_of_files;i++)
    {
      memset(file_name,0,MAX_NO_FILE_NAME_LEN);
      snprintf(file_name,MAX_NO_FILE_NAME_LEN,"file_name_%d",i);
      
      if(create_and_write_config_file((uint8_t*)dcfg_org_4000_g,file_handler,table_file_handler,light_fs_mem,file_size,file_name,LFS_MODE_FAST)  != LFS_TESTBENCH_SUCCESS)
      {
        NRF_LOG_INFO("Error creating new file");
       // while(1);
      }
      total_file_size+=file_size;
    }

    // get remaining space
    remaining_bytes=0;
   used_mem_bytes = 0;
   
   if(lfs_get_remaining_space(&remaining_bytes,&used_mem_bytes,LFS_CONFIG_FILE,table_file_handler) != LFS_SUCCESS){
      NRF_LOG_INFO("Error in getting remanining bytes");
   }
   else{
      NRF_LOG_INFO("Remaining bytes = %d,Used memory bytes = %d",
        remaining_bytes,
        used_mem_bytes);
   }
   // read back from written config page
  if(check_config_files((uint8_t*)dcfg_org_4000_g,&error_counter) == LFS_TESTBENCH_SUCCESS){
    NRF_LOG_INFO("Config files check success");
  NRF_LOG_INFO("****** End of Test 6 ****************");
  }
}