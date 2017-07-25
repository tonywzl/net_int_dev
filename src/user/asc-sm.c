/*
 * asc-sm.c
 * 	Implementation of Agent Service Channel Module: State Machine
 */

#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid.h"
#include "nid_shared.h"
#include "mpk_if.h"
#include "adt_if.h"
#include "ascg_if.h"
#include "asc.h"
#include "asc_if.h"
#include "dsm_if.h"
#include "pp2_if.h"

/*
 * Return:
 * 	1: have delay job
 * 	0: done
 */
#if 0
int
asc_sm_i_housekeeping(void *data)
{
	char *log_header = "asc_sm_i_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *job = priv_p->p_start_job;
	nid_log_debug("%s: start (%s, need_start:%d)...",
		log_header, priv_p->p_devname, priv_p->p_need_start);
	if (priv_p->p_need_start) {
		priv_p->p_need_start = 0;
		tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, 0);
		return 1;
	}
	return 0;
}
#else
int
asc_sm_i_housekeeping(void *data)
{
	char *log_header = "asc_sm_i_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *job = priv_p->p_start_job;
	int job_rejected;

	nid_log_debug("%s: start (%s, need_start:%d)...",
			log_header, priv_p->p_devname, priv_p->p_need_start);

	asc_p->as_op->as_op_get(asc_p);
	job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, 0);
	/*
	 * Need to put operation here as it won't be put inside the job due to job rejected
	 */
	if (job_rejected)
		asc_p->as_op->as_op_put(asc_p);

	return 0;
}
#endif

/*
 * Return:
 * 	 0: done
 * 	 1: there is some delay job to do
 * 	-1: this state has something wrong
 */
#if 0
int
asc_sm_w_housekeeping(void *data)
{
	char *log_header = "asc_sm_w_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int chan_id = priv_p->p_chan_id;
	int chan_type = priv_p->p_chan_type;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *iw_job = priv_p->p_iw_job;
	struct asc_job *rw_job;
	struct tp_jobheader *jh_p;
	struct pp2_interface *dyn_pp2_p = priv_p->p_dyn_pp2;
	int rc = 0;

	nid_log_debug("%s: start %s...", log_header, priv_p->p_devname);
	if (priv_p->p_to_recover) {
		nid_log_debug("%s: skip to_recover %s", log_header, priv_p->p_devname);
		return 0;
	}

	if (priv_p->p_need_iw_hook) {
		priv_p->p_need_iw_hook = 0;
		jh_p = (struct tp_jobheader *)iw_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 5);
		rc = 1;
	} else if (priv_p->p_need_rw_hook) {
		priv_p->p_need_rw_hook = 0;

		rw_job = (struct asc_job *)dyn_pp2_p->pp2_op->pp2_get(dyn_pp2_p, sizeof(*rw_job));
		*rw_job = *(priv_p->p_rw_job);
		jh_p = (struct tp_jobheader *)rw_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 5);
		rc = 1;
	}

	if (priv_p->p_ka_rtimer)
		priv_p->p_ka_rtimer--;
	if (!(priv_p->p_ka_rtimer)) {
		nid_log_notice("%s: timeout %s", log_header, priv_p->p_devname);

		if (priv_p->p_rsfd >= 0) {
			close(priv_p->p_rsfd);
			priv_p->p_rsfd = -1;
		}
		if (priv_p->p_dsfd >= 0) {
			close(priv_p->p_dsfd);
			priv_p->p_dsfd = -1;
		}

		priv_p->p_to_recover = 1;
		ascg_p->ag_op->ag_xdev_recover_channel(ascg_p, chan_type, chan_id);
		return rc;
	}

	priv_p->p_ka_xtimer--;
	if (!priv_p->p_ka_xtimer) {
		/* do keepalive message */
		asc_p->as_op->as_xdev_keepalive_channel(asc_p);
		priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	}
	return rc;
}
#else
int
asc_sm_w_housekeeping(void *data)
{
	char *log_header = "asc_sm_w_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int chan_id = priv_p->p_chan_id;
	int chan_type = priv_p->p_chan_type;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *iw_job = priv_p->p_iw_job;
	struct asc_job *rw_job = priv_p->p_rw_job;
	struct tp_jobheader *jh_p;
	int job_rejected;

	nid_log_debug("%s: start %s...", log_header, priv_p->p_devname);
	if (priv_p->p_to_recover) {
		nid_log_debug("%s: skip to_recover %s", log_header, priv_p->p_devname);
		return 0;
	}

	if (iw_job) {
		asc_p->as_op->as_op_get(asc_p);
		jh_p = (struct tp_jobheader *)iw_job;
		job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 5);
		/*
		 * Need to put operation here as it won't be put inside the job due to job rejected
		 */
		if (job_rejected)
			asc_p->as_op->as_op_put(asc_p);
	}

	if (rw_job) {
		asc_p->as_op->as_op_get(asc_p);
		jh_p = (struct tp_jobheader *)rw_job;
		job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 5);
		/*
		 * Need to put operation here as it won't be put inside the job due to job rejected
		 */
		if (job_rejected)
			asc_p->as_op->as_op_put(asc_p);
	}

	if (priv_p->p_ka_rtimer)
		priv_p->p_ka_rtimer--;
	if (!(priv_p->p_ka_rtimer)) {
		nid_log_error("%s: timeout %s", log_header, priv_p->p_devname);

		if (priv_p->p_rsfd >= 0) {
			close(priv_p->p_rsfd);
			priv_p->p_rsfd = -1;
		}
		if (priv_p->p_dsfd >= 0) {
			close(priv_p->p_dsfd);
			priv_p->p_dsfd = -1;
		}

		priv_p->p_to_recover = 1;
		ascg_p->ag_op->ag_xdev_recover_channel(ascg_p, chan_type, chan_id);
		return 0;
	}

	priv_p->p_ka_xtimer--;
	if (!priv_p->p_ka_xtimer) {
		/* do keepalive message */
		asc_p->as_op->as_xdev_keepalive_channel(asc_p);
		priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	}
	return 0;
}
#endif

