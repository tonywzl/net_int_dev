/*
 * twccg_if.h
 * 	Interface of Write Through Cache (twc) Channel Guardian Module
 */
#ifndef NID_TWCCG_IF_H
#define NID_TWCCG_IF_H

struct twcg_interface;
struct twccg_interface;
struct twccg_operations {
	void*			(*wcg_accept_new_channel)(struct twccg_interface *, int, char *);
	void			(*wcg_do_channel)(struct twccg_interface *, void *);
	struct twcg_interface*	(*wcg_get_twcg)(struct twccg_interface *);
};

struct twccg_interface {
	void			*wcg_private;
	struct twccg_operations	*wcg_op;
};

struct umpk_interface;
struct twccg_setup {
	struct twcg_interface	*twcg;
	struct umpk_interface	*umpk;
};

extern int twccg_initialization(struct twccg_interface *, struct twccg_setup *);
#endif
