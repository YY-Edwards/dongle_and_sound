/*
* SynInterface.cpp
*
* Created: 2017/12/19
* Author: EDWARDS
*/
#include "syninterface.h"


//时间a的值增加ms毫秒 
void timeraddMS(struct timeval *a, unsigned int ms)
{
	a->tv_usec += ms * 1000;
	if (a->tv_usec >= 1000000)
	{
		a->tv_sec += a->tv_usec / 1000000;
		a->tv_usec %= 1000000;
	}
}


CriSection::CriSection()
{
	

#ifdef WIN32
	InitializeCriticalSection(&m_critclSection);

#else
	//m_critclSection = NULL;
	pthread_mutex_init(&m_critclSection, NULL);

#endif

}
CriSection::~CriSection()
{
	
#ifdef WIN32
	DeleteCriticalSection(&m_critclSection);

#else

	pthread_mutex_destroy(&m_critclSection);

#endif

}
void CriSection::Lock()
{
	
#ifdef WIN32
	EnterCriticalSection((LPCRITICAL_SECTION)&m_critclSection);

#else

	pthread_mutex_lock(&m_critclSection);

#endif

}
void CriSection::Unlock()
{

#ifdef WIN32
	LeaveCriticalSection((LPCRITICAL_SECTION)&m_critclSection);

#else

	pthread_mutex_unlock(&m_critclSection);

#endif

}



Mutex::Mutex(const char * lockUniName)
//:m_mutex(NULL)
{

#ifdef WIN32
	//lpName第三个参数用来设置互斥量的名称，在多个进程中的线程就是通过名称来确保它们访问的是同一个互斥量
	m_mutex = CreateMutex(nullptr, FALSE, (LPCTSTR)lockUniName);
#else
	pthread_mutex_init(&m_mutex, NULL);

#endif

}
Mutex::~Mutex()
{
#ifdef WIN32
	CloseHandle(m_mutex);

#else

	pthread_mutex_destroy(&m_mutex);

#endif
}
void Mutex::Lock()
{
#ifdef WIN32
	WaitForSingleObject(m_mutex, INFINITE);

#else

	pthread_mutex_lock(&m_mutex);

#endif

}
void Mutex::Unlock()
{
#ifdef WIN32
	ReleaseMutex(m_mutex);

#else

	pthread_mutex_unlock(&m_mutex);

#endif

}

MySynSem::MySynSem()
{
#ifdef WIN32
	m_sem = CreateSemaphore(NULL, 0, 100, NULL);
#else
	sem_init(&m_sem, 0, 0);

#endif

}
MySynSem::~MySynSem()
{
#ifdef WIN32
	CloseHandle(m_sem);
#else
	sem_destroy(&m_sem);

#endif	

}
int MySynSem::SemWait(int waittime)
{
	int ret = 0;

#ifdef WIN32
	ret = WaitForSingleObject(m_sem, waittime);//等待信号量触发,waittime:/ms
	if ((ret == WAIT_ABANDONED) || (ret == WAIT_FAILED))
	{
		ret = -1;
	}

#else
	struct timeval now;
	struct timespec outtime;

	gettimeofday(&now, NULL);
	timeraddMS(&now, waittime);//ms级别
	outtime.tv_sec = now.tv_sec;
	outtime.tv_nsec = now.tv_usec * 1000;
	while ((ret = sem_timedwait(&m_sem, &outtime) != 0) && errno == EINTR)//linux 下暂时未测试其效果
		continue;

	if (ret != 0)
	{
		if (errno == ETIMEDOUT)
		{
			ret = 1;
			//timeout
		}
		else
		{
			ret = -1;
			//failed
		}
		return ret;
	}
	else

#endif	

	return ret;
}
void MySynSem::SemPost()
{
#ifdef WIN32
	ReleaseSemaphore(m_sem, 1, NULL);//触发信号量
#else
	sem_post(&m_sem);

#endif	


}

MySynCond::MySynCond()
{
	//m_mutex = new Mutex("condlock");

#ifdef WIN32

	m_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	trigger_flag = false;
	pthread_cond_init(&m_cond, NULL);
	pthread_mutex_init(&m_mutex, NULL);

#endif


}
MySynCond::~MySynCond()
{

#ifdef WIN32
		
	CloseHandle(m_cond);
#else
	pthread_cond_destroy(&m_cond);

	pthread_mutex_destroy(&m_mutex);
#endif


}
int MySynCond::CondWait(int waittime)
{
	int ret = 0;

#ifdef WIN32
	int wait_time = 0;
	if (0 == waittime)
		 wait_time = INFINITE;

	ret = WaitForSingleObject(m_cond, wait_time);//等待信号量触发,waittime:/ms
	if ((ret == WAIT_ABANDONED) || (ret == WAIT_FAILED))
	{
		ret = -1;
	}

#else
	int wait_ret = 0;
	struct timeval now;
	struct timespec outtime;

	pthread_mutex_lock(&m_mutex);

	if(waittime!=0)
	{
		gettimeofday(&now, NULL);
		timeraddMS(&now, waittime);//ms级别
		outtime.tv_sec = now.tv_sec;
		outtime.tv_nsec = now.tv_usec * 1000;
	}

	while ((trigger_flag == false) && (wait_ret != ETIMEDOUT)){
		if(0 == waittime)
			wait_ret = pthread_cond_wait(&m_cond, &m_mutex);//会意外苏醒，注意考虑此情况的处理机制
		else
		{
			wait_ret = pthread_cond_timedwait(&m_cond, &m_mutex, &outtime);
			//printf("pthread_cond_timedwait wait_ret : %d\n", wait_ret);
		}
		//fprintf(stderr, "pthread_cond_timedwait ret : %d\n", ret);
	}
	if (wait_ret == ETIMEDOUT)
	{
		ret = 1;

	}
	else //flag or waitcond
	{
	/*	if (trigger_flag == true)
			printf("trigger_flag true\n");
		if (wait_ret == 0)
			printf("wait_ret : %d\n", wait_ret);*/

		ret =0;
	}
	pthread_mutex_unlock(&m_mutex);

#endif
	//printf("ret : %d\n", ret);
	return ret;

}
void MySynCond::CondTrigger(bool isbroadcast)
{

#ifdef WIN32

	SetEvent(m_cond);

#else

	pthread_mutex_lock(&m_mutex);

	trigger_flag = true;

	if (isbroadcast)
		pthread_cond_broadcast(&m_cond);
	else
	{
		pthread_cond_signal(&m_cond);
	}

	pthread_mutex_unlock(&m_mutex);
#endif



}

#ifdef WIN32
MyCreateThread::MyCreateThread(void *func, void *ptr)
:thread_handle(NULL)
{
	thread_handle = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void *))func, ptr, 0, NULL);
	if (thread_handle == NULL)
	{
		std::cout << "create thread failed" << std::endl;
		system("pause");
	}
}
#else

MyCreateThread::MyCreateThread(void *(*func)(void *), void *ptr)
{
	int err =0;
	err = pthread_create(&thread_handle, NULL, func, ptr);
	if (err != 0){
		//fprintf(stderr, "func create fail...\n");
		log_warning("thread create fail...\n");
	}
	//pthread_detach(thread_handle);//分离创建的线程，线程退出后资源自动回收
}

#endif

MyCreateThread::~MyCreateThread()
{

#ifdef WIN32
	WaitForSingleObject(thread_handle, INFINITE);

	CloseHandle(thread_handle);
#else
	pthread_join(thread_handle,NULL);//等待回收线程

#endif

	thread_handle = 0;


}