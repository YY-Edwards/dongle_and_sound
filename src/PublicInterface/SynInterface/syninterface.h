/*
* SynInterface.h
*
* Created: 2017/12/19
* Author: EDWARDS
*/
#ifndef _SYNINTERFACE_H
#define _SYNINTERFACE_H

#include "config.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h> 
#endif

class ILock;
class ISem;
class ICond;
class CriSection;
class Mutex;
class MySynSem;
class MySynCond;
class MyCreateThread;

//
typedef enum {
	SYN_FAILED		=-2,
	SYN_ABANDONED	=-1,
	SYN_OBJECT_o	=0,
	SYN_TIMEOUT		=1
} SynInterface_States;

//锁接口类  
class ILock
{
public:

	virtual  ~ILock(){}

	virtual void Lock() = 0;
	virtual void Unlock() = 0;
};

//信号量接口类 
class ISem
{
public:
	virtual ~ISem() {}

	virtual int SemWait(int waittime) = 0 ;
	virtual void SemPost() = 0;
};

//事件/条件信号量接口类 
class ICond
{
public:
	virtual ~ICond() {}

	virtual int CondWait(int waittime)=0;
	virtual void CondTrigger(bool isbroadcast)=0;
};




//临界区锁类:速度快，消耗资源小 
class CriSection : public ILock
{
public:
	CriSection();
	~CriSection();

	virtual void Lock() ;
	virtual void Unlock() ;

private:
	GOCRITICAL_SECTION m_critclSection;
};

//互斥锁类：可以实现不同进程中线程之间互斥
class Mutex : public ILock
{
public:
	Mutex(const char * lockUniName);
	~Mutex();

	virtual void Lock() ;
	virtual void Unlock() ;

#ifdef WIN32

#else

	int Trylock()
	{
		return pthread_mutex_trylock(&m_mutex);

	}

#endif

	//句柄获取貌似无效
	void  GetMutex(GOMUTEX_T &t_mutex){ t_mutex = m_mutex; }

private:
	GOMUTEX_T m_mutex;
};

//同步信号量
class MySynSem : public ISem
{
public:
	MySynSem();
	~MySynSem();

	virtual int SemWait(int waittime) ;
	virtual void SemPost() ;

private:
	GOSEM_T m_sem;

};

//条件信号量/事件
class MySynCond : public ICond
{
public:
	MySynCond ();
	~MySynCond ();

	virtual int CondWait(int waittime) ;
	virtual void CondTrigger(bool isbroadcast) ;
	void Clear_Trigger_Flag(){ trigger_flag = false; }

private:

	//Mutex *m_mutex;
	GOMUTEX_T m_mutex;
	GOCOND_T m_cond;
	bool trigger_flag;

};

//线程接口类
class MyCreateThread
{
public:

#ifdef WIN32
	/*
	创建线程
	*/
	MyCreateThread(void *func, void *ptr);

	HANDLE GetThreadHandle(){ return thread_handle; }

#else

	MyCreateThread(void *(*func)(void *), void *ptr);

#endif
	/*
	获取线程ID
	*/
	int GetPthreadID()
	{
		#ifdef WIN32
			return GetCurrentThreadId();
		#else
			return pthread_self();
		#endif
		
	}
	/*
	回收线程
	*/
	~MyCreateThread();



private:

#ifdef WIN32
	HANDLE thread_handle;
#else

	pthread_t thread_handle;

#endif



};




#endif