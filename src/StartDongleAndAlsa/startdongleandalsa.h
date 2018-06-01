
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
	//void get_voice_cache_from_file(const char* file_name);

public:

	//static void extract_hotplug_info(hotplug_info_t *hpug_ptr);//单线程模式
	//void extract_hotplug_info_func(hotplug_info_t *hpug_ptr);

private:
	static CStartDongleAndSound *pThis;
	char *voice_cache_ptr;
	int cache_nbytes;
	int next;
	timer_t timerid;
	bool timer_start_flag;
	void timer();//用来定时(20ms)触发串口读，写数据，并将CSerialDongle对象句柄传入定时器信号处理函数中
	//CSerialDongle m_serialdongle;
	CSerialDongle *m_new_dongle_ptr;

	struct cmp_str//需要重写比较方式，不然STL内置比较的是指针而不是字符串。
	{
		bool operator()(char const *a, char const *b)
		{
			return ::strcmp(a, b) < 0;
		}
	};
	map<const char *, CSerialDongle *, cmp_str> dongle_map;

};

void timer_routine(union sigval v);



#endif 
