#include "startdongleandalsa.h"
CStartDongleAndSound * CStartDongleAndSound::pThis = nullptr;

CStartDongleAndSound::CStartDongleAndSound()
{
	pThis = this;
	voice_cache_ptr = nullptr;
	m_new_dongle_ptr = nullptr;
	//lpszDevice_str_ptr_ = nullptr;
	cache_nbytes = 0;
	next = 0;
	timerid = 0;
	timer_start_flag = false;
	dongle_map.clear();

	pcm_voice_fd = open("/opt/pcm.data", O_RDWR | O_APPEND | O_CREAT);
	if (pcm_voice_fd < 0)
	{
		log_warning("pcm_voice_fd open error\r\n");
		close(pcm_voice_fd);
		//return;
	}
	else
	{
		/* ����ļ� */
		ftruncate(pcm_voice_fd, 0);

		/* ���������ļ�ƫ���� */
		lseek(pcm_voice_fd, 0, SEEK_SET);
	}

	log_info("New: CStartDongleAndSound\n");
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
	//if (lpszDevice_str_ptr_ != nullptr)
	//{
	//	delete []lpszDevice_str_ptr_;
	//	lpszDevice_str_ptr_ = nullptr;
	//}

	close(pcm_voice_fd);

	log_info("Destory: CStartDongleAndSound\n");
}

bool CStartDongleAndSound::start(const char *lpszDevice, const char *pcm_name)
{
	auto  result = false;
	m_new_dongle_ptr = nullptr;
	//std::string lpszDevice_str = lpszDevice;
	//lpszDevice_str_ptr_ = new std::string(lpszDevice);
	//lpszDevice_str_ptr_ = new char[strlen(lpszDevice)+1];
	//strcpy(lpszDevice_str_ptr_, lpszDevice);

	m_new_dongle_ptr = new CSerialDongle;
	if (m_new_dongle_ptr != nullptr){

		dongle_map[lpszDevice] = m_new_dongle_ptr;//insert map
		result = m_new_dongle_ptr->open_dongle(lpszDevice, dongle_aio_completion_hander);
		if (result != true)
		{
			log_warning("open dongle failure!\n");
			return false;
		}
		m_new_dongle_ptr->SetDongleRxDataCallBack(dongle_ondata_func);
		log_info("open a new dongle okay\n");
		m_new_dongle_ptr->send_dongle_initialization();

		//timer();
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

			log_info("stop dongle:%s\n", it->first);
			it->second->close_dongle();
			delete it->second;
			it->second = nullptr;
		
		}	
		dongle_map.erase(it);

	}
	if (m_new_dongle_ptr != nullptr)
	{
		m_new_dongle_ptr = nullptr;
	}
	/*if (lpszDevice_str_ptr_ != nullptr)
	{
		lpszDevice_str_ptr_ = nullptr;
	}*/

	if (timerid!=0)
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
		log_info("stop dongle:%s\n", device);
	}
	else
	{
		log_warning("note,no dongle:%s!!!\n", device);
	}

}

void CStartDongleAndSound::run_timer()
{

	timer();

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

	evp.sigev_value.sival_ptr = &dongle_map;
	//evp.sigev_value.sival_ptr = m_new_dongle_ptr;        //Ҳ�Ǳ�ʶ��ʱ���ģ����timerid��ʲô���𣿻ص��������Ի��  
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
	log_info("create timer okay.\n");

}

void CStartDongleAndSound::read_voice_file(char* pBuffer, int len)
{
	int AMBE_fragment_bytes = THEAMBEFRAMEFLDSAMPLESLENGTH;
	int q_len = len / AMBE_fragment_bytes;
	int index = 0;
	map<string, CSerialDongle *>::iterator it;
	for (it = dongle_map.begin(); it != dongle_map.end();)
	{
		it->second->extract_voice((pBuffer + index), AMBE_fragment_bytes);
		index += AMBE_fragment_bytes;
		if (index >= len)break;
		it++;
		if (it == dongle_map.end())//ѭ��
		{
			it = dongle_map.begin();
		}
	}


	/*if (m_new_dongle_ptr != nullptr)
		m_new_dongle_ptr->extract_voice(pBuffer, len);*/
	log_info("extract voice data over. \n");
	//m_serialdongle.extract_voice(pBuffer, len);
}

void timer_routine(union sigval v)
{
	//CSerialDongle *ptr = (CSerialDongle*)v.sival_ptr;
	/*ptr->send_any_ambe_to_dongle();
	ptr->get_read_dongle_data();*/
	
	map<string, CSerialDongle *> *ptr = (map<string, CSerialDongle *> *)v.sival_ptr;
	if (ptr->size() == 0)return;
	static map<string, CSerialDongle *>::iterator it = ptr->begin();

	it->second->send_any_ambe_to_dongle();
	it->second->get_read_dongle_data();
	it++;
	if (it == ptr->end())//ѭ��
	{
		it = ptr->begin();
	}

	

}

void CStartDongleAndSound::dongle_ondata_func(void *ptr, short ptr_len)
{

	if ((ptr != NULL) && (ptr_len != 0) && (pThis->pcm_voice_fd != 0))
	{
		auto ret = write(pThis->pcm_voice_fd, ptr, ptr_len);
		if (ret < 0)
		{
			log_warning("write pcm-voice err!!!\r\n");
		}
		else
		{
			if (ret=!THEPCMFRAMEFLDSAMPLESLENGTH)
			{
				log_warning("write pcm-voice uncompleted:%ld\r\n", ret);
			}
			log_info("save pcm data okay.\n");
		}
	}

}

