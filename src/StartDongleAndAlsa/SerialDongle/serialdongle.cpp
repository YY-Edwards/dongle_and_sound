

// SerialDongle.cpp: implementation of the CSerialDongle class.
//
//////////////////////////////////////////////////////////////////////

#include "serialdongle.h"

CSerialDongle *CSerialDongle::pThis = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSerialDongle::CSerialDongle()
:m_PleaseStopSerial(false)
, tx_serial_event_cond(nullptr)
, rx_serial_event_cond(nullptr)
, serial_tx_thread_p(nullptr)
, serial_rx_thread_p(nullptr)
{
	DongleRxDataCallBackFunc = nullptr;
	m_dwExpectedDongleRead = AMBE3000_PCM_BYTESINFRAME;
	//m_dwExpectedDongleRead = AMBE3000_AMBE_BYTESINFRAME
	the_pcm_sample_queue_ptr = new DynRingQueue(30, 320);
	memset(thePCMFrameFldSamples, 0, THEPCMFRAMEFLDSAMPLESLENGTH);
	dataType = 0;
	m_rComm = 0;
	m_wComm = 0;
	pcm_voice_fd = 0;
	pThis = this;
	log_info("New: CSerialDongle\n");
}

CSerialDongle::~CSerialDongle()
{
	close_dongle();
	log_info("Destory: CSerialDongle\n");
}

//int	CSerialDongle::open_dongle(const char *lpsz_Device, void(*func_ptr)(int signo, siginfo_t *info, void *context))
int	CSerialDongle::open_dongle(const char *lpsz_Device)
{
	m_ParserState = FIND_START;
	m_RxMsgLength = 0;
	m_RxMsgIndex = 0;
	m_AMBEBufHead = 0;
	m_AMBEBufTail = 0;
	m_PCMBufHead = 0;
	m_PCMBufTail = 0;
	m_bPleasePurgeAMBE = false;
	m_bPleasePurgePCM = false;

	fWaitingOnWrite = false;
	fWaitingOnPCM = false;
	fWaitingOnAMBE = false;
	dongle_name = lpsz_Device;

	log_info("new dongle name:%s \n", dongle_name.c_str());
	auto r_ret = m_usartwrap.init_usart_config(lpsz_Device, O_RDONLY, DONGLEBAUDRATE, DONGLEBITS, DONGLESTOP, DONGLEPARITY);
	auto w_ret = m_usartwrap.init_usart_config(lpsz_Device, O_WRONLY, DONGLEBAUDRATE, DONGLEBITS, DONGLESTOP, DONGLEPARITY);
	if ((r_ret<0) || (w_ret<0))
	{
		log_warning("open usart failed\n");
		return false;
	}
	else
	{
		log_info("open usart okay,r_fd:%d\n", r_ret);
		log_info("open usart okay,w_fd:%d\n", w_ret);

		m_rComm = r_ret;
		m_wComm = w_ret;

		////填充struct aiocb 结构体 
		bzero(&r_cbp, sizeof(r_cbp));
		bzero(&w_cbp, sizeof(w_cbp));

		////read-aio
		////指定缓冲区
		r_cbp.aio_buf = m_DongleRxBuffer;
		//r_cbp.aio_buf = (volatile void*)malloc(AIO_BUFSIZE + 1);
		//请求读取的字节数
		//r_cbp.aio_nbytes = AIO_BUFSIZE;
		//文件偏移
		r_cbp.aio_offset = 0;
		//读取的文件描述符
		r_cbp.aio_fildes = m_rComm;
		//发起读请求

		////////设置异步通知方式
		/////*用信号通知*/
		//struct sigaction sig_r_act;
		////设置信号处理函数
		//sigemptyset(&sig_r_act.sa_mask);
		//sig_r_act.sa_flags = SA_SIGINFO;
		//sig_r_act.sa_sigaction = aio_read_completion_hander;

		////连接AIO请求和信号处理函数
		//r_cbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		////设置产生的信号
		//r_cbp.aio_sigevent.sigev_signo = SIGIO;
		////传入aiocb 结构体
		//r_cbp.aio_sigevent.sigev_value.sival_ptr = &r_cbp;

		////将信号与信号处理函数绑定
		//sigaction(SIGIO, &sig_r_act, NULL);


		////write-aio
		////指定缓冲区
		//w_cbp.aio_buf = m_PCM_CirBuff[m_PCMBufTail].All[0];
		//w_cbp.aio_buf = (volatile void*)malloc(AIO_BUFSIZE + 1);
		//请求读取的字节数
		//w_cbp.aio_nbytes = AIO_BUFSIZE;
		//文件偏移
		w_cbp.aio_offset = 0;
		//读取的文件描述符
		w_cbp.aio_fildes = m_wComm;
		//发起读请求

#ifdef AIO_WRITE_SIGNAL 

		//////设置异步通知方式
		///*用信号通知*/
		struct sigaction sig_w_act;
		//设置信号处理函数
		sigemptyset(&sig_w_act.sa_mask);
		sig_w_act.sa_flags = SA_SIGINFO;
		sig_w_act.sa_sigaction = func_ptr;//共用一个信号处理函数
		//sig_w_act.sa_sigaction = aio_write_completion_hander;

		//连接AIO请求和信号处理函数
		w_cbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		//设置产生的信号
		w_cbp.aio_sigevent.sigev_signo = SIGIO;


		//传入CSerialDongle对象指针
		w_cbp.aio_sigevent.sigev_value.sival_ptr = pThis;

		//传入aiocb 结构体
		//w_cbp.aio_sigevent.sigev_value.sival_ptr = &w_cbp;

		
		//将信号与信号处理函数绑定
		sigaction(SIGIO, &sig_w_act, NULL);

#elif AIO_WRITE_CALLBACK


		////用线程回调
		w_cbp.aio_sigevent.sigev_notify = SIGEV_THREAD;
		//设置回调函数
		w_cbp.aio_sigevent.sigev_notify_function = aio_write_completion_hander;
		//传入aiocb 结构体
		w_cbp.aio_sigevent.sigev_value.sival_ptr = &w_cbp;
		//设置属性为默认
		w_cbp.aio_sigevent.sigev_notify_attributes = NULL;

#endif


	}




	tx_serial_event_cond = new MySynCond;
	rx_serial_event_cond = new MySynCond;
	//默认条件信号不触发
	//reset_rx_serial_event();
	//reset_tx_serial_event();

	auto ret = CreateSerialRxThread();
	ret = CreateSerialTxThread();


	//pcm_voice_fd = open("/opt/pcm.data", O_RDWR | O_APPEND | O_CREAT);
	//if (pcm_voice_fd < 0)
	//{
	//	log_warning("pcm_voice_fd open error\r\n");
	//	close(pcm_voice_fd);
	//	//return;
	//}
	//else
	//{
	//	/* 清空文件 */
	//	ftruncate(pcm_voice_fd, 0);

	//	/* 重新设置文件偏移量 */
	//	lseek(pcm_voice_fd, 0, SEEK_SET);
	//}

	return true;

}

