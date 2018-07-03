
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
		std::this_thread::get_id(),
		psz_file,
		line_no,
		psz_funcsig,
		msg);

	{
		std::lock_guard<std::mutex> guard(mutex_);
		queue_.emplace_back(psz_level, content);//C++11��Ч�ʱ�push_back����
	
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
			fflush(warning_fp_);//���д��Ч��
		}
		fwrite((void*)msg_str.c_str(), msg_str.length(), 1, info_fp_);
		fflush(info_fp_);//���д��Ч��
		queue_.pop_front();

	}
}