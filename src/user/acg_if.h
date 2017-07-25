/*
 * acg_if.h
 * 	Interface of Agent Channel Guardian Module
 */
#ifndef NID_acg_IF_H
#define NID_acg_IF_H

#include <stdint.h>

struct acg_interface;
struct acg_message {
	struct asc_interface	*m_asc;
	uint8_t			m_req;
	int			m_owner_code;
	int			(*m_callback)(struct acg_interface *, struct acg_message *);
};

struct asc_description;
struct ini_interface;
struct list_head;
struct acg_operations {
	void*			(*ag_accept_new_channel)(struct acg_interface *, int, char *);
	void			(*ag_do_channel)(struct acg_interface *, void *);
	int			(*ag_is_stop)(struct acg_interface *);
	void			(*ag_set_stop)(struct acg_interface *);
	void			(*ag_cleanup)(struct acg_interface *);
};

struct acg_interface {
	void			*ag_private;
	struct acg_operations	*ag_op;
};

struct adt_interface;
struct ini_interface;
struct tp_interface;
struct acg_setup {
	struct ini_interface	*ini;
	struct tp_interface	*tp;
	struct pp2_interface	*pp2;
	struct pp2_interface	*dyn_pp2;
	struct pp2_interface	*dyn2_pp2;
	struct list_head	agent_keys;
	int			support_driver;
	struct umpka_interface	*umpka;
};

extern int acg_initialization(struct acg_interface *, struct acg_setup *);
#endif
