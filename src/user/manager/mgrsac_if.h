/*
 * mgrsac_if.h
 * 	Interface of Manager to SAC Channel Module
 */
#ifndef _MGRSAC_IF_H
#define _MGRSAC_IF_H

#include <sys/types.h>

struct mgrsac_interface;
struct mgrsac_operations {
	int	(*sa_information)(struct mgrsac_interface *, char *);
	int	(*sa_information_all)(struct mgrsac_interface *);
	int	(*sa_list_stat)(struct mgrsac_interface *, char *);
	int 	(*sa_add)(struct mgrsac_interface *, char *, uint8_t, uint8_t, uint8_t, uint32_t, char *, char *, char *, uint32_t, char *);
	int 	(*sa_delete)(struct mgrsac_interface *, char *);
	int 	(*sa_switch_bwc)(struct mgrsac_interface *, char *, char *);
	int	(*sa_set_keepalive)(struct mgrsac_interface *, char *, uint16_t, uint16_t);
	int	(*sa_fast_release)(struct mgrsac_interface *, char *, uint8_t);
	int	(*sa_ioinfo_start)(struct mgrsac_interface *, char *);
	int	(*sa_ioinfo_start_all)(struct mgrsac_interface *);
	int	(*sa_ioinfo_stop)(struct mgrsac_interface *, char *);
	int	(*sa_ioinfo_stop_all)(struct mgrsac_interface *);
	int	(*sa_ioinfo_check)(struct mgrsac_interface *, char *);
	int	(*sa_ioinfo_check_all)(struct mgrsac_interface *);
	int	(*sa_display)(struct mgrsac_interface *, char *, uint8_t);
	int	(*sa_display_all)(struct mgrsac_interface *, uint8_t);
	int	(*sa_hello)(struct mgrsac_interface *);
};

struct mgrsac_interface {
	void				*sa_private;
	struct mgrsac_operations	*sa_op;
};

struct umpk_interface;
struct mgrsac_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrsac_initialization(struct mgrsac_interface *, struct mgrsac_setup *);
#endif