/*
 * Return:
 * 	0: not start
 * 	1: trigger a start jobd
 */
#if 0
int
asc_sm_r_housekeeping(void *data)
{
	char *log_header = "asc_sm_r_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *start_job = priv_p->p_start_job;
	struct asc_job *wr_job = priv_p->p_wr_job;
	struct tp_jobheader *jh_p;

	nid_log_debug("%s: start(%s, need_start:%d) ...",
		log_header, priv_p->p_devname, priv_p->p_need_start);
	if (priv_p->p_need_wr_hook && !priv_p->p_wr_timeout) {
		priv_p->p_need_wr_hook = 0;
		jh_p = (struct tp_jobheader *)wr_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		return 1;
	} else {
		priv_p->p_wr_timeout--;
	}

	if (priv_p->p_need_start) {
		priv_p->p_need_start = 0;
		jh_p = (struct tp_jobheader *)start_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		return 1;
	}
	return 0;
}
#else
int
asc_sm_r_housekeeping(void *data)
{
	char *log_header = "asc_sm_r_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *start_job = priv_p->p_start_job;
	struct asc_job *wr_job = priv_p->p_wr_job;
	struct tp_jobheader *jh_p;
	int job_rejected;

	nid_log_debug("%s: start(%s, need_start:%d) ...",
		log_header, priv_p->p_devname, priv_p->p_need_start);

	if (wr_job) {
		asc_p->as_op->as_op_get(asc_p);
		jh_p = (struct tp_jobheader *)wr_job;
		job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		/*
		 * Need to put operation here as it won't be put inside the job due to job rejected
		 */
		if (job_rejected)
			asc_p->as_op->as_op_put(asc_p);
	}

	asc_p->as_op->as_op_get(asc_p);
	jh_p = (struct tp_jobheader *)start_job;
	job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
	/*
	 * Need to put operation here as it won't be put inside the job due to job rejected
	 */
	if (job_rejected)
		asc_p->as_op->as_op_put(asc_p);

	return 0;
}
#endif

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_rw_housekeeping(void *data)
{
	char *log_header = "asc_sm_rw_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	nid_log_debug("%s: start [uuid:%s]...", log_header, priv_p->p_uuid);
	return 0;
}

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_iw_housekeeping(void *data)
{
	char *log_header = "asc_sm_iw_housekeeping";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	nid_log_debug("%s: start [uuid:%s] ...", log_header, priv_p->p_uuid);
	return 0;
}

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_xdev_delete_device(void *data)
{
	char *log_header = "asc_sm_xdev_delete_device";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int minor = priv_p->p_chan_id;

	nid_log_notice("%s: start [%s, minor:%d] ...",
		log_header, priv_p->p_devname, priv_p->p_chan_id);
	ascg_p->ag_op->ag_xdev_delete_device(ascg_p, minor);
	return 0;
}

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_xdev_init_device(void *data)
{
	char *log_header = "asc_sm_xdev_init_device";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int minor = priv_p->p_chan_id;
	uint64_t size = priv_p->p_size & (~(4096UL - 1));
	uint32_t blksize = priv_p->p_blksize;

	nid_log_notice("%s: minor:%d", log_header, minor);
	ascg_p->ag_op->ag_xdev_init_device(ascg_p, minor, size, blksize);
	return 0;
}

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_xdev_start_channel(void *data)
{
	char *log_header = "asc_sm_xdev_start_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int chan_id = priv_p->p_chan_id;
	uint8_t chan_type = priv_p->p_chan_type;
	int rsfd = priv_p->p_rsfd;
	int dsfd = priv_p->p_dsfd;

	nid_log_notice("%s: chan_id:%d", log_header, chan_id);
	ascg_p->ag_op->ag_xdev_start_channel(ascg_p, chan_id, chan_type, rsfd, dsfd);
	priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	return 0;
}

