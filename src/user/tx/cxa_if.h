/*
 * cxa_if.h
 * 	Interface of Control Exchange Active Module
 */

#ifndef NID_CXA_IF_H
#define NID_CXA_IF_H

#include "nid_shared.h"
#include <sys/socket.h>

#define CXA_TYPE_WRONG		0
#define CXA_TYPE_MREPLICATION	1

struct cxa_interface;
struct cxt_interface;
struct cxa_operations {
	int			(*cx_make_control_connection)(struct cxa_interface *);
	void			(*cx_do_channel)(struct cxa_interface *);
	struct cxt_interface*	(*cx_get_cxt)(struct cxa_interface *);
	void*			(*cx_get_handle)(struct cxa_interface *);
	void			(*cx_cleanup)(struct cxa_interface *);
	int			(*cx_keepalive_channel)(struct cxa_interface *);
	int			(*cx_drop_channel)(struct cxa_interface *);
	int			(*cx_housekeeping)(struct cxa_interface *);
	char*			(*cx_get_server_ip)(struct cxa_interface *);
	char*			(*cx_get_chan_uuid)(struct cxa_interface *);
	void			(*cx_mark_recover_cxt)(struct cxa_interface *);
	int			(*cx_start)(struct cxa_interface *);
};

struct cxa_interface {
	void			*cx_private;
	struct cxa_operations	*cx_op;
};

struct ds_interface;
struct tp_interface;
struct umpk_interface;
struct cxa_setup {
	struct ds_interface	*ds;
	struct cxtg_interface	*cxtg;
	struct tp_interface 	*tp;
	char			uuid[NID_MAX_UUID];
	char			peer_uuid[NID_MAX_UUID];
	char			ip[NID_MAX_IP];
	void			*handle;
	char			cxt_name[NID_MAX_UUID];
	struct umpk_interface	*umpk;
};

extern int cxa_initialization(struct cxa_interface *, struct cxa_setup *);
#endif