void CSerialDongle::send_dongle_initialization(void)//send control packets
{

	/*
	bool result;

	m_DongleControlFrame.base.Sync     = AMBE3000_SYNC_BYTE;
	m_DongleControlFrame.base.LengthH  = AMBE3000_CCP_MODE_LENGTHH;
	m_DongleControlFrame.base.LengthL  = AMBE3000_CCP_MODE_LENGTHL;
	m_DongleControlFrame.base.Type     = AMBE3000_CCP_TYPE_BYTE;
	m_DongleControlFrame.base.empty[0] = AMBE3000_CCP_DCMODE;
	m_DongleControlFrame.base.empty[1] = AMBE3000_CCP_MODE_NOISEH;
	m_DongleControlFrame.base.empty[2] = AMBE3000_CCP_MODE_NOISEL;
	m_DongleControlFrame.base.empty[3] = AMBE3000_PARITYTYPE_BYTE;
	m_DongleControlFrame.base.empty[4] = CheckSum(&m_DongleControlFrame);
	result = SendDVSIMsg(&m_DongleControlFrame);
	*/


}


void CSerialDongle::close_dongle(void)
{
	log_info("close dongle resource...\n");
	SetThreadExitFlag();
	if (m_wComm != 0)
	{
		auto w_ret = aio_cancel(m_wComm, &w_cbp);
		if (w_ret < 0 )
			log_info("m_wComm aio_cancle errno:%s\n", strerror(errno));
		else
		{
			log_info("m_wComm aio_cancle return:%d\n", w_ret);
		}
	}
	
	if (m_rComm != 0)
	{
		auto r_ret = aio_cancel(m_rComm, &r_cbp);
		if (r_ret < 0)
			log_info("m_rComm aio_cancle errno:%s\n", strerror(errno));
		else
		{
			log_info("m_rComm aio_cancle return:%d\n", r_ret);
		}
	}

	if (serial_tx_thread_p != nullptr)
	{
		set_tx_serial_event();
		delete serial_tx_thread_p;
		serial_tx_thread_p = nullptr;
	}

	if (serial_rx_thread_p != nullptr)
	{
		set_rx_serial_event();
		delete serial_rx_thread_p;
		serial_rx_thread_p = nullptr;
	}

	if (tx_serial_event_cond != nullptr)
	{
		delete tx_serial_event_cond;
		tx_serial_event_cond = nullptr;
	}

	if (rx_serial_event_cond != nullptr)
	{
		delete rx_serial_event_cond;
		rx_serial_event_cond = nullptr;
	}
	DongleRxDataCallBackFunc = nullptr;
	m_usartwrap.close_usart_dev();
	m_rComm = 0;
	m_wComm = 0;
	if (pcm_voice_fd != 0)
	{
		close(pcm_voice_fd);
		pcm_voice_fd = 0;
	}

	if (the_pcm_sample_queue_ptr != nullptr)
	{
		delete the_pcm_sample_queue_ptr;
		the_pcm_sample_queue_ptr = nullptr;
	}


}

