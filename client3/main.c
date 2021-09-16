//*******************************************************************************
//**
//** Client application for accessing remote server
//**
//*******************************************************************************

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "netcom.h"

#define MAX 80
#define PORT 2700
#define SA struct sockaddr

int g_count =0;
volatile sig_atomic_t   run_flag = 1;

void stopcmd( int sockfd );
void resetcmd( int sockfd );
void downloadcmd( int sockfd );
void send_ping( int sockfd );
void rxdata( int sockfd );
void process_buffer_download( union DATA_BUFFER* p_ip,  FILE *fp);
void netcom_CRC(uint8_t* p_msg, uint16_t* p_crc, int NumBytes );
void netcom_CRC16( uint16_t* p_crc, uint8_t x);
void file_convert(void);

//** contrl-c handler
void stop_process(int sig)
{
	run_flag = 0; //** stop download
}

int main()
{
    char chr;


	int sockfd;
	struct sockaddr_in servaddr;
	struct timeval tv;

	signal(SIGINT, stop_process);

	// socket create and varification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
      {
	  printf("socket creation failed...\n");
      exit(0);
	  }
	else
	  printf("Socket successfully created..\n");

	//** Set 5 second timeout for receiving messages
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


	bzero(&servaddr, sizeof(servaddr));

	//** assign IP, PORT; IP address of remote server
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("192.168.1.3");
	servaddr.sin_port = htons(PORT);

	//** connect the client socket to server socket
	if( connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)
      {
      printf("connection with the server failed...\n");
      close(sockfd);
      //exit(0);
	  }
	else
	  printf("connected to the server..\n");


	//** loop until 'q' or cntrl-c
    while(1)
      {

      printf("Options:\n d  download data\n c convert binary data\n r Reset Server buffers\n s Stop Server Code\n p Ping Server\n q exit program\n\n");
      printf("Enter a character: \n");

      //** scanf("%c",&chr) leaves a newline in input buffer which has to be consumed
      //** scanf(" %c",&chr) the " %c" format specifier ignores leading whitespace characters
      scanf(" %c", &chr);
      printf("\nYou entered %c.  \n", chr);
      if( chr == 'q') break;

      //** Menu

      //** download data buffer
      if( chr == 'd')
        {
        g_count =0;
        downloadcmd(sockfd );
        rxdata( sockfd );
        }

      //** halt acquisition program
      if( chr == 's')
        {
        stopcmd(sockfd );
        }

      //** reset buffers
      if( chr == 'r')
        {
        resetcmd(sockfd );
        }

      //** convert binary file to other formats
      if( chr == 'c')
        {
        file_convert();
        }


      //** ping message to remote server
      if( chr == 'p')
        {
        send_ping(sockfd);
        rxdata( sockfd );
        printf("\n\n");
        }

      }



	//** close the socket
	close(sockfd);
}

//*********************************************
//** PING messsges
//*********************************************
void send_ping( int sockfd )
{
    union DATA_BLOCK test_blk;
    uint16_t crc;

    //** send command to download data
    printf("Send ping request\n");
    test_blk.hdr.sync = SYNC_VALUE;
	test_blk.hdr.mid = MID_PING_REQUEST;
	test_blk.hdr.block_num = 1;
    test_blk.hdr.num_data_bytes = 0; //** no data, just mid request

    netcom_CRC( &test_blk.buff[0], &crc, STANDARD_HEADER_SIZE );
    test_blk.hdr.crc = crc;

    write(sockfd, &test_blk.buff[0],  sizeof(test_blk.hdr) );
}

//*********************************************
//** Reset buffer command - command in reserve
//*********************************************
void resetcmd( int sockfd )
{
    union DATA_BLOCK test_blk;
    uint16_t crc;

    //** send command to download data
    printf("Download Command\n");
    test_blk.hdr.sync = SYNC_VALUE;
	test_blk.hdr.mid = MID_RESET_REQUEST;
	test_blk.hdr.block_num = 1;
    test_blk.hdr.num_data_bytes = 0; //** no data, just mid request

    netcom_CRC( &test_blk.buff[0], &crc, STANDARD_HEADER_SIZE );
    test_blk.hdr.crc = crc;

    write(sockfd, &test_blk.buff[0],  sizeof(test_blk.hdr) );
}

//*********************************************
//** Stop remote server application
//*********************************************
void stopcmd( int sockfd )
{
    union DATA_BLOCK test_blk;
    uint16_t crc;

    //** send command to download data
    printf("Download Command\n");
    test_blk.hdr.sync = SYNC_VALUE;
	test_blk.hdr.mid = MID_STOP_REQUEST;
	test_blk.hdr.block_num = 1;
    test_blk.hdr.num_data_bytes = 0; //** no data, just mid request

    netcom_CRC( &test_blk.buff[0], &crc, STANDARD_HEADER_SIZE );
    test_blk.hdr.crc = crc;

    write(sockfd, &test_blk.buff[0],  sizeof(test_blk.hdr) );
}


