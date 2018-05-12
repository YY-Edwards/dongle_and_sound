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
	int init_usart_config(const char*dev, int flag, int baud_rate, int bits, int stop, int parity);

	int recv(int fd,char *buf, int nbytes, unsigned int timout);

	int send(int fd, char *buf, int size);

	void close_usart_dev(){
		if (w_fd != 0)
		{
			close(w_fd);
			w_fd = 0;
		}
		if (r_fd != 0)
		{
			close(r_fd);
			r_fd = 0;
		}
	}

	void flush_dev(int fd, int flags);


private:
	int  w_fd;
	int  r_fd;

	int open_dev(const char *dev, int flag);

	void set_speed(int fd, int speed);
	/*
	*@brief   ���ô�������λ��ֹͣλ��Ч��λ
	*@param  fd     ����  int  �򿪵Ĵ����ļ����*
	*@param  databits ����  int ����λ   ȡֵ Ϊ 7 ����8*
	*@param  stopbits ����  int ֹͣλ   ȡֵΪ 1 ����2*
	*@param  parity  ����  int  Ч������ ȡֵΪN,E,O,,S
	*/
	int set_parity(int fd, int databits, int stopbits, int parity);

	int tread(int fd, void *buf, unsigned int nbytes, unsigned int timout);

};





#endif