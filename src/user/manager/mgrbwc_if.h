/*
 * mgrbwc_if.h
 * 	Interface of Manager to BWC Channel Module
 */
#ifndef _MGRBWC_IF_H
#define _MGRBWC_IF_H

#include <sys/types.h>

struct mgrbwc_stat {
};

struct mgrbwc_interface;
struct mgrbwc_operations {
	int 	(*bw_dropcache_start)(struct mgrbwc_interface *, char *, char *, int);
	int 	(*bw_dropcache_stop)(struct mgrbwc_interface *, char *, char *);
	int 	(*bw_information_flushing)(struct mgrbwc_interface *, char *);
	int 	(*bw_information_stat)(struct mgrbwc_interface *, char *);
	int 	(*bw_information_throughput_stat)(struct mgrbwc_interface *, char *);
	int 	(*bw_throughput_stat_reset)(struct mgrbwc_interface *, char *);
	int 	(*bw_information_rw_stat)(struct mgrbwc_interface *, char *, char *);
	int	(*bw_information_delay_stat)(struct mgrbwc_interface *, char *);
	int 	(*bw_add)(struct mgrbwc_interface *, char *, char *, uint32_t, uint8_t, uint8_t, uint8_t, uint8_t,
			double, double, double, double, double, double, char *, uint8_t, uint8_t, uint16_t, uint16_t, uint16_t, uint16_t);
	int 	(*bw_remove)(struct mgrbwc_interface *, char *);
	int 	(*bw_fflush_start)(struct mgrbwc_interface *, char *, char *);
	int 	(*bw_fflush_get)(struct mgrbwc_interface *, char *, char *);
	int 	(*bw_fflush_stop)(struct mgrbwc_interface *, char *, char *);
	int 	(*bw_recover_start)(struct mgrbwc_interface *, char *);
	int 	(*bw_recover_get)(struct mgrbwc_interface *, char *);
	int 	(*bw_snapshot_admin)(struct mgrbwc_interface *, char *, char *, int);
	int	(*bw_water_mark)(struct mgrbwc_interface *, char *, uint16_t, uint16_t);
	int	(*bw_ioinfo_start)(struct mgrbwc_interface *, char *, char *, uint8_t);
	int	(*bw_ioinfo_stop)(struct mgrbwc_interface *, char *, char *, uint8_t);
	int	(*bw_ioinfo_check)(struct mgrbwc_interface *, char *, char *, uint8_t);
	int	(*bw_ioinfo_start_all)(struct mgrbwc_interface *, char *, uint8_t);
	int	(*bw_ioinfo_stop_all)(struct mgrbwc_interface *, char *, uint8_t);
	int	(*bw_ioinfo_check_all)(struct mgrbwc_interface *, char *, uint8_t);
	int	(*bw_delay)(struct mgrbwc_interface *, char *, uint16_t, uint16_t, uint32_t, uint32_t);
	int	(*bw_flush_empty_start)(struct mgrbwc_interface *, char *);
	int	(*bw_flush_empty_stop)(struct mgrbwc_interface *, char *);
	int	(*bw_display)(struct mgrbwc_interface *, char *, uint8_t);
	int	(*bw_display_all)(struct mgrbwc_interface *, uint8_t);
	int	(*bw_hello)(struct mgrbwc_interface *);
	int 	(*bw_information_list_stat)(struct mgrbwc_interface *, char *, char *);
};

struct mgrbwc_interface {
	void				*bw_private;
	struct mgrbwc_operations	*bw_op;
};

struct umpk_interface;
struct mgrbwc_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrbwc_initialization(struct mgrbwc_interface *, struct mgrbwc_setup *);
#endif
