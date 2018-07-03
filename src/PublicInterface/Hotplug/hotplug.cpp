#include "hotplug.h"

#define INVALIDSOCKHANDLE   INVALID_SOCKET  
#define ISSOCKHANDLE(x)    (x>0)    
#define BLOCKREADWRITE      MSG_WAITALL  
#define NONBLOCKREADWRITE   MSG_DONTWAIT  
#define SENDNOSIGNAL        MSG_NOSIGNAL  
#define ETRYAGAIN(x)        (x==EAGAIN||x==EWOULDBLOCK||x==EINTR)  
#define gxsprintf           snprintf  

CHotplug * CHotplug::pThis = NULL;

//��������ģʽ
int CHotplug::SocketBlock(HSocket hs, bool bblock)
{
	unsigned long mode;
	if (ISSOCKHANDLE(hs))
	{
#ifdef WIN32 
		mode = bblock ? 0 : 1;
		return ioctlsocket(hs, FIONBIO, &mode);
#else
		mode = fcntl(hs, F_GETFL, 0);                  //��ȡ�ļ���flagsֵ��  
		//���ó�����ģʽ      ������ģʽ  
		return bblock ? fcntl(hs, F_SETFL, mode&~O_NONBLOCK) : fcntl(hs, F_SETFL, mode | O_NONBLOCK);
#endif  
	}
	return -1;
}

// if timeout occurs, nbytes=-1, nresult=1  
// if socket error, nbyte=-1, nresult=-1  
// if the other side has disconnected in either block mode or nonblock mode, nbytes=0, nresult=-1  
void CHotplug::SocketRecv(HSocket hs, char *ptr, int nbytes, transresult_t &rt)
{
	rt.nbytes = 0;
	rt.nresult = 0;
	if (!ptr || nbytes<1) return;

	//rt.nbytes = recv(hs, ptr, nbytes, BLOCKREADWRITE); 
	rt.nbytes = recv(hs, ptr, nbytes, NONBLOCKREADWRITE);
	if (rt.nbytes>0)
	{
		return;
	}
	else if (rt.nbytes == 0)
	{
		rt.nresult = -1;
	}
	else
	{
		rt.nresult = GetLastSocketError();
		rt.nresult = ETRYAGAIN(rt.nresult) ? 1 : -1;
	}

}


static void replace_char(char *ptr, int ptr_len, const  char old_c, const  char new_c)
{
	for (int i =0; i < ptr_len; i++)
	{
		if (ptr[i] == old_c)
		{
			ptr[i] = new_c;
		}

	}
}

CHotplug::CHotplug()
:s_netlink_client(INVALID_SOCKET)
,hotplug_monitor_thread_p(nullptr)
,event_parse_thread_p(nullptr)
, hotplug_callback_func_ptr(nullptr)
{
	log_info("New: CHotplug \n");
	set_thread_exit_flag = false;
	pThis = this;
	event_queue_ptr = nullptr;
	event_queue_ptr = new DynRingQueue(30, 1200);
	memset(&my_netlink_addr, 0x00, sizeof(my_netlink_addr));

}


CHotplug::~CHotplug()
{
	SocketClose(s_netlink_client);
	if (event_queue_ptr != nullptr)
	{
		delete event_queue_ptr;
		event_queue_ptr = nullptr;
	}
	log_info("Destory: CHotplug \n");

}

void CHotplug::SocketClose(HSocket &handle)
{
	if (ISSOCKHANDLE(handle))
	{
#ifdef WIN32 
		closesocket(handle);
#else
		close(handle);
#endif  
		handle = INVALIDSOCKHANDLE;
	}
}