int CSerialDongle::aio_read_file(struct aiocb *r_cbp_ptr, int fd, int size)
{
	//填充struct aiocb 结构体 
	//bzero(&r_cbp, sizeof(r_cbp));
	bzero((void *)r_cbp.aio_buf, INTERNALCOMBUFFSIZE);

	//read-aio
	//请求读取的字节数
	r_cbp_ptr->aio_nbytes = size;

	//读取的文件描述符
	r_cbp_ptr->aio_fildes = fd;

	//read
	auto ret = aio_read(r_cbp_ptr);
	if (ret<0)
	{
		log_warning("aio_read failed:%s\n", strerror(errno));
		//break;

	}

	return ret;
	////设置异步通知方式
	////用线程回调
	//r_cbp.aio_sigevent.sigev_notify = SIGEV_THREAD;
	////设置回调函数
	//r_cbp.aio_sigevent.sigev_notify_function = aio_read_completion_hander;
	////传入aiocb 结构体
	//r_cbp.aio_sigevent.sigev_value.sival_ptr = &r_cbp;
	////设置属性为默认
	//r_cbp.aio_sigevent.sigev_notify_attributes = NULL;


}

int CSerialDongle::aio_write_file(struct aiocb *w_cbp_ptr, int fd, void *buff, int size)
{
	//write-aio
	//请求读取的字节数
	w_cbp_ptr->aio_buf = buff;

	w_cbp_ptr->aio_nbytes = size;

	//读取的文件描述符
	w_cbp_ptr->aio_fildes = fd;

	//read
	auto ret = aio_write(w_cbp_ptr);
	if (ret<0)
	{
		log_warning("aio_write failed:%s\n", strerror(errno));
		//break;

	}

	return ret;


}

void CSerialDongle::purge_dongle(int com, int flags)
{
	if (0 != com)m_usartwrap.flush_dev(com, flags);
}

int CSerialDongle::CreateSerialRxThread()
{

	serial_rx_thread_p = new MyCreateThread(SerialRxThread, this);
	if (serial_rx_thread_p != nullptr)return 0;
	else
	{
		return -1;
	}

}

int CSerialDongle::CreateSerialTxThread()
{

	serial_tx_thread_p = new MyCreateThread(SerialTxThread, this);
	if (serial_tx_thread_p != nullptr)return 0;
	else
	{
		return -1;
	}

}