//void CStartDongleAndSound::extract_hotplug_info(hotplug_info_t *hpug_ptr)//���߳�ģʽ
//{
//
//	if (pThis != nullptr)
//	{
//		pThis->extract_hotplug_info_func(hpug_ptr);
//	}
//
//}
//void CStartDongleAndSound::extract_hotplug_info_func(hotplug_info_t *hpug_ptr)
//{
//		hotplug_info_t *temp_ptr = hpug_ptr;
//		string action_add = "add";
//		string action_remove = "remove";
//		string action_change = "change";
//		string compare_subsystem = "tty";
//		string compare_id_driver = "cdc_acm";
//	
//	
//		//if ((temp_ptr->subsystem.compare(compare_subsystem) == 0))
//		if ((temp_ptr->subsystem.compare(compare_subsystem) == 0) 
//			&& (temp_ptr->id_driver.compare(compare_id_driver) == 0)
//			)
//		{
//			log_info("find the dongle device\n");
//			log_info("action:%s\n", temp_ptr->action.c_str());
//			log_info("devpath:%s\n", temp_ptr->path.c_str());
//			log_info("devname:%s\n", temp_ptr->devname.c_str());
//			if (temp_ptr->action.compare(action_add) == 0)
//			{
//				start(temp_ptr->devname.c_str());
//				sleep(30);
//				read_voice_file(voice_cache_ptr, cache_nbytes);
//				//m_startdongle.start(temp_ptr->devname.c_str());
//				/*if (pBuffer!=nullptr)m_startdongle.read_voice_file(pBuffer, nread);*/
//			}
//			else if (temp_ptr->action.compare(action_remove) == 0)
//			{
//				stop(temp_ptr->devname.c_str());
//				//m_startdongle.stop(temp_ptr->devname.c_str());
//			}
//			else if (temp_ptr->action.compare(action_change) == 0)
//			{
//	
//			}
//	
//		}
//		else
//		{
//			log_warning("find no dongle!\n");
//		}
//		
//
//}
//void CStartDongleAndSound::get_voice_cache_from_file(const char* file_name)
//{
//	auto file_fd = open(file_name, O_RDONLY);
//	if (file_fd < 0)
//	{
//		log_warning("can't open :%s\n", file_name);
//		close(file_fd);
//	}
//	auto file_length = lseek(file_fd, 0, SEEK_END);
//	lseek(file_fd, 0, SEEK_SET);
//	voice_cache_ptr = new  char[file_length];
//	auto nread = read(file_fd, voice_cache_ptr, file_length);
//	if (nread == file_length)
//	{
//		cache_nbytes = nread;
//		log_info("get voice file success\r\n");
//	}
//	else
//	{
//		log_warning("get voice file no all\r\n");
//	}
//
//	//start("/dev/ttyACM0");
//	//read_voice_file(voice_cache_ptr, cache_nbytes);
//
//}

void CStartDongleAndSound::dongle_aio_completion_hander(int signo, siginfo_t *info, void *context)
{

	aio_hander_t *my_aio_hander_ptr;
	struct aiocb  *req;
	CSerialDongle *this_ptr;

	int           ret;
	static int nwrited = 0;

	if (info->si_signo == SIGIO)//ȷ����������Ҫ���ź�
	{
		//log_info("w-signal code:%d\n", info->si_code); 
		//��ȡaiocb �ṹ�����Ϣ
		//req = (struct aiocb*) info->si_value.sival_ptr;
		my_aio_hander_ptr = (struct aio_hander_t*) info->si_value.sival_ptr;
		req = my_aio_hander_ptr->w_cbp_ptr;
		this_ptr = (CSerialDongle *)my_aio_hander_ptr->the_CSerialDongle_pthis;

		//if (req->aio_fildes == this_ptr->w_cbp.aio_fildes)//ȷ���ź�����ָ�����ļ�������
		{
			/*AIO������ɣ�*/
			ret = aio_error(req);
			log_info("\n\n");
			//log_info("aio write status:%d\n", ret);
			switch (ret)
			{
			case EINPROGRESS://working
				log_info("aio write is EINPROGRESS.\n");
				break;

			case ECANCELED://cancelled
				log_info("aio write is ECANCELLED.\n");
				break;

			case -1://failure
				log_info("aio write is failure, errno:%s\n", strerror(errno));
				break;

			case 0://success

				ret = aio_return(req);
				log_info("fd:%d,[%s aio_write :%d bytes.]\n", req->aio_fildes, this_ptr->dongle_name.c_str(), ret);
				nwrited += ret;
				if (nwrited == AMBE3000_AMBE_BYTESINFRAME || nwrited == AMBE3000_PCM_BYTESINFRAME)
				{
					log_info("aio_write complete[.]\n");
					nwrited = 0;
					if (this_ptr->fWaitingOnPCM){
						this_ptr->m_PCMBufTail = (this_ptr->m_PCMBufTail + 1) & MAXDONGLEPCMFRAMESMASK;
					}
					if (this_ptr->fWaitingOnAMBE){
						this_ptr->m_AMBEBufTail = (this_ptr->m_AMBEBufTail + 1) & MAXDONGLEAMBEFRAMESMASK;
					}

					this_ptr->fWaitingOnWrite = false;
					this_ptr->fWaitingOnPCM = false;
					this_ptr->fWaitingOnAMBE = false;

					this_ptr->send_index++;
					log_info("%s send ambe index:%d\n", this_ptr->dongle_name.c_str(), this_ptr->send_index);
				}

				break;
			default:
				break;
			}
		}
		/*else
		{
			log_warning("note,other fd: %d \n", req->aio_fildes);
		}*/
	}
	else
	{
		log_warning("note,other singal:%d \n", info->si_signo);
	}


}
