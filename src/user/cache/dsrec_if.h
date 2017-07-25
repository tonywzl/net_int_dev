/*
 * dsrec_if.h
 * 	Interface of Device Space Reclaim Module
 */
#ifndef NID_DSREC_IF_H
#define NID_DSREC_IF_H

#include "nid.h"

struct dsrec_interface;
struct block_node;
struct list_head;
struct content_description_node;
struct dsrec_operations {
	void			(*sr_insert_block)(struct dsrec_interface *, struct content_description_node *);
	void			(*sr_touch_block)(struct dsrec_interface *, struct content_description_node *, char *);
	struct dsrec_stat	(*sr_get_dsrec_stat)(struct dsrec_interface *);
};

struct dsrec_interface {
	void			*sr_private;
	struct dsrec_operations	*sr_op;
};

struct rcache_interface;
struct dsmgr_interface;
struct rc_interface;
struct blksn_interface;
struct cdn_interface;
struct dsrec_setup {
	struct cdn_interface	*cdn;
	struct blksn_interface	*blksn;
	struct dsmgr_interface	*dsmgr;
	struct rc_interface	*rc;
	uint64_t		rc_size;	// total number of blocks in the read cache
	struct fpc_interface	*fpc;
};

struct dsrec_stat {
	uint64_t p_lru_nr[NID_SIZE_FP_WGT];		// number of nodes in different lru list.
	uint64_t p_lru_total_nr;				// total number of nodes in lru list.
	uint64_t p_lru_max;					// max number of nodes in lru list.
	uint64_t p_rec_nr;					// number of nodes in reclaim list
};

extern int dsrec_initialization(struct dsrec_interface *, struct dsrec_setup *);

#endif
