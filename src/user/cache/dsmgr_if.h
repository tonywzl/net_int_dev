/*
 * dsmgr_if.h
 * 	Interface of Device Space Manager Module
 */
#ifndef NID_DSMGR_IF_H
#define NID_DSMGR_IF_H

#include <stdint.h>

#include "list.h"

struct dsmgr_interface;
struct block_size_node;
struct list_head;
struct dsmgr_operations {
	int	(*dm_get_space)(struct dsmgr_interface *, uint64_t *size, struct list_head *);
	void	(*dm_put_space)(struct dsmgr_interface *, struct block_size_node *);
	void	(*dm_extend_space)(struct dsmgr_interface *, struct block_size_node *, uint64_t);
	void	(*dm_add_new_node)(struct dsmgr_interface *, struct block_size_node *);
	void	(*dm_delete_node)(struct dsmgr_interface *, struct block_size_node *);
	int	(*dm_get_page_shift)(struct dsmgr_interface *);
	void    (*dm_display_space_nodes)( struct dsmgr_interface* dsmgr_p );
	struct umessage_crc_information_resp_freespace_dist*    (*dm_check_space)( struct dsmgr_interface* dsmgr_p );
	uint64_t  (*dm_get_num_free_blocks)( struct dsmgr_interface* dsmgr_p );
	int*  	(*dm_get_sp_heads_size)( struct dsmgr_interface* dsmgr_p );
	struct list_head* (*dm_traverse_dsbmp_rtree)( struct dsmgr_interface* dsmgr_p );
};

struct dsmgr_interface {
	void			*dm_private;
	struct dsmgr_operations	*dm_op;
};

struct brn_interface;
struct allocator_interface;
struct dsmgr_setup {
	struct allocator_interface	*allocator;
	struct brn_interface		*brn;
	struct blksn_interface		*blksn;
	int				block_shift;
	int				size;
	uint64_t			*bitmap;
};

extern int dsmgr_initialization(struct dsmgr_interface *, struct dsmgr_setup *);
#endif
