/*
 * doa_if.h
 * 	Interface of Delegation of Authority Module
 */
#ifndef NID_DOAC_IF_H
#define NID_DOAC_IF_H


#include "list.h"
#include "nid.h"
#define DOA_MAX_ID	1024
#define DOA_CMD_REQUEST 1
#define DOA_CMD_CHECK 2
#define DOA_CMD_RELEASE 3

#define NID_DOA_OVERTIME 30

struct doac_interface;
struct doac_operations {
	int		(*dc_accept_new_channel)(struct doac_interface *, int);
	void		(*dc_do_channel)(struct doac_interface *);
	void		(*dc_cleanup)(struct doac_interface *);

};

struct doac_interface {
	void			*dc_private;
	struct doac_operations	*dc_op;
};

struct doac_setup {
	struct doacg_interface	*doacg;
	struct umpk_interface	*umpk;
	uint8_t			is_old_command;
};

extern int doac_initialization(struct doac_interface *, struct doac_setup *);
#endif