void *CSerialDongle::SerialRxThread(void* p)//must be static since is thread
{

	auto obj = (CSerialDongle*)p;
	auto ret = 0;
	if (obj!=NULL)
		 ret = obj->SerialRxThreadFunc();

	return (void *)0;

}
int CSerialDongle::SerialRxThreadFunc()
{
	log_info("SerialRxThreadFunc is running, pid:%ld", syscall(SYS_gettid));
	int dwBytesConsumed;
	int  AssembledCount;
	auto read_nbytes = 0;
	auto ret = 0;
	auto dwImmediateExpectations = AMBE3000_PCM_BYTESINFRAME;
	auto dwCurrentTimeout = 10;//ms
	//auto dwImmediateExpectations = AMBE3000_AMBE_BYTESINFRAME;
	//异步阻塞列表
	struct aiocb*   aiocb_list[1];
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 5*1000*1000;
	recv_index = 0;

	do
	{
		//ret = rx_serial_event_cond->CondWait(0);
		//reset_rx_serial_event();
		//if (ret == 0)
		//{
			//read
			ret = aio_read_file(&r_cbp, m_rComm, dwImmediateExpectations);
			if (ret < 0)break;

		//}

		aiocb_list[0] = &r_cbp;
		//超时阻塞，直到请求完成才会继续执行后面的语句
		ret = aio_suspend((const struct aiocb* const*)aiocb_list, 1, NULL);
		//ret = aio_suspend((const struct aiocb* const*)aiocb_list, 1, &timeout);
		if (ret != 0)
		{
			if (errno == EAGAIN)//timeout
			{
				log_warning("aio_suspend() EAGAIN\n");
			}
			else
			{
				log_warning("aio_suspend() ret:%d, errno:%d, %s\n", ret, errno, strerror(errno));
				break;
			}

		}
		else
		{
			if ((ret = aio_error(&r_cbp)) != 0)//此处应该不会发生请求尚未完成的状态
			{
				log_info("qio_error() ret:%d", ret);
				if (ret == -1)
				{
					log_warning("qio_error() errno:%s\n", strerror(errno));
					break;
				}
			}
			else
			{
				//请求操作完成，获取返回值
				read_nbytes = aio_return(&r_cbp);
				log_info("%s recv pcm:%d bytes\n", dongle_name.c_str(), read_nbytes);
				if (read_nbytes > 0)
				{
					//assemble:注意是同步解析。如需要异步，则用环形队列作缓冲。
					AssembledCount = AssembleMsg(read_nbytes, &dwBytesConsumed);

				}
				else if (read_nbytes < 0)
				{
					log_info("aio_return() ret:%d, errno:%s\n", read_nbytes, strerror(errno));
					break;
				}

			}
		}

		if (!m_PleaseStopSerial)
		{
			if (dwImmediateExpectations != m_dwExpectedDongleRead)//根据实际情况切换
			{
				dwImmediateExpectations = m_dwExpectedDongleRead;
				purge_dongle(m_rComm, TCIFLUSH);//刷新收到的数据
				log_info("switch read voice type:\n");
			}
		}

		//read_nbytes = m_usartwrap.recv(&m_DongleRxBuffer, dwImmediateExpectations, dwCurrentTimeout);
		//if (read_nbytes >= 0)
		//{
		//	log_info("dongle recv pcm:%d bytes\n", read_nbytes);
		//	//assemble:注意是同步解析。如需要异步，则用环形队列作缓冲。
		//	AssembledCount = AssembleMsg(read_nbytes, &dwBytesConsumed);
		//
		//}
		//else
		//{
		//	break;
		//}

	} while (!m_PleaseStopSerial);

	log_info("exit SerialRxThreadFunc: 0x%x\r\n", serial_rx_thread_p->GetPthreadID());
	return ret;
}
void *CSerialDongle::SerialTxThread(void* p)//must be static since is thread
{

	auto obj = (CSerialDongle*)p;
	auto ret = 0;
	if (obj != NULL)
		ret = obj->SerialTxThreadFunc();

	return (void *)0;

}
int CSerialDongle::SerialTxThreadFunc()
{
	log_info("SerialTxThreadFunc is running, pid:%ld", syscall(SYS_gettid));
	auto ret = 0;
	int  snapPCMBufHead;
	int  snapAMBEBufHead;
	send_index = 0;
	struct aiocb*   aiocb_list[1];
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 50* 1000 * 1000;//ms
	aiocb_list[0] = &w_cbp;
	int nwrited = 0;

	do
	{
		ret = tx_serial_event_cond->CondWait(0);
		{

			//std::lock_guard<std::mutex> guard(m_aio_syn_mutex_);

			switch (ret)
			{
			case SERIAL_TIMEOUT:
				log_warning("tx-timeout:should no happen,but must check!");
				break;

			case SERIAL_TICKLE:
				//Something may be ready to send.
				reset_tx_serial_event();
				if (m_PleaseStopSerial){
					break;
				}
				if (true == m_bPleasePurgeAMBE){
					m_AMBEBufTail = m_AMBEBufHead;
					m_bPleasePurgeAMBE = false;
				}
				if (true == m_bPleasePurgePCM){
					m_PCMBufTail = m_PCMBufHead;
					m_bPleasePurgePCM = false;
				}

				if (fWaitingOnWrite == true){//Previous write did not complete.
					//Try some errorrecovery.
					log_warning("%s:Previous write did not complete!!!\n", dongle_name.c_str());
					purge_dongle(m_wComm, TCOFLUSH);//刷新写入的数据
					fWaitingOnWrite = false;
					fWaitingOnPCM = false;
					fWaitingOnAMBE = false;
					break;
				}

				snapAMBEBufHead = m_AMBEBufHead;		//Try AMBE
				if (snapAMBEBufHead != m_AMBEBufTail){  //AMBE to send
					fWaitingOnAMBE = true;
					fWaitingOnWrite = true;
					log_info("%s send ambe buff:\n", dongle_name.c_str());

					do
					{				
						auto aio_ret = aio_write_file(&w_cbp, m_wComm, &(m_AMBE_CirBuff[m_AMBEBufTail].All[0 + nwrited]), AMBE3000_AMBE_BYTESINFRAME - nwrited);
						//m_usartwrap.send(&(m_AMBE_CirBuff[m_AMBEBufTail].All[0]), AMBE3000_AMBE_BYTESINFRAME);
						if (aio_ret < 0)//Some dreadful error occurred.
						{
							purge_dongle(m_wComm, TCOFLUSH);//刷新写入的数据
							fWaitingOnWrite = false;
							fWaitingOnPCM = false;
							fWaitingOnAMBE = false;
							break;
						}
						//超时阻塞，直到请求完成才会继续执行后面的语句
						aio_ret = aio_suspend((const struct aiocb* const*)aiocb_list, 1, &timeout);
						if (aio_ret != 0)
						{
							if (errno == EAGAIN)//timeout
							{
								log_warning("[w_cbp],aio_suspend() EAGAIN:timeout\n");
								purge_dongle(m_wComm, TCOFLUSH);//刷新写入的数据
								fWaitingOnWrite = false;
								fWaitingOnPCM = false;
								fWaitingOnAMBE = false;
								break;
							}
							else
							{
								log_warning("[w_cbp],aio_suspend() ret:%d, errno:%d, %s\n", ret, errno, strerror(errno));
								break;
							}

						}
						else
						{
							aio_ret = aio_error(&w_cbp);

							log_info("\n\n");
							switch (ret)
							{
							case EINPROGRESS://working，no should happen,but must check.
								log_info("aio write is EINPROGRESS.\n");
								break;

							case ECANCELED://cancelled
								log_info("aio write is ECANCELLED.\n");
								break;

							case -1://failure
								log_info("aio write is failure, errno:%s\n", strerror(errno));
								purge_dongle(m_wComm, TCOFLUSH);//刷新写入的数据
								fWaitingOnWrite = false;
								fWaitingOnPCM = false;
								fWaitingOnAMBE = false;
								break;

							case 0://success

								aio_ret = aio_return(&w_cbp);
								log_info("fd:%d,[%s aio_write :%d bytes.]\n", w_cbp.aio_fildes, dongle_name.c_str(), aio_ret);
								nwrited += ret;
								if (nwrited == AMBE3000_AMBE_BYTESINFRAME || nwrited == AMBE3000_PCM_BYTESINFRAME)
								{
									log_info("aio_write complete[.]\n");
									nwrited = 0;//Successful Tx Complete.
									if (fWaitingOnPCM == true){
										m_PCMBufTail = (m_PCMBufTail + 1) & MAXDONGLEPCMFRAMESMASK;
									}
									if (fWaitingOnAMBE == true){
										m_AMBEBufTail = (m_AMBEBufTail + 1) & MAXDONGLEAMBEFRAMESMASK;
									}

									fWaitingOnWrite = false;
									fWaitingOnPCM = false;
									fWaitingOnAMBE = false;

									send_index++;
									log_info("%s send ambe index:%d\n", dongle_name.c_str(), send_index);
								}

								break;
							default:
								break;
							}//end of aio_error() switch

						}
					} while ((fWaitingOnWrite == true) && (!m_PleaseStopSerial));//end of send completed

				}
				break;

			default:
				break;
			}//End of Event Switch.
		}

	} while (!m_PleaseStopSerial);

	log_info("exit SerialTxThreadFunc: 0x%x\r\n", serial_tx_thread_p->GetPthreadID());
	return ret;
}



