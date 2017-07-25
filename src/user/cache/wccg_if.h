/*
 * wccg_if.h
 * 	Interface of Write Cache Channel Guardian Module
 */
#ifndef NID_WCCG_IF_H
#define NID_WCCG_IF_H

struct wccg_interface;
struct wc_interface;
struct wccg_operations {
	void*			(*wcg_accept_new_channel)(struct wccg_interface *, int, char *);
	void			(*wcg_do_channel)(struct wccg_interface *, void *);
	struct wc_interface *	(*wcg_get_wc)(struct wccg_interface *, char *);
};

struct wccg_interface {
	void			*wcg_private;
	struct wccg_operations	*wcg_op;
};

struct scg_interface;
struct umpk_interface;
struct list_head;
struct wccg_setup {
	struct bwcg_interface	*bwcg;
	struct twcg_interface	*twcg;
	struct umpk_interface	*umpk;
};

extern int wccg_initialization(struct wccg_interface *, struct wccg_setup *);
#endif
