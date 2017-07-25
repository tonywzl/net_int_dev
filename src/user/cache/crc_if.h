/*
 * crc_if.h
 * 	Interface of CAS (Content-Addressed Storage) Read Cache Module
 */
#ifndef NID_CRC_IF_H
#define NID_CRC_IF_H

#include <stdint.h>
#include "rc_if.h"
#include "nid_shared.h"

// As currently we only have crc type for rc module, so crc_rc_cbn == rc_wc_cbn
#define crc_rc_cb_node rc_wc_cb_node

typedef void (*crc_callback)(int errcode, struct crc_rc_cb_node *arg);

#define CRC_FP_SIZE	32

struct crc_interface;
struct rc_channel_info;
struct sub_request_node;
struct umessage_crc_information_resp_cse_hit;
struct list_head;
struct iovec;
struct crc_operations {
	void*           (*cr_create_channel)(struct crc_interface *, void *, struct rc_channel_info *, char *, int *);
	void*           (*cr_prepare_channel)(struct crc_interface *, struct rc_channel_info *, char *);
	void		(*cr_search)(struct crc_interface *, void *, struct sub_request_node *, struct list_head *);
	void		(*cr_search_fetch)(struct crc_interface *, void *, struct sub_request_node *, crc_callback, struct crc_rc_cb_node *);
	void		(*cr_updatev)(struct crc_interface *, void *, struct iovec *, int, off_t, struct list_head *);
	void 		(*cr_update)(struct crc_interface *crc_p, void *rc_handle, void *data, ssize_t len,  off_t offset, struct list_head *);
	int		(*cr_setup_cache_data)(struct crc_interface *, char *, struct list_head *);
	int		(*cr_setup_cache_bmp)(struct crc_interface *, struct list_head *, int);
	int		(*cr_setup_cache_mdata)(struct crc_interface *, struct list_head *, struct list_head *);
	void		(*cr_flush_content)(struct crc_interface *, char *, uint64_t, struct list_head *);
	uint64_t	(*cr_freespace)(struct crc_interface * );
	int*		(*cr_sp_heads_size)(struct crc_interface * );
	struct umessage_crc_information_resp_freespace_dist*	(*cr_freespace_dist)(struct crc_interface *);
	void		(*cr_cse_hit)(struct crc_interface *, int *, int *);
	void		(*cr_touch_block)(struct crc_interface *, void *, void *);
	void		(*cr_insert_block)(struct crc_interface *, void *);
	int		(*cr_read_block)(struct crc_interface *, char *, uint64_t);
	void		(*cr_drop_block)(struct crc_interface *, void *);
	void		(*cr_set_fp_wgt)(struct crc_interface *, uint8_t);
	struct dsrec_stat (*cr_get_dsrec_stat)(struct crc_interface *);
	int		(*cr_chan_inactive)(struct crc_interface *, void *);
	int		(*cr_chan_inactive_res)(struct crc_interface *, const char *);
	int		(*cr_stop)(struct crc_interface *);
	char*		(*cr_get_uuid)(struct crc_interface *);
	int		(*cr_dropcache_start)(struct crc_interface *, char *, int);
	int		(*cr_dropcache_stop)(struct crc_interface *, char *);
	int		(*cr_get_nse_stat)(struct crc_interface *, char *, struct nse_stat *);
	void*		(*cr_nse_traverse_start)(struct crc_interface *, char *);
	void		(*cr_nse_traverse_stop)(struct crc_interface *, void *);
	int		(*cr_nse_traverse_next)(struct crc_interface *, void *, char *, uint64_t *);
	struct list_head* (*cr_dsbmp_rtree_list)(struct crc_interface *crc_p );
	unsigned char 	(*cr_check_fp)(struct crc_interface *crc_p );
	void		(*cr_recover)(struct crc_interface *);
};

struct crc_interface {
	void			*cr_private;
	struct crc_operations	*cr_op;
};

struct fpn_interface;
struct srn_interface;
struct pp_interface;
struct rc_interface;
struct allocator_interface;
struct crc_setup {
	char				uuid[NID_MAX_UUID];
	char				pp_name[NID_MAX_PPNAME];
	struct allocator_interface	*allocator;
	struct fpn_interface		*fpn;
	struct brn_interface		*brn;
	struct rc_interface		*rc;
	struct srn_interface		*srn;
	struct pp_interface		*pp;
	struct fpc_interface		*fpc;
	char				cachedev[NID_MAX_PATH];
	int				cachedevsz;
	uint64_t			blocksz;	// bytes of block
//	uint64_t			size;		// number of blocks

};

extern int crc_initialization(struct crc_interface *, struct crc_setup *);
#endif