/*
 * Return:
 * 	1: expecting rdev_ioerror to finish
 */
int
asc_sm_xdev_ioerror_device(void *data)
{
	char *log_header = "asc_sm_xdev_ioerror_device";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int minor = priv_p->p_chan_id;

	nid_log_warning("%s: start [%s, minor:%d]...", log_header, priv_p->p_devname, minor);
	ascg_p->ag_op->ag_xdev_ioerror_device(ascg_p, minor);
	return 1;
}

/*
 * Return:
 * 	Always 0
 */
int
asc_sm_xdev_keepalive_channel(void *data)
{
	char *log_header = "asc_sm_xdev_keepalive_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int chan_id = priv_p->p_chan_id;
	int chan_type = priv_p->p_chan_type;

	nid_log_debug("%s: start (chan_type:%s, chan_id:%d) ...", log_header, priv_p->p_chan_typestr, chan_id);
	ascg_p->ag_op->ag_xdev_keepalive_channel(ascg_p, chan_type, chan_id);
	priv_p->p_ka_xtimer = NID_KEEPALIVE_XTIME;
	return 0;
}

/*
 * Return:
 * 	0: there is no rw_job to do, done.
 * 	1: trigger iw_job as a delayed job
 */
#if 0
int
asc_sm_rdev_rw_start_channel(void *data)
{
	char *log_header = "asc_sm_rdev_rw_start_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *rw_job;
	struct tp_jobheader *jh_p;
	struct pp2_interface *dyn_pp2_p = priv_p->p_dyn_pp2;

	nid_log_notice("%s: start[%s] ...", log_header, priv_p->p_devname);
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	if (priv_p->p_rw_job) {
		rw_job = (struct asc_job *)dyn_pp2_p->pp2_op->pp2_get(dyn_pp2_p, sizeof(*rw_job));
		*rw_job = *(priv_p->p_rw_job);
		jh_p = (struct tp_jobheader *)rw_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		return 1;
	} else {
		return 0;
	}
}
#else
int
asc_sm_rdev_rw_start_channel(void *data)
{
	char *log_header = "asc_sm_rdev_rw_start_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *rw_job = priv_p->p_rw_job;
	struct tp_jobheader *jh_p;
	int job_rejected;

	nid_log_notice("%s: start[%s] ...", log_header, priv_p->p_devname);
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	if (priv_p->p_rw_job) {
		priv_p->p_need_rw_hook = 1;
		jh_p = (struct tp_jobheader *)rw_job;
		job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		/*
		 * The caller (asc_rdev_start_channel()) will put operation if 0 returned (job rejected).
		 * Otherwise, the job will put the operation itself.
		 */
		return !job_rejected;
	} else {
		return 0;
	}
}
#endif

/*
 * Return:
 * 	0: there is no iw_job to do, done.
 * 	1: trigger iw_job as a delayed job
 */
#if 0
int
asc_sm_rdev_iw_start_channel(void *data)
{
	char *log_header = "asc_sm_rdev_iw_start_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *iw_job = priv_p->p_iw_job;
	struct tp_jobheader *jh_p;

	nid_log_notice("%s: start [%s]...", log_header, priv_p->p_devname);
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	if (iw_job) {
		jh_p = (struct tp_jobheader *)iw_job;
		tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		return 1;
	} else {
		return 0;
	}
}
#else
int
asc_sm_rdev_iw_start_channel(void *data)
{
	char *log_header = "asc_sm_rdev_iw_start_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct asc_job *iw_job = priv_p->p_iw_job;
	struct tp_jobheader *jh_p;
	int job_rejected;

	nid_log_notice("%s: start [%s]...", log_header, priv_p->p_devname);
	priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	if (iw_job) {
		priv_p->p_need_iw_hook = 1;
		jh_p = (struct tp_jobheader *)iw_job;
		job_rejected = tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		/*
		 * The caller (asc_rdev_start_channel()) will put operation if 0 returned (job rejected).
		 * Otherwise, the job will put the operation itself.
		 */
		return !job_rejected;
	} else {
		return 0;
	}
}
#endif

