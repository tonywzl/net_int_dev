/*
 * txn_if.h
 * 	Interface of Transfer Node Module
 */
#ifndef NID_TXN_IF_H
#define NID_TXN_IF_H

#include "list.h"
#include "node_if.h"

struct tx_node {
	struct node_header	tn_header;
	struct list_head	tn_list;
	size_t			tn_len;
	uint64_t		tn_dseq;
	void			*tn_data;
	void			*tn_data_pos;
	void			*tn_ref_node;
};

struct txn_interface;
struct txn_operations {
	struct tx_node*		(*tn_get_node)(struct txn_interface *);
	void			(*tn_put_node)(struct txn_interface *, struct tx_node *);
};

struct txn_interface {
	void			*tn_private;
	struct txn_operations	*tn_op;
};

struct allocator_interface;
struct txn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int txn_initialization(struct txn_interface *, struct txn_setup *);
#endif
