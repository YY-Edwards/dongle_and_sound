
/**
* Dongle encode/decode application
*
* @platform: linux-4.4.52
*
* @author: Edwards
*
* @revision time :20180504
*/

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <stdio.h>
#include "hotplug.h"
#include "startdongleandalsa.h"
#include "config.h"
//#include "log.h"


using namespace std;

int exit_flag = 0;
char *pBuffer = nullptr;
int nread = 0;
CStartDongleAndSound *m_startdongle = nullptr;

static void  signal_handler(int sig_num) {
	// Reinstantiate signal handler
	signal(sig_num, signal_handler);

#ifndef _WIN32
	// Do not do the trick with ignoring SIGCHLD, cause not all OSes (e.g. QNX)
	// reap zombies if SIGCHLD is ignored. On QNX, for example, waitpid()
	// fails if SIGCHLD is ignored, making system() non-functional.
	if (sig_num == SIGCHLD) {
		log_info("\n...Recovery of zombie threads ...\r\n");
		do {} while (waitpid(-1, &sig_num, WNOHANG) > 0);
	}
	else
#endif
	{
		exit_flag = sig_num;
		log_info("\n...User would like to kill Main-Process...\r\n");
	}
}


static void extract_hotplug_info_func(hotplug_info_t *hpug_ptr)
{
	hotplug_info_t *temp_ptr = hpug_ptr;
	string action_add = "add";
	string action_remove = "remove";
	string action_change = "change";
	string compare_subsystem = "tty";
	string compare_id_model = "AVR32_UC3_CDC";
	string compare_id_vendor = "ATMEL";



	//if ((temp_ptr->subsystem.compare(compare_subsystem) == 0))
	if ((temp_ptr->subsystem.compare(compare_subsystem) == 0)
		&& (temp_ptr->id_model.compare(compare_id_model) == 0)
		&& (temp_ptr->id_vendor.compare(compare_id_vendor) == 0)
		)
	{
		log_info("\r\n");
		log_info("find the dongle device\n");
		log_info("action:%s\n", temp_ptr->action.c_str());
		log_info("devpath:%s\n", temp_ptr->path.c_str());
		log_info("devname:%s\n", temp_ptr->devname.c_str());
		if (temp_ptr->action.compare(action_add) == 0)
		{
			if (m_startdongle != nullptr)
			{
				m_startdongle->start(temp_ptr->devname.c_str());
				//sleep(30);
				m_startdongle->read_voice_file(pBuffer, nread); 
			}
	
		}
		else if (temp_ptr->action.compare(action_remove) == 0)
		{
			if (m_startdongle!=nullptr)
				m_startdongle->stop(temp_ptr->devname.c_str());
		}
		else if (temp_ptr->action.compare(action_change) == 0)
		{

		}

	}
	else
	{
		log_warning("find no dongle!\n");
	}


}


int main(int argc, char** argv)
{

	//openmylog("dongle-app", LOG_PID | LOG_CONS, LOG_LOCAL7);
	CLogger::get_instance().set_file_name(LOG_INFO_FILE_PATH, LOG_WARNING_FILE_PATH);
	CLogger::get_instance().start();

	log_info("/*******************************/\n");
	log_info("dongle main-process start\n");

	// Setup signal handler: quit on Ctrl-C
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
#ifndef _WIN32
	signal(SIGCHLD, signal_handler);
#endif

	CHotplug netlink_server;
	//CStartDongleAndSound m_startdongle;
	//CStartDongleAndSound *m_startdongle = new CStartDongleAndSound;
	m_startdongle = new CStartDongleAndSound;
	//设置热插拔信息回调函数，并启动热插拔监测
	netlink_server.set_hotplug_callback_func(extract_hotplug_info_func);
	netlink_server.monitor_start();

	//const char *dev_path = "/dev/ttyACM0";
	//m_startdongle.start(dev_path);


	log_info("argv[1]:%s\n", argv[1]);
	auto file_fd = open(argv[1], O_RDONLY);
	if (file_fd < 0)
	{
		log_warning("can't open :%s\n", argv[1]);
		close(file_fd);
		netlink_server.monitor_stop();
		delete m_startdongle;
		m_startdongle = nullptr;
		sleep(1);
		CLogger::get_instance().stop();
		return -1;
	}
	auto file_length = lseek(file_fd, 0 , SEEK_END);
	lseek(file_fd, 0, SEEK_SET);
	pBuffer = new  char[file_length];
    nread = read(file_fd, pBuffer, file_length);
	//char *pBuffer = new  char[file_length];
	//auto nread = read(file_fd, pBuffer, file_length);
	if (nread == file_length)
	{
		log_info("get file_fd file success\r\n");
	}
	else
	{
		log_warning("get file_fd file no all\r\n");
	}
	//m_startdongle->read_voice_file(pBuffer, nread);
	//delete[] pBuffer;
	//m_startdongle.get_voice_cache_from_file(argv[1]);

	for (;;)
	{
		if (exit_flag != 0)
		{
			log_info("...closing threads...\r\n");
			close(file_fd);
			netlink_server.monitor_stop();
			delete[] pBuffer;
			pBuffer = nullptr;
			m_startdongle->stop();
			delete m_startdongle;
			m_startdongle = nullptr;
			break;
		}
		else
		{
			usleep(900 * 1000);
			//log_info("...main-process wating...\r\n");
		}
	}
	log_info("exit main-process...\r\n");
	sleep(1);
	CLogger::get_instance().stop();
	//closemylog();
	return 0;
}
