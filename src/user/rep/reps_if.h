/*
 * reps_if.h
 * 	Interface of Replication Src Module
 */
#ifndef NID_REPS_IF_H
#define NID_REPS_IF_H
#include <stdbool.h>
#include <sys/uio.h>
#include <sys/time.h>
#include "list.h"
#include "rept_if.h"
#include "nid_shared.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "dxt_if.h"

#define NS_PAGE_SIZE 4096
#define FP_SIZE 32
#define BITMAP_LEN 1024
// To increase PP_SIZE and POOL_SIZE could improve the replication speed.
// However, larger PP_SIZE and POOL_SIZE will let replication occupies more memory.
#define PP_SIZE 32	//request number in each pool page
#define POOL_SIZE 16	//number of pool
#define PP_MARGIN 8	//controls the request limit, the greater margin is, the lower limit will be.
#define PAGE_SIZE_MIN 4

#define FP_TYPE_ZERO_PLACEHOLDER 1	//used on s, in *fp, there are zero fp for entries don't have value
#define FP_TYPE_NONE_ZERO 2		//from s to t, in *fp, there is fp without zero place holder
#define FP_TYPE_NONE_FP 3		//from t to s, in *fp, there is no fp, just missed bitmap
#define DATA_TYPE_NONE 4		//from s to t(fp pkg), or t to s(missed bitmap pkg), no data
#define DATA_TYPE_YES 5			//from s to t, has data
#define DATA_TYPE_DONE 6		//from t to s, data write done
#define DATA_TYPE_YES_OFFSET 7		//from s to t, has data, the offset will be used to update target p_write_data_size, it is for resuming repl

#define UMSG_REPS_REQ_FP 1
#define UMSG_REPS_REQ_DATA 2
#define UMSG_REPS_REQ_HEARTBEAT 3

#define UMSG_REPS_REQ_CODE_NO 0

#define REP_TIMEOUT 180

struct rep_data_node {
	struct list_head	rep_data_list;	//to save fp with zero place holder on source, for getting fp with missed bitmap
	off_t		offset;
	size_t		len;
	int 		fp_type;
	size_t		bitmap_len;
	size_t		fp_len;
	void		*rep_p;
	struct iovec	iov_data;
	bool		*bitmap;
	bool		*bitmap_miss;
	void		*fp;
	void		*data;
};

struct rep_fp_data_pkg {
	struct list_head	rep_pkg_list;	//to save data done pkg, for updating expect offset(breakpoint)
	char		voluuid[NID_MAX_UUID];
	off_t		offset;
	size_t		len;
	size_t		fp_pkg_size;	//total pkg size, including struct rep_fp_data_pkg
	int 		fp_type;
	size_t		bitmap_len;
	size_t		fp_len;
	int		data_type;
	char		data[];	//continuously store bitmap+fp+data
};

struct reps_interface;
struct reps_operations {
	char*			(*rs_get_name)(struct reps_interface *);
	int			(*rs_start)(struct reps_interface *);
	int			(*rs_start_snap)(struct reps_interface *, const char *);
	int			(*rs_start_snapdiff)(struct reps_interface *, const char *, const char *);
	float			(*rs_get_progress)(struct reps_interface *);
	int			(*rs_set_pause)(struct reps_interface *);
	int			(*rs_set_continue)(struct reps_interface *);
};

struct reps_interface {
	void			*rs_private;
	struct reps_operations	*rs_op;
};

struct reps_setup {
	char		rs_name[NID_MAX_UUID];
	char		rt_name[NID_MAX_UUID];
	char		dxa_name[NID_MAX_UUID];
	struct dxag_interface	*dxag_p;
	char		voluuid[NID_MAX_UUID];
	char		snapuuid[NID_MAX_UUID];
	char		snapuuid2[NID_MAX_UUID];
	size_t		bitmap_len;
	size_t		vol_len;
	struct allocator_interface	*allocator;
	struct tp_interface *tp;
	struct umpk_interface *umpk;
};

extern int reps_initialization(struct reps_interface *, struct reps_setup *);
#endif
