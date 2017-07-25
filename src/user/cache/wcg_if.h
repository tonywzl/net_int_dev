/*
 * wcg_if.h
 * 	Interface of Write Cache Guardian Module
 */
#ifndef NID_WCG_IF_H
#define NID_WCG_IF_H

struct wcg_interface;
struct wc_interface;
struct sdsg_interface;
struct srn_interface;
struct allocator_interface;
struct lstn_interface;
struct io_interface;
struct tpg_interface;

struct wcg_operations {
	struct bwcg_interface* (*wg_get_bwcg)(struct wcg_interface *);
	struct twcg_interface* (*wg_get_twcg)(struct wcg_interface *);
	struct wc_interface* (*wg_search)(struct wcg_interface *, char *);
	struct wc_interface*	(*wg_search_and_create_wc)(struct wcg_interface *, char *);
	void (*wg_recover_all_wc)(struct wcg_interface *);


};

struct wcg_interface {
	void			*wg_private;
	struct wcg_operations	*wg_op;
};

struct ini_interface;
struct wcg_setup {
	int							support_twc;
	int							support_bwc;
	struct ini_interface		*ini;
	struct sdsg_interface		*sdsg;
	struct srn_interface		*srn;
	struct allocator_interface	*allocator;
	struct lstn_interface		*lstn;
	struct io_interface			*io;
	struct tpg_interface		*tpg;
	struct mqtt_interface		*mqtt;
};

extern int wcg_initialization(struct wcg_interface *, struct wcg_setup *);
#endif
