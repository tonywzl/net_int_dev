/*
 * cse_if.h
 * 	Interface of Content Search Engine Module
 */
#ifndef NID_CSE_IF_H
#define NID_CSE_IF_H

#include <stdint.h>

struct cse_crc_cb_node;
struct cse_interface;
struct list_head;
struct iovec;
struct content_description_node;
typedef void (*cse_callback)(int errcode, struct cse_crc_cb_node *arg);

struct cse_info_hit {
	uint64_t 	cs_hit_counter;
	uint64_t 	cs_unhit_counter;
};

struct cse_operations {
	void		(*cs_search_list)(struct cse_interface *, struct list_head *, struct list_head *, struct list_head *,struct list_head *, int*);
	void		(*cs_search_fetch)(struct cse_interface *, struct list_head *, struct list_head *, cse_callback, struct cse_crc_cb_node*);
	void		(*cs_updatev)(struct cse_interface *, struct iovec *, int, struct list_head *);
	void		(*cs_drop_block)(struct cse_interface *, struct content_description_node *);
	int		(*cs_check_fp)(struct cse_interface *);
	void		(*cs_info_hit)(struct cse_interface *, struct cse_info_hit *info_hit);
	void		(*cs_recover_fp)(struct cse_interface *, struct list_head *);
};

struct cse_interface {
	void			*cs_private;
	struct cse_operations	*cs_op;
};

struct allocator_interface;
struct srn_interface;
struct fpn_interface;
struct fpc_interface;
struct pp_interface;
struct cdn_interface;
struct rc_interface;
struct blksn_interface;
struct cse_setup {
	struct allocator_interface	*allocator;
	struct rc_interface		*rc;
	struct srn_interface		*srn;
	struct fpn_interface		*fpn;
	struct cdn_interface		*cdn;
	struct pp_interface		*pp;
	struct fpc_interface		*fpc;
	struct blksn_interface		*blksn;
	uint8_t				fp_size;
	int				block_shift;
	uint64_t			d_blocks_num;   // number of data blocks in rc device
};

extern int cse_initialization(struct cse_interface *, struct cse_setup *);
#endif
