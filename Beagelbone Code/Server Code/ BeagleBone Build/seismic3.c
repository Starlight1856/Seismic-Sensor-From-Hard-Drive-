
//****************************************************************************
//**
//** Name        : seismic3.c
//** Author      : P.Byrne
//** Version     : 1.01
//** Copyright   : Copyright (C) 2021 GPL (free to use)
//** Description : Handles interface to PRU shared memory. Copies data into
//**               circular buffer.
//**
//****************************************************************************



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>

#include "netcom.h"

//** netcom mutex
extern pthread_mutex_t netcom_lock;

//** PRU data area, uses common header eventually?
#define PAGE_SIZE 200
#define MAX_BUFF_SIZE  400
#define PAGE1_FULL  (MAX_BUFF_SIZE + 1)
#define PAGE2_FULL  (MAX_BUFF_SIZE + 2)
#define RESET_FLAG  (MAX_BUFF_SIZE + 3)
#define EXIT_FLAG   (MAX_BUFF_SIZE + 4)


//** Cicular Data buffer
union DATA_BUFFER* p_seismic_data_buf = (union DATA_BUFFER*)NULL;
int seismic_circ_buffer_count = 0;
int seismic_sample_count = 0;
uint32_t seismic_block_count =0;

//** Read shared memory area
volatile sig_atomic_t seismic_flag =1;
volatile sig_atomic_t seismic_reset_data =0;


//** shared memory *****************
#define MAP_SIZE 0x80000
#define MAP_MASK (MAP_SIZE - 1)
const unsigned long OFFSET = 0x200;
const unsigned long OFFSET2 = 0x10200;
//**********************************

static pthread_t acq_thread_seismic_data;
static pthread_attr_t attr_seismic_data;

static void seismic_msg_buffer( int32_t* pacq_buff );
void seismic_data_start(void);
void seismic_data_stop(void);
void seismic_data_reset(void);
void *seismic_data_acq(void *ptr);


//*********************************************************************
//**
//** Obtain data from PRU shared memory and store in circular buffer
//** for later transmission
//**
//*********************************************************************
void seismic_data_start(void)
{
	int i;
	int iret2;
	char *message2 = "Thread adc";

	seismic_circ_buffer_count =0;

    //** allocate data buffer; deallocated on thread termination
	p_seismic_data_buf = malloc( CIRC_DATA_BUFFER_SIZE * sizeof(union DATA_BUFFER) );
	if( p_seismic_data_buf ==  (union DATA_BUFFER*)NULL )
	  {
          printf("Failed to allocate buffer memory! Exit Thread\n");
         return;
	  }
	else
	  {
	  // initialise buffer with fixed values
	  for(i=0; i< CIRC_DATA_BUFFER_SIZE; i++)
	    {
        p_seismic_data_buf[i].sies_data.sync = SYNC_VALUE;
	    p_seismic_data_buf[i].sies_data.mid = MID_ACQ_DATA_REPLY;
	    p_seismic_data_buf[i].sies_data.num_data_bytes = SAMP_MESSAGE_BYTES + 12;// int data[200] +  (uint32_t  last_block) +  ( uint64_t  timestamp )
	    }
	  }

      // start threads
	  pthread_attr_init(&attr_seismic_data);
	  pthread_attr_setdetachstate(&attr_seismic_data, PTHREAD_CREATE_DETACHED);
	  iret2 = pthread_create( &acq_thread_seismic_data, &attr_seismic_data, seismic_data_acq, (void*)message2);


	  printf("Thread adc returns: %d\n", iret2);
}

//*************************************************
//**
//** End thread loop; called when netcom loop shut down
//** and exited.
//**
//*************************************************
void seismic_data_stop(void)
{
  seismic_flag = 0;
}

//*************************************************
//**
//** Reset data buffers.
//**
//*************************************************

void seismic_data_reset(void)
{
	seismic_reset_data = 1; //** set reset flag

}


