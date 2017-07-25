/*
 * rept_if.h
 * 	Interface of Replication Target Module
 */
#ifndef NID_REPT_IF_H
#define NID_REPT_IF_H
#include <stdbool.h>
#include <sys/uio.h>
#include "list.h"
#include "reps_if.h"
#include "nid_shared.h"
#include "dxp_if.h"
#include "dxpg_if.h"
#include "dxt_if.h"

#define UMSG_REPT_REQ_FP_MISSED 1
#define UMSG_REPT_REQ_DATA_WRITE_DONE 2

#define UMSG_REPT_REQ_CODE_NO 0


struct rept_interface;
struct rept_operations {
	char*			(*rt_get_name)(struct rept_interface *);
	void			(*rt_start)(struct rept_interface *);
	float                   (*rt_get_progress)(struct rept_interface *);
};

struct rept_interface {
	void			*rt_private;
	struct rept_operations	*rt_op;
};

struct rept_setup {
	char		rt_name[NID_MAX_UUID];
	char		dxp_name[NID_MAX_UUID];
	struct dxpg_interface	*dxpg_p;
	char		voluuid[NID_MAX_UUID];
	size_t		bitmap_len;
	size_t		vol_len;
	struct allocator_interface	*allocator;
	struct tp_interface		*tp;
};

extern int rept_initialization(struct rept_interface *, struct rept_setup *);
#endif
