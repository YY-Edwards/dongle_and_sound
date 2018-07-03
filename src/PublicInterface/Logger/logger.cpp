
/**
* 日志类实现文件, Logger.cpp
*
* @platform: linux-4.4.52
*
* @author: Edwards
*
* @revision time :20180628
**/
  
  #include "logger.h"
  #include <time.h>
  #include <stdio.h>
  #include <memory>
  #include <stdarg.h>

void CLogger::set_file_name(const char* info_filename, const char* warning_filename)
{

	info_filename_ = info_filename;
	warning_filename_ = warning_filename;
	info_fp_ = NULL;
	warning_fp_ = NULL;
}

bool CLogger::start()
{
	if (info_filename_.empty())//使用默认的文件名
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//拷贝数据并贴上时间戳
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

	if (warning_filename_.empty())//使用默认的文件名
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//拷贝数据并贴上时间戳
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

	
	info_fp_ = fopen(info_filename_.c_str(), "wt+");
	warning_fp_ = fopen(warning_filename_.c_str(), "wt+");
	if (warning_fp_ == NULL || info_fp_ == NULL)
	{
		return false;
	}
	else
	{
		auto ret = fseek(info_fp_, 0, SEEK_END);
		ret = fseek(warning_fp_, 0, SEEK_END);
		if (ret != 0)
			return false;

		/*
		bind是这样一种机制，它可以将参数绑定于可调用对象，产生一个新的可调用实体，
		这种机制在函数回调时颇为有用。C++98中，有两个函数bind1st和bind2nd，
		它们分别用来绑定functor的第一个和第二个参数，都只能绑定一个参数。
		C++98提供的这些特性已经由于C++11的到来而过时，由于各种限制，
		我们经常使用bind而非bind1st和bind2nd。在C++11标准库中，
		它们均在functional头文件中。而C++STL很大一部分由Boost库扩充，
		STL中的shared_ptr等智能指针，bind及function都是由Boost库引入。
		在写代码过程中，要养成使用bind，function，lambda和智能指针的习惯，
		它们非常强大简洁实用。

		@bind的返回值是可调用实体，可以直接赋给std::function对象； 
		@对于绑定的指针、引用类型的参数，使用者需要保证在可调用实体调用之前，这些参数是可用的；
		@类的this可以通过对象或者指针来绑定

		*/
		
		//创建线程
		exit_flag_ = false;
		s_pthread_.reset(new std::thread(std::bind(&CLogger::threadfunc, this)));
		return true;
	}

}

void CLogger::stop()
{

	exit_flag_ = true;
	cv_.notify_one();

	s_pthread_->join();
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
	//C语言中解决变参问题的一组宏,所在头文件：#include <stdarg.h>,用于获取不确定个数的参数 
	va_list vArgList;

	//对va_list变量进行初始化，将vArgList指针指向参数列表中的第一个参数
	va_start(vArgList, psz_fmt.c_str());

	//将vArgList(通常是字符串) 按format格式写入字符串string中
	vsnprintf(msg, 1024, psz_fmt.c_str(), vArgList);
	//回收vArgList指针
	va_end(vArgList);

	char content[1536] = { 0 };
	time_t temp_now = time(NULL);
	struct tm t;
	localtime_r(&temp_now, &(t));//拷贝数据并贴上时间戳
	sprintf(content,
		"[%04d-%02d-%02d %02d:%02d:%02d][%s][0x%04x][%s:%d %s]: %s\n",
		t.tm_year + 1900,
		t.tm_mon + 1,
		t.tm_mday,
		t.tm_hour,
		t.tm_min,
		t.tm_sec,
		psz_level,
		std::this_thread::get_id(),
		psz_file,
		line_no,
		psz_funcsig,
		msg);

	{
		std::lock_guard<std::mutex> guard(mutex_);
		queue_.emplace_back(psz_level, content);//C++11中效率比push_back更高
	
	}



	cv_.notify_one();

}

void CLogger::threadfunc()
{
	if (info_fp_ == NULL || warning_fp_ ==NULL)
		return;

	while (!exit_flag_)
	{
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
			fflush(warning_fp_);//提高写入效率
		}
		fwrite((void*)msg_str.c_str(), msg_str.length(), 1, info_fp_);
		fflush(info_fp_);//提高写入效率
		queue_.pop_front();

	}
}