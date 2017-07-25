/*
 * twcg_if.h
 * 	Interface of Write Through Cache Guardian Module
 */
#ifndef NID_TWCG_IF_H
#define NID_TWCG_IF_H

#define TWCG_MAX_TWC	16

#include "list.h"

struct umessage_twc_information_resp_stat;
struct twc_throughput_stat;
struct twc_rw_stat;
struct twcg_interface;
struct twcg_operations {
	struct wc_interface*	(*wg_search_twc)(struct twcg_interface *, char *);
	struct wc_interface*	(*wg_search_and_create_twc)(struct twcg_interface *, char *);
	int			(*wg_add_twc)(struct twcg_interface *, char *, int, char *);
	int			(*wg_recover_twc)(struct twcg_interface *, char *);
	void			(*wg_recover_all_twc)(struct twcg_interface *);
	void			(*wg_get_info_stat)(struct twcg_interface *, char *, struct umessage_twc_information_resp_stat *);
	void			(*wg_get_rw)(struct twcg_interface *, char *, char *, struct twc_rw_stat *);
	struct twc_setup*	(*wg_get_all_twc_setup)(struct twcg_interface *, int *);
	int*			(*wg_get_working_twc_index)(struct twcg_interface *, int *);
	void			(*wg_get_all_twc_stat)(struct twcg_interface *, struct list_head *out_head);
};

struct twcg_interface {
	void			*wg_private;
	struct twcg_operations	*wg_op;
};

struct ini_interface;
struct sdsg_interface;
struct tpg_interface;
struct srn_interface;
struct allocator_interface;
struct lstn_interface;
struct twcg_setup {
	struct ini_interface		**ini;
	struct sdsg_interface		*sdsg;
	struct tp_interface		*wtp;
	struct srn_interface		*srn;
	struct allocator_interface	*allocator;
	struct lstn_interface		*lstn;
	struct tpg_interface		*tpg;
};

extern int twcg_initialization(struct twcg_interface *, struct twcg_setup *);
#endif
