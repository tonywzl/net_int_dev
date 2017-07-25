/*
 * mgrall_if.h
 * 	Interface of Manager to ALL Modules
 */
#ifndef _MGRALL_IF_H
#define _MGRALL_IF_H

#include <sys/types.h>
#include "mgrtp_if.h"
#include "mgrpp_if.h"
#include "mgrbwc_if.h"
#include "mgrtwc_if.h"
#include "mgrcrc_if.h"
#include "mgrsds_if.h"
#include "mgrmrw_if.h"
#include "mgrdrw_if.h"
#include "mgrdx_if.h"
#include "mgrcx_if.h"
#include "mgrsac_if.h"

struct mgrall_interface;
struct mgrall_operations {
	int	(*al_display)(struct mgrall_interface *,
				struct mgrtp_interface *,
				struct mgrpp_interface *,
				struct mgrbwc_interface *,
				struct mgrtwc_interface *,
				struct mgrcrc_interface *,
				struct mgrsds_interface *,
				struct mgrmrw_interface *,
				struct mgrdrw_interface *,
				struct mgrdx_interface *,
				struct mgrcx_interface *,
				struct mgrsac_interface *);
};

struct mgrall_interface {
	void				*al_private;
	struct mgrall_operations	*al_op;
};

struct umpk_interface;
struct mgrall_setup {
	char			*ipstr;
	u_short			port;
	struct umpk_interface	*umpk;
};

extern int mgrall_initialization(struct mgrall_interface *, struct mgrall_setup *);
#endif
