

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

int netcom_start(void);
void netcom_run(void);
void netcom_CRC( uint8_t* p_msg, uint16_t* p_crc, int NumBytes );
void netcom_CRC16( uint16_t* p_crc, uint8_t x);
void netcom_process_message(union DATA_BLOCK* p_ip);
void netcom_tx_acq_data(void);

//** from seismic module
extern union DATA_BUFFER* p_seismic_data_buf;
extern int seismic_circ_buffer_count;
extern int seismic_sample_count;
extern void seismic_data_start(void);
extern void seismic_data_stop(void);

void stop_process(int sig)
{
	netcom_run_flag = 0;
//	seismic_data_stop();
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

  signal(SIGINT, stop_process);

  netcom_serve_socket = socket(AF_INET, SOCK_STREAM, 0);

//  struct timeval tv;
//  tv.tv_sec = 5;
//  tv.tv_usec = 0;
//  setsockopt( netcom_serve_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1 );

  ioctl( netcom_serve_socket, SIOCGIFADDR, &ifr);

  memset( server_addr,0,16);
  sprintf( server_addr,"%s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
  printf("Hello I am Seismic Sensor Operating as Server %s\n",  server_addr);

  
  netcom_serv_addr.sin_family = AF_INET;
  netcom_serv_addr.sin_port = htons(net_port);
  netcom_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 // netcom_serv_addr.sin_addr.s_addr = inet_addr("192.168.1.122");

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


   netcom_run();
   pthread_mutex_destroy(&netcom_lock);

   return 1;
}

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

     //** TCP/IP stream handling
     ip_data_bytes = read(netcom_connect_socket,   buff, TCPIP_MAX_BUFF_SIZE );
     printf("ip data bytes =%d\n", ip_data_bytes);

     if( ip_data_bytes > 1 )
       {
       //** Message acculumation not yet begun;
       //** State: Search for  sync
       for(i =0; i< ip_data_bytes ; i++ )
         {
         ip.buff[ip_buff_count] = buff[i];
         ip_buff_count++;
	     if(ip_buff_count == TCPIP_MAX_BUFF_SIZE )
	       {
               sync_found = false;
               ip_buff_count = 0;	   
	       ip_data_bytes =0;
	       break;
	       }

         if( sync_found == false )
           {
           //**NO sync AND sufficient bytes for header have been received
           if( ip_buff_count == STANDARD_HEADER_SIZE )
             {
             if(   ( ip.hdr.sync == SYNC_VALUE )
                 &&( ip.hdr.mid > 1999 )
                 &&( ip.hdr.mid < 2005 ) )
                {
                sync_found = true;
                total_bytes_expected = ip.hdr.num_data_bytes + STANDARD_HEADER_SIZE+2;
		        printf("sync found; expected bytes = %d\n", total_bytes_expected);
                }
              else
                {
                //** sync NOT found
                //** move up one byte in buffer; backup buff count by 1 ( =13)
                //** This will move along buffer by 1 byte until sync is found
                memmove( &ip.buff[0], &ip.buff[1], STANDARD_HEADER_SIZE);
                ip_buff_count = 13;
                }
             }
           }
         else // sync found
           {
           printf("msg: bytes ip=%d   expected=%d\n", ip_buff_count, total_bytes_expected);
           if( ip_buff_count == total_bytes_expected )
             {
             conv.buff[0] = ip.buff[total_bytes_expected-2];
             conv.buff[1] = ip.buff[total_bytes_expected-1];
             netcom_CRC( &ip.buff[0], &crc, total_bytes_expected-2);
             total_bytes_expected =0;

             // process_message if crc ok
             if( conv.val == crc )
               {
               netcom_process_message( &ip );
               }
             else
               {
               printf("CRC FAIL!\n");
               }
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

  void netcom_process_message(union DATA_BLOCK* p_ip)
  {
	  switch(p_ip->hdr.mid)
	    {
	   case MID_START_ACQ_REQUEST:
		  printf("ACK Request\n"); 
		  netcom_tx_acq_data();
	     break;

	   default:
		 break;

	    }


  }

 void netcom_tx_acq_data(void)
 {
  pthread_mutex_lock(&netcom_lock);

  int i;
  int data_point;
  int circ_buf_access_point;
  int start_point, num_data_points;
  int size_msg, size_crc;
  uint16_t crc;


  //** last sample number
  circ_buf_access_point = seismic_circ_buffer_count;

   //** condition circular buffer not yet rolled over
    if( seismic_sample_count < CIRC_DATA_BUFFER_SIZE)
     {
	 start_point =0;
	 num_data_points = seismic_sample_count;
     }
  else
    {
	//** condition circular buffer  rolled over
	start_point = circ_buf_access_point + 1;
	if(start_point  == CIRC_DATA_BUFFER_SIZE) start_point=0;
	num_data_points = CIRC_DATA_BUFFER_SIZE;
    }

  size_msg =  sizeof( union DATA_BUFFER);   // size of entire message to be tx
  size_crc = size_msg -2; //** size message minus crc value

  data_point = start_point;
  printf("Sample count =%d start point =%d  num data points =%d\n", seismic_sample_count, start_point, num_data_points);

  for(i=0; i<num_data_points; i++)
    {

	netcom_CRC( p_seismic_data_buf[data_point].buff, &crc, size_crc);
	p_seismic_data_buf[data_point].sies_data.crc = crc;

    write(netcom_connect_socket, p_seismic_data_buf[data_point].buff,  size_msg );
    data_point++;
    if( data_point == CIRC_DATA_BUFFER_SIZE ) data_point = 0;
    }

  pthread_mutex_unlock(&netcom_lock);

 }


