#ifndef __HOTPLUG_H
#define __HOTPLUG_H

#include "config.h"
#include "syninterface.h"
#include "socketwrap.h"
#include <string>
#include <linux/netlink.h>

#define UEVENT_MSG_LEN 1500

struct luther_gliethttp {
	const char *action;
	const char *path;
	const char *subsystem;
	const char *firmware;
	int major;
	int minor;
};

class CHotplug
{
public:

	CHotplug();

	~CHotplug();

	void monitor_start(void);
	
	void monitor_stop(void);

private:

	static CHotplug *pThis;

	/*
	声明socket接口类
	*/
	HSocket s_netlink_client;
	/*
	netlink通讯的套接字地址
	*/
	struct sockaddr_nl my_netlink_addr; 


	/*
	init netlink socket
	*/
	int init_netlink_socket(bool bForceClose);

	void SocketClose(HSocket &handle);
	int SocketBlock(HSocket hs, bool bblock);
	void SocketRecv(HSocket hs, char *ptr, int nbytes, transresult_t &rt);
	/*
	Monitoring hotplu by netlink
	*/
	MyCreateThread * hotplug_monitor_thread_p;
	void CreateHotplugMonitorThread();
	static void *HotplugMonitorThread(void* p);
	int HotplugMonitorThreadFunc();

	/*
	parse the event
	*/
	void parse_event(const char *msg, struct luther_gliethttp *luther_gliethttp);
	

	bool set_thread_exit_flag;
	/*
	设置线程退出标志
	*/
	void SetThreadExitFlag()   { set_thread_exit_flag = true; }

};




#endif