void *seismic_data_acq(void *ptr)
{
    int fd, i;
    void *map_base, *virt_addr;
    struct timespec tim, tim2;
    //unsigned long read_result, writeval;
    //unsigned int numberOutputSamples = 1;
    off_t target = 0x4a300000;
    int*pshared_mem;
    int32_t acq_buff[PAGE_SIZE];


    //** Set naonsleep delay for 50mS
    tim.tv_sec = 0;
    tim.tv_nsec = 50000*1000;

   puts("!!!Here we go!!!\n");
   fflush(stdout);

    //** acess PRU shared memory

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
      {
      printf("Failed to open memory!\n");
      free( p_seismic_data_buf );
      pthread_attr_destroy( &attr_seismic_data );
      pthread_exit(NULL);;
      }


    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1)
      {
      printf("Failed to map base address\n");
      free( p_seismic_data_buf );
      pthread_attr_destroy( &attr_seismic_data );
      pthread_exit(NULL);;
      }


   target = 0x4a300000;
   virt_addr = map_base + OFFSET2 + (target & MAP_MASK);
   pshared_mem = ( int32_t *) virt_addr;
 
    pshared_mem[RESET_FLAG] = 1; // Reset PRU buffers

   nanosleep(&tim , &tim2);

    fflush(stdout);

    //** Process thread loop. Obtain data from PRU shared memory
    //** Each page of the 2 -page buffer contains 1 seconds worth of data
    while( seismic_flag == 1)
      {

      if( pshared_mem[PAGE1_FULL] == 1)
        {
    	//printf("Page1\n");

    	//** copy to local buffer
    	for(i=0;i<PAGE_SIZE; i++ ) acq_buff[i] = (int16_t)pshared_mem[i];
    	pshared_mem[PAGE1_FULL] = 0; //** Reset PRU buffer 1 flag

      	//** store in circular buffer
    	seismic_msg_buffer( acq_buff );

        }
      else if( pshared_mem[PAGE2_FULL] == 1)
        {
    	//printf("Page2\n");

      	//** copy to local buffer
      	for(i=0;i<PAGE_SIZE; i++ ) acq_buff[i] = (int16_t)pshared_mem[PAGE_SIZE+i];
      	pshared_mem[PAGE2_FULL] = 0; //** Reset PRU buffer 2 flag

      	//** store in circular buffer
      	seismic_msg_buffer( acq_buff );
    	}

      // fflush(stdout);

      nanosleep(&tim , &tim2); //** Free up cpu as loop rate of of 20Hz is more than adequate for 1 second data buffer.


      if( seismic_reset_data == 1)
        {
    	seismic_reset_data = 0;
    	printf("Reset\n");
        pshared_mem[RESET_FLAG] = 1;  //** Reset PRU buffers
        seismic_circ_buffer_count =0; //** Intialise local circular buffer
        seismic_sample_count =0;      //** Initialise roll-over monitor
        }

      } //end while

    free( p_seismic_data_buf ); //** de-allocate buffer memory

    fflush(stdout);

    //pshared_mem[EXIT_FLAG] =1; //** shut down PRU code

    //** unmap shared memory
    if(munmap(map_base, MAP_SIZE) == -1)
      {
      printf("Failed to unmap memory");
      }

    close(fd);

    pthread_attr_destroy( &attr_seismic_data );
    pthread_exit(NULL);
 }

//******************************************************************************
//**
//** Store data in circular buffer. Each entry contains 1 seconds worth of data.
//** This buffer can store up to 30 Hours of data
//**
//******************************************************************************

void seismic_msg_buffer( int32_t* pacq_buff )
{
	pthread_mutex_lock(&netcom_lock);

	memcpy( p_seismic_data_buf[seismic_circ_buffer_count].sies_data.data, pacq_buff,  SAMP_MESSAGE_BYTES );
	p_seismic_data_buf[seismic_circ_buffer_count].sies_data.block_num = seismic_block_count++;
	p_seismic_data_buf[seismic_circ_buffer_count].sies_data.timestamp = (uint64_t)time(NULL);
	p_seismic_data_buf[seismic_circ_buffer_count].sies_data.last_block =0;

	seismic_circ_buffer_count++;
	if( seismic_circ_buffer_count == CIRC_DATA_BUFFER_SIZE)
	  {
	  seismic_circ_buffer_count =0;
	  seismic_sample_count = CIRC_DATA_BUFFER_SIZE; //** identify when circular filled, and has rolled over
	  }


	pthread_mutex_unlock(&netcom_lock);

}
