
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
#include "log.h"


using namespace std;

int exit_flag = 0;

static void  signal_handler(int sig_num) {
	// Reinstantiate signal handler
	signal(sig_num, signal_handler);

#ifndef _WIN32
	// Do not do the trick with ignoring SIGCHLD, cause not all OSes (e.g. QNX)
	// reap zombies if SIGCHLD is ignored. On QNX, for example, waitpid()
	// fails if SIGCHLD is ignored, making system() non-functional.
	if (sig_num == SIGCHLD) {
		log("\n...Recovery of zombie threads ...\r\n");
		do {} while (waitpid(-1, &sig_num, WNOHANG) > 0);
	}
	else
#endif
	{
		exit_flag = sig_num;
		log("\n...User would like to kill Main-Process...\r\n");
	}
}


int main(int argc, char** argv)
{
	// Setup signal handler: quit on Ctrl-C
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
#ifndef _WIN32
	signal(SIGCHLD, signal_handler);
#endif

	openmylog("dongle-app", LOG_PID | LOG_CONS, LOG_LOCAL7);

	log_debug("\r\n\r\n");
	log_debug("/*******************************/");
	log_debug("dongle main-process start\n");

	CHotplug netlink_server;
	CStartDongleAndSound *m_startdongle = new CStartDongleAndSound;

	//�����Ȳ�μ��
	netlink_server.monitor_start();

	const char *dev_path = "/dev/ttyACM2";

	m_startdongle->start(dev_path);


	log_debug("argv[1]:%s\n", argv[1]);
	auto file_fd = open(argv[1], O_RDONLY);
	if (file_fd < 0)
	{
		log_warning("can't open :%s\n", argv[1]);
		close(file_fd);
		netlink_server.monitor_stop();
		m_startdongle->stop();
		delete m_startdongle;
		return -1;
	}
	auto file_length = lseek(file_fd, 0 , SEEK_END);
	lseek(file_fd, 0, SEEK_SET);
	char *pBuffer = new  char[file_length];
	auto nread = read(file_fd, pBuffer, file_length);
	if (nread == file_length)
	{
		log_debug("get file_fd file success\r\n");
	}
	else
	{
		log_warning("get file_fd file no all\r\n");
	}
	m_startdongle->read_voice_file(pBuffer, nread);
	delete[] pBuffer;

	for (;;)
	{
		if (exit_flag != 0)
		{
			log_debug("...closing threads...\r\n");
			netlink_server.monitor_stop();
			m_startdongle->stop();
			delete m_startdongle;
			break;
		}
		else
		{
			usleep(900 * 1000);
			//log_debug("...main-process wating...\r\n");
		}
	}
	log_debug("exit main-process...\r\n");
	closemylog();
	return 0;
}