//***************************************************
//** Send download data command data to remote server
//***************************************************
void downloadcmd( int sockfd )
{
    union DATA_BLOCK test_blk;
    uint16_t crc;

    //** send command to download data
    printf("Download Command\n");
    test_blk.hdr.sync = SYNC_VALUE;
	test_blk.hdr.mid = MID_START_ACQ_REQUEST;
	test_blk.hdr.block_num = 1;
    test_blk.hdr.num_data_bytes = 0; //** no data, just mid request

    netcom_CRC( &test_blk.buff[0], &crc, STANDARD_HEADER_SIZE );
    test_blk.hdr.crc = crc;

    write(sockfd, &test_blk.buff[0],  sizeof(test_blk.hdr) );


}

//************************************************************
//**
//** Handles messages from remote server.
//** Acquires data buffer. Exits on receipt of last block flag.
//** Handles 'PING' message response.
//** Will exit on 5 second comms timeout.
//************************************************************
void rxdata( int sockfd )
{
    uint8_t buff[TCPIP_MAX_BUFF_SIZE];
    union DATA_BUFFER ip;
    int ip_data_bytes =0;
    int ip_buff_count =0;
    uint16_t crc;
    union CONVERT_16 conv;
    int total_bytes_expected;
    FILE *fp;
    bool sync_found = false;
    bool start_block_download = false;
    int i;
    int download_count =0;

    run_flag = 1;

    while( run_flag == 1)
      {

      //** Process for message framing.
      //** 1 Receive bytes from stream
      //** 2 When number of bytes for header have been received test for sync
      //** 3 If sync received examine header data for expected number of bytes for message- If no sync - goto 1. Move along buffer by one byte at a time until sync found
      //** 4 Sync found; when total number of expected bytes for message received examine CRC.
      //** 5 If CRC ok process message, CRC fail ignore message.

      //** 5 second receive timeout set
      ip_data_bytes = read( sockfd,   buff, TCPIP_MAX_BUFF_SIZE );

      //** if after 5 seconds no data the exit loop
      //printf("num ip_data_bytes =%d\n", ip_data_bytes );
      if( ip_data_bytes  == -1)
         {
         printf("------> Comms Timeout <--------\n");
         break;
         }

      if( ip_data_bytes > 0 )
        {
        //** Message framing
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
            //** Sufficient bytes for header have been received; examine for sync
            if( ip_buff_count == STANDARD_HEADER_SIZE )
              {
              if(   ( ip.sies_data.sync == SYNC_VALUE )
                 &&( ip.sies_data.mid > 1999 )
                 &&( ip.sies_data.mid < 2008 ) )
                {
                sync_found = true;
                total_bytes_expected = ip.sies_data.num_data_bytes + STANDARD_HEADER_SIZE+2; //** Total expected bytes in message
		        //printf("sync found; expected bytes = %d  buff count=%d\n", total_bytes_expected, ip_buff_count);
                }
              else
                {
                //** sync NOT found
                //** move up one byte in buffer; backup buff count by 1 ( =13)
                //** This will move along buffer by 1 byte until sync is found
                memmove( &ip.buff[0], &ip.buff[1], STANDARD_HEADER_SIZE);
                ip_buff_count = STANDARD_HEADER_SIZE-1;
                }
              }
           }
         else // sync found = true
           {
           //printf("msg: bytes ip=%d   expected=%d\n", ip_buff_count, total_bytes_expected);

           //** Sync found and expected bytes received
           if( ip_buff_count == total_bytes_expected )
             {
             //** Test message CRC
             conv.buff[0] = ip.buff[total_bytes_expected-2];
             conv.buff[1] = ip.buff[total_bytes_expected-1];
             netcom_CRC( &ip.buff[0], &crc, total_bytes_expected-2);

             //printf("msg: bytes ip=%d   expected=%d msg CRC=%04X calc CRC= %04X\n", ip_buff_count, total_bytes_expected, conv.val, crc);

             //** process_message if crc ok
             if( conv.val == crc )
               {
                //** download data buffer
               if( ip.sies_data.mid ==  MID_ACQ_DATA_REPLY)
                 {
                 //** Begining of download
                 if( ( start_block_download == false ) && ip.sies_data.last_block == 0 )
                   {
                   start_block_download = true;
                   fp = fopen("/tmp/data1.bin", "wb");
                   if( fp == NULL )
                      {
                      printf("The error is - %s\n", strerror(errno));
                      printf("Error opening file cannot proceed\n");
                      return;
                      }
                   printf("Download start\n");
                   download_count=0;
                   }

                   fwrite(ip.buff, sizeof(union DATA_BUFFER),1, fp );

                   download_count++;


                   if(  (start_block_download == true ) && (ip.sies_data.last_block == 1 ) )
                     {
                     printf("\nLast Block\n");
                     start_block_download = false;
                     fclose(fp);
                     printf("End rx\n\n\n");
                     run_flag =0;
                     }

                 }
              else if( ip.sies_data.mid ==  MID_PING_REQUEST )
                {
                printf("SERVER PING RESPONSE\n") ;
                printf("End rx\n\n\n");
                run_flag =0;
                }

              if(download_count % 10)  printf("%d\n", download_count*10); //* to show something is happening


               }
             else
               {
               printf("CRC FAIL!\n");
               }

             //** Reset for next message
             total_bytes_expected =0;
             ip_buff_count = 0;
             sync_found = false;
             }
            } // sync == TRUE
          }// next i
        } // if( ip_data_bytes > 1 )
      } // end while

      //** if rx timeout or cntrl-c while download buffer in progress close file
      if( start_block_download == true)
        {
        fclose(fp);
        printf("End rx\n");
        }
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

