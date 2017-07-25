/* * dxt_if.h
 * 	Interface of Data Exchange Transfer Module
 */
#ifndef NID_DXT_IF_H
#define NID_DXT_IF_H

#include <stdint.h>
#include "nid_shared.h"

#define  DXT_RETRY_INTERVAL	1

typedef void (*nid_tx_callback)(void *, void *);

struct dxt_status {
	uint64_t		dsend_seq;	// max seq of send data message
	uint64_t		send_seq;	// max seq of send control message
	uint64_t		ack_seq;	// max seq of ack message send out
	uint64_t		recv_seq;	// max seq of received control message
	uint64_t		recv_dseq;	// max dseq received data message
	uint32_t		cmsg_left;	// un-processed bytes in control message receive buffer
};

struct dxt_interface;
struct dxa_interface;
struct dxp_interface;
struct list_node;
struct list_head;
struct iovec;
struct dxt_operations {
	void			(*dx_do_channel)(struct dxt_interface *);
	struct list_node*	(*dx_get_request)(struct dxt_interface *);
	void			(*dx_put_request)(struct dxt_interface *, struct list_node *);
	void			(*dx_get_request_list)(struct dxt_interface *, struct list_head *);
	void			(*dx_put_request_list)(struct dxt_interface *, struct list_head *);
	void*			(*dx_get_buffer)(struct dxt_interface *, uint32_t);
	ssize_t			(*dx_send)(struct dxt_interface *, void *, size_t, void *, size_t);
	int			(*dx_is_data_ready)(struct dxt_interface *, int, uint64_t);
	char*			(*dx_data_position)(struct dxt_interface *, int, uint64_t, uint32_t *);
	char*			(*dx_get_name)(struct dxt_interface *);
	void*			(*dx_get_handle)(struct dxt_interface *);
	void			(*dx_stop)(struct dxt_interface *);
	void			(*dx_cleanup)(struct dxt_interface *);
	void			(*dx_recover)(struct dxt_interface *, int, int);
	void			(*dx_bind_dxa)(struct dxt_interface *, struct dxa_interface *, nid_tx_callback);
	void			(*dx_bind_dxp)(struct dxt_interface *, struct dxp_interface *, nid_tx_callback);
	char*			(*dx_get_dbuf)(struct dxt_interface *, struct list_node *);
	void			(*dx_put_dbuf)(char *);
	int			(*dx_get_dbuf_vec)(struct dxt_interface *, struct list_node *, struct iovec *, uint32_t *);
	void			(*dx_put_dbuf_vec)(struct dxt_interface *, struct iovec *, uint32_t);
	void			(*dx_get_status)(struct dxt_interface *, struct dxt_status *);
	void			(*dx_freeze)(struct dxt_interface *);
	int			(*dx_is_working)(struct dxt_interface *);
	int			(*dx_is_new)(struct dxt_interface *);
	int			(*dx_all_frozen)(struct dxt_interface *);
	int			(*dx_reset_all)(struct dxt_interface *);
};

struct dxt_interface {
	void			*dx_private;
	struct dxt_operations	*dx_op;
};

struct txn_interface;
struct lstn_interface;
struct txn_interface;
struct ds_interface;
struct pp2_interface;
struct dxt_setup {
	struct ds_interface	*ds;
	struct pp2_interface	*pp2;
	struct lstn_interface	*lstn;
	struct txn_interface	*txn;
	int			csfd;
	int			dsfd;
	uint32_t		req_size;
	char			dxt_name[NID_MAX_UUID];
	char			ds_name[NID_MAX_UUID];
	void			*handle;
	nid_tx_callback		dxt_callback;
};

extern int dxt_initialization(struct dxt_interface *, struct dxt_setup *);
#endif
