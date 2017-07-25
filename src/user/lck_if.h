/*
 * lck_if.h
 * 	Interface of Lock Module
 */
#ifndef NID_LCK_IF_H
#define NID_LCK_IF_H

#include <pthread.h>
#include <stdint.h>

struct lck_node {
	uint16_t	ln_rcounter;
	char		ln_wbusy:1;	// somebody is waiting to write
	char		ln_ubusy:1;	// somebody is waiting to update
};

struct lck_interface;
struct lck_operations {
	void	(*l_rlock)(struct lck_interface *, struct lck_node *);
	void	(*l_runlock)(struct lck_interface *, struct lck_node *);
	void	(*l_wlock)(struct lck_interface *, struct lck_node *);
	void	(*l_wunlock)(struct lck_interface *, struct lck_node *);
	void	(*l_uplock)(struct lck_interface *, struct lck_node *);
	void	(*l_downlock)(struct lck_interface *, struct lck_node *);
	void	(*l_destroy)(struct lck_interface *);
};

struct lck_interface {
	void			*l_private;
	struct lck_operations	*l_op;
};

struct lck_setup {
};

extern int lck_initialization(struct lck_interface *, struct lck_setup *);
extern int lck_node_init(struct lck_node *);
#endif
