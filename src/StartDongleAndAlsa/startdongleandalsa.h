
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
	void run_timer(int time_s = 0, int time_ms = 20);//������ʱ��ms
	void pause_timer();//ֹͣ��ʱ����
	//void get_voice_cache_from_file(const char* file_name);

public:

	//static void extract_hotplug_info(hotplug_info_t *hpug_ptr);//���߳�ģʽ
	//void extract_hotplug_info_func(hotplug_info_t *hpug_ptr);

private:
	static CStartDongleAndSound *pThis;

	int pcm_voice_fd;
	static void dongle_ondata_func(void *ptr, short ptr_len);
	//static void dongle_aio_completion_hander(int signo, siginfo_t *info, void *context);


	char *voice_cache_ptr;
	int cache_nbytes;
	int next;
	timer_t timerid;
	bool timer_created_flag;
	//������ʱ��
	void timer();//������ʱ(20ms)�������ڶ���д���ݣ�����CSerialDongle���������붨ʱ���źŴ�������
	//CSerialDongle m_serialdongle;
	CSerialDongle *m_new_dongle_ptr;

	//std::string *lpszDevice_str_ptr_;
	//char * lpszDevice_str_ptr_;


	std::mutex						map_mutex_;
	map<string, CSerialDongle *> dongle_map;

	static void timer_routine(union sigval v);

	void enum_dongle(const char *dev_path="/dev");//dongle����

};

//void timer_routine(union sigval v);



#endif 
