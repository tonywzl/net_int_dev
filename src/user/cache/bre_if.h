/*
 * bre_if.h
 * 	Interface of BIO Release Engine Module
 */
#ifndef NID_BRE_IF_H
#define NID_BRE_IF_H

#include <stdint.h> 

struct bre_interface;
struct data_description_node;
struct list_head;
struct bre_operations {
	void		(*br_release_page)(struct bre_interface *, void *);
	void		(*br_release_page_async)(struct bre_interface *, void *);
	void		(*br_quick_release_page)(struct bre_interface *, void *);
	void		(*br_release_page_immediately)(struct bre_interface *, void *);
	uint32_t	(*br_get_page_counter)(struct bre_interface *);
	void		(*br_close)(struct bre_interface *);
	void		(*br_dropcache_start)(struct bre_interface *, int);
	void		(*br_dropcache_stop)(struct bre_interface *);

	void		(*br_release_page_sequence)(struct bre_interface *, void *);
	void		(*br_update_sequence)(struct bre_interface *, uint64_t seq);
};

struct bre_interface {
	void			*br_private;
	struct bre_operations	*br_op;
};

struct pp_interface;
struct ddn_interface;
struct bse_interface;
struct bre_setup {
	struct pp_interface	*pp;
	struct ddn_interface	*ddn;
	struct bse_interface	*bse;
	struct wc_interface 	*wc;
	void			*wc_chain_handle;
	int8_t			sequence_mode;
};

extern int bre_initialization(struct bre_interface *, struct bre_setup *);
#endif
