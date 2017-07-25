#ifndef _NID_LOG_H
#define _NID_LOG_H

#include <linux/printk.h>

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7


extern int nid_log_level;

#define nid_log_set_level(level)			\
do {							\
	nid_log_level = level;				\
} while(0)

#define nid_log_core(level, kern_level, fmt, args...)			\
do {								\
	if (nid_log_level >= level)				\
		printk(kern_level "%s:%d "fmt"\n", __FILE__, __LINE__, ##args);	\
} while(0);
	
#define nid_log_emerg(fmt, args...)	nid_log_core(LOG_EMERG, KERN_EMERG, fmt, ##args)
#define nid_log_alert(fmt, args...)	nid_log_core(LOG_ALERT, KERN_ALERT, fmt, ##args)
#define nid_log_crit(fmt, args...)	nid_log_core(LOG_CRIT, KERN_CRIT, fmt, ##args)
#define nid_log_error(fmt, args...)	nid_log_core(LOG_ERR, KERN_ERR, fmt, ##args)
#define nid_log_warning(fmt, args...)	nid_log_core(LOG_WARNING, KERN_WARNING, fmt, ##args)
#define nid_log_notice(fmt, args...)	nid_log_core(LOG_NOTICE, KERN_NOTICE, fmt, ##args)
#define nid_log_info(fmt, args...)	nid_log_core(LOG_INFO, KERN_INFO, fmt, ##args)
#define nid_log_debug(fmt, args...)	nid_log_core(LOG_DEBUG, KERN_DEBUG, fmt, ##args)

#endif
