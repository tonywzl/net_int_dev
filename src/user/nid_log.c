/*
 * nid_log.c
 */

#include <syslog.h>

#define DEFAULT_LOG_LEVEL	LOG_WARNING

#ifdef DEBUG_LEVEL
int nid_log_level = DEBUG_LEVEL;
#else
int nid_log_level = DEFAULT_LOG_LEVEL;
#endif
