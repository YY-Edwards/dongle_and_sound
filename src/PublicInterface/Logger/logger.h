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

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>


//#define LogInfo(...)
//	CLogger::get_instance().AddToQueue("INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
//#define LogWarning(...)
//	CLogger::get_instance().AddToQueue("WARNING", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
//#define LogError(...)
//	CLogger::get_instance().AddToQueue("ERROR", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

class CLogger
{

public:

	static CLogger& get_instance()
	{
		static CLogger theClogger;
		return theClogger;
	}

	void set_file_name(const char* filename);
	bool start();
	void stop();

	void add_to_queue(const char* psz_level,
					const char* psz_file,
					int line_no,
					const char* psz_funcsig,
					char* psz_fmt, ...);
	
private:
	CLogger() = default;//显示声明使用默认构造函数
	~CLogger() = default;//显示声明使用默认析构函数
	CLogger(const CLogger& rhs) = delete;//禁止使用类对象之间的拷贝
	CLogger& operator =(CLogger& rhs) = delete;//禁止使用类对象之间的赋值
	
	void threadfunc();


private:
	std::string  filename_;
	FILE*		 fp_;
	std::shared_ptr<std::thread>	s_pthread_;
	std::mutex						mutex_;
	std::condition_variable			cv_;

	bool		exit_flag_;
	std::list<std::string>			queue_;

};



#endif __LOGGER_H__