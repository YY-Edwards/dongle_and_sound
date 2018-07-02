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
					char* psz_fmt, ...);
	
private:
	CLogger() = default;//显示声明使用默认构造函数
	~CLogger() = default;//显示声明使用默认析构函数
	CLogger(const CLogger& rhs) = delete;//禁止使用类对象之间的拷贝
	CLogger& operator =(CLogger& rhs) = delete;//禁止使用类对象之间的赋值
	
	void threadfunc();


private:
	std::string  info_filename_;
	std::string  warning_filename_;
	std::string  err_filename_;

	FILE*		 info_fp_;
	FILE*		 warning_fp_;

	std::shared_ptr<std::thread>	s_pthread_;
	std::mutex						mutex_;
	std::condition_variable			cv_;

	bool		exit_flag_;
	//log_level,log_msg
	std::list<std::pair<std::string, std::string>>			queue_;

};



#endif //__LOGGER_H__