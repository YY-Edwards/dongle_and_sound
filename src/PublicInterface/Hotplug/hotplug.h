#ifndef __HOTPLUG_H
#define __HOTPLUG_H

#include "config.h"
#include "syninterface.h"
#include "socketwrap.h"
#include "fifoqueue.h"
#include <string>
#include <linux/netlink.h>

#define UEVENT_MSG_LEN 1500

struct hotplug_info_t {
	string action;
	string path;
	string subsystem;
	string devname;
	int major;//ttyUSB:188;ttyACM:166
	int minor;
	string id_vendor;//ATMEL or other
	string id_model;//AVR32_UC3_CDC or other

};

class CHotplug
{
public:

	CHotplug();

	~CHotplug();

	void monitor_start(void);
	
	void monitor_stop(void);

//公共接口：设置回调
public:
	void set_hotplug_callback_func(void(*func_ptr)(hotplug_info_t *));
private:
	void(*hotplug_callback_func_ptr)(hotplug_info_t *);

private:

	static CHotplug *pThis;

	string libudev_delim;
	string action_delim;
	string devpath_delim;
	string subsystem_delim;
	string devname_delim;
	string major_delim;
	string minor_delim;
	string id_vendor_delim;
	string id_model_delim;

	struct hotplug_info_t hotplug_info;

	DynRingQueue *event_queue_ptr;

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
	Monitoring hotplug by netlink
	*/
	MySynSem queue_sem;//syn queue

	MyCreateThread * hotplug_monitor_thread_p;
	MyCreateThread * event_parse_thread_p;

	void CreateHotplugMonitorThread();
	void CreateEventParseThread();

	static void *EventParseThread(void* p);
	int EventParseThreadFunc();

	static void *HotplugMonitorThread(void* p);
	int HotplugMonitorThreadFunc();

	/*
	parse the event
	*/
	void parse_event(const char *msg);
	

	bool set_thread_exit_flag;
	/*
	设置线程退出标志
	*/
	void SetThreadExitFlag()   { set_thread_exit_flag = true; }

};




#endif