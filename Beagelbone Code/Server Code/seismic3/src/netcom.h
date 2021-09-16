//****************************************************************************
//**
//** Name        : netcom.h
//** Author      : P.Byrne
//** Version     : 1.01
//** Copyright   : Copyright (C) 2021 GPL (free to use)
//** Description : header file for buffer and message definitions
//**               used for messaging and message framing.
//**
//****************************************************************************


#ifndef __MESSAGES_H  //Required for current module
#define __MESSAGES_H


#pragma pack(2)

#define  SYNC_VALUE (uint32_t)0xFFFFFFFF
#define  STANDARD_HEADER_SIZE 14
#define  TCPIP_MAX_BUFF_SIZE 1024
#define  NUM_SAMP_MESSAGE 200
#define  SAMP_MESSAGE_BYTES 800

#define CIRC_DATA_BUFFER_SIZE 108000

#define MID_START_ACQ_REQUEST	(uint16_t)2000
#define MID_WEATHER_ACQ_REQUEST	(uint16_t)2001
#define MID_SET_TIMESTAMP       (uint16_t)2002
#define MID_ACQ_DATA_REPLY   	(uint16_t)2003
#define MID_WEATHER_REPLY   	(uint16_t)2004
#define MID_STOP_REQUEST        (uint16_t)2005
#define MID_RESET_REQUEST       (uint16_t)2006
#define MID_PING_REQUEST        (uint16_t)2007


//** for crc calculations
union CONVERT_16
  {
  uint16_t val;
  uint8_t   buff[2];
  };


//** The following are used in TCP/IP comms
struct STANDARD_HEADER
  {
   uint32_t sync;                 // 4
   uint16_t mid;                  // 2
   uint32_t block_num;            // 4
   uint32_t num_data_bytes;       // 4          14
   uint16_t crc;                  // 16
  } __attribute__( (aligned(2) ) );




struct ACQ_RESPONSE16
  {
   uint32_t  sync;                        // 4     4
   uint16_t  mid;                         // 2     6
   uint32_t  block_num;                   // 4     10
   uint32_t  num_data_bytes;              // 4     14
   uint32_t  last_block;                  // 4     18
   uint64_t  timestamp;                   // 8     26
   int32_t   data[NUM_SAMP_MESSAGE];      // 800   826 pointer to  data
   uint16_t crc;                          // 2     828
  } __attribute__ ( (aligned(2) ) );



  union DATA_BLOCK
    {
	struct STANDARD_HEADER hdr;
	struct ACQ_RESPONSE16 sies_data;
	int16_t buff16[512];
	uint8_t buff[1024];
	} __attribute__ ( (aligned(2) ) );


//** This is used to store data in circular buffer
union DATA_BUFFER
	{
	struct ACQ_RESPONSE16 sies_data;
	uint8_t buff[828];
	}__attribute__ ( (aligned(2) ) );


#endif
