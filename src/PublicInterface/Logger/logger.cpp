
/**
* ��־��ʵ���ļ�, Logger.cpp
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
	if (filename_.empty())//ʹ��Ĭ�ϵ��ļ���
	{
		time_t temp_now = time(NULL);
		struct tm t;
		localtime_r(&temp_now, &(t));//�������ݲ�����ʱ���
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
	//C�����н����������һ���,����ͷ�ļ���#include <stdarg.h>,���ڻ�ȡ��ȷ�������Ĳ��� 
	va_list vArgList;

	//��va_list�������г�ʼ������vArgListָ��ָ������б��еĵ�һ������
	va_start(vArgList, pszFmt);

	//��vArgList(ͨ�����ַ���) ��format��ʽд���ַ���string��
	vsnprintf(msg, 256, pszFmt, vArgList);
	//����vArgListָ��
	va_end(vArgList);

	char content[1024] = { 0 };
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