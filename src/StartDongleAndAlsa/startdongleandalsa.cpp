#include "startdongleandalsa.h"
#include <dirent.h>  
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
	timer_created_flag = false;
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
		/* 清空文件 */
		ftruncate(pcm_voice_fd, 0);

		/* 重新设置文件偏移量 */
		lseek(pcm_voice_fd, 0, SEEK_SET);
	}

	enum_dongle();

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

		//result = m_new_dongle_ptr->open_dongle(lpszDevice, dongle_aio_completion_hander);
		result = m_new_dongle_ptr->open_dongle(lpszDevice);
		if (result != true)
		{
			log_warning("open dongle failure!\n");
			return false;
		}
		m_new_dongle_ptr->SetDongleRxDataCallBack(dongle_ondata_func);
		log_info("open a new dongle okay\n");
		m_new_dongle_ptr->send_dongle_initialization();

		dongle_map[lpszDevice] = m_new_dongle_ptr;//insert map

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

	if (timerid != 0)
		timer_delete(timerid);
	timer_created_flag = false;

	std::lock_guard<std::mutex> guard(map_mutex_);
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


}

void CStartDongleAndSound::stop(const char *device)//stop one dongle
{
	std::lock_guard<std::mutex> guard(map_mutex_);
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

void CStartDongleAndSound::run_timer(int time_s, int time_ms)
{

	if (!timer_created_flag)return;
	//timer();

	// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
	// timerid--定时器标识  
	// flags--0表示相对时间，1表示绝对时间，通常使用相对时间  
	// new_value--定时器的新初始值和间隔，如下面的it  
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值  
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值  
	//it.it_interval可以理解为周期  
	struct itimerspec it;
	it.it_interval.tv_sec = time_s;
	it.it_interval.tv_nsec = time_ms * 1000000;//间隔20ms
	it.it_value.tv_sec = 1;
	it.it_value.tv_nsec = 0;

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		log_warning("fail to timer_settime:%s\n", strerror(errno));
		timer_delete(timerid);
		//return -1;
	}



}

void CStartDongleAndSound::pause_timer()
{
	if (!timer_created_flag)return;
	//timer();

	// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
	// timerid--定时器标识  
	// flags--0表示相对时间，1表示绝对时间，通常使用相对时间  
	// new_value--定时器的新初始值和间隔，如下面的it  
	// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值  
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值  
	//it.it_interval可以理解为周期  
	struct itimerspec it;
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_nsec = 0;//间隔0
	it.it_value.tv_sec = 0;
	it.it_value.tv_nsec = 0;

	if (timer_settime(timerid, 0, &it, NULL) == -1)
	{
		log_warning("fail to timer_settime:%s\n", strerror(errno));
		timer_delete(timerid);
		//return -1;
	}

}

void CStartDongleAndSound::timer()//用来定时(20ms)触发串口读，写数据，并将CSerialDongle对象句柄传入定时器信号处理函数中
{
	// XXX int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);  
	// clockid--值：CLOCK_REALTIME,CLOCK_MONOTONIC,CLOCK_PROCESS_CPUTIME_ID,CLOCK_THREAD_CPUTIME_ID  
	// evp--存放环境值的地址,结构成员说明了定时器到期的通知方式和处理方式等  
	// timerid--定时器标识符  
	//timer_t timerid;
	if (timer_created_flag)return;
	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));   //清零初始化  

	evp.sigev_value.sival_ptr = &dongle_map;
	//evp.sigev_value.sival_ptr = m_new_dongle_ptr;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得  
	//evp.sigev_value.sival_ptr = &m_serialdongle;        //也是标识定时器的，这和timerid有什么区别？回调函数可以获得  
	evp.sigev_notify = SIGEV_THREAD;        //线程通知的方式，派驻新线程  
	evp.sigev_notify_function = timer_routine;   //线程函数地址  

	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1)
	{
		log_warning("fail to timer_create:%s\n", strerror(errno));
		//return -1;
	}

	//// XXX int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value,struct itimerspec *old_value);  
	//// timerid--定时器标识  
	//// flags--0表示相对时间，1表示绝对时间，通常使用相对时间  
	//// new_value--定时器的新初始值和间隔，如下面的it  
	//// old_value--取值通常为0，即第四个参数常为NULL,若不为NULL，则返回定时器的前一个值  
	////第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长,就是说it.it_value变0的时候会装载it.it_interval的值  
	////it.it_interval可以理解为周期  
	//struct itimerspec it;
	//it.it_interval.tv_sec = 0;  
	//it.it_interval.tv_nsec = 20000000;//间隔20ms
	//it.it_value.tv_sec = 0;
	//it.it_value.tv_nsec = 20000000;

	//if (timer_settime(timerid, 0, &it, NULL) == -1)
	//{
	//	log_warning("fail to timer_settime:%s\n", strerror(errno));
	//	timer_delete(timerid);
	//	//return -1;
	//}

	timer_created_flag = true;
	log_info("create timer okay.\n");

}

