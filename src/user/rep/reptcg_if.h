/*
 * reptcg_if.h
 *	Interface of Replication Target Channel Guardian Module
 */
#ifndef NID_REPTCG_IF_H
#define NID_REPTCG_IF_H

struct rept_interface;
struct reptcg_interface;
struct reptcg_operations {
	void*			(*rtcg_accept_new_channel)(struct reptcg_interface *, int, char *);
	void			(*rtcg_do_channel)(struct reptcg_interface *, void *);
};

struct reptcg_interface {
	void			*r_private;
	struct reptcg_operations *r_op;
};

struct umpk_interface;
struct reptcg_setup {
	struct reptg_interface *reptg;
	struct umpk_interface *umpk;
};

extern int reptcg_initialization(struct reptcg_interface *, struct reptcg_setup *);
#endif
