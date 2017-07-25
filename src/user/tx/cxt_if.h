/*
 * cxt_if.h
 * 	Interface of Control Exchange Transfer Module
 */
#ifndef NID_CXT_IF_H
#define NID_CXT_IF_H

#include <stdint.h>
#include "nid_shared.h"

#define CXT_RETRY_INTERVAL	1

typedef void (*nid_tx_callback)(void *, void *);

struct cxt_status {
	uint64_t		send_seq;	// max seq of send control message
	uint64_t		ack_seq;	// max seq of ack message send out
	uint64_t		recv_seq;	// max seq of received control message
	uint32_t		cmsg_left;	// un-processed bytes in control message receive buffer
};

struct cxt_interface;
struct list_node;
struct list_head;
struct cxa_interface;
struct cxp_interface;
struct cxt_operations {
	void			(*cx_do_channel)(struct cxt_interface *);
	struct list_node*	(*cx_get_next_one)(struct cxt_interface *);
	void			(*cx_put_one)(struct cxt_interface *, struct list_node *);
	void			(*cx_get_list)(struct cxt_interface *, struct list_head *);
	void			(*cx_put_list)(struct cxt_interface *, struct list_head *);
	void*			(*cx_get_buffer)(struct cxt_interface *, uint32_t);
	ssize_t			(*cx_send)(struct cxt_interface *, void *, size_t);
	void			(*cx_stop)(struct cxt_interface *);
	void			(*cx_cleanup)(struct cxt_interface *);
	char*			(*cx_get_name)(struct cxt_interface *);
	void *			(*cx_get_handle)(struct cxt_interface *);
	void			(*cx_freeze)(struct cxt_interface *);
	void			(*cx_bind_cxa)(struct cxt_interface *, struct cxa_interface *, nid_tx_callback);
	void			(*cx_bind_cxp)(struct cxt_interface *, struct cxp_interface *, nid_tx_callback);
	void			(*cx_recover)(struct cxt_interface *, int);
	void			(*cx_get_status)(struct cxt_interface *, struct cxt_status *);
	int			(*cx_is_working)(struct cxt_interface *);
	int			(*cx_is_new)(struct cxt_interface *);
	int			(*cx_all_frozen)(struct cxt_interface *);
	int			(*cx_reset_all)(struct cxt_interface *);
};

struct cxt_interface {
	void			*cx_private;
	struct cxt_operations	*cx_op;
};

struct txn_interface;
struct lstn_interface;
struct txn_interface;
struct ds_interface;
struct pp2_interface;
struct cxt_setup {
	struct ds_interface	*ds;
	struct pp2_interface	*pp2;
	struct lstn_interface	*lstn;
	struct txn_interface	*txn;
	int			sfd;
	uint32_t		req_size;
	char			cxt_name[NID_MAX_UUID];
	char			ds_name[NID_MAX_UUID];
	void			*handle;
	struct cxa_interface	*cxa;
	struct cxp_interface	*cxp;
};

extern int cxt_initialization(struct cxt_interface *, struct cxt_setup *);
#endif
