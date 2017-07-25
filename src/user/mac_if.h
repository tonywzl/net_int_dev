/*
 * mac_if.h
 * 	Interface of Manage Service Channel Module
 */
#ifndef _mac_IF_H
#define _mac_IF_H

#include <sys/types.h>

struct mac_stat {
};

struct mac_interface;
struct mac_operations {
	int	(*m_stop)(struct mac_interface *);
	int	(*m_ioerror)(struct mac_interface *, char *);
	int	(*m_delete)(struct mac_interface *, char *);
	int	(*m_stat_reset)(struct mac_interface *);
	int	(*m_stat_get)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_wd)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_wu)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_wud)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_rwwd)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_rwwu)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_rwwud)(struct mac_interface *, struct mac_stat *);
	int	(*m_stat_get_ud)(struct mac_interface *, struct mac_stat *);
	int	(*m_update)(struct mac_interface *);
	int	(*m_check_agent)(struct mac_interface *);
	int	(*m_check_conn)(struct mac_interface *, char *);
	int	(*m_upgrade)(struct mac_interface *, char *);
	int	(*m_set_log_level)(struct mac_interface *, int);
	int	(*m_get_version)(struct mac_interface *);
	int	(*m_upgrade_force)(struct mac_interface *, char *);
	int	(*m_set_hook)(struct mac_interface *, int);
};

struct mac_interface {
	void			*m_private;
	struct mac_operations	*m_op;
};

struct mpk_interface;
struct mac_setup {
	char			*ipstr;
	u_short			port;
	struct mpk_interface	*mpk;
};

extern int mac_initialization(struct mac_interface *, struct mac_setup *);
#endif