//****************************************************************
//** Convert binary data
//** convert data1.bin binary data in /tmp directory to text format
//** The binary data is converted into two text files, one for
//** the analogue data, the second for data parameters, times etc..
//****************************************************************

void file_convert(void)
{
    union DATA_BUFFER ip;
    FILE *fp, *op1;
    time_t start_buffer_timestamp = 0;
    time_t end_buffer_timestamp = 0;
    int block_size = sizeof(union DATA_BUFFER);
    //unsigned long file_size;
    int num_samples;
    double sample_inc;
    int i;
    struct tm *local;


    printf("File convert selected\n");

    //** open binary file for reading
    fp = fopen("/tmp/data1.bin", "rb");
    //fp = fopen("/home/paul/testdata/data1a.bin", "rb");
    if( fp == NULL )
      {
      printf("The error is - %s\n", strerror(errno));
      printf("Error opening file cannot proceed\n");
      return;
      }

    //** open text file for writing
    op1 = fopen("/tmp/data1.txt", "w");
    //op1 = fopen("/home/paul//testdata/data1b.txt", "w");
    if( op1 == NULL )
      {
      printf("The error is - %s\n", strerror(errno));
      printf("Error opening file cannot proceed\n");
      fclose(fp);
      return;
      }

    //** calculate true sample increment
    //** Get first and last timestamps; calculate sample increment
    fread(ip.buff, sizeof(union DATA_BUFFER),1, fp );
    start_buffer_timestamp = ip.sies_data.timestamp;

    fseek(fp, -block_size, SEEK_END );
    fread(ip.buff, sizeof(union DATA_BUFFER),1, fp );
    end_buffer_timestamp = ip.sies_data.timestamp;

    fseek(fp, 0L, SEEK_END);
    //file_size = ftell(fp);

    num_samples =0;


    //**set pointer back to beggining
    fseek(fp, 0L, SEEK_SET);

    //** read block of data
    while( fread(ip.buff, sizeof(union DATA_BUFFER),1, fp ) > 0 )
       {
       //** for each block  of data
       for(i=0; i<NUM_SAMP_MESSAGE; i++)
        {

        fprintf( op1, "%d\n", ip.sies_data.data[i]); //**

        num_samples++;
        }
       }

  fclose(fp);
  fclose(op1);

   sample_inc = difftime(end_buffer_timestamp, start_buffer_timestamp )/(double)num_samples;

   local = localtime(&start_buffer_timestamp);
   printf("Start Time is %02d:%02d:%02d    %02d/%02d/%d\n", local->tm_hour, local->tm_min, local->tm_sec,
                                                  local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);

   local = localtime(&end_buffer_timestamp);
   printf("End Time is %02d:%02d:%02d    %02d/%02d/%d\n", local->tm_hour, local->tm_min, local->tm_sec,
                                                  local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);


  printf("Num samples=%d\n", num_samples );
  printf("Elasped time seconds= %f\n", difftime(end_buffer_timestamp, start_buffer_timestamp ) );
  printf("Sample increment %f\n", sample_inc );

  printf("File conversion complete\n\n\n");

  //** Store data parameters in second file
   op1 = fopen("/tmp/data2.txt", "w");
    //op1 = fopen("/home/paul/testdata/data2b.txt", "w");
   if( op1 == NULL )
      {
      printf("The error is - %s\n", strerror(errno));
      printf("Error opening file cannot proceed\n");
      return;
      }

   //** store all as integer values
   fprintf( op1, "%ld\n", start_buffer_timestamp);
   fprintf( op1, "%ld\n", end_buffer_timestamp);
   fprintf( op1, "%d\n", (int) ((sample_inc*1000000)+0.5)  ); //** convert to integer X10^6
   fprintf( op1, "%d\n", num_samples);
   fprintf( op1, "%ld\n", ( end_buffer_timestamp - start_buffer_timestamp) );


    fclose(op1);


}
