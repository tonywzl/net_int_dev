/*
r * mrw_wnd_if.h
 *
 *  Created on: Nov 11, 2015
 *      Author: root
 */

#ifndef SRC_USER_MRW_WN_IF_H_
#define SRC_USER_MRW_WN_IF_H_

#include <sys/uio.h>
#include "node_if.h"
#include "mrw_wsn_if.h"

#define MRW_AWRITE_IOV_MAX 32		// Max vector size for a writev operation

struct mrw_wn {
	struct node_header      wa_nd_header;
	struct mrw_wsn     	*wa_fpw_node;
	struct timeval		wa_recv_time;
	struct timeval		wa_resp_time;
	// Below parameters just for writev only.
	// As write in our system is called seldom,
	// so make write arg and writev arg use same structure,
	// for simplify codes.
	char                    *wa_fp_buf;
	size_t			wa_fp_buf_num;
	int                     wa_iov_cnt;
	struct iovec            wa_iov[MRW_AWRITE_IOV_MAX];
	off_t                   wa_offset;
	size_t                  wa_more;
};

struct mrw_wn_interface;
struct mrw_wn_operations {
	struct mrw_wn*	(*wn_get_node)(struct mrw_wn_interface *);
	void		(*wn_put_node)(struct mrw_wn_interface *, struct mrw_wn *);
};

struct mrw_wn_interface {
	void				*wn_private;
	struct mrw_wn_operations	*wn_op;
};

struct allocator_interface;
struct mrw_wn_setup {
	struct allocator_interface	*allocator;
	int				seg_size;
};

extern int mrw_wn_initialization(struct mrw_wn_interface *, struct mrw_wn_setup *);

#endif /* SRC_USER_MRW_WN_IF_H_ */
