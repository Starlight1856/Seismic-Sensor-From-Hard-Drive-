
//****************************************************************************
//**
//** Name        : netcom.c
//** Author      : P.Byrne
//** Version     : 1.01
//** Copyright   : Copyright (C) 2021 GPL (free to use)
//** Description : Main program. Handles network communications.
//**
//**
//****************************************************************************

#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <net/if.h>
#include <linux/tcp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "netcom.h"

static struct sockaddr_in netcom_clientAddr;
static  uint32_t netcom_clientAddressLength;
volatile sig_atomic_t   netcom_run_flag = 1;
static int   netcom_serve_socket;
static int   netcom_connect_socket;
static  int net_port;
static struct sockaddr_in netcom_serv_addr;
static char buff[TCPIP_MAX_BUFF_SIZE];

pthread_mutex_t netcom_lock;


static void netcom_run(void);
static void netcom_CRC( uint8_t* p_msg, uint16_t* p_crc, int NumBytes );
static void netcom_CRC16( uint16_t* p_crc, uint8_t x);
static void netcom_process_message(union DATA_BLOCK* p_ip);
static void netcom_tx_acq_data(void);
static void netcom_ping_response(void);

//** from seismic module
extern union DATA_BUFFER* p_seismic_data_buf;
extern volatile sig_atomic_t seismic_flag;
extern int seismic_circ_buffer_count;
extern int seismic_sample_count;
extern void seismic_data_start(void);
extern void seismic_data_stop(void);
extern void seismic_data_reset(void);


//** Cntrl-c handler - requires socket timeout to be operating
void netcom_stop_process(int sig)
{
	netcom_run_flag = 0;
}