void CStartDongleAndSound::read_voice_file(char* pBuffer, int len)
{
	//int AMBE_fragment_bytes = THEAMBEFRAMEFLDSAMPLESLENGTH;
	//int q_len = len / AMBE_fragment_bytes;
	int index = 0;
	int read_len = 0;
	int left_len = len;
	map<string, CSerialDongle *>::iterator it;
	for (it = dongle_map.begin(); it != dongle_map.end();)
	{

		read_len = (left_len >= THEAMBEFRAMEFLDSAMPLESLENGTH) ? THEAMBEFRAMEFLDSAMPLESLENGTH : left_len;
		if (read_len <= 0)break;

		it->second->extract_voice((pBuffer + index), read_len);

		index += read_len;
		left_len -= read_len;

		it++;
		if (it == dongle_map.end())//循环
		{
			it = dongle_map.begin();
		}
	}


	/*if (m_new_dongle_ptr != nullptr)
		m_new_dongle_ptr->extract_voice(pBuffer, len);*/
	log_info("extract voice data over. \n");
	//m_serialdongle.extract_voice(pBuffer, len);
}

void CStartDongleAndSound::timer_routine(union sigval v)
{
	//CSerialDongle *ptr = (CSerialDongle*)v.sival_ptr;
	/*ptr->send_any_ambe_to_dongle();
	ptr->get_read_dongle_data();*/

	std::lock_guard<std::mutex> guard(pThis->map_mutex_);

	//map<string, CSerialDongle *> *ptr = (map<string, CSerialDongle *> *)v.sival_ptr;
	//if (ptr->size() == 0)return;


	//static map<string, CSerialDongle *>::iterator it = ptr->begin();
	for (map<string, CSerialDongle *>::iterator it = pThis->dongle_map.begin(); it != pThis->dongle_map.end(); it++)
	{
		it->second->send_any_ambe_to_dongle();
		it->second->get_read_dongle_data();
		usleep(5000);
		
	}

	//it->second->send_any_ambe_to_dongle();
	//it->second->get_read_dongle_data();
	//it++;
	//if (it == ptr->end())//循环
	//{
	//	it = ptr->begin();
	//}

	

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
			//log_info("save pcm data okay.\n");
		}
	}

}


void CStartDongleAndSound::enum_dongle(const char *dev_path)
{
	DIR *Dir_p;
	struct dirent *ent;
	int i = 0;
	char childpath[512] = { 0 };

	Dir_p = opendir(dev_path);
	while ((ent = readdir(Dir_p))!=NULL)
	{
		if (ent->d_type & DT_DIR)//目录
		{
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				continue;

			//sprintf(childpath, "%s/%s", dev_path, ent->d_name);
		}
		else//文件
		{
			std::string temp_str = ent->d_name;
			auto  start_index = temp_str.find("ttyACM", 0);//"ttyACM"
			if (start_index != string::npos) //hint:  here "string::npos"means find failed  
			{
				//log_info("find device:%s/%s \n", dev_path, temp_str.c_str());
				std::string dev_path_str = dev_path;
				std::string device_str = dev_path_str + "/" + temp_str;
				log_info("find device:%s \n", device_str.c_str());
				bool result = false;
				m_new_dongle_ptr = nullptr;
				m_new_dongle_ptr = new CSerialDongle;
				if (m_new_dongle_ptr != nullptr){

					result = m_new_dongle_ptr->open_dongle(device_str.c_str(), false);
					if (result != true)
					{
						log_warning("open device failure! \n");
						continue;
					}
					char product_id[100] = {0};
					char version[200] = {0};
					auto ret = m_new_dongle_ptr->get_dongle_version(product_id, version);
					if (ret != 0)
					{
						log_warning("this is not dongle! \n");
						continue;
					}
					else
					{

						m_new_dongle_ptr->SetDongleRxDataCallBack(dongle_ondata_func);
						log_info("open a new dongle okay, id:%s, version:%s \n",product_id, version);
						m_new_dongle_ptr->send_dongle_initialization();

						{
							std::lock_guard<std::mutex> guard(map_mutex_);
							dongle_map[device_str.c_str()] = m_new_dongle_ptr;//insert map
						}
						timer();
					}
				}

			}
		}

	}
	closedir(Dir_p);

}

