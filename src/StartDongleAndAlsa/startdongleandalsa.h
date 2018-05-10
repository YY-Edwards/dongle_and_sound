
#ifndef _STARTDONGLEANDALSA_H_  
#define _STARTDONGLEANDALSA_H_  
#pragma once
#include "serialdongle.h"
#include "audioalsa.h"
#include <time.h>  


class CStartDongleAndSound
{
public:

	CStartDongleAndSound();
	~CStartDongleAndSound();
	bool start(const char *lpszDevice, const char *pcm_name = "plughw:0,0");
	void stop();
	void read_voice_file(char* pBuffer, int len);

public:

private:
	timer_t timerid;
	void timer();//用来定时(20ms)触发串口读，写数据，并将CSerialDongle对象句柄传入定时器信号处理函数中
	CSerialDongle m_serialdongle;

};

void timer_routine(union sigval v);



#endif 
