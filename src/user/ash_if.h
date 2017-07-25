/*
 * ash_if.h
 * 	Interface of Agent Service Handshake Module
 */

#ifndef _ASH_IF_H
#define _ASH_IF_H

#include <sys/types.h>
#include "nid.h"

struct ash_channel_info {
	int		i_rsfd;
	int		i_dsfd;
	uint64_t	i_size;
};

struct ini_interface;
struct ash_interface;
struct ash_operations {
	int	(*h_create_channel)(struct ash_interface *, struct ash_channel_info *);
	int	(*h_exp_generator)(struct ash_interface *, char *, char *);
	int	(*h_port_generator)(struct ash_interface *, char *, u_short, char *);
	void	(*h_cleanup)(struct ash_interface *);
};

struct ash_interface {
	void			*h_private;
	struct ash_operations	*h_op;
};

struct ash_setup {
	struct ini_interface	*ini;
	char			*uuid;
	char			*sip;
};

extern int ash_initialization(struct ash_interface *, struct ash_setup *);

#endif
