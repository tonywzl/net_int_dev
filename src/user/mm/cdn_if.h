/*
 * cdn_if.h
 * 	Interface of Content Description Node Module
 */
#ifndef NID_CDN_IF_H
#define NID_CDN_IF_H

#include "list.h"
#include "node_if.h"

#define CDN_STATE_LRU		0x0001
#define CDN_STATE_ONDISK	0x0002

struct content_description_node {
	struct node_header	cn_header;
	unsigned long		cn_data;	// pointer or index
	struct list_head	cn_slist;
	struct list_head	cn_flist;
	struct list_head	cn_plist;
	uint8_t			cn_is_lru;
	uint8_t			cn_is_ondisk;
	char			cn_fp[0];
};

struct cdn_interface;
struct cdn_operations {
	struct content_description_node *	(*cn_get_node)(struct cdn_interface *);
	void					(*cn_put_node)(struct cdn_interface *, struct content_description_node *);
};

struct cdn_interface {
	void			*cn_private;
	struct cdn_operations	*cn_op;
};

struct allocator_interface;
struct cdn_setup {
	struct allocator_interface	*allocator;
	int				set_id;
	int				seg_size;
	int				fp_size;
};

extern int cdn_initialization(struct cdn_interface *, struct cdn_setup *);
#endif
