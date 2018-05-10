#ifndef __LOG_H
#define __LOG_H

#include <syslog.h>
#include <stdio.h>

#define openmylog(app_name, arg_1, arg_2)  openlog(app_name, arg_1, arg_2)
#define closemylog() closelog()
#define log_debug(fmt, arg...) syslog(LOG_DEBUG, fmt, ##arg)
#define log_warning(fmt, arg...) syslog(LOG_WARNING,fmt, ##arg)
#define log(fmt, arg...) printf(fmt, ##arg)
#endif