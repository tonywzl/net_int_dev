/*
 * cxp_if.h
 * 	Interface of Control Exchange Passive Module
 */

#ifndef NID_CXP_IF_H
#define NID_CXP_IF_H

#include <sys/socket.h>
#include "nid_shared.h"

#define CXP_TYPE_WRONG		0
#define CXP_TYPE_MREPLICATION	1

struct cxt_interface;
struct cxp_interface;
struct umessage_tx_hdr;
struct cxp_operations {
	struct cxt_interface*	(*cx_get_cxt)(struct cxp_interface *);
	void*			(*cx_get_handle)(struct cxp_interface *);
	char*			(*cx_get_chan_uuid)(struct cxp_interface *);
	void			(*cx_do_channel)(struct cxp_interface *);
	void			(*cx_cleanup)(struct cxp_interface *);
	void			(*cx_mark_recover_cxt)(struct cxp_interface *);
	void			(*cx_bind_socket)(struct cxp_interface *, int);
	int			(*cx_drop_channel)(struct cxp_interface *, struct umessage_tx_hdr *);
	int     	(*cx_start)(struct cxp_interface *);
};

struct cxp_interface {
	void			*cx_private;
	struct cxp_operations	*cx_op;
};

struct umpk_interface;
struct cxpg_interface;
struct cxp_setup {
	struct cxtg_interface	*cxtg;
	char			uuid[NID_MAX_UUID];
	char			peer_uuid[NID_MAX_UUID];
	char			cxt_name[NID_MAX_UUID];
	int			csfd;
	void			*handle;
	struct umpk_interface	*umpk;
	struct cxpg_interface	*cxpg;
};

extern int cxp_initialization(struct cxp_interface *, struct cxp_setup *);
#endif
