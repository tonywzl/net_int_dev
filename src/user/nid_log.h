#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <syslog.h>
#include <stdlib.h>

#include "nid.h"
/*
 * Sample:
 *	nid_log_open();
 *	nid_log_debug("this is a format test %d", 10);
 *	nid_log_info("this is a format test %d", 11);
 *	nid_log_notice("this is a format test %d", 12);
 *	nid_log_error("this is a format test %d", 13);
 *	nid_log_crit("this is a format test %d", 14);
 *	nid_log_alert("this is a format test %d", 15);
 *	nid_log_emerg("this is a format test %d", 16);
 *	nid_log_close();
 */

extern int nid_log_level;

#define nid_log_open()					\
do {							\
	openlog("nid", LOG_NDELAY|LOG_PID, LOG_LOCAL0);	\
} while(0)

#define nid_log_close()		\
do {				\
	closelog();		\
} while (0)

#define nid_log_set_level(level)			\
do {							\
	nid_log_level = level;				\
} while(0)

#define	nid_log_core(level, fmt, args...)			\
do {								\
	if (nid_log_level >= level)				\
		syslog(level, "(%s:%d)"fmt, __FILE__, __LINE__, ##args);\
} while(0)

#define nid_log_emerg(fmt, args...) 	nid_log_core(LOG_EMERG,  fmt, ## args)
#define nid_log_alert(fmt, args...) 	nid_log_core(LOG_ALERT,  fmt, ## args)
#define nid_log_crit(fmt, args...)	nid_log_core(LOG_CRIT,   fmt, ## args)
#define nid_log_error(fmt, args...)	nid_log_core(LOG_ERR,    fmt, ## args)
#define nid_log_warning(fmt, args...)	nid_log_core(LOG_WARNING,fmt, ## args)
#define nid_log_notice(fmt, args...)	nid_log_core(LOG_NOTICE, fmt, ## args)
#define nid_log_info(fmt, args...)	nid_log_core(LOG_INFO,   fmt, ## args)
#define nid_log_debug(fmt, args...)	nid_log_core(LOG_DEBUG,  fmt, ## args)

#endif
