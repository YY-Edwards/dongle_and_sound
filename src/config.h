#ifndef _GLOBAL_CONFIG_H
#define _GLOBAL_CONFIG_H


#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h> 
#include <errno.h> 
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>  
#include <errno.h>
#include <sys/time.h>
#include <sys/syscall.h>  

#else
#include <winsock2.h>
#include <process.h>
#endif // WIN32
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdint.h>
#include <map>
using namespace std;
//#include "log.h"
#include "logger.h"





#define SELECT_TIMEOUT							5       //selectµÄtimeout seconds
#define	LOG_INFO_FILE_PATH					"/opt/log/dong_app_info.log"
#define	LOG_WARNING_FILE_PATH				"/opt/log/dong_app_warning.log"

#ifdef WIN32
typedef  CRITICAL_SECTION GOCRITICAL_SECTION;
typedef  HANDLE GOMUTEX_T;
typedef  HANDLE GOSEM_T;
typedef  HANDLE GOCOND_T;
typedef  HANDLE GOTHREAD_T;
typedef  SOCKET HSocket;
//#define GOTHREADCREATE(x, y, z, q, w, e) _beginthreadex((x), (y), (z), (q), (w), (e))


#else
//#define GOTHREADCREATE(x, y, z, q, w, e) pthread_create((x), (y), (z), (q))
typedef  int GOTHREAD_T;
typedef  pthread_mutex_t	GOCRITICAL_SECTION;
typedef  pthread_mutex_t	GOMUTEX_T;
typedef  sem_t				GOSEM_T;
typedef  pthread_cond_t		GOCOND_T;
typedef int HSocket;
#define SOCKET_ERROR  (-1)  
#define INVALID_SOCKET  0  

#endif


#endif // !_GLOBAL_CONFIG_H