//void CSerialDongle::aio_read_completion_hander(int signo, siginfo_t *info, void *context)
//{
//	struct aiocb  *req;
//	int           ret;
//
//	if (info->si_signo == SIGIO)//确定是我们需要的信号
//	{
//
//		log_info("r-signal code:%d\n", info->si_code);
//		//获取aiocb 结构体的信息
//		//req = (struct aiocb*) sigval.sival_ptr;
//		req = (struct aiocb*) info->si_value.sival_ptr;
//
//		/*AIO请求完成？*/
//		if ((ret = aio_error(req)) == 0)
//		{
//			log_info("\r\n\r\n");
//			log_info("aio_read complete[.]\n");
//			ret = aio_return(req);
//			log_info("aio_read :%d bytes\n", ret);
//		
//		}
//		else
//		{
//			log_info("AIO read uncomplete:%d\n", ret);
//		}
//	}
//	else
//	{
//		log_warning("note,other singal:%d\n", info->si_signo);
//	}
//
//}


#if 0
#ifdef AIO_WRITE_SIGNAL 

void CSerialDongle::aio_write_completion_hander(int signo, siginfo_t *info, void *context)

#else//AIO_WRITE_CALLBACK

void CSerialDongle::aio_write_completion_hander(sigval_t sigval)//must be static since is thread

#endif
{

	struct aiocb  *req;
	int           ret;
	static int nwrited = 0;

#ifdef AIO_WRITE_SIGNAL 
	if (info->si_signo == SIGIO)//确定是我们需要的信号
#endif
	{
		//log_info("w-signal code:%d\n", info->si_code); 
		//获取aiocb 结构体的信息
#ifdef AIO_WRITE_SIGNAL 
		req = (struct aiocb*) info->si_value.sival_ptr;
#else
		req = (struct aiocb*) sigval.sival_ptr;
#endif
		if (req->aio_fildes == pThis->w_cbp.aio_fildes)//确定信号来自指定的文件描述符
		{
			/*AIO请求完成？*/
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
				log_info("fd:%d,[%s aio_write :%d bytes.]\n", req->aio_fildes, pThis->dongle_name.c_str(), ret);
				nwrited += ret;
				if (nwrited == AMBE3000_AMBE_BYTESINFRAME || nwrited == AMBE3000_PCM_BYTESINFRAME)
				{
					log_info("aio_write complete[.]\n");
					nwrited = 0;
					if (pThis->fWaitingOnPCM){
						pThis->m_PCMBufTail = (pThis->m_PCMBufTail + 1) & MAXDONGLEPCMFRAMESMASK;
					}
					if (pThis->fWaitingOnAMBE){
						pThis->m_AMBEBufTail = (pThis->m_AMBEBufTail + 1) & MAXDONGLEAMBEFRAMESMASK;
					}

					pThis->fWaitingOnWrite = false;
					pThis->fWaitingOnPCM = false;
					pThis->fWaitingOnAMBE = false;

					pThis->send_index++;
					log_info("dongle send ambe index:%d\n", pThis->send_index);
				}

				break;
			default:
				break;
			}
		}
		else
		{
			log_warning("note,other fd: %d \n", req->aio_fildes);
		}
	}

#ifdef AIO_WRITE_SIGNAL 
	else
	{
		log_warning("note,other singal:%d \n", info->si_signo);
	}
#endif


}

#endif

void CSerialDongle::set_rx_serial_event()
{
	if (rx_serial_event_cond != nullptr)
	{
		rx_serial_event_cond->CondTrigger(false);
		log_info("timer set read event[:]\n");
	}

}
void CSerialDongle::reset_rx_serial_event()
{
	if (rx_serial_event_cond != nullptr)
	{
		rx_serial_event_cond->Clear_Trigger_Flag();
	}

}