int CHotplug::init_netlink_socket(bool bForceClose)
{
	auto rt = -1;
	auto opt_val = 1;

	my_netlink_addr.nl_family = AF_NETLINK;
	my_netlink_addr.nl_pid = getpid();
	my_netlink_addr.nl_groups = 0xffffffff;

	if (ISSOCKHANDLE(s_netlink_client) && bForceClose) SocketClose(s_netlink_client);
	if (!ISSOCKHANDLE(s_netlink_client))
	{
		s_netlink_client = socket(PF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
	}
	else//socket�Ѵ���
	{
		log_info("s_netlink_client already exists\n");
		return 0;
	}

	//����recv��������С
	//rt = setsockopt(s_netlink_client, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
	//��ַ����
	rt = setsockopt(s_netlink_client, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt_val, sizeof(opt_val)) == 0 ? 0 : 0x1;
	if (rt != 0)
	{
		log_warning("The s_netlink_client SO_REUSEADDR fail!\n");
		SocketClose(s_netlink_client);
		return -1;
	}

	//��
	rt = bind(s_netlink_client, (struct sockaddr *)&my_netlink_addr, sizeof(my_netlink_addr));
	if (rt != 0)
	{
		log_warning("The s_netlink_client bind fail!\n");
		SocketClose(s_netlink_client);
		return -1;
	}

	//����Ϊ������ģʽ
	SocketBlock(s_netlink_client, 0);
	
	log_info("socket_fd: %d\n", s_netlink_client);


	return 0;
}

void CHotplug::CreateHotplugMonitorThread()
{
	hotplug_monitor_thread_p = new MyCreateThread(HotplugMonitorThread, this);
}

void CHotplug::CreateEventParseThread()
{
	event_parse_thread_p = new MyCreateThread(EventParseThread, this);
}


void *CHotplug::HotplugMonitorThread(void* p)
{
	auto *arg = (CHotplug*)p;
	auto return_value = 0;
	if (arg != NULL)
	{
		return_value = arg->HotplugMonitorThreadFunc();
	}
	return ((void*)0);


}
int CHotplug::HotplugMonitorThreadFunc()
{
	auto return_value = 0;
	fd_set readfds;
	//fd_set writefds;
	struct timespec timeout;
	timeout.tv_sec = SELECT_TIMEOUT;
	timeout.tv_nsec = 0;
	transresult_t rt;
	char msg[UEVENT_MSG_LEN + 2];
	memset(msg, 0x00, (UEVENT_MSG_LEN + 2));


	log_info("HotplugMonitorThreadFunc is running, pid:%ld\n", syscall(SYS_gettid));

	while (!set_thread_exit_flag)
	{

		FD_ZERO(&readfds);
		FD_SET(s_netlink_client, &readfds);
		return_value = pselect(s_netlink_client + 1, &readfds, NULL, NULL, &timeout, NULL);
		if (return_value < 0)
		{
			log_warning("pselect netlink fail:%s\n", strerror(errno));
			return_value = -1;
			break;
		}
		else if (return_value == 0)
		{
			continue;//timeout
		}
		else//okay data
		{

			SocketRecv(s_netlink_client, (char *)msg, UEVENT_MSG_LEN, rt);
			if (rt.nbytes > 0)
			{
				//log_info("rt.nbytes:%d\n", rt.nbytes);
				replace_char(msg, rt.nbytes, '\0', '\n');//�滻���Ը��ϵĽ�����
				msg[rt.nbytes] = '\0';
				msg[rt.nbytes + 1] = '\0';
				auto return_result = event_queue_ptr->PushToQueue(msg, (rt.nbytes+2));
				if (return_result != true)
				{
					log_warning("event_queue_ptr push: full!!!\n");
				}
				else
				{
					queue_sem.SemPost();
				}
				//parse_event(msg);
			}
			else if ((rt.nbytes == -1) && (rt.nresult == 1))
			{
				log_info("SocketRecv Timeout\n");
			}
			else if ((rt.nresult == -1))
			{
				log_warning("someone close socket\n");
				break;
			}
			else
			{
			}
			memset(msg, 0x00, (UEVENT_MSG_LEN + 2));
		}

	}

	log_info("exit HotplugMonitorThreadFunc: 0x%x \n", hotplug_monitor_thread_p->GetPthreadID());

	return return_value;



}
 

void *CHotplug::EventParseThread(void* p)
{
	auto *arg = (CHotplug*)p;
	auto return_value = 0;
	if (arg != NULL)
	{
		return_value = arg->EventParseThreadFunc();
	}
	return ((void*)0);
}
int CHotplug::EventParseThreadFunc()
{
	log_info("EventParseThreadFunc is running, pid:%ld\n", syscall(SYS_gettid));

	int return_value = 0;
	char readbuff[1500] = { 0 };
	auto read_nbytes = 0;
	while (!set_thread_exit_flag)
	{
		queue_sem.SemWait(0);//block for waiting
		if (set_thread_exit_flag)break;
		auto ret = event_queue_ptr->TakeFromQueue(readbuff, read_nbytes, true);
		if (0 == ret)
		{
			parse_event(readbuff);
			memset(readbuff, 0x00, sizeof(readbuff));
		}
		else
		{
			log_info("event_queue: empty!!!\r\n");
		}

	}

	log_info("exit EventParseThreadFunc: 0x%x \n", event_parse_thread_p->GetPthreadID());

	return return_value;

}

void CHotplug::parse_event(const char *msg)
{
	string temp_str = msg;//ת��Ϊstring����
	string tt = "";

	//log_info("recv:%s\n", msg);
	//log("%s\n", msg);
	//log_info("recv hotplug info:\n");
	auto  last = 0;
	auto  end_index = 0;

	hotplug_info.action = "";
	hotplug_info.path = "";
	hotplug_info.subsystem = "";
	hotplug_info.devname = "";
	hotplug_info.major = 0;
	hotplug_info.minor = 0;
	hotplug_info.id_vendor = "";
	hotplug_info.id_model = "";

#if 1
	auto  start_index = temp_str.find(libudev_delim, last);//"libudev"
	if (start_index != string::npos) //hint:  here "string::npos"means find failed  
	{
		start_index = temp_str.find(action_delim, last);//"ACTION="
		if (start_index != string::npos)
		{
			start_index = temp_str.find_first_of('=', start_index);//"="
			end_index = temp_str.find_first_of('\n', start_index);//"\n"
			tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
			if (!tt.empty())
			{
				hotplug_info.action = tt;//copy "action"
			}
			tt.clear();//clear tt

			start_index = end_index;//����ƫ��
			start_index = temp_str.find(devpath_delim, start_index);//"DEVPATH="
			if (start_index != string::npos)
			{
				start_index = temp_str.find_first_of('=', start_index);//"="
				end_index = temp_str.find_first_of('\n', start_index);//"\n"
				tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
				if (!tt.empty())
				{
					hotplug_info.path = tt;//copy "devpath"
				}
				tt.clear();//clear tt

				start_index = end_index;//����ƫ��
				start_index = temp_str.find(subsystem_delim, start_index);//"SUBSYSTEM="
				if (start_index != string::npos)
				{
					start_index = temp_str.find_first_of('=', start_index);//"="
					end_index = temp_str.find_first_of('\n', start_index);//"\n"
					tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
					if (!tt.empty())
					{
						hotplug_info.subsystem = tt;//copy "subsystem"
					}
					tt.clear();//clear tt

					start_index = end_index;//����ƫ��
					start_index = temp_str.find(devname_delim, start_index);//"DEVNAME="
					if (start_index != string::npos)
					{
						start_index = temp_str.find_first_of('=', start_index);//"="
						end_index = temp_str.find_first_of('\n', start_index);//"\n"
						tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
						if (!tt.empty())
						{
							hotplug_info.devname = tt;//copy "devname"
						}
						tt.clear();//clear tt

						start_index = end_index;//����ƫ��
						start_index = temp_str.find(major_delim, start_index);//"MAJOR="
						if (start_index != string::npos)
						{
							start_index = temp_str.find_first_of('=', start_index);//"="
							end_index = temp_str.find_first_of('\n', start_index);//"\n"
							tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
							if (!tt.empty())
							{
								sscanf(tt.c_str(), "%d", &(hotplug_info.major));//copy "major"
							}
							tt.clear();//clear tt

							start_index = end_index;//����ƫ��
							start_index = temp_str.find(minor_delim, start_index);//"MINOR="
							if (start_index != string::npos)
							{
								start_index = temp_str.find_first_of('=', start_index);//"="
								end_index = temp_str.find_first_of('\n', start_index);//"\n"
								tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
								if (!tt.empty())
								{
									sscanf(tt.c_str(), "%d", &(hotplug_info.minor));//copy "minor"
								}
								tt.clear();//clear tt

								start_index = end_index;//����ƫ��
								start_index = temp_str.find(id_vendor_delim, start_index);//"ID_VENDOR="
								if (start_index != string::npos)
								{
									start_index = temp_str.find_first_of('=', start_index);//"="
									end_index = temp_str.find_first_of('\n', start_index);//"\n"
									tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
									if (!tt.empty())
									{
										hotplug_info.id_vendor = tt;//copy "id_vendor"
									}
									tt.clear();//clear tt

									start_index = end_index;//����ƫ��
									start_index = temp_str.find(id_model_delim, start_index);//"ID_MODEL="
									if (start_index != string::npos)
									{
										start_index = temp_str.find_first_of('=', start_index);//"="
										end_index = temp_str.find_first_of('\n', start_index);//"\n"
										tt = temp_str.substr((start_index + 1), (end_index - start_index - 1));
										if (!tt.empty())
										{
											hotplug_info.id_model = tt;//copy "id_model"
										}
										tt.clear();//clear tt

										//���ûص�
										/*log_info("\r\n");
										log_info("recv hotplug info:\n");
										log_info("action:%s\n", hotplug_info.action.c_str());
										log_info("devpath:%s\n", hotplug_info.path.c_str());
										log_info("subsystem:%s\n", hotplug_info.subsystem.c_str());
										log_info("devname:%s\n", hotplug_info.devname.c_str());
										log_info("major:%d\n", hotplug_info.major);
										log_info("minor:%d\n", hotplug_info.minor);
										log_info("id_vendor:%s\n", hotplug_info.id_vendor.c_str());
										log_info("id_model:%s\n", hotplug_info.id_model.c_str());*/
										hotplug_callback_func_ptr(&hotplug_info);

									}//end find "ID_MODEL"
								}//end find "ID_VENDOR"
							}//end find "MINOR"
						}//end find "MAJOR"
					}//end find "DEVNAME"
				}//end find "SUBSYSTEM"
			}//end find "DEVPATH"
		}//end find "ACTION"
	}//end find "libudev"

#endif

}




void CHotplug::monitor_start(void)
{
	libudev_delim = "libudev";
	action_delim = "ACTION=";
	devpath_delim = "DEVPATH=";
	subsystem_delim = "SUBSYSTEM=";
	devname_delim = "DEVNAME=";
	major_delim = "MAJOR=";
	minor_delim = "MINOR=";
	id_vendor_delim = "ID_VENDOR=";
	id_model_delim = "ID_MODEL=";

	hotplug_info.action = "";
	hotplug_info.path = "";
	hotplug_info.subsystem = "";
	hotplug_info.devname = "";
	hotplug_info.major = 0;
	hotplug_info.minor = 0;
	hotplug_info.id_vendor = "";
	hotplug_info.id_model = "";


	init_netlink_socket(false);
	if (event_parse_thread_p == nullptr)
	{
		CreateEventParseThread();
	}
	if (hotplug_monitor_thread_p == nullptr)
	{
		CreateHotplugMonitorThread();
	}


}

void CHotplug::monitor_stop(void)
{
	log_info("SetThreadExit: HotplugMonitorThread\n");
	SetThreadExitFlag();
	if (hotplug_monitor_thread_p != nullptr)
	{
		delete hotplug_monitor_thread_p;
		hotplug_monitor_thread_p = nullptr;
	}

	if (event_parse_thread_p != nullptr)
	{
		queue_sem.SemPost();
		delete event_parse_thread_p;
		event_parse_thread_p = nullptr;
	}

	if ((event_queue_ptr->QueueIsEmpty()) != true)
	{
		event_queue_ptr->ClearQueue();
	}

	int ret = 0;
	do
	{
		ret = queue_sem.SemWait(20);
		if (ret = -1)break;

	} while (0 != ret);
		


}

void CHotplug::set_hotplug_callback_func(void(*func_ptr)(hotplug_info_t *))
{

	hotplug_callback_func_ptr = func_ptr;//��func_ptr��������ڵ�ַ��ֵ��hotplug_callback_func_ptr

}
