/**
* Logger header,logger.h
*
* @platform: linux-4.4.52
*
* @author: Edwards
*
* @revision time :20180627
*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

//#define USE_STD_C_11 //是否启用C++11标准

#include <string>
#include <list>

#ifdef USE_STD_C_11

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#else

#ifdef WIN32

#include <winsock2.h>
#include <process.h>
#include <iostream>

#else
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>  
#include <stdio.h>
#include <sys/syscall.h>  
#endif


#endif


#define log_info(...)	CLogger::get_instance().add_to_queue("INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define log_warning(...) CLogger::get_instance().add_to_queue("WARNING", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#define log_error(...)	CLogger::get_instance().add_to_queue("ERROR", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

class CLogger
{

public:

	static CLogger& get_instance()
	{
		static CLogger theClogger;
		return theClogger;
	}

	void set_file_name(const char* info_filename, const char* warning_filename);
	bool start();
	void stop();

	void add_to_queue(const char* psz_level,
					const char* psz_file,
					int line_no,
					const char* psz_funcsig,
					std::string psz_fmt, ...);
					//char* psz_fmt, ...);
	
private:

#ifdef USE_STD_C_11

	CLogger() = default;//显示声明使用默认构造函数
	~CLogger() = default;//显示声明使用默认析构函数
	CLogger(const CLogger& rhs) = delete;//禁止使用类对象之间的拷贝
	CLogger& operator =(CLogger& rhs) = delete;//禁止使用类对象之间的赋值

#else

	CLogger();
	~CLogger();

#endif


#ifdef USE_STD_C_11

#else

	static void *log_thread(void *);

#endif
	
	void threadfunc();


private:
	std::string  info_filename_;
	std::string  warning_filename_;
	std::string  err_filename_;

	FILE*		 info_fp_;
	FILE*		 warning_fp_;

#ifdef USE_STD_C_11

	std::shared_ptr<std::thread>	s_pthread_;
	std::mutex						mutex_;
	std::condition_variable			cv_;

#else

#ifdef WIN32

	HANDLE s_pthread_;
	HANDLE mutex_;
	HANDLE sem_;
#else//linux

	pthread_t		s_pthread_;
	pthread_mutex_t mutex_;
	sem_t			sem_;

#endif



#endif

	bool		exit_flag_;
	//log_level,log_msg
	std::list<std::pair<std::string, std::string>>			queue_;

};



#endif //__LOGGER_H__