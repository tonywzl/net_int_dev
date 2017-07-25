/*
 * util_nw.h
 */
#ifndef NID_UTIL_NW_H
#define NID_UTIL_NW_H

#include "stdint.h"

#define SOCKET_TIME_OUT 60

extern int util_nw_read_stop(int, char *, uint32_t, int32_t, uint8_t *);
extern int util_nw_read_n(int, void *, int);
extern int util_nw_read_n_timeout(int, char *, uint32_t, int32_t);
extern int util_nw_write_timeout_stop(int, char *, uint32_t, int32_t, uint8_t *);
extern int util_nw_write_n(int, void *, int);
extern int util_nw_write_timeout(int, char *, int, int);
extern int util_nw_write_timeout_rc(int, char *, int, int, int *);
extern int util_nw_make_connection(char *, u_short);
extern int util_nw_make_connection2(char *, u_short);
extern int util_nw_connection(char *, u_short, int);
extern int util_nw_open();
extern int util_nw_bind(int, u_short);
extern int util_nw_write_two_byte(int, uint16_t);
extern int util_nw_write_two_byte_timeout(int, uint16_t, int);
extern char* util_nw_getip(char *);
#endif
