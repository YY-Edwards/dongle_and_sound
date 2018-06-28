
/**
* 日志类实现文件, Logger.cpp
*
* @platform: linux-4.4.52
*
* @author: Edwards
*
* @revision time :20180628
**/
  
  #include "Logger.h"
  #include <time.h>
  #include <stdio.h>
  #include <memory>
  #include <stdarg.h>

void CLogger::set_file_name(const char* filename)
{

	filename_ = filename;

}

bool CLogger::start()
{
	if (filename_.empty())//使用默认的文件名
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//拷贝数据并贴上时间戳
		char timestr[64] = { 0 };
		sprintf(timestr,
			"%04d%02d%02d%02d%02d%02d.loggerserver.log",
			t.tm_year + 1900,
			t.tm_mon + 1,
			t.tm_mday,
			t.tm_hour,
			t.tm_min,
			t.tm_sec);
		filename_ = timestr;
	}
	
	fp_ = fopen(filename_.c_str(), "wt+");
	if (fp_ == NULL)
	{
		return false;
	}
	else
	{
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
		s_pthread_.reset(new std::thread(std::bind(&CLogger::threadfunc, this)));
		return true;
	}

}

void CLogger::stop()
{

	exit_flag_ = true;
	cv_.notify_one();

	s_pthread_->join();

}

void CLogger::add_to_queue(const char* psz_level,
	const char* psz_file,
	int line_no,
	const char* psz_funcsig,
	char* psz_fmt, ...)
{

	char msg[256] = { 0 };
	//C语言中解决变参问题的一组宏,所在头文件：#include <stdarg.h>,用于获取不确定个数的参数 
	va_list vArgList;

	//对va_list变量进行初始化，将vArgList指针指向参数列表中的第一个参数
	va_start(vArgList, pszFmt);

	//将vArgList(通常是字符串) 按format格式写入字符串string中
	vsnprintf(msg, 256, pszFmt, vArgList);
	//回收vArgList指针
	va_end(vArgList);

	char content[1024] = { 0 };
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
		queue_.emplace_back(content);
	
	}



	cv_.notify_one();

}