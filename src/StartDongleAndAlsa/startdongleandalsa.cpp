#include "startdongleandalsa.h"



CStartDongleAndSound::CStartDongleAndSound()
{
	log_debug("New: CStartDongleAndSound\n");
}

CStartDongleAndSound::~CStartDongleAndSound()
{
	log_debug("Destory: CStartDongleAndSound\n");
}

bool CStartDongleAndSound::start(const char *lpszDevice, const char *pcm_name)
{
	auto  result = false;
	//timer
	timer();
	//dongle
	result = m_serialdongle.open_dongle(lpszDevice);
	if (result != true)
	{
		return false;
	}
	
	m_serialdongle.send_dongle_initialization();


	//Aduio ALSA

	return true;
}
void CStartDongleAndSound::stop()
{
	timer_delete(timerid);
	m_serialdongle.close_dongle();

}

void CStartDongleAndSound::timer()//������ʱ(20ms)�������ڶ���д���ݣ�����CSerialDongle���������붨ʱ���źŴ�������
{
	// XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);  
	// clockid--ֵ��CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID  
	// evp--��Ż���ֵ�ĵ�ַ,�ṹ��Ա˵���˶�ʱ�����ڵ�֪ͨ��ʽ�ʹ���ʽ��  
	// timerid--��ʱ����ʶ��  
	//timer_t timerid;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));   //�����ʼ��  

	evp.sigev_value.sival_ptr = &m_serialdongle;        //Ҳ�Ǳ�ʶ��ʱ���ģ����timerid��ʲô���𣿻ص��������Ի��  
	evp.sigev_notify = SIGEV_THREAD;        //�߳�֪ͨ�ķ�ʽ����פ���߳�  
	evp.sigev_notify_function = timer_routine;   //�̺߳�����ַ  

	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		log_warning("fail to timer_create:%d\n",errno);
		//return -1;
	}

	// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
	// timerid--��ʱ����ʶ  
	// flags--0��ʾ���ʱ�䣬1��ʾ����ʱ�䣬ͨ��ʹ�����ʱ��  
	// new_value--��ʱ�����³�ʼֵ�ͼ�����������it  
	// old_value--ȡֵͨ��Ϊ0�������ĸ�������ΪNULL,����ΪNULL���򷵻ض�ʱ����ǰһ��ֵ  

	//��һ�μ��it.it_value��ô��,�Ժ�ÿ�ζ���it.it_interval��ô��,����˵it.it_value��0��ʱ���װ��it.it_interval��ֵ  
	//it.it_interval�������Ϊ����  
	struct itimerspec it;
	it.it_interval.tv_sec = 0;  
	it.it_interval.tv_nsec = 500000000;//���20ms
	it.it_value.tv_sec = 1;
	it.it_value.tv_nsec = 500000000;

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		log_warning("fail to timer_settime:%d\n",errno);
		timer_delete(timerid);
		//return -1;
	}

	log_debug("create timer okay.\n");

}

void CStartDongleAndSound::read_voice_file(char* pBuffer, int len)
{
	m_serialdongle.extract_voice(pBuffer, len);
}

void timer_routine(union sigval v)
{
	CSerialDongle *ptr = (CSerialDongle*)v.sival_ptr;
	
	ptr->send_any_ambe_to_dongle();
	ptr->get_read_dongle_data();
}