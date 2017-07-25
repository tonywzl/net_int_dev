/*
 * mrw_wsnds_if.h
 *
 *  Created on: Nov 11, 2015
 *      Author: root
 */

#ifndef SRC_USER_MRW_WSN_IF_H_
#define SRC_USER_MRW_WSN_IF_H_

#include "node_if.h"
#include "rw_if.h"

struct mrw_wsn {
	struct node_header	fn_header;
	void			*fn_priv_p;
	struct timeval		fn_recv_time;
	struct timeval		fn_resp_time;
	volatile size_t         fn_not_done_fp_num;
	rw_callback_fn		fn_callback_upstair;
	struct rw_callback_arg	*fn_arg_upstair;
};

struct mrw_wsn_interface;
struct mrw_wsn_operations {
	struct mrw_wsn*	(*wsn_get_node)(struct mrw_wsn_interface *);
	void		(*wsn_put_node)(struct mrw_wsn_interface *, struct mrw_wsn *);
};

struct mrw_wsn_interface {
	void				*wsn_private;
	struct mrw_wsn_operations	*wsn_op;
};

struct allocator_interface;
struct mrw_wsn_setup {
	struct allocator_interface	*allocator;
	int				seg_size;
};

extern int mrw_wsn_initialization(struct mrw_wsn_interface *, struct mrw_wsn_setup *);

#endif /* SRC_USER_MRW_WSN_IF_H_ */
