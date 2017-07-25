/*
 * rc_if.h
 * 	Interface of Read Cache Module
 */
#ifndef NID_RC_IF_H
#define NID_RC_IF_H

#define RC_TYPE_NONE		0
#define RC_TYPE_CAS		1	// Content Address Cache
#define RC_TYPE_MAX		2

#define RC_RECOVER_NONE		0
#define RC_RECOVER_DOING	1
#define RC_RECOVER_DONE		2

struct list_head;
struct rc_interface;
struct sub_request_node;
struct iovec;
struct rc_wc_cb_node;
struct rc_twc_cb_node;
struct dsrec_stat;
struct nse_stat;

struct rc_channel_info {
	void	*rc_rw;
};


typedef void (*rc_callback)(int errcode, struct rc_wc_cb_node *arg);
typedef void (*rc_callback_twc)(int errcode, struct rc_twc_cb_node *arg);

struct rc_operations {
	void*	(*rc_create_channel)(struct rc_interface *, void *, struct rc_channel_info *, char *, int *);
	void*	(*rc_prepare_channel)(struct rc_interface *, struct rc_channel_info *, char *);
	void	(*rc_search)(struct rc_interface *, void *, struct sub_request_node *, struct list_head *);
	void	(*rc_search_fetch)(struct rc_interface *, void *, struct sub_request_node *, rc_callback, struct rc_wc_cb_node*);
	void	(*rc_search_fetch_twc)(struct rc_interface *, void *, struct sub_request_node *, rc_callback_twc, struct rc_twc_cb_node*);
	void	(*rc_updatev)(struct rc_interface *, void *, struct iovec *, int, off_t, struct list_head *);
	void	(*rc_update)(struct rc_interface *, void *, void *, ssize_t, off_t, struct list_head *);
	ssize_t	(*rc_setup_cache_mdata)(struct rc_interface *, struct list_head *, struct list_head *);
	ssize_t	(*rc_setup_cache_data)(struct rc_interface *, char *, struct list_head *);
	ssize_t	(*rc_setup_cache_bmp)(struct rc_interface *, struct list_head *);
	void	(*rc_flush_content)(struct rc_interface *, char *, uint64_t, struct list_head *);
	void	(*rc_touch_block)(struct rc_interface *, void *, void *);
	void	(*rc_insert_block)(struct rc_interface *, void *);
	int	(*rc_read_block)(struct rc_interface *, char *, uint64_t);
	void	(*rc_drop_block)(struct rc_interface *, void *);
	void	(*rc_set_fp_wgt)(struct rc_interface *, uint8_t);
	struct dsrec_stat (*rc_get_dsrec_stat)(struct rc_interface *);
	int	(*rc_chan_inactive_res)(struct rc_interface *, const char *);
	char*	(*rc_get_uuid)(struct rc_interface *);
	uint64_t	(*rc_freespace)(struct rc_interface * );
	struct umessage_crc_information_resp_freespace_dist*	(*rc_freespace_dist)(struct rc_interface * );
	int	(*rc_dropcache_start)(struct rc_interface *, char *, int);
	int	(*rc_dropcache_stop)(struct rc_interface *, char *);
	int	(*rc_get_nse_stat)(struct rc_interface *, char *, struct nse_stat *);
	void*	(*rc_nse_traverse_start)(struct rc_interface *, char *);
	void	(*rc_nse_traverse_stop)(struct rc_interface *, void *);
	int	(*rc_nse_traverse_next)(struct rc_interface *, void *, char *, uint64_t *);
	struct list_head* (*rc_dsbmp_rtree_list)(struct rc_interface *rc_p );
  	unsigned char 	(*rc_check_fp)(struct rc_interface *rc_p);
  	struct crc_interface* 	(*rc_get_crc)(struct rc_interface *rc_p);
  	char* 	(*rc_get_name)(struct rc_interface *rc_p);
	void	(*rc_set_recover_state)(struct rc_interface *, int);
	int	(*rc_get_recover_state)(struct rc_interface *);
	void	(*rc_recover)(struct rc_interface *);
};

struct rc_interface {
	void			*rc_private;
	struct rc_operations	*rc_op;
};

struct fpn_interface;
struct srn_interface;
struct pp_interface;
struct allocator_interface;
struct rc_setup {
	char				*uuid;
	char				*pp_name;
	int				type;
	struct allocator_interface	*allocator;
	struct fpn_interface		*fpn;
	struct brn_interface		*brn;
	struct srn_interface		*srn;
	struct pp_interface		*pp;
	char				*cachedev;
	int				cachedevsz;

};

extern int rc_initialization(struct rc_interface *, struct rc_setup *);
#endif
