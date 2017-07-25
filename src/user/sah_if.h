/*
 * sah_if.h
 * 	Interface of Service Agent Handshake Module
 */

#ifndef _NID_SAH_IF_H
#define _NID_SAH_IF_H

#include <sys/types.h>
#include "nid.h"

struct ini_interface;
struct sah_interface;
struct sah_operations {
	int			(*h_channel_establish)(struct sah_interface *);
	int			(*h_helo_generator)(struct sah_interface *, char *);
	char*			(*h_get_uuid)(struct sah_interface *);
	char*			(*h_get_exportname)(struct sah_interface *);
	int			(*h_get_dsfd)(struct sah_interface *);
	void			(*h_cleanup)(struct sah_interface *);
	char*			(*h_get_ipaddr)(struct sah_interface *);
	struct sac_info*	(*h_get_sac_info)(struct sah_interface *);
};

struct sah_interface {
	void			*h_private;
	struct sah_operations	*h_op;
};

struct sacg_interface;
struct rwg_interface;
struct sah_setup {
	int			rsfd;
	struct sacg_interface	*sacg;
	struct rwg_interface	*rwg;
};

extern int sah_initialization(struct sah_interface *, struct sah_setup *);

#endif