void CSerialDongle::set_tx_serial_event()
{

	if (tx_serial_event_cond != nullptr)
	{
		//log_info("timer set write event[:]\n");
		tx_serial_event_cond->CondTrigger(false);
		
	}

}
void CSerialDongle::reset_tx_serial_event()
{
	if (tx_serial_event_cond != nullptr)
	{
		tx_serial_event_cond->Clear_Trigger_Flag();
	}


}

void CSerialDongle::SetDongleRxDataCallBack(void(*Func)(void *ptr, short ptr_len))
{
	DongleRxDataCallBackFunc = Func;
}


//Called from Net Thread.
tAMBEFrame* CSerialDongle::GetFreeAMBEBuffer(void)
{
	tAMBEFrame* pAMBEFrame;

	//pre-supply header.
	pAMBEFrame = (tAMBEFrame*)(&(m_AMBE_CirBuff[m_AMBEBufHead]));
	pAMBEFrame->fld.Sync = AMBE3000_SYNC_BYTE;
	pAMBEFrame->fld.LengthH = AMBE3000_AMBE_LENGTH_HBYTE;
	pAMBEFrame->fld.LengthL = AMBE3000_AMBE_LENGTH_LBYTE;
	pAMBEFrame->fld.Type = AMBE3000_AMBE_TYPE_BYTE;
	pAMBEFrame->fld.ID = AMBE3000_AMBE_CHANDID_BYTE;
	pAMBEFrame->fld.Num = AMBE3000_AMBE_NUMBITS_BYTE;
	pAMBEFrame->fld.PT = AMBE3000_PARITY_TYPE_BYTE;

	return pAMBEFrame;
}

//Called from Net Thread.
bool CSerialDongle::MarkAMBEBufferFilled(void)
{
	int NextIndex;
	auto ret = false;
	DVSI3000struct* pAMBEBuffer;

	//Calculate checksum on filled/marked buffer.
	pAMBEBuffer = (DVSI3000struct*)(&(m_AMBE_CirBuff[m_AMBEBufHead]));
	pAMBEBuffer->AMBEType.theAMBEFrame.fld.PP = CheckSum(pAMBEBuffer);

	NextIndex = (m_AMBEBufHead + 1) & MAXDONGLEAMBEFRAMESMASK;
	if (NextIndex != m_AMBEBufTail){ //No collision.
		m_AMBEBufHead = NextIndex;
		ret = true;
	}
	else
	{
		ret = false;;
	}
	return ret;
}



//Called from various Threads
void CSerialDongle::deObfuscate(ScrambleDirection theDirection, tAMBEFrame* pAMBEFrame)
{
	char DebugBitsComingIn[49];
	char BitsGoingOut[49];
	int  i, j;
	int  InColumn, OutColumn;
	const int * DirectionArray;
	char theByte;

	//Code to break apart and shuffle the bits.
	switch (theDirection)
	{
	case IPSCTODONGLE:
		DirectionArray = &IPSCTODONGLETABLE[0];
		break;
	case DONGLETOIPSC:
		DirectionArray = &DONGLETOIPSCTABLE[0];
		break;
	default: //Shouldn't happen.
		return;
	}

	for (i=0; i<6; i++) {   //Will treat last bit as special case.
		InColumn = i*8 + 7; //7, 15, 23, 31, 39,47
		theByte = pAMBEFrame->fld.ChannelBits[i];
		for (j=0; j<8; j++){
			OutColumn = *(DirectionArray + InColumn);

			DebugBitsComingIn[InColumn] = theByte & 0x01;
			BitsGoingOut[OutColumn]     = theByte & 0x01;
			InColumn--;
			theByte = theByte>>1;
		}
	}
	InColumn = 48;  //Special case of last bit.
	OutColumn = *(DirectionArray + 48);
	theByte = ((pAMBEFrame->fld.ChannelBits[6])>>7) & 0x01;
	DebugBitsComingIn[48]   = theByte;
	BitsGoingOut[OutColumn] = theByte;


	//Code to re-assemble the shuffled bits.
	OutColumn = 0; //More efficient to increment than calculate(??)
	for (i=0; i<6; i++){  //Will treat last bit as special case.
		theByte = 0;
		for (j=0; j<8; j++){
			theByte = (theByte<<1) + BitsGoingOut[OutColumn++];
		}
	 pAMBEFrame->fld.ChannelBits[i] = theByte;
	}
	theByte = BitsGoingOut[48]<<7;  //Handle special case.
	pAMBEFrame->fld.ChannelBits[6] = theByte;

	//将数据写入ambe.bit
	if (theDirection == DONGLETOIPSC)
	{
		
		char buffer[10];
		memset(buffer, 0, 10);
		memcpy(buffer, pAMBEFrame->fld.ChannelBits, THEAMBEFRAMEFLDSAMPLESLENGTH);
		//WriteVoice(buffer, 7);
	}
}