/*
 * Return:
 * 	Always 0
 */
#if 0
int
asc_sm_rdev_recover_channel(void *data)
{
	char *log_header = "asc_sm_rdev_recover_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct asc_job *wr_job = priv_p->p_wr_job;

	nid_log_notice("%s: start %s ...", log_header, priv_p->p_devname);
	close(priv_p->p_rsfd);
	priv_p->p_rsfd = -1;
	close(priv_p->p_dsfd);
	priv_p->p_dsfd = -1;
	priv_p->p_to_recover = 0;
	priv_p->p_need_start = 1;
	if (priv_p->p_wr_hook) {
		if (wr_job) {
			priv_p->p_need_wr_hook = 1;
			priv_p->p_wr_timeout = priv_p->p_wr_timewait;
		} else {
			priv_p->p_need_wr_hook = 0;
		}
		priv_p->p_wr_action_done = 0;
	}
	return 0;
}
#else
int
asc_sm_rdev_recover_channel(void *data)
{
	char *log_header = "asc_sm_rdev_recover_channel";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct asc_job *wr_job = priv_p->p_wr_job;

	nid_log_warning("%s: start %s ...", log_header, priv_p->p_devname);
	close(priv_p->p_rsfd);
	priv_p->p_rsfd = -1;
	close(priv_p->p_dsfd);
	priv_p->p_dsfd = -1;
	priv_p->p_to_recover = 0;
	priv_p->p_need_start = 1;
	if (wr_job) {
		priv_p->p_need_wr_hook = 1;
		priv_p->p_wr_timeout = priv_p->p_wr_timewait;
	}
	priv_p->p_wr_action_done = 0;
	return 0;
}
#endif

int
asc_sm_rdev_keepalive(void *data)
{
	char *log_header = "asc_sm_rdev_keepalive";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;

	nid_log_debug("%s: start ...", log_header);


	if (priv_p->p_srv_recv_seq != priv_p->p_srv_save_recv_seq) {
		priv_p->p_srv_save_recv_seq = priv_p->p_srv_recv_seq;
		priv_p->p_srv_save_wait_seq = priv_p->p_srv_wait_seq;
		priv_p->p_srv_save_count = 0;
	} else if (priv_p->p_srv_recv_seq < priv_p->p_srv_wait_seq) {
		if (priv_p->p_srv_status == ASC_WHERE_BUSY){
			nid_log_error("%s: nidsever side request buffer busy,  count:%d, save_recv:%lu, save_wait:%lu, recv:%lu, wait:%lu",
				log_header, priv_p->p_srv_save_count, priv_p->p_srv_recv_seq, priv_p->p_srv_wait_seq, priv_p->p_srv_save_recv_seq, priv_p->p_srv_save_wait_seq);
			priv_p->p_srv_save_count = 0;
		} else {
			nid_log_error("%s: data connection stuck,  count:%d, save_recv:%lu, save_wait:%lu,recv:%lu, wait:%lu",
				log_header, priv_p->p_srv_save_count, priv_p->p_srv_recv_seq, priv_p->p_srv_wait_seq, priv_p->p_srv_save_recv_seq, priv_p->p_srv_save_wait_seq);
			priv_p->p_srv_save_count++;
		}
	}

	if (priv_p->p_srv_save_count < 10) {
		priv_p->p_ka_rtimer = NID_KEEPALIVE_TO;
	} else {
		nid_log_error("%s: forcibly timeout due to data connection issue, count:%d, recv:%lu, wait:%lu",
			log_header, priv_p->p_srv_save_count, priv_p->p_srv_save_recv_seq, priv_p->p_srv_save_wait_seq);
		priv_p->p_ka_rtimer = 0;	// timeout in next housekeeping
	}
	return 0;
}

int
asc_sm_rdev_delete_device(void *data)
{
	char *log_header = "asc_sm_rdev_delete_device";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	nid_log_notice("%s: %s", log_header, priv_p->p_devname);
	return 0;
}

/*
 * Return:
 * 	0: done
 */
int
asc_sm_rdev_ioerror_device(void *data)
{
	char *log_header = "asc_sm_rdev_ioerror_device";
	struct asc_interface *asc_p = (struct asc_interface *)data;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	nid_log_warning("%s: %s", log_header, priv_p->p_devname);
	return 0;
}

