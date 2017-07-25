/*
 * mgrrc_if.h
 * 	Interface of Manager to rc Channel Module
 */
#ifndef _MGRRC_IF_H
#define _MGRRC_IF_H

#include <sys/types.h>

struct mgrcrc_interface;
struct mgrcrc_operations {
	int 	(*mcr_dropcache_start)(struct mgrcrc_interface *, char *, char *, int);
	int 	(*mcr_dropcache_stop)(struct mgrcrc_interface*, char *, char *);
	int 	(*mcr_set_wgt)(struct mgrcrc_interface *, char *, int);
  	int 	(*mcr_info_freespace)(struct mgrcrc_interface*, char *);
  	int 	(*mcr_info_sp_heads_size)(struct mgrcrc_interface*, char *);
	int	(*mcr_info_freespace_dist)(struct mgrcrc_interface *, char * );
	int	(*mcr_info_nse_stat)(struct mgrcrc_interface *, char *, char *);
	int	(*mcr_info_nse_detail)(struct mgrcrc_interface *, char *, char *);
  	int 	(*mcr_info_dsbmp_rtree)(struct mgrcrc_interface *mgrrc_p, char *rc_uuid);
  	int	(*mcr_info_check_fp)(struct mgrcrc_interface *mgrrc_p, char *rc_uuid);
  	int	(*mcr_info_cse_hit)(struct mgrcrc_interface *mgrrc_p, char *);
  	int	(*mcr_info_dsrec_stat)(struct mgrcrc_interface *mgrrc_p, char *, char *);
  	int	(*mcr_add)(struct mgrcrc_interface *mgrrc_p, char *, char *, char *, int);
	int	(*mcr_display)(struct mgrcrc_interface *mgrrc_p, char *, uint8_t);
	int	(*mcr_display_all)(struct mgrcrc_interface *mgrrc_p, uint8_t);
	int	(*mcr_hello)(struct mgrcrc_interface *);
};

struct mgrcrc_interface {
	void			*mcr_private;
	struct mgrcrc_operations	*mcr_op;
};

struct umpk_interface;
struct mgrcrc_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrcrc_initialization(struct mgrcrc_interface *, struct mgrcrc_setup *);
#endif
