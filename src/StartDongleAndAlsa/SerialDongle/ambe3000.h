
#ifndef AMBE3000_H_
#define AMBE3000_H_
#include "config.h"

const    char AMBE3000_SYNC_BYTE           = (const char)0x61;
const    char AMBE3000_PARITY_TYPE_BYTE    = (const char)0x2F;

//Channel Packet
const    char AMBE3000_PCM_LENGTH_HBYTE    = (const char)0x01;
const    char AMBE3000_PCM_LENGTH_LBYTE    = (const char)0x44;
const    char AMBE3000_PCM_TYPE_BYTE       = (const char)0x02;
const    char AMBE3000_PCM_SPEECHID_BYTE   = (const char)0x00;
const    char AMBE3000_PCM_NUMSAMPLES_BYTE = (const char)0xA0;
const    int  AMBE3000_PCM_INTSAMPLES_BYTE = (const int )0xA0;

//Speech Packet
const    char AMBE3000_AMBE_LENGTH_HBYTE   = (const char)0x00;
const    char AMBE3000_AMBE_LENGTH_LBYTE   = (const char)0x0B;
const    char AMBE3000_AMBE_TYPE_BYTE      = (const char)0x01;
const    char AMBE3000_AMBE_CHANDID_BYTE   = (const char)0x01;
const    char AMBE3000_AMBE_NUMBITS_BYTE   = (const char)0x31;
const    char AMBE3000_AMBE_NUMBYTES_BYTE  = (const char)0x07;

//Control/Configuration Packet
const    char AMBE3000_CCP_TYPE_BYTE       = (const char)0x00;
const    char AMBE3000_CCP_ECMODE	       = (const char)0x05;
const    char AMBE3000_CCP_DCMODE          = (const char)0x06;

const    char AMBE3000_CCP_PRODID_BYTE     = (const char)0x30;
const    char AMBE3000_CCP_VERSTRING_BYTE  = (const char)0x31;

const    char AMBE3000_CCP_MODE_LENGTHH    = (const char)0x00;
const    char AMBE3000_CCP_MODE_LENGTHL    = (const char)0x05;
const    char AMBE3000_CCP_MODE_NOISEH     = (const char)0x00;
const    char AMBE3000_CCP_MODE_NOISEL     = (const char)0x40;

const    int  AMBE3000_PCM_BYTESINFRAME    = 328;
const    int  AMBE3000_AMBE_BYTESINFRAME    = 15;




#pragma pack(1)
typedef union
{
	struct
	{
		uint8_t             Sync;
		uint8_t             LengthH;
		uint8_t             LengthL;
		uint8_t             Type;
		uint8_t             ID;
		uint8_t             Num;
		uint8_t             Samples[320]; //Bytes!
		uint8_t             PT;
		uint8_t             PP;
	}fld;
	uint8_t                 All[328];
}tPCMFrame;

typedef union
{
	struct
	{
		uint8_t             Sync;
		uint8_t             LengthH;
		uint8_t             LengthL;
		uint8_t             Type;
		uint8_t             ID;
		uint8_t             Num;
		uint8_t             ChannelBits[7];
		uint8_t             PT;
		uint8_t             PP;
		uint8_t				reserved;
	}fld;
	uint8_t                 All[16];
}tAMBEFrame;

typedef union
{
	struct BASE
	{
		uint8_t             Sync;
		uint8_t             LengthH;
		uint8_t             LengthL;
		uint8_t             Type;
		uint8_t				empty[324];
	}base;

	//PCM Frame; Type = 0x02
	struct PCMTYPE
	{
		tPCMFrame        thePCMFrame;
	}PCMType;

	//AMBE Frame; Type = 0x01
	struct AMBETYPE
	{
		tAMBEFrame      theAMBEFrame;
		uint8_t empty[312];
	}AMBEType;

	uint8_t     All[328];

}DVSI3000struct;
#pragma pack()

#endif /*AMBE3000_H_*/
