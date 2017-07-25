/*
 * mrw_rnd_if.h
 *
 *  Created on: Nov 11, 2015
 *      Author: root
 */

#ifndef SRC_USER_MRW_RN_IF_H_
#define SRC_USER_MRW_RN_IF_H_

#include "node_if.h"
#include "rw_if.h"

struct mrw_rn {
	struct node_header      rc_nd_header;
	void			*rc_buf;
	size_t                  rc_count;
	rw_callback_fn          rc_callback_upstair;
	struct rw_callback_arg  *rc_arg_upstair;
	struct mrw_private      *rc_priv_p;
	struct timeval		rc_recv_time;
	struct timeval		rc_resp_time;
};

struct mrw_rn_interface;
struct mrw_rn_operations {
	struct mrw_rn*	(*rn_get_node)(struct mrw_rn_interface *);
	void		(*rn_put_node)(struct mrw_rn_interface *, struct mrw_rn *);
};

struct mrw_rn_interface {
	void				*rn_private;
	struct mrw_rn_operations	*rn_op;
};

struct allocator_interface;
struct mrw_rn_setup {
	struct allocator_interface	*allocator;
	int				seg_size;
};

extern int mrw_rn_initialization(struct mrw_rn_interface *, struct mrw_rn_setup *);

#endif /* SRC_USER_MRW_RN_IF_H_ */
