
// SerialDongle.h: interface for the CSerialDongle class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SERIALDONGLE_H_  
#define _SERIALDONGLE_H_  

#pragma once
#include <fcntl.h>
#include <aio.h>
#include "ambe3000.h"
#include "usartwrap.h"
#include "syninterface.h"
#include "fifoqueue.h"


const int  INTERNALCOMBUFFSIZE	= 2048; 
const int  DONGLEBAUDRATE		= 230400;
const int8_t DONGLEBITS			= 8;
const int8_t DONGLEPARITY		= 'N';
const int8_t DONGLESTOP			= 1;

enum	PARSERSTATE	{	FIND_START,
						HIGH_LENGTH,
						LOW_LENGTH,
						READ_DATA    };

//wxj
enum    ScrambleDirection  {
	IPSCTODONGLE,
	DONGLETOIPSC
};

//Serial Events (two different arrays).
const int SERIAL_TIMEOUT = 1;
const int SERIAL_TICKLE = 0;

//I've made this fairly long for debugging. It need not be.
//Must be a power of 2.
const int  MAXDONGLEAMBEFRAMES       = 2048; //About 41 Seconds.
const int  MAXDONGLEAMBEFRAMESMASK   = MAXDONGLEAMBEFRAMES - 1;
const int  MAXDONGLEPCMFRAMES        = 2048; //About 41 Seconds.
const int  MAXDONGLEPCMFRAMESMASK    = MAXDONGLEPCMFRAMES - 1;


const int IPSCTODONGLETABLE[49] =
//0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12. 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
{ 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 41, 43, 45, 47, 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 42, 44, 46, 48, 2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38 };

const int DONGLETOIPSCTABLE[49] =
//0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12. 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
{ 0, 18, 36, 1, 19, 37, 2, 20, 38, 3, 21, 39, 4, 22, 40, 5, 23, 41, 6, 24, 42, 7, 25, 43, 8, 26, 44, 9, 27, 45, 10, 28, 46, 11, 29, 47, 12, 30, 48, 13, 31, 14, 32, 15, 33, 16, 34, 17, 35 };

//wx
const int THEPCMFRAMEFLDSAMPLESLENGTH = 320;

#define AIO_BUFSIZE 512

class CSerialDongle  
{
public:
	CSerialDongle();
	virtual ~CSerialDongle();

	//Called from Dlg/User.
	int		open_dongle(const char *lpsz_Device);
	void	send_dongle_initialization(void);//send control packets
	void	close_dongle(void);

	//moto-ambe 数据在进行解压之前，需要先将每一帧数据进行位变换，然后发送到dongle，dongle才能正常解压成PCM数据。反之亦然
	tAMBEFrame* GetFreeAMBEBuffer(void);
	bool        MarkAMBEBufferFilled(void);
	void deObfuscate(ScrambleDirection theDirection, tAMBEFrame* pAMBEFrame);

	//设置串口接收到完整数据后的回调函数
	void SetDongleRxDataCallBack(void(*Func)(void *ptr, short ptr_len));
	
	void send_any_ambe_to_dongle(void);

private:


	static CSerialDongle *pThis;

	bool	m_PleaseStopSerial;

	int		m_hComm;//usart description
	struct aiocb    r_cbp;//read
	struct aiocb    w_cbp;//write

	MySynCond *tx_serial_event_cond;
	MySynCond *rx_serial_event_cond;

	MyCreateThread * serial_tx_thread_p;
	MyCreateThread * serial_rx_thread_p;

	int            m_RxMsgLength;
	int            m_RxMsgIndex;
	DVSI3000struct m_RxDVSImsg;
	PARSERSTATE    m_ParserState;
	uint8_t m_DongleRxBuffer[INTERNALCOMBUFFSIZE];


	bool  fWaitingOnWrite;
	bool  fWaitingOnPCM;
	bool  fWaitingOnAMBE;

	//AMBE-Queue
	tAMBEFrame      m_AMBE_CirBuff[MAXDONGLEAMBEFRAMES];
	int             m_AMBEBufHead;
	int             m_AMBEBufTail;

	//PCM-Queue
	tPCMFrame       m_PCM_CirBuff[MAXDONGLEPCMFRAMES];
	int             m_PCMBufHead;
	int             m_PCMBufTail;

	//Queue-flag
	bool			m_bPleasePurgeAMBE;
	bool			m_bPleasePurgePCM;

	int           m_dwExpectedDongleRead;//需根据不同需求切换当前是要读AMBE类型数据还是PCM数据类型
	int  CreateSerialRxThread();
	int  CreateSerialTxThread();
	/*
	设置线程退出标志
	*/
	void SetThreadExitFlag()   { m_PleaseStopSerial = true; }


	//set and reset serial bevent
	void set_rx_serial_event();
	void reset_rx_serial_event();

	void set_tx_serial_event();
	void reset_tx_serial_event();

	//Functions running in Serial Thread:
	static void *SerialRxThread(void* p);//must be static since is thread
	static void *SerialTxThread(void* p);//must be static since is thread

	int SerialRxThreadFunc();
	int SerialTxThreadFunc();

	//asyn-callback
	static void aio_read_completion_hander(sigval_t sigval);
	static void aio_write_completion_hander(sigval_t sigval);
	//void aio_read_completion_hander_func();
	//void aio_write_completion_hander_func();

	void(*DongleRxDataCallBackFunc)(void *ptr, short ptr_len);//回调
	
	int   AssembleMsg(int numBytes, int * dwBytesAssembled);
	/*void        ParseDVSImsg(DVSI3000struct* pMsg);
	*/
	//Called from otherThreads.
	uint8_t	CheckSum(DVSI3000struct* pMsg);//校验数据包接口
	void	ParseDVSImsg(DVSI3000struct* pMsg);

	int aio_read_file(struct aiocb *r_cbp_t, int fd, int size);
	int aio_write_file(struct aiocb *w_cbp_t, int fd, void *buff, int size);

	void purge_dongle(int flags);


public:

	void extract_voice(char * pBuffer, int len);
	uint8_t * read_dongle_data();
	void set_read_dongle_data();
	void get_read_dongle_data();

private:
	unsigned int dataType;
	uint8_t thePCMFrameFldSamples[THEPCMFRAMEFLDSAMPLESLENGTH];
	CUsartWrap	m_usartwrap;

};

#endif 