//void CStartDongleAndSound::extract_hotplug_info(hotplug_info_t *hpug_ptr)//单线程模式
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

//void CStartDongleAndSound::dongle_aio_completion_hander(int signo, siginfo_t *info, void *context)
//{
//
//	struct aiocb  *req = NULL;
//	CSerialDongle *this_ptr = NULL;
//
//	int           ret;
//	static int nwrited = 0;
//
//	if (info->si_signo == SIGIO)//确定是我们需要的信号
//	{
//		//log_info("w-signal code:%d\n", info->si_code); 
//		//获取aiocb 结构体的信息
//		//req = (struct aiocb*) info->si_value.sival_ptr;
//		this_ptr = (CSerialDongle *)info->si_value.sival_ptr;
//		req = &(this_ptr->w_cbp);
//
//		//if (req->aio_fildes == this_ptr->w_cbp.aio_fildes)//确定信号来自指定的文件描述符
//		{
//			/*AIO请求完成？*/
//			ret = aio_error(req);
//
//			//信号处理函数与主函数之间的死锁
//			//当主函数访问临界资源时，通常需要加锁，如果主函数在访问临界区时，给临界资源上锁，此时发生了一个信号，
//			//那么转入信号处理函数，如果此时信号处理函数也对临界资源进行访问，那么信号处理函数也会加锁，
//			//由于主程序持有锁，信号处理程序等待主程序释放锁。又因为信号处理函数已经抢占了主函数，
//			//因此，主函数在信号处理函数结束之前不能运行。因此，必然造成死锁。
//			/*信号处理相当于软中断，会暂停所有的线程执行*/
//			/**/
//			std::lock_guard<std::mutex> guard(this_ptr->m_aio_syn_mutex_);
//			log_info("\n\n");
//			//log_info("aio write status:%d\n", ret);
//			switch (ret)
//			{
//			case EINPROGRESS://working
//				log_info("aio write is EINPROGRESS.\n");
//				break;
//
//			case ECANCELED://cancelled
//				log_info("aio write is ECANCELLED.\n");
//				break;
//
//			case -1://failure
//				log_info("aio write is failure, errno:%s\n", strerror(errno));
//				this_ptr->purge_dongle(this_ptr->m_wComm, TCOFLUSH);//刷新写入的数据
//				break;
//
//			case 0://success
//
//				ret = aio_return(req);
//				log_info("fd:%d,[%s aio_write :%d bytes.]\n", req->aio_fildes, this_ptr->dongle_name.c_str(), ret);
//				nwrited += ret;
//				if (nwrited == AMBE3000_AMBE_BYTESINFRAME || nwrited == AMBE3000_PCM_BYTESINFRAME)
//				{
//					log_info("aio_write complete[.]\n");
//					nwrited = 0;//Successful Tx Complete.
//					if (this_ptr->fWaitingOnPCM == true){
//						this_ptr->m_PCMBufTail = (this_ptr->m_PCMBufTail + 1) & MAXDONGLEPCMFRAMESMASK;
//					}
//					if (this_ptr->fWaitingOnAMBE == true){
//						this_ptr->m_AMBEBufTail = (this_ptr->m_AMBEBufTail + 1) & MAXDONGLEAMBEFRAMESMASK;
//					}
//
//					this_ptr->fWaitingOnWrite = false;
//					this_ptr->fWaitingOnPCM = false;
//					this_ptr->fWaitingOnAMBE = false;
//
//					this_ptr->send_index++;
//					log_info("%s send ambe index:%d\n", this_ptr->dongle_name.c_str(), this_ptr->send_index);
//				}
//
//				break;
//			default:
//				break;
//			}
//		}
//		/*else
//		{
//			log_warning("note,other fd: %d \n", req->aio_fildes);
//		}*/
//	}
//	else
//	{
//		log_warning("note,other singal:%d \n", info->si_signo);
//	}
//
//
//}
