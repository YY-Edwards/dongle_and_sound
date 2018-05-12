#include "usartwrap.h"

int speed_arr_temp[] = {
	B921600, B460800, B230400, B115200, B57600, B38400, B19200,
	B9600, B4800, B2400, B1200, B300,
};

int name_arr_temp[] = {
	921600, 460800, 230400, 115200, 57600, 38400, 19200,
	9600, 4800, 2400, 1200, 300,
};


CUsartWrap::CUsartWrap()
:w_fd(0)
, r_fd(0)
{
	log_debug("New: CUsartWrap \n");
}

CUsartWrap::~CUsartWrap()
{
	if (w_fd != 0){
		close(w_fd);
	}
	if (r_fd != 0){
		close(r_fd);
	}
	log_debug("Destory: CUsartWrap \n");
}
void CUsartWrap::flush_dev(int dev_fd, int flags)
{
		
	tcflush(dev_fd, flags);// 读写前先清空缓冲区数据，以免串口读取了数据，但是用户没有读取。对同步读写时需要注意每次读写前后是否清空。
	usleep(20000);
}

int CUsartWrap::init_usart_config(const char*dev, int flag, int baud_rate, int bits, int stop, int parity)
{

	if (flag == O_RDONLY)
	{
		r_fd = open_dev(dev, flag);

		if (r_fd > 0) {
			set_speed(r_fd, baud_rate);
		}
		else {
			log_warning("Error opening %s: %s\n", dev, strerror(errno));
			return -1;
		}

		if (set_parity(r_fd, bits, stop, parity) == MC_FALSE) {
			log_warning("Set Parity Error\n");
			close(r_fd);
			r_fd = 0;
			return -1;
		}

		return r_fd;
	}
	else
	{
		w_fd = open_dev(dev, flag);

		if (w_fd > 0) {
			set_speed(w_fd, baud_rate);
		}
		else {
			log_warning("Error opening %s: %s\n", dev, strerror(errno));
			return -1;
		}

		if (set_parity(w_fd, bits, stop, parity) == MC_FALSE) {
			log_warning("Set Parity Error\n");
			close(w_fd);
			w_fd = 0;
			return -1;
		}

		return w_fd;

	}

}



int CUsartWrap::tread(int dev_fd, void *buf, unsigned int nbytes, unsigned int timout)//ms
{

	if (dev_fd == 0)return -1;
	fd_set readfds;
	struct timespec  tv;
	tv.tv_sec = 0;
	tv.tv_nsec = timout* 1000000;

	FD_ZERO(&readfds);
	FD_SET(dev_fd, &readfds);
	auto nfds = pselect(dev_fd + 1, &readfds, NULL, NULL, &tv, NULL);
	if (nfds < 0)
	{
		log_warning("pselect dev_fd fail!\n");
		nfds = -1;
		return nfds;
	}
	else if (nfds == 0)
	{
		log_warning("pselect dev_fd timeout!\n");
		errno = ETIME;
		return -2;
		//continue;//timeout
	}
	else//okay data
	{
		return(read(dev_fd, buf, nbytes));
	}

}


int CUsartWrap::send(int dev_fd, char *buf, int size)
{
	return (write(dev_fd, buf, size));
	//tcdrain(dev_fd);

}
int CUsartWrap::recv(int dev_fd, char *buf, int nbytes, unsigned int timout)
{
	//int  nleft;
	int  nread;
	auto nleft = nbytes;

	while (nleft > 0) 
	{
		if ((nread = tread(dev_fd, buf, nleft, timout)) < 0) 
		{
			if (nleft == nbytes)
			{
				if (nread != -2)
					log_warning("tread err!!!\n");

				return nread;
				// return(-1); /* error, return -1 */
			}
			else
			{
				log_warning("tread err:incomplete recving!\n");
				break;      /* error, return amount read so far */
			}
				
		}
		else if (nread == 0) 
		{
			log_warning("should no happen,but must check!\n");
			break;          /* EOF */
		}
		nleft -= nread;
		buf += nread;
	}
	return(nbytes - nleft);      /* return >= 0 */

}
int CUsartWrap::open_dev(const char *dev, int flag)
{
	auto device_file_path = (char *)dev;
	auto fd = open(device_file_path, flag);         //| O_NOCTTY | O_NDELAY
	if (-1 == fd) { /*\C9\E8\D6\C3\CA\dev_fd\BE\DDλ\CA\dev_fd*/
		log_warning("Can't Open Serial Port");
		return -1;
	}
	else
		return fd;


}

void CUsartWrap::set_speed(int dev_fd, int speed)
{
	int   i;
	int   status;
	struct termios   Opt;
	if (dev_fd == 0)return;
	tcgetattr(dev_fd, &Opt);

	for (i = 0; i < sizeof(speed_arr_temp) / sizeof(int); i++) {
		if (speed == name_arr_temp[i])	{
			tcflush(dev_fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr_temp[i]);
			cfsetospeed(&Opt, speed_arr_temp[i]);
			status = tcsetattr(dev_fd, TCSANOW, &Opt);
			if (status != 0)
				log_warning("tcsetattr fd1");
			return;
		}
		tcflush(dev_fd, TCIOFLUSH);
	}

	if (i == 12){
		log_warning("\tSorry, please set the correct baud rate!\n\n");
	}
}

int CUsartWrap::set_parity(int dev_fd, int databits, int stopbits, int parity)
{
	if (dev_fd == 0)return MC_FALSE;
	struct termios options;
	if (tcgetattr(dev_fd, &options) != 0) {
		log_warning("SetupSerial 1");
		return(MC_FALSE);
	}
	options.c_cflag &= ~CSIZE;
	switch (databits) /*\C9\E8\D6\C3\CA\dev_fd\BE\DDλ\CA\dev_fd*/ {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		log_warning("Unsupported data size\n");
		return (MC_FALSE);
	}

	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* \C9\E8\D6\C3Ϊ\C6\E6Ч\D1\E9*/
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* ת\BB\BBΪżЧ\D1\E9*/
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		log_warning("Unsupported parity\n");
		return (MC_FALSE);
	}
	/* \C9\E8\D6\C3ֹͣλ*/
	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		log_warning("Unsupported stop bits\n");
		return (MC_FALSE);
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	options.c_cc[VTIME] = 50; // 5 seconds
	options.c_cc[VMIN] = 0;//读到数据则返回,否则每个字符最多等待5s

	//options.c_lflag &= ~(ECHO | ICANON);
	options.c_lflag &= ~(ECHO | ICANON | ECHOE | ISIG);

	options.c_oflag &= ~OPOST;

	options.c_cflag |= CLOCAL | CREAD;

	options.c_iflag &= ~(BRKINT | ISTRIP | ICRNL | IXON);//\BD\E2\B6\FE\BD\F80d\A1\A20x11\A1\A20x13\B5\BF\BB\B6\AA\BF\CE\CC	

	tcflush(dev_fd, TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(dev_fd, TCSANOW, &options) != 0) {
		log_warning("SetupSerial 3");
		return (MC_FALSE);
	}
	return (MC_TRUE);


}