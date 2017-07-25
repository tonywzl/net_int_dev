/*
 * nse_if.h
 * 	Interface of Namespace Search Engine Module
 */
#ifndef NID_nse_IF_H
#define NID_nse_IF_H

#include <stdint.h>

struct nse_info_hit {
	uint64_t 	ns_hit_counter;
	uint64_t 	ns_unhit_counter;
};

struct nse_interface;
struct list_head;
struct sub_request_node;
struct iovec;
struct nse_crc_cb_node;
struct nse_stat;

typedef void (*nse_callback)(int errcode, struct nse_crc_cb_node *arg);

struct nse_operations {
	void		(*ns_search)(struct nse_interface *, struct sub_request_node *rn_p,
				struct list_head *, struct list_head *, struct list_head *);
	void		(*ns_search_fetch)(struct nse_interface *, struct sub_request_node *rn_p,
				nse_callback, struct nse_crc_cb_node *);
	void		(*ns_updatev)(struct nse_interface *, struct iovec *, int, off_t,
				struct list_head *);
	void		(*ns_dropcache_start)(struct nse_interface *, int);
	void		(*ns_dropcache_stop)(struct nse_interface *);
	void		(*ns_get_stat)(struct nse_interface *, struct nse_stat *);
	int		(*ns_traverse_start)(struct nse_interface *);
	void		(*ns_traverse_stop)(struct nse_interface *);
	int		(*ns_traverse_next)(struct nse_interface *, char *, uint64_t *);
};

struct nse_interface {
	void			*ns_private;
	struct nse_operations	*ns_op;
};

struct brn_interface;
struct srn_interface;
struct fpn_interface;
struct nse_setup {
	struct allocator_interface	*allocator;
	struct brn_interface		*brn;
	struct srn_interface		*srn;
	struct fpn_interface		*fpn;
	uint64_t			size;
	int				block_shift;
};

struct nse_stat {
	uint64_t	fp_num;
	uint64_t	fp_max_num;
	uint64_t	fp_min_num;
	uint64_t	fp_rec_num;
	volatile int	hit_num;
	volatile int	unhit_num;

};

extern int nse_initialization(struct nse_interface *, struct nse_setup *);
#endif
