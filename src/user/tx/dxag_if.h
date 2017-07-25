/*
 * dxag_if.h
 * 	Interface of Data Exchange Active Guardian Module
 */

#ifndef NID_DXAG_IF_H
#define NID_DXAG_IF_H

#include <stdint.h>

struct dxa_interface;
struct dxag_interface;
struct dxag_message {
	struct dxa_interface	*m_dxa;
	uint8_t			m_req;
	int			m_owner_code;
	int			(*m_callback)(struct dxag_interface *, struct dxag_message *);
};

struct dxag_operations {
	struct dxa_interface*	(*dxg_create_channel)(struct dxag_interface *, char *);
	int			(*dxg_drop_channel)(struct dxag_interface *, struct dxa_interface*);
	int			(*dxg_drop_channel_by_uuid)(struct dxag_interface *, char *);
	struct dxa_interface*	(*dxg_get_dxa_by_uuid)(struct dxag_interface *, char *);
	struct dxa_setup*	(*dxg_get_all_dxa_setup)(struct dxag_interface *, int *);
	char**			(*dxg_get_working_dxa_name)(struct dxag_interface *, int *);
};

struct dxag_interface {
	void			*dxg_private;
	struct dxag_operations	*dxg_op;
};

struct lstn_interface;
struct ini_interface;
struct dxtg_interface;
struct pp2_interface;
struct dxag_setup {
	struct lstn_interface		*lstn;
	struct ini_interface		*ini;
	struct dxtg_interface		*dxtg;
	struct umpk_interface		*umpk;
	struct pp2_interface		*pp2;
};

extern int dxag_initialization(struct dxag_interface *, struct dxag_setup *);
#endif
