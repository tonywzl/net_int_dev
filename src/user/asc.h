/*
 * asc.h
 */

#include "tp_if.h"
#include "asc_if.h"

#define		ASC_WHERE_FREE 	0
#define		ASC_WHERE_BUSY  1

struct asc_job {
	struct tp_jobheader     header;
	int			res;
	int			delay;
	struct tp_interface	*tp;
	struct asc_interface	*asc;
	struct ascg_interface	*ascg;
	struct ash_interface	*ash;
	void			*ascg_chan;
	char			*cmd;
};

struct asc_private {
	int			p_chan_type;
	char			p_chan_typestr[16];
	struct mpk_interface	*p_mpk;
	struct tp_interface	*p_tp;
	struct ash_interface	*p_ash;
	struct ascg_interface	*p_ascg;
	struct dsm_interface	*p_dsm;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn_pp2;
	pthread_mutex_t		p_op_lck;
	pthread_mutex_t		p_ex_lck;
	pthread_mutex_t		p_drv_lck;
	pthread_cond_t		p_drv_cond;
	void			*p_ascg_chan;
	struct asc_job		*p_start_job;
	struct asc_job		*p_wr_job;
	struct asc_job		*p_iw_job;
	struct asc_job		*p_rw_job;
	struct asc_job		*p_up_job;
	char			p_sip[NID_MAX_IP];
	char			p_uuid[NID_MAX_UUID];
	char			p_devname[NID_MAX_DEVNAME];
	int			p_fd;
	int			p_ioctl_fd;
	int			p_wr_timewait;	// wr: work -> recover
	int			p_wr_timeout;
	int			p_ka_xtimer;	// ka: send keepalive when decreased to 0
	int			p_ka_rtimer;	// timeout when decreased to 0
	char			p_iw_hook[NID_MAX_CMD];
	char			p_wr_hook[NID_MAX_CMD];
	char			p_rw_hook[NID_MAX_CMD];
	char			p_up_hook[NID_MAX_CMD];
	uint32_t		p_op_counter;	// how many operations in processing
	uint64_t		p_size;
	uint32_t		p_blksize;
	int			p_rsfd;
	int			p_dsfd;
	uint64_t		p_srv_recv_seq;
	uint64_t		p_srv_wait_seq;
	uint64_t		p_srv_save_recv_seq;
	uint64_t		p_srv_save_wait_seq;
	uint32_t		p_srv_status;
	int			p_srv_save_count;
	int			p_chan_id;	// minor of the device for "asc" type
	struct asc_stat		p_stat;
	uint8_t			p_alevel;
	char			p_busy;
	char			p_done;
	char			p_established;
	uint8_t			p_drv_init_code;
	char			p_wr_action_done;
	char			p_doing_op;
	char			p_doing_ex;
	char			p_to_recover;
	char			p_need_start;
	char			p_need_wr_hook;
	char			p_need_iw_hook;
	char			p_need_rw_hook;
	char			p_wr_hook_invoked; // Issue USX-76924: As per milio, rw hook requires a previous wr hook successfully done.
	char			p_doing_job[NID_MAX_CMD];
};
