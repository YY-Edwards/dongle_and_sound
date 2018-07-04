
/**
* ��־��ʵ���ļ�, Logger.cpp
*
* @platform: linux-4.4.52
*
* @author: Edwards
*
* @revision time :20180628
**/
  
  #include "logger.h"
  #include <time.h>
  #include <stdarg.h>



#ifndef USE_STD_C_11

CLogger::CLogger()
{

#ifdef WIN32
	sem_ = CreateSemaphore(NULL, 0, 200, NULL);	
	mutex_ = CreateMutex(NULL, FALSE, (LPCTSTR)"mutex");


#else

	sem_init(&sem_, 0, 0);
	pthread_mutex_init(&mutex_, NULL);

#endif


}


CLogger::~CLogger()
{

#ifdef WIN32

	CloseHandle(sem_);
	CloseHandle(mutex_);
#else

	sem_destroy(&sem_);
	pthread_mutex_destroy(&mutex_);

#endif	

}

#endif


void CLogger::set_file_name(const char* info_filename, const char* warning_filename)
{

	info_filename_ = info_filename;
	warning_filename_ = warning_filename;
	info_fp_ = NULL;
	warning_fp_ = NULL;
}

bool CLogger::start()
{
	if (info_filename_.empty())//ʹ��Ĭ�ϵ��ļ���
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//�������ݲ�����ʱ���
		char timestr[64] = { 0 };
		sprintf(timestr,
			"%04d%02d%02d%02d%02d%02d.loggerserver_info.log",
			t.tm_year + 1900,
			t.tm_mon + 1,
			t.tm_mday,
			t.tm_hour,
			t.tm_min,
			t.tm_sec);
		info_filename_ = timestr;
	}

	if (warning_filename_.empty())//ʹ��Ĭ�ϵ��ļ���
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//�������ݲ�����ʱ���
		char timestr[64] = { 0 };
		sprintf(timestr,
			"%04d%02d%02d%02d%02d%02d.loggerserver_warning.log",
			t.tm_year + 1900,
			t.tm_mon + 1,
			t.tm_mday,
			t.tm_hour,
			t.tm_min,
			t.tm_sec);
		warning_filename_ = timestr;
	}

	
	info_fp_ = fopen(info_filename_.c_str(), "at+");
	warning_fp_ = fopen(warning_filename_.c_str(), "at+");
	if (warning_fp_ == NULL || info_fp_ == NULL)
	{
		return false;
	}
	else
	{
		int ret = fseek(info_fp_, 0, SEEK_END);
		ret = fseek(warning_fp_, 0, SEEK_END);
		if (ret != 0)
			return false;

		/*
		bind������һ�ֻ��ƣ������Խ��������ڿɵ��ö��󣬲���һ���µĿɵ���ʵ�壬
		���ֻ����ں����ص�ʱ��Ϊ���á�C++98�У�����������bind1st��bind2nd��
		���Ƿֱ�������functor�ĵ�һ���͵ڶ�����������ֻ�ܰ�һ��������
		C++98�ṩ����Щ�����Ѿ�����C++11�ĵ�������ʱ�����ڸ������ƣ�
		���Ǿ���ʹ��bind����bind1st��bind2nd����C++11��׼���У�
		���Ǿ���functionalͷ�ļ��С���C++STL�ܴ�һ������Boost�����䣬
		STL�е�shared_ptr������ָ�룬bind��function������Boost�����롣
		��д��������У�Ҫ����ʹ��bind��function��lambda������ָ���ϰ�ߣ�
		���Ƿǳ�ǿ����ʵ�á�

		@bind�ķ���ֵ�ǿɵ���ʵ�壬����ֱ�Ӹ���std::function���� 
		@���ڰ󶨵�ָ�롢�������͵Ĳ�����ʹ������Ҫ��֤�ڿɵ���ʵ�����֮ǰ����Щ�����ǿ��õģ�
		@���this����ͨ���������ָ������

		*/
		
		//�����߳�
		exit_flag_ = false;

#ifdef USE_STD_C_11

		s_pthread_.reset(new std::thread(std::bind(&CLogger::threadfunc, this)));

#else

#ifdef WIN32


		s_pthread_ = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void *))log_thread, this, 0, NULL);
		if (s_pthread_ == NULL)
		{
			std::cout << "create thread failed" << std::endl;
		}


#else


		int err = 0;
		err = pthread_create(&s_pthread_, NULL, log_thread, this);
		if (err != 0){
			fprintf(stderr, "threadfunc create fail...\n");
		}

#endif


#endif
		return true;
	}

}

void CLogger::stop()
{

	exit_flag_ = true;

#ifdef USE_STD_C_11

	cv_.notify_one();
	s_pthread_->join();

#else

#ifdef WIN32

		ReleaseSemaphore(sem_, 1, NULL);//�����ź���
		WaitForSingleObject(s_pthread_, INFINITE);
		CloseHandle(s_pthread_);
		//CloseHandle(mutex_);


#else

		sem_post(&sem_);
		pthread_join(s_pthread_, NULL);//�ȴ������߳�
		s_pthread_ = 0;
		//pthread_mutex_destroy(&mutex_);

#endif

	

#endif

	fclose(info_fp_);
	fclose(warning_fp_);

}

