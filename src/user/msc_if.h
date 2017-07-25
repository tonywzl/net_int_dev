/*
 * msc_if.h
 * 	Interface of Manage Service Channel Module
 */
#ifndef _msc_IF_H
#define _msc_IF_H

#include <sys/types.h>

struct msc_stat {
};

struct msc_interface;
struct msc_operations {
	int	(*m_stop)(struct msc_interface *);
	int	(*m_stat_reset)(struct msc_interface *);
	int	(*m_stat_get)(struct msc_interface *, struct msc_stat *);
	int	(*m_update)(struct msc_interface *);
	int	(*m_check_conn)(struct msc_interface *, char *uuid);
	int	(*m_set_log_level)(struct msc_interface *, int);
	int	(*m_get_version)(struct msc_interface *);
	int	(*m_check_server)(struct msc_interface *);
	int	(*m_bio_fast_flush)(struct msc_interface *);
	int	(*m_bio_stop_fast_flush)(struct msc_interface *);
	int 	(*m_bio_vec_start)(struct msc_interface *);
	int 	(*m_bio_vec_stop)(struct msc_interface *);
	int 	(*m_bio_vec_stat)(struct msc_interface *);
	int 	(*m_bio_release_start)(struct msc_interface *, char *);
	int 	(*m_bio_release_stop)(struct msc_interface *, char *);
	int	(*m_mem_size)(struct msc_interface *);
	int	(*m_do_read)(struct msc_interface *, char *, off_t, int, char *);
};

struct msc_interface {
	void			*m_private;
	struct msc_operations	*m_op;
};

struct mpk_interface;
struct msc_setup {
	char			*ipstr;
	u_short			port;
	struct mpk_interface	*mpk;
};

extern int msc_initialization(struct msc_interface *, struct msc_setup *);
#endif
