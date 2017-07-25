/*
 * ddn_if.h
 * 	Interface of Data Description Node Module
 */
#ifndef NID_DDN_IF_H
#define NID_DDN_IF_H

#include <stdint.h>
#include <list.h>
#include <node_if.h>

#define DDN_WHERE_NO			0
#define DDN_WHERE_FLUSH			1
#define DDN_WHERE_RELEASE		2
#define DDN_WHERE_NOT_READY_FLUSH	3
#define DDN_WHERE_SHADOW		4

struct ddn_segment;
struct data_description_node {
	struct node_header	bn_header;
	struct list_head	d_list;
	struct list_head	d_slist;		// search list
	struct list_head	d_flist;		// flush list
	struct pp_page_node	*d_page_node;		// pointer to a page node
	void			*d_interface;
	void			*d_parent;
	void			*d_callback_arg;
	int64_t			d_pos;			// pointer to a position of the page
	int64_t			d_flush_pos;
	uint64_t		d_seq;
	off_t			d_offset;
	off_t			d_flush_offset;
	uint32_t		d_len;
	uint32_t		d_flush_len;
	volatile int		d_user_counter;
	volatile int		d_flush_overwritten;	// nubmer of overwritten requests in flushing
	uint8_t			d_flushing;
	uint8_t			d_flushed;
	uint8_t			d_flushed_back;
	uint8_t			d_flush_done;
	uint8_t			d_released;
	uint8_t			d_where;
	uint8_t			d_location_disk;
};

struct ddn_interface;
struct ddn_operations {
	struct data_description_node*	(*d_get_node)(struct ddn_interface *);
	int				(*d_put_node)(struct ddn_interface *, struct data_description_node *);
	void				(*d_destroy)(struct ddn_interface *);
};

struct ddn_interface {
	void			*d_private;
	struct ddn_operations	*d_op;
};

struct allocator_interface;
struct ddn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
};

extern int ddn_initialization(struct ddn_interface *, struct ddn_setup *);
#endif