void CLogger::add_to_queue(const char* psz_level,
	const char* psz_file,
	int line_no,
	const char* psz_funcsig,
	std::string psz_fmt, ...)
	//char* psz_fmt, ...)
{

	char msg[1024] = { 0 };
	//C�����н����������һ���,����ͷ�ļ���#include <stdarg.h>,���ڻ�ȡ��ȷ�������Ĳ��� 
	va_list vArgList;

	//��va_list�������г�ʼ������vArgListָ��ָ������б��еĵ�һ������
	va_start(vArgList, psz_fmt.c_str());

	//��vArgList(ͨ�����ַ���) ��format��ʽд���ַ���string��
	vsnprintf(msg, 1024, psz_fmt.c_str(), vArgList);
	//����vArgListָ��
	va_end(vArgList);

	char content[1536] = { 0 };
	time_t temp_now = time(NULL);
	struct tm t;
	localtime_r(&temp_now, &(t));//�������ݲ�����ʱ���

	sprintf(content,
		"[%04d-%02d-%02d %02d:%02d:%02d][%s][0x%04x][%s:%d %s]: %s\n",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour,
		t.tm_min,
		t.tm_sec,
		psz_level,

#ifdef USE_STD_C_11
		std::this_thread::get_id(),
#else
#ifdef WIN32
		GetCurrentThreadId(),
#else
		pthread_self(),
#endif
#endif
		psz_file,
		line_no,
		psz_funcsig,
		msg);

#ifdef USE_STD_C_11

	{
		std::lock_guard<std::mutex> guard(mutex_);
		queue_.emplace_back(psz_level, content);//C++11��Ч�ʱ�push_back����
	
	}

	cv_.notify_one();

#else

#ifdef WIN32

	WaitForSingleObject(mutex_, INFINITE);

	queue_.emplace_back(psz_level, content);

	ReleaseMutex(mutex_);

	ReleaseSemaphore(sem_, 1, NULL);//�����ź���
	
#else//linux

	pthread_mutex_lock(&mutex_);

	queue_.emplace_back(psz_level, content);

	pthread_mutex_unlock(&mutex_);

	sem_post(&sem_);

#endif



#endif 

}

#ifdef USE_STD_C_11

#else
void *CLogger::log_thread(void * p)
{
	CLogger *arg = (CLogger*)p;
	if (arg != NULL)
	{
		 arg->threadfunc();
	}
	return (void*)0;
	
}
#endif

void CLogger::threadfunc()
{
	if (info_fp_ == NULL || warning_fp_ ==NULL)
		return;

	while (!exit_flag_)
	{

#ifdef USE_STD_C_11

		std::unique_lock<std::mutex> guard(mutex_);
		while (queue_.empty())
		{
			if (exit_flag_)
				return;
			cv_.wait(guard);

		}

		const std::string &msg_str = queue_.front().second;
		const std::string &level_str = queue_.front().first;
		if (level_str.compare("WARNING") == 0)
		{
			fwrite((void*)msg_str.c_str(), msg_str.length(), 1, warning_fp_);
			fflush(warning_fp_);//���д��Ч��
		}
		fwrite((void*)msg_str.c_str(), msg_str.length(), 1, info_fp_);
		fflush(info_fp_);//���д��Ч��
		queue_.pop_front();
#else
#ifdef WIN32

		int ret = 0;

		ret = WaitForSingleObject(sem_, INFINITE);//�ȴ��ź�������
		if ((ret == WAIT_ABANDONED) || (ret == WAIT_FAILED))
		{
			return;
		}

		WaitForSingleObject(mutex_, INFINITE);

		if (queue_.empty() || exit_flag_)
		{
			ReleaseMutex(mutex_);
			return;
		}

		const std::string &msg_str = queue_.front().second;
		const std::string &level_str = queue_.front().first;
		if (level_str.compare("WARNING") == 0)
		{
			fwrite((void*)msg_str.c_str(), msg_str.length(), 1, warning_fp_);
			fflush(warning_fp_);//���д��Ч��
		}
		fwrite((void*)msg_str.c_str(), msg_str.length(), 1, info_fp_);
		fflush(info_fp_);//���д��Ч��
		queue_.pop_front();

		ReleaseMutex(mutex_);

#else


		sem_wait(&sem_);

		pthread_mutex_lock(&mutex_);

		if (queue_.empty() || exit_flag_)//Ϊ�գ����쳣�����߳��˳���־ʹ��
		{	
			pthread_mutex_unlock(&mutex_);
			return;
		}

		const std::string &msg_str = queue_.front().second;
		const std::string &level_str = queue_.front().first;
		if (level_str.compare("WARNING") == 0)
		{
			fwrite((void*)msg_str.c_str(), msg_str.length(), 1, warning_fp_);
			fflush(warning_fp_);//���д��Ч��
		}
		fwrite((void*)msg_str.c_str(), msg_str.length(), 1, info_fp_);
		fflush(info_fp_);//���д��Ч��
		queue_.pop_front();

		pthread_mutex_unlock(&mutex_);

#endif

#endif
	

	}
}