#ifndef _USARTWRAP_H_  
#define _USARTWRAP_H_  

#include "config.h"  
#include <termios.h>

#define MC_FALSE 0
#define MC_TRUE 1

class CUsartWrap
{
public:

	CUsartWrap();
	~CUsartWrap();

	//return：dev_fd
	int init_usart_config(const char*dev, int baud_rate, int bits, int stop, int parity);

	int recv(char *buf, int nbytes, unsigned int timout);

	int send(char *buf, int size);

	void close_usart_dev(){
		if (dev_fd != 0)
		{
			close(dev_fd);
			dev_fd = 0;
		}
	}

	void flush_dev(int flags);


private:
	int  dev_fd;

	int open_dev(const char *dev);

	void set_speed(int speed);
	/*
	*@brief   设置串口数据位，停止位和效验位
	*@param  fd     类型  int  打开的串口文件句柄*
	*@param  databits 类型  int 数据位   取值 为 7 或者8*
	*@param  stopbits 类型  int 停止位   取值为 1 或者2*
	*@param  parity  类型  int  效验类型 取值为N,E,O,,S
	*/
	int set_parity(int databits, int stopbits, int parity);

	int tread(void *buf, unsigned int nbytes, unsigned int timout);

};





#endif