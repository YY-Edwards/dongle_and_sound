
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
	void stop();//stop tatol dongle and alsa
	void stop(const char *);//stop one dongle
	void read_voice_file(char* pBuffer, int len);

public:

private:
	int max_dongle_num;
	timer_t timerid;
	void timer();//������ʱ(20ms)�������ڶ���д���ݣ�����CSerialDongle���������붨ʱ���źŴ�������
	//CSerialDongle m_serialdongle;
	CSerialDongle *m_new_dongle_ptr;

	struct cmp_str//��Ҫ��д�ȽϷ�ʽ����ȻSTL���ñȽϵ���ָ��������ַ�����
	{
		bool operator()(char const *a, char const *b)
		{
			return std::strcmp(a, b) < 0;
		}
	};
	map<const char *, CSerialDongle *, cmp_str> dongle_map;

};

void timer_routine(union sigval v);



#endif 
