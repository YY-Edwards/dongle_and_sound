#include "startdongleandalsa.h"
CStartDongleAndSound * CStartDongleAndSound::pThis = nullptr;

CStartDongleAndSound::CStartDongleAndSound()
{
	pThis = this;
	voice_cache_ptr = nullptr;
	m_new_dongle_ptr = nullptr;
	cache_nbytes = 0;
	next = 0;
	timer_start_flag = false;
	dongle_map.clear();
	log_debug("New: CStartDongleAndSound\n");
}

CStartDongleAndSound::~CStartDongleAndSound()
{
	if (voice_cache_ptr != nullptr)
	{
		delete []voice_cache_ptr;
		voice_cache_ptr = nullptr;
	}
	if (m_new_dongle_ptr != nullptr)
	{
		delete m_new_dongle_ptr;
		m_new_dongle_ptr = nullptr;
	}

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
		log_debug("open a new dongle\n");
		m_new_dongle_ptr->send_dongle_initialization();

		timer();
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
	timer_delete(timerid);
	timer_start_flag = false;

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

void CStartDongleAndSound::timer()//������ʱ(20ms)�������ڶ���д���ݣ�����CSerialDongle���������붨ʱ���źŴ�������
{
	// XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);  
	// clockid--ֵ��CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID  
	// evp--��Ż���ֵ�ĵ�ַ,�ṹ��Ա˵���˶�ʱ�����ڵ�֪ͨ��ʽ�ʹ���ʽ��  
	// timerid--��ʱ����ʶ��  
	//timer_t timerid;
	if (timer_start_flag)return;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));   //�����ʼ��  

	evp.sigev_value.sival_ptr = m_new_dongle_ptr;        //Ҳ�Ǳ�ʶ��ʱ���ģ����timerid��ʲô���𣿻ص��������Ի��  
	//evp.sigev_value.sival_ptr = &m_serialdongle;        //Ҳ�Ǳ�ʶ��ʱ���ģ����timerid��ʲô���𣿻ص��������Ի��  
	evp.sigev_notify = SIGEV_THREAD;        //�߳�֪ͨ�ķ�ʽ����פ���߳�  
	evp.sigev_notify_function = timer_routine;   //�̺߳�����ַ  

	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		log_warning("fail to timer_create:%s\n", strerror(errno));
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
	it.it_interval.tv_nsec = 20000000;//���20ms
	it.it_value.tv_sec = 0;
	it.it_value.tv_nsec = 20000000;

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		log_warning("fail to timer_settime:%s\n", strerror(errno));
		timer_delete(timerid);
		//return -1;
	}

	timer_start_flag = true;
	log_debug("create timer okay.\n");

}

void CStartDongleAndSound::read_voice_file(char* pBuffer, int len)
{
	if (m_new_dongle_ptr != nullptr)
		m_new_dongle_ptr->extract_voice(pBuffer, len);

	//m_serialdongle.extract_voice(pBuffer, len);
}

void timer_routine(union sigval v)
{
	CSerialDongle *ptr = (CSerialDongle*)v.sival_ptr;
	
	ptr->send_any_ambe_to_dongle();
	ptr->get_read_dongle_data();
}

void CStartDongleAndSound::extract_hotplug_info(hotplug_info_t *hpug_ptr)//���߳�ģʽ
{

	if (pThis != nullptr)
	{
		pThis->extract_hotplug_info_func(hpug_ptr);
	}

}
void CStartDongleAndSound::extract_hotplug_info_func(hotplug_info_t *hpug_ptr)
{
		hotplug_info_t *temp_ptr = hpug_ptr;
		string action_add = "add";
		string action_remove = "remove";
		string action_change = "change";
		string compare_subsystem = "tty";
		string compare_id_driver = "cdc_acm";
	
	
		//if ((temp_ptr->subsystem.compare(compare_subsystem) == 0))
		if ((temp_ptr->subsystem.compare(compare_subsystem) == 0) 
			&& (temp_ptr->id_driver.compare(compare_id_driver) == 0)
			)
		{
			log_debug("find the dongle device\n");
			log_debug("action:%s\n", temp_ptr->action.c_str());
			log_debug("devpath:%s\n", temp_ptr->path.c_str());
			log_debug("devname:%s\n", temp_ptr->devname.c_str());
			if (temp_ptr->action.compare(action_add) == 0)
			{
				start(temp_ptr->devname.c_str());
				read_voice_file(voice_cache_ptr, cache_nbytes);
				//m_startdongle.start(temp_ptr->devname.c_str());
				/*if (pBuffer!=nullptr)m_startdongle.read_voice_file(pBuffer, nread);*/
			}
			else if (temp_ptr->action.compare(action_remove) == 0)
			{
				stop(temp_ptr->devname.c_str());
				//m_startdongle.stop(temp_ptr->devname.c_str());
			}
			else if (temp_ptr->action.compare(action_change) == 0)
			{
	
			}
	
		}
		else
		{
			log_warning("find no dongle!\n");
		}
		

}
void CStartDongleAndSound::get_voice_cache_from_file(const char* file_name)
{
	auto file_fd = open(file_name, O_RDONLY);
	if (file_fd < 0)
	{
		log_warning("can't open :%s\n", argv[1]);
		close(file_fd);
	}
	auto file_length = lseek(file_fd, 0, SEEK_END);
	lseek(file_fd, 0, SEEK_SET);
	voice_cache_ptr = new  char[file_length];
	auto nread = read(file_fd, voice_cache_ptr, file_length);
	if (nread == file_length)
	{
		cache_nbytes = nread;
		log_debug("get voice file success\r\n");
	}
	else
	{
		log_warning("get voice file no all\r\n");
	}

}