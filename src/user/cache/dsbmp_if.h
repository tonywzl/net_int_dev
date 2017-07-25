/*
 * dsbmp_if.h
 * 	Interface of Device Space Bitmap Module
 */
#ifndef NID_DSBMP_IF_H
#define NID_DSBMP_IF_H

#include <stdint.h>

struct dsbmp_interface;
struct block_size_node;
struct dsbmp_operations {
	struct block_size_node *	(*sb_put_node)(struct dsbmp_interface *, struct block_size_node *);
	struct block_size_node *	(*sb_get_node)(struct dsbmp_interface *, struct block_size_node *);
	struct block_size_node *	(*sb_split_node)(struct dsbmp_interface *, struct block_size_node *, uint64_t);
  	uint64_t 			(*sb_get_free_blocks)( struct dsbmp_interface *);
	struct list_head* 		(*sb_traverse_rtree)( struct dsbmp_interface* ); 
};

struct dsbmp_interface {
	void			*sb_private;
	struct dsbmp_operations	*sb_op;
};

struct brn_interface;
struct dsmgr_interface;
struct blksn_interface;
struct allocator_interface;
struct dsbmp_setup {
	uint64_t			size;		// bitmap size in number of uint64_t
	uint64_t			*bitmap;
	struct allocator_interface	*allocator;
	struct dsmgr_interface		*dsmgr;
	struct brn_interface		*brn;
	struct blksn_interface		*blksn;
};

extern int dsbmp_initialization(struct dsbmp_interface *, struct dsbmp_setup *);
#endif