//Called from External Threads.
uint8_t CSerialDongle::CheckSum(DVSI3000struct* pMsg)
{
	int i, length;
	uint8_t sum;
	length = (pMsg->base.LengthH)<<8;
	length += pMsg->base.LengthL;
	sum = 0;
	for (i=1; i<length+3; i++){
		sum ^= pMsg->All[i];
	}
	return sum;
}


////Called from Sound In Thread
//void CSerialDongle::SendDVSIPCMMsgtoDongle( unsigned __int8* pData )
//{
//	//int i;
//	//int snapPCMBufTail;
//
//	//snapPCMBufTail = (m_PCMBufTail - 1) & MAXDONGLEPCMFRAMESMASK;
//	//if (snapPCMBufTail != m_PCMBufHead){ //No collision.
//	//	//Put new message into circular buffer.
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.Sync = AMBE3000_SYNC_BYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.LengthH = AMBE3000_PCM_LENGTH_HBYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.LengthL = AMBE3000_PCM_LENGTH_LBYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.Type = AMBE3000_PCM_TYPE_BYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.ID = AMBE3000_PCM_SPEECHID_BYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.Num = AMBE3000_PCM_NUMSAMPLES_BYTE;
//	//	for (i=0; i< AMBE3000_PCM_INTSAMPLES_BYTE; i++){
//	//		m_PCM_CirBuff[m_PCMBufHead].fld.Samples[1+(i<<1)] = *pData++; //Endian conversion.
//	//		m_PCM_CirBuff[m_PCMBufHead].fld.Samples[  (i<<1)] = *pData++;
//	//	}
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.PT = AMBE3000_PARITYTYPE_BYTE;
//	//	m_PCM_CirBuff[m_PCMBufHead].fld.PP = CheckSum((DVSI3000struct *)&m_PCM_CirBuff[m_PCMBufHead]);
//	//	//!!!!!!!
//	//	m_PCMBufHead = (m_PCMBufHead + 1) & MAXDONGLEPCMFRAMESMASK;
//	//	//g_SoundCard.BigEndianSoundOut(&m_PCM_CirBuff[m_PCMBufHead].fld.Samples[0]);
//	//}
//
//}


//Called from Serial Thread.
int CSerialDongle::AssembleMsg(int numBytes, int * dwBytesAssembled)
{
	
	int Index;
	uint8_t ch;
	int WholeMessageCount = 0;
	int bytecount = 0;

	*dwBytesAssembled = 0;
	Index = 0;
	while (0 < numBytes--){
		bytecount++;
		ch = m_DongleRxBuffer[Index++];
		switch(m_ParserState)	//Simple state machine to get generic DVSI msg
		{
			case FIND_START:		//search for start byte
				if (AMBE3000_SYNC_BYTE == ch){
					m_RxDVSImsg.base.Sync = AMBE3000_SYNC_BYTE;
					m_ParserState = HIGH_LENGTH;	//get high byte of length
				}
				break;
			case HIGH_LENGTH:	//here for high byte of length
				m_RxDVSImsg.base.LengthH = ch;
				m_RxMsgLength = ch<<8;
				m_ParserState = LOW_LENGTH;	//get low byte of length
				break;
			case LOW_LENGTH:	//here for low byte of length
				m_RxDVSImsg.base.LengthL = ch;
				m_RxMsgLength += ch;

				if (0x144 < m_RxMsgLength){
					m_RxMsgLength = 0x144;
				}
				m_RxMsgLength += 1; //length remaining.
				m_RxMsgIndex = 3;
				m_ParserState = READ_DATA;	//get rest of data
				break;


			case READ_DATA:	//try to read the rest of the message or to end of buffer
				m_RxDVSImsg.All[m_RxMsgIndex++] = ch;
				if ((--m_RxMsgLength) == 0){
					m_RxMsgLength = ((m_RxDVSImsg.base.LengthH)<<8) + (m_RxDVSImsg.base.LengthL) + 4; //Total length.
					m_ParserState = FIND_START;	//go back to first stage
					ParseDVSImsg(&m_RxDVSImsg);	//call DVSI message parsing routine	
					*dwBytesAssembled += bytecount;
					bytecount = 0;
					WholeMessageCount++;
					log_info("Parse pcm-msg okay.\n");
					//recv_index++;
					//log_info("recv pcm-msg index:%d\n", recv_index);
				}
				break;
			default:
				m_ParserState = FIND_START;	//reset parser to first stage
				break;
		} //end switch statement
	}
	return WholeMessageCount;

	
}

