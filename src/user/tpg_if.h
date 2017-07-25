/*
 * tpg_if.h
 * 	Interface of Thread Pool Guardian Module
 */
#ifndef NID_TPG_IF_H
#define NID_TPG_IF_H

struct tpg_interface;
struct tpg_operations {
	struct tp_interface *	(*tpg_create_channel)(struct tpg_interface *, char *);
	struct tp_interface *	(*tpg_search_tp)(struct tpg_interface *, char *);
	struct tp_interface **	(*tpg_get_all_tp)(struct tpg_interface *, int *);
	struct tp_interface *	(*tpg_search_and_create_tp)(struct tpg_interface *, char *);
	int			(*tpg_add_tp)(struct tpg_interface *, char *, int);
	int			(*tpg_delete_tp)(struct tpg_interface *, char *);
	struct tp_setup *	(*tpg_get_all_tp_setup)(struct tpg_interface *, int *);
	char **			(*tpg_get_working_tp_name)(struct tpg_interface *, int *);
	void			(*tpg_cleanup_tp)(struct tpg_interface *);
};

struct tpg_interface {
	void			*tg_private;
	struct tpg_operations	*tg_op;
};

struct ini_interface;
struct pp2_interface;
struct tpg_setup {
		struct ini_interface	*ini;
		struct pp2_interface	*pp2;
};

extern int tpg_initialization(struct tpg_interface *, struct tpg_setup *);
#endif
