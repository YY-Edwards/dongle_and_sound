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

	//return��dev_fd
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
	*@brief   ���ô�������λ��ֹͣλ��Ч��λ
	*@param  fd     ����  int  �򿪵Ĵ����ļ����*
	*@param  databits ����  int ����λ   ȡֵ Ϊ 7 ����8*
	*@param  stopbits ����  int ֹͣλ   ȡֵΪ 1 ����2*
	*@param  parity  ����  int  Ч������ ȡֵΪN,E,O,,S
	*/
	int set_parity(int databits, int stopbits, int parity);

	int tread(void *buf, unsigned int nbytes, unsigned int timout);

};





#endif