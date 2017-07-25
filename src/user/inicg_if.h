/*
 * inicg_if.h
 *	Interface of ini Channel Guardian Module
 */
#ifndef NID_INICG_IF_H
#define NID_INICG_IF_H

struct ini_interface;
struct inicg_interface;
struct inicg_operations {
	void*			(*icg_accept_new_channel)(struct inicg_interface *, int, char *);
	void			(*icg_do_channel)(struct inicg_interface *, void *);
};

struct inicg_interface {
	void			*icg_private;
	struct inicg_operations *icg_op;
};

struct umpk_interface;
struct inicg_setup {
	struct ini_interface *ini;
	struct umpk_interface *umpk;
};

extern int inicg_initialization(struct inicg_interface *, struct inicg_setup *);
#endif
