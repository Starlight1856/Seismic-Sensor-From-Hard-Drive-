
//***************************************************************************
//** ad677d
//** BeagleBone Black PRU driver for AD677 16 Bit SAR ADC
//**
//** Fundamental acquisition rate 10kHz
//**
//** Each ADC data sample is acquired at max recommended rate, but delay
//** between each sample sets fundamental rate of 10kHz.
//**
//** Every 50 samples of 16 bit data summed to produce 200Hz rate
//** Result stored as 32bit ints. Averaging performed in main code.
//**
//** 2-page bufffer used with flag set when each page buffer full.
//** Each page stores one seconds worth of data @200Hz rate.
//**

#include <stdint.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include <pru_ctrl.h>
#include "resource_table_empty.h"


#define PAGE_SIZE 200
#define MAX_BUFF_SIZE  400
#define PAGE1_FULL  (MAX_BUFF_SIZE + 1)
#define PAGE2_FULL  (MAX_BUFF_SIZE + 2)
#define RESET_FLAG  (MAX_BUFF_SIZE + 3)
#define EXIT_FLAG   (MAX_BUFF_SIZE + 4)


#define PRU0_DRAM       0x00000         // Offset to DRAM
#define PRU_SHAREDMEM   0x10000

//** 00 0000

const int DAIGNOS  = 0x000000080;  // bit 7
const int ADC_CLK  = 0x000000020;  // bit 5
const int ADC_SAMP = 0x000000008;  // bit 3
const int ADC_CAL  = 0x000000002;  // bit 1

const int ADC_BUSY =  0x000000004; // bit 2
const int ADC_SDATA = 0x000000001; // bit 0

const int NUM_SAMPLES = 50;

volatile register uint32_t __R30;
volatile register uint32_t __R31;

//** Shared memory
// Skip the first 0x200 byte of DRAM since the Makefile allocates
// 0x100 for the STACK and 0x100 for the HEAP.
volatile int32_t *pru0_dram = (int *)(PRU0_DRAM + 0x200);

volatile int32_t *pru_shared_mem = ( int *)(PRU_SHAREDMEM + 0x200);


//** Input signals
//** bit 5 = adc clock: pru op
//** bit 3 = samp: pru op
//** bit 1 = cal: pru op

//** Output signals
//** bit 2 = sdata: pru inp - serial data
//** bit 0 = busy: pru inp

void get_adc( short* p_adcsamp);
void cal_adc(void);

/**
 * main.c
 */
void main(void)
{
    short adc_samp;
    int i=0;
    int active_page;
    int data_count;
    int32_t pru_accumulator =0;



    /* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
     CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;
     CT_CFG.GPCFG0 = 0;

     pru_shared_mem[PAGE1_FULL] = 0;
     pru_shared_mem[PAGE2_FULL] = 0;
     pru_shared_mem[RESET_FLAG] = 0;
     pru_shared_mem[EXIT_FLAG]  = 0;

     data_count =0;
     pru_accumulator = 0;
     active_page =1;


     //** reset outputs, startup delay, calibrate ADC
     __R30= 0x0000;
     __delay_cycles(200000000);
     cal_adc();

     //*
     while( 1 )
       {
       get_adc( &adc_samp );
       pru_accumulator += adc_samp;
       i++;

       //** accumulate 50 samples for 200Hz; averaging will be performed in main code

       if( i == NUM_SAMPLES ) //** samples accumulated; now store in page
         {
         pru_shared_mem[data_count] = (pru_accumulator/NUM_SAMPLES);
         data_count++;

         //** Active page must be either page 1 or 2
         if( active_page == 1)
            {
            if( data_count == PAGE_SIZE) //** page 1 full = 1/2 MAX_BUFF_SIZE
              {
              //** Note data_count is not reset here as it continues to load page 2
              //** Page 1 full; now start filling page 2
              //** Userspace application resets page full flag
              pru_shared_mem[PAGE1_FULL] = 1;
              active_page = 2;
              }
            }
         else //** page 2 active
           {
           if( data_count == MAX_BUFF_SIZE) //** page 2 full
             {
             //** Page 2 full; now start filling page 1
             //** Userspace application resets page full flag
             pru_shared_mem[PAGE2_FULL] = 1;
             data_count =0; //** start filling from page 1
             active_page = 1;
             }
           }

       //** reset for next NUM_SAMPLES
       pru_accumulator =0;
       i=0;
       }

       pru0_dram[10] = adc_samp; // for testing

       if( pru_shared_mem[RESET_FLAG] == 1)
          {
           pru_shared_mem[PAGE1_FULL] = 0;
           pru_shared_mem[PAGE2_FULL] = 0;
           pru_shared_mem[RESET_FLAG] = 0;
           data_count =0;
           pru_accumulator = 0;
           active_page =1;
           pru_shared_mem[RESET_FLAG] = 0;
           __delay_cycles(100000000);
           }

       //if( pru_shared_mem[EXIT_FLAG] == 1) break;

       //** delay for next sample
       //__delay_cycles(197000); // 0.001 seconds 1000Hz
       //__delay_cycles(34000); // 0.005 seconds 5kHz
       __delay_cycles(14990); // 0.001 seconds 10kHz
       //__delay_cycles(13650); // 0.000073 seconds 12.8kHz
        }



}

//**********************************************
//** Get ADC data.
//** 16bit result returned in  short * p_adcsamp
//**********************************************
void get_adc( short * p_adcsamp)
{
    unsigned short mask = 0x8000;
    int i;

    *p_adcsamp = 0x0000;

    //** request data
    __R30 =  __R30 | ADC_SAMP;

    //** ts delay 2.5uS
    __delay_cycles(500);

    //** reset samp line
    __R30 =  __R30 & ~ADC_SAMP;

    //** First clock delay - tfcd min 50nS
    __delay_cycles(40); //** 200nS



    //**

    for(i=0; i<17; i++ )
      {
        //** clock
      __R30 =  __R30 | 0x000000020;
      __delay_cycles(100);      //** CLK high min 50nS ; 500S
      __R30 =  __R30 & 0xFFFFFFFCF;
       __delay_cycles(20);    //** Data present on falling edge of CLK 100nS

      //** data appears from second pulse
      if(i > 0)
        {
         if( __R31 & ADC_SDATA)
           {
           *p_adcsamp |= mask;
           }
         mask >>= 1;
        }
      __delay_cycles(100);    //** Data present on falling edge of CLK 500S
      }
    //__R30 =  __R30 & ~DAIGNOS; // bit 7 used for timing measurement
}

//******************************************
//** Perform ADC calibration
//******************************************
void cal_adc(void)
{

    //** request cal
     __R30 =  __R30 | ADC_CAL;

     //** tcalh min 50nS
     __delay_cycles(40); //** 200nS

     //** reset cal line
     __R30 =  __R30 & ~ADC_CAL;

     //** tfcd min 50nS
     __delay_cycles(50); //** 250nS

     //** clock until busy clear
     while( __R31 & ADC_BUSY )
       {
       __R30 =  __R30 | 0x000000020;
       __delay_cycles(50);
       __R30 =  __R30 & 0xFFFFFFFCF;
       __delay_cycles(50);
       }


}