//Called from Serial Thread.
void CSerialDongle::ParseDVSImsg(DVSI3000struct* pMsg)
{
	
	uint8_t mType;
	mType = pMsg->base.Type;	//strip out just type field
	dataType = mType;
	auto ret = false;
	switch (mType){
	case AMBE3000_AMBE_TYPE_BYTE:
		//is a compressed data frame from the Dongle
		deObfuscate(DONGLETOIPSC, (tAMBEFrame*)pMsg);
		//g_MyNet.NetStuffTxVoice((unsigned char*)&(pMsg->AMBEType.theAMBEFrame.fld.ChannelBits[0]));
		break;
	case AMBE3000_PCM_TYPE_BYTE:
		//is a codec uncompressed 8000sps 16 bit data packet from the Dongle
		//send dongle codec data to soundcard output speaker.
		//SHORT coming from Dongle is Big-endian. 
		//Do conversion in destination.
		//m_directSound.BigEndianSoundOut( (unsigned __int8*)&(pMsg->PCMType.thePCMFrame.fld.Samples[0]) );
		//memcpy(thePCMFrameFldSamples, (uint8_t*)&(pMsg->PCMType.thePCMFrame.fld.Samples[0]), THEPCMFRAMEFLDSAMPLESLENGTH);
		ret = the_pcm_sample_queue_ptr->PushToQueue((void*)&(pMsg->PCMType.thePCMFrame.fld.Samples[0]), THEPCMFRAMEFLDSAMPLESLENGTH);
		if (ret != true)
		{
			log_warning("the_pcm_sample_queue_ptr->PushToQueue full!!!\n");
		}

		break;
	case AMBE3000_CCP_TYPE_BYTE:
	
		break;
	}
	
}



void CSerialDongle::send_any_ambe_to_dongle(void)
{
	if (m_AMBEBufHead != m_AMBEBufTail){
		if (AMBE3000_PCM_BYTESINFRAME != m_dwExpectedDongleRead){
			m_dwExpectedDongleRead = AMBE3000_PCM_BYTESINFRAME;

			//SetEvent(m_hTickleRxSerialEvent);
		}
		//set_rx_serial_event();
		//SetEvent(m_hTickleTxSerialEvent);
		set_tx_serial_event();
	}
	/*else
	{
		log_warning("aio_write underrun...\n");
	}*/
}


void CSerialDongle::extract_voice(char * pBuffer, int len)
{
	auto leftLen = len;

	tAMBEFrame* pAMBEFrame;
	pAMBEFrame = GetFreeAMBEBuffer();
	auto readLen = (leftLen >= THEAMBEFRAMEFLDSAMPLESLENGTH) ? THEAMBEFRAMEFLDSAMPLESLENGTH : leftLen;
	memcpy(pAMBEFrame->fld.ChannelBits, pBuffer, readLen);
	leftLen -= readLen;
	auto index = readLen;
	while (readLen == THEAMBEFRAMEFLDSAMPLESLENGTH)
	{
		deObfuscate(IPSCTODONGLE, pAMBEFrame);
		if (MarkAMBEBufferFilled() != true)//将数据推送到环形队列中
		{
			log_warning("AMBE buff full!!!\n");
		}
		pAMBEFrame = GetFreeAMBEBuffer();
		if (NULL == pAMBEFrame)
		{

			return;
		}
		readLen = (leftLen >= THEAMBEFRAMEFLDSAMPLESLENGTH) ? THEAMBEFRAMEFLDSAMPLESLENGTH : leftLen;
		if (readLen <= 0)
		{
			break;
		}
		memcpy(pAMBEFrame->fld.ChannelBits, pBuffer + index, readLen);
		leftLen -= readLen;
		index += readLen;
	}
}

uint8_t * CSerialDongle::read_dongle_data()
{
	

	//if (AMBE3000_PCM_TYPE_BYTE == dataType)
	//{
	//	dataType = 0;
	//	return thePCMFrameFldSamples;
	//}
	int len = 0;
	auto ret = the_pcm_sample_queue_ptr->TakeFromQueue(thePCMFrameFldSamples, (int &)len, true);
	if (ret == 0)
	{
		//dataType = 0;
		return thePCMFrameFldSamples;
	}
	else
	{
		//log_warning("aio_read underrun!\n");
		return NULL;
	}

}

void CSerialDongle::get_read_dongle_data()
{
	uint8_t * pBuffer = NULL;

	pBuffer = read_dongle_data();
	if ((pBuffer != NULL) && (DongleRxDataCallBackFunc != nullptr))
	{
		if (AMBE3000_PCM_TYPE_BYTE == dataType)
		{
			DongleRxDataCallBackFunc(pBuffer, THEPCMFRAMEFLDSAMPLESLENGTH);//回调
			//dataType = 0;
			recv_index++;
			log_info("%s save pcm-msg index:%d\n", dongle_name.c_str(), recv_index);
		}
		//else//ambe
		//{
		//	DongleRxDataCallBackFunc(pBuffer, THEAMBEFRAMEFLDSAMPLESLENGTH);
		//}
		//dataType = 0;
	}

	//if ((pBuffer != NULL) && (pcm_voice_fd!=0))
	//{
	//	auto ret = write(pcm_voice_fd, pBuffer, THEPCMFRAMEFLDSAMPLESLENGTH);
	//	if (ret < 0)
	//	{
	//		log_warning("write pcm-voice err!!!\r\n");
	//	}
	//	else
	//	{
	//		if (ret=!THEPCMFRAMEFLDSAMPLESLENGTH)
	//		{
	//			log_warning("write pcm-voice uncompleted:%ld\r\n", ret);
	//		}
	//		log_info("save pcm data okay.\n");
	//	}
	//}
	set_read_dongle_data();
	

}

void CSerialDongle::set_read_dongle_data()
{
	memset(thePCMFrameFldSamples, 0, THEPCMFRAMEFLDSAMPLESLENGTH);

}