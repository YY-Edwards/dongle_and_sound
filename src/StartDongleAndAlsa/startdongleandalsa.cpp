#include "startdongleandalsa.h"



CStartDongleAndSound::CStartDongleAndSound()
{
	m_new_dongle_ptr = nullptr;
	log_debug("New: CStartDongleAndSound\n");
}

CStartDongleAndSound::~CStartDongleAndSound()
{
	log_debug("Destory: CStartDongleAndSound\n");
}

bool CStartDongleAndSound::start(const char *lpszDevice, const char *pcm_name)
{
	auto  result = false;
	m_new_dongle_ptr = nullptr;
	m_new_dongle_ptr = new CSerialDongle;
	if (m_new_dongle_ptr != nullptr){

		dongle_map[lpszDevice] = m_new_dongle_ptr;//insert map
		result = m_new_dongle_ptr->open_dongle(lpszDevice);
		if (result != true)
		{
			log_warning("open dongle failure!\n");
			return false;
		}

		m_new_dongle_ptr->send_dongle_initialization();
	}


	////timer
	////timer();
	////dongle
	//result = m_serialdongle.open_dongle(lpszDevice);
	//if (result != true)
	//{
	//	log_warning("open dongle failure!\n");
	//	return false;
	//}
	//
	//m_serialdongle.send_dongle_initialization();

	//Aduio ALSA

	return true;
}
void CStartDongleAndSound::stop()
{
	//timer_delete(timerid);
	//m_serialdongle.close_dongle();

	while (dongle_map.size() > 0)
	{
		auto it = dongle_map.begin();
		if (it->second != nullptr){

			it->second->close_dongle();
			delete it->second;
			it->second = nullptr;
		}
		log_debug("stop dongle:%s\n", it->first);
		dongle_map.erase(it);

	}
	m_new_dongle_ptr = nullptr;

}

void CStartDongleAndSound::stop(const char *device)//stop one dongle
{
	
	auto it = dongle_map.find(device);
	if (it != dongle_map.end())
	{
		if (it->second != nullptr){

			it->second->close_dongle();
			delete it->second;
			it->second = nullptr;
		}
		dongle_map.erase(it);
		log_debug("stop dongle:%s\n", device);
	}
	else
	{
		log_warning("note,no dongle:%s!!!\n", device);
	}

}

void CStartDongleAndSound::timer()//用来定时(20ms)触发串口读，写数据，并将CSerialDongle对象句柄传入定时器信号处理函数中
{
	// XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);  
	// clockid--值：CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID  
	// evp--存放环境值的地址,结构成员说明了定时器到期的通知方式和处理方式等  
	// timerid--定时器标识符  
	//timer_t timerid;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));   //清零初始化  

	evp.sigev_value.sival_ptr = m_new_dongle_ptr;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得  
	//evp.sigev_value.sival_ptr = &m_serialdongle;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得  
	evp.sigev_notify = SIGEV_THREAD;        //线程通知的方式，派驻新线程  
	evp.sigev_notify_function = timer_routine;   //线程函数地址  

	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		log_warning("fail to timer_create:%s\n", strerror(errno));
		//return -1;
	}

	// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
	// timerid--定时器标识  
	// flags--0表示相对时间，1表示绝对时间，通常使用相对时间  
	// new_value--定时器的新初始值和间隔，如下面的it  
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值  

	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值  
	//it.it_interval可以理解为周期  
	struct itimerspec it;
	it.it_interval.tv_sec = 0;  
	it.it_interval.tv_nsec = 20000000;//间隔20ms
	it.it_value.tv_sec = 0;
	it.it_value.tv_nsec = 20000000;

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		log_warning("fail to timer_settime:%s\n", strerror(errno));
		timer_delete(timerid);
		//return -1;
	}

	log_debug("create timer okay.\n");

}

void CStartDongleAndSound::read_voice_file(char* pBuffer, int len)
{

	//m_serialdongle.extract_voice(pBuffer, len);
}

void timer_routine(union sigval v)
{
	CSerialDongle *ptr = (CSerialDongle*)v.sival_ptr;
	
	//ptr->send_any_ambe_to_dongle();
	//ptr->get_read_dongle_data();
}