int main(void)
{
  struct ifreq ifr;
  char server_addr[20];
  netcom_run_flag = true;
  net_port = 2700;


  if (pthread_mutex_init( &netcom_lock, NULL ) != 0)
    {
    printf("\n mutex init failed\n");
    return 1;
    }

  seismic_data_start();

  signal(SIGINT, netcom_stop_process);

  netcom_serve_socket = socket(AF_INET, SOCK_STREAM, 0);

//* If socket connect timeout required
//*  struct timeval tv;
//*  tv.tv_sec = 5;
//*  tv.tv_usec = 0;
//*  setsockopt( netcom_serve_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1 );

  ioctl( netcom_serve_socket, SIOCGIFADDR, &ifr);

  memset( server_addr,0,16);
  sprintf( server_addr,"%s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
  printf("Hello I am Seismic Sensor Operating as Server %s\n",  server_addr);

  
  netcom_serv_addr.sin_family = AF_INET;
  netcom_serv_addr.sin_port = htons(net_port);
  netcom_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


  if( bind( netcom_serve_socket, (struct sockaddr *) &netcom_serv_addr, sizeof(netcom_serv_addr)) < 0 )
     {
     printf("Cannot bind socket\n");
     return 0;
     }
   else
     {
     printf("Socket bind\n");
     }


  if( listen( netcom_serve_socket, 5) < 0 )
     {
     printf("Listen Fail\n");
     return 0;
     }
   else
     {
     printf("Socket Listen\n");
     }

   //** Main loop
   netcom_run();

   pthread_mutex_destroy(&netcom_lock);

   return 1;
}

//*************************
//** Main loop
//*************************
void netcom_run(void)
{
  int i;
  uint16_t crc;
  union CONVERT_16 conv;
  union DATA_BLOCK ip;
  bool  soc_shunt_down;
  int ip_data_bytes =0;
  int ip_buff_count =0;
  int total_bytes_expected =0;
  bool sync_found  = false;
  struct sockaddr_in  cli;
  unsigned int len;

  len = sizeof(cli);

  netcom_run_flag = 1;
  soc_shunt_down =true;

  while( netcom_run_flag == 1 )
    {

    if( soc_shunt_down == true)
      {

      //** Block until client connects
      printf("Waiting for TCP connection on port %d\n", net_port );
      netcom_clientAddressLength = sizeof( netcom_clientAddr);
      netcom_connect_socket = accept( netcom_serve_socket, (struct sockaddr *) &cli,  &len);

      if(netcom_connect_socket < 0)
        {
        printf("cannot accept connection ");
        // netcom_run_flag = false;
        }
      else
        {
        printf("Connected to client %s   Port=%d   Size=%d\n",
                          inet_ntoa( cli.sin_addr), cli.sin_port,
                          sizeof( struct sockaddr_in)    );
        soc_shunt_down = false;
        }
      }


    //** Process for message framing.
    //** 1 Receive bytes from stream
    //** 2 When number of bytes for header have been received test for sync
    //** 3 If sync received examine header data for expected number of bytes for message- If no sync - goto 1. Move along buffer by one byte at a time until sync found
    //** 4 Sync found; when total number of expected bytes for message received examine CRC.
    //** 5 CRC ok process message, CRC fail ignore message.


     //** TCP/IP stream handling
     ip_data_bytes = read(netcom_connect_socket,   buff, TCPIP_MAX_BUFF_SIZE );
     printf("ip data bytes =%d\n", ip_data_bytes);

     if( ip_data_bytes > 1 )
       {
       //** Message acculumation not yet begun;
       //** State: Search for  sync process byte at a time
       for(i =0; i< ip_data_bytes ; i++ )
         {
         ip.buff[ip_buff_count] = buff[i];
         ip_buff_count++;

         //** If max buffer capacity reached  but no sync then reset
         //** as there must be a problem.
	     if(ip_buff_count == TCPIP_MAX_BUFF_SIZE )
	       {
           sync_found = false;
           ip_buff_count = 0;
	       ip_data_bytes =0;
	       break;
	       }

	     //** Waiting for sync
         if( sync_found == false )
           {
           //**If sufficient bytes for header have been received, test for sync.
           //** As extra test confirm MID value are within expected parameters.
           if( ip_buff_count == STANDARD_HEADER_SIZE )
             {
             if(   ( ip.hdr.sync == SYNC_VALUE )
                 &&( ip.hdr.mid > 1999 )
                 &&( ip.hdr.mid < 2008 ) )
                {
            	//** Sync found; determine total numer of bytes expected from
            	//** header data
                sync_found = true;
                total_bytes_expected = ip.hdr.num_data_bytes + STANDARD_HEADER_SIZE+2;
		        printf("sync found; expected bytes = %d\n", total_bytes_expected);
                }
              else
                {
                //** sync NOT found despite sufficient bytes being recieved
                //** move up one byte in buffer; backup buff count by 1 ( =13)
                //** This will move along buffer by 1 byte until sync is found
                memmove( &ip.buff[0], &ip.buff[1], STANDARD_HEADER_SIZE);
                ip_buff_count = STANDARD_HEADER_SIZE -1; //** To process next byte until sync found
                }
             }
           }
         else //** sync found
           {
           //** Total number of bytes expected for message have been received,
           //** now test message CRC.
           if( ip_buff_count == total_bytes_expected )
             {
             conv.buff[0] = ip.buff[total_bytes_expected-2];
             conv.buff[1] = ip.buff[total_bytes_expected-1];
             netcom_CRC( &ip.buff[0], &crc, total_bytes_expected-2);

             //** process_message if crc ok
             if( conv.val == crc )
               {
	       printf(" msg id %d ok\n", ip.hdr.mid);	       
               netcom_process_message( &ip );
               }
             else
               {
               printf("CRC FAIL!\n");
               }
             //** reset for next message
             total_bytes_expected =0;
             ip_buff_count = 0;
             sync_found = false;
             }
           } // sync == TRUE
         }// next i
      } // if( ip_data_bytes > 1 )
   else //** if zero indicates disconnection
     {
     //printf("zero ip bytes : client disconnection: shutdown sockets\n");
     shutdown(netcom_connect_socket, SHUT_RDWR );
     soc_shunt_down =true;
     usleep(500000);
     //sleep(1);
     }
    } // end while

    seismic_data_stop();

    shutdown(netcom_connect_socket, SHUT_RDWR );
    close(netcom_connect_socket);

}

  //*************************************************
  //**
  //** netcom_CRC
  //**
  //** calculates  CRC of message pointed to by p_msg
  //**
  //**************************************************
  void netcom_CRC(uint8_t* p_msg, uint16_t* p_crc, int NumBytes )
  {

    int i;
    *p_crc = 0;

    //** Ignore sync; first 4 bytes
    for(i=4; i < NumBytes; i++)
      {
      netcom_CRC16( p_crc, p_msg[i] ); //** 16 bit crs
      }


  }
  //**************************************
  //** netcom_CRC16
  //**
  //** Impelments CRC16 algorithm
  //**
  //*************************************

  void netcom_CRC16( uint16_t* p_crc, uint8_t x)
  {

    uint16_t temp;
    temp  = (uint16_t)((*p_crc  >> 8) ^ x);
    temp ^= (uint16_t) (temp << 1);
    *p_crc   = (uint16_t)((*p_crc  << 8) ^ (temp << 1));
    temp ^= (uint16_t)((temp >> 2) ^ (temp >> 4) ^ (temp >> 6));
    if (temp & 2)
       {
       *p_crc = *p_crc ^ 0x8003;
       }

  }


  //*******************************************
  //**
  //** Process received messages
  //**
  //*******************************************

  void netcom_process_message(union DATA_BLOCK* p_ip)
  {
	  switch(p_ip->hdr.mid)
	    {
	   case MID_START_ACQ_REQUEST:
		  printf("ACK Request\n"); 
		  netcom_tx_acq_data();
	     break;

	   case MID_STOP_REQUEST:
		  printf("Stop Request\n");
		  netcom_run_flag = 0;
		  break;

	   case MID_RESET_REQUEST:
		  printf("Reset Request\n");
		  seismic_data_reset();
		  break;

	   case MID_PING_REQUEST:
  	    printf("PING Request\n");
		netcom_ping_response();
		break;


	   default:
		 break;

	    }

  }


 //*******************************************
 //**
 //** Messsage response: transmit buffer
 //**
  //******************************************
 void netcom_tx_acq_data(void)
 {
  pthread_mutex_lock(&netcom_lock);

  int i;
  int data_point;
  int start_point, num_data_points;
  int size_msg, size_crc;
  uint16_t crc;


   //** condition circular buffer not yet rolled over
    if( seismic_sample_count < CIRC_DATA_BUFFER_SIZE)
     {
	 start_point =0;
	 num_data_points = seismic_circ_buffer_count;
     }
  else
    {
	//** condition circular buffer  rolled over; start from last entry point
	start_point = seismic_circ_buffer_count + 1;
	if(start_point  == CIRC_DATA_BUFFER_SIZE) start_point=0;
	num_data_points = CIRC_DATA_BUFFER_SIZE;
    }

  size_msg =  sizeof( union DATA_BUFFER);   // size of entire message to be tx
  size_crc = size_msg -2; //** size message minus crc value

  data_point = start_point; //** data access in circular buffer from this position

  printf("Sample count =%d start point =%d  num data points =%d\n", seismic_sample_count, start_point, num_data_points);

  for( i=0; i<num_data_points; i++ )
    {
	//** If last block in buffer set flag
    if( i == num_data_points -1)
   	  p_seismic_data_buf[data_point].sies_data.last_block = 1;
    else
      p_seismic_data_buf[data_point].sies_data.last_block = 0;


	netcom_CRC( p_seismic_data_buf[data_point].buff, &crc, size_crc);
	p_seismic_data_buf[data_point].sies_data.crc = crc;

    write(netcom_connect_socket, p_seismic_data_buf[data_point].buff,  size_msg );
    data_point++;
    if( data_point == CIRC_DATA_BUFFER_SIZE ) data_point = 0;
    }

  //** Reset buffer counts
  seismic_sample_count =0;
  seismic_circ_buffer_count =0;

  pthread_mutex_unlock(&netcom_lock);

 }

void  netcom_ping_response(void)
{
  union DATA_BLOCK test_blk;
  uint16_t crc;

    //** send ping response
    usleep(100000);
    test_blk.hdr.sync = SYNC_VALUE;
    test_blk.hdr.mid = MID_PING_REQUEST;
    test_blk.hdr.block_num = 1;
    test_blk.hdr.num_data_bytes = 0; //** no data, just mid request
    netcom_CRC( &test_blk.buff[0], &crc, STANDARD_HEADER_SIZE );
    test_blk.hdr.crc = crc;

    printf("PING TX CRC=%04X\n", crc);
    write( netcom_connect_socket, &test_blk.buff[0],  STANDARD_HEADER_SIZE+2 );
}
