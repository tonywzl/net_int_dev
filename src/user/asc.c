/*
 * asc.c
 * 	Implementation of Agent Service Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>

#include "util_nw.h"
#include "nid_log.h"
#include "agent.h"
#include "mpk_if.h"
#include "tp_if.h"
#include "ash_if.h"
#include "asc_if.h"
#include "asc.h"
#include "adt_if.h"
#include "ascg_if.h"
#include "dsm_if.h"
#include "pp2_if.h"

#if 0
static void
asc_free_job(struct tp_jobheader *jh_p)
{
	struct asc_job *job = (struct asc_job *)jh_p;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)jh_p);
}

static void
asc_free_dyn_job(struct tp_jobheader *jh_p)
{
	struct asc_job *job = (struct asc_job *)jh_p;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct pp2_interface *dyn_pp2_p = priv_p->p_dyn_pp2;

	dyn_pp2_p->pp2_op->pp2_put(dyn_pp2_p, (void *)jh_p);
}
#endif

static void
_do_wr_hook_job(struct tp_jobheader *header)
{
	char *log_header = "_do_wr_hook_job";
	struct asc_job *job = (struct asc_job *)header;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	char *cmd = job->cmd;
	FILE *fp;
	char buff[1024];
	int rc = -2, do_it = 0;
	struct ascg_interface *ascg_p = job->ascg;

	nid_log_notice("%s: start, %s cmd:{%s}",
		log_header, priv_p->p_devname, cmd);

	if (!priv_p->p_need_wr_hook) {
		nid_log_notice("%s: do not need wr hook job, need_wr_hook:%d",
				log_header, priv_p->p_need_wr_hook);
		asc_p->as_op->as_op_put(asc_p);
		return;
	}
	if (priv_p->p_wr_timeout) {
		nid_log_notice("%s: do not need wr hook job, wr_timeout:%d",
				log_header, priv_p->p_wr_timeout);
		priv_p->p_wr_timeout--;
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	if (!ascg_p->ag_op->ag_get_hook(ascg_p)) {
		nid_log_notice("%s: hook function is disabled, will retry.", log_header);
		priv_p->p_need_wr_hook = 1;
		priv_p->p_wr_timeout = priv_p->p_wr_timewait;
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	pthread_mutex_lock(&priv_p->p_op_lck);
	if (!priv_p->p_doing_op) {
		do_it = 1;
		priv_p->p_doing_op = 1;
		sprintf(priv_p->p_doing_job, "_do_wr_hook_job");
	}
	pthread_mutex_unlock(&priv_p->p_op_lck);

	if (!do_it) {
		nid_log_debug("%s: %s is running for %s",
			log_header, priv_p->p_doing_job, priv_p->p_devname);
		goto out;
	}

	fp = popen(cmd, "r");
	if (fp) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			nid_log_debug("%s", buff);
		}
		rc = pclose(fp);
	}

	priv_p->p_wr_hook_invoked = 1;
	if (!rc) {
		priv_p->p_need_wr_hook = 0;
	} else {
		priv_p->p_need_wr_hook = 1;
		if (rc == 11)
			priv_p->p_wr_timeout = 1;	// re-try quickly if failed due to locking issue
		else
			priv_p->p_wr_timeout = priv_p->p_wr_timewait;
	}

out:
	if (do_it) {
		pthread_mutex_lock(&priv_p->p_op_lck);
		priv_p->p_doing_op = 0;
		*(priv_p->p_doing_job) = '\0';
		pthread_mutex_unlock(&priv_p->p_op_lck);
	}
	nid_log_debug("%s: done, %s rc:%d",
		log_header, priv_p->p_devname, rc);
	nid_log_debug("%s: a_op_put %s", log_header, priv_p->p_devname);
	asc_p->as_op->as_op_put(asc_p);
}

static void
_do_iw_hook_job(struct tp_jobheader *header)
{
	char *log_header = "_do_iw_hook_job";
	struct asc_job *job = (struct asc_job *)header;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	char *cmd = job->cmd;
	FILE *fp;
	char buff[1024];
	int rc = -2, do_it = 0;
	struct ascg_interface *ascg_p = job->ascg;

	nid_log_notice("%s: start, %s cmd:{%s}",
		log_header, priv_p->p_devname, cmd);

	if (!priv_p->p_need_iw_hook) {
		nid_log_notice("%s: do not need iw hook job", log_header);
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	if (!ascg_p->ag_op->ag_get_hook(ascg_p)) {
		nid_log_notice("%s: hook function is disabled, will retry.", log_header);
		//priv_p->p_need_iw_hook = 1;
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	pthread_mutex_lock(&priv_p->p_op_lck);
	if (!priv_p->p_doing_op) {
		do_it = 1;
		priv_p->p_doing_op = 1;
		sprintf(priv_p->p_doing_job, "_do_iw_hook_job");
	}
	pthread_mutex_unlock(&priv_p->p_op_lck);
	if (!do_it) {
		nid_log_debug("%s: %s is running, give up, %s",
			log_header, priv_p->p_doing_job, priv_p->p_devname);
		goto out;
	}

	fp = popen(cmd, "r");
	if (fp) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			nid_log_debug("%s", buff);
		}
		rc = pclose(fp);
	}
	if (!rc)
		priv_p->p_need_iw_hook = 0;
	else
		priv_p->p_need_iw_hook = 1;

out:
	if (do_it) {
		pthread_mutex_lock(&priv_p->p_op_lck);
		priv_p->p_doing_op = 0;
		*(priv_p->p_doing_job) = '\0';
		pthread_mutex_unlock(&priv_p->p_op_lck);
	} else {
		priv_p->p_need_iw_hook = 1;
	}
	nid_log_debug("%s: done, do_it:%d, rc:%d, %s done",
		log_header, do_it, rc, priv_p->p_devname);
	nid_log_debug("%s: a_op_put %s",
		log_header, asc_p->as_op->as_get_devname(asc_p));
	asc_p->as_op->as_op_put(asc_p);
}

static void
_do_rw_hook_job(struct tp_jobheader *header)
{
	char *log_header = "_do_rw_hook_job";
	struct asc_job *job = (struct asc_job *)header;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	char *cmd = job->cmd;
	FILE *fp;
	char buff[1024];
	int rc = -2, do_it = 0;
	struct ascg_interface *ascg_p = job->ascg;

	nid_log_notice("%s: start, %s cmd:{%s}",
		log_header, priv_p->p_devname, cmd);

	if (!priv_p->p_need_rw_hook) {
		nid_log_notice("%s: do not need rw hook job", log_header);
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	if (!ascg_p->ag_op->ag_get_hook(ascg_p)) {
		nid_log_notice("%s: hook function is disabled, will retry.", log_header);
		priv_p->p_need_rw_hook = 1;
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	pthread_mutex_lock(&priv_p->p_op_lck);
	if (!priv_p->p_doing_op) {
		do_it = 1;
		priv_p->p_doing_op = 1;
		sprintf(priv_p->p_doing_job, "_do_rw_hook_job");
	}
	pthread_mutex_unlock(&priv_p->p_op_lck);
	if (!do_it) {
		nid_log_debug("%s: %s is running, give up, %s",
			log_header, priv_p->p_doing_job, priv_p->p_devname);
		goto out;
	}

	/*
	 * Put the wr_hook_done judgment here in case the previous wr hook is ongoing
	 */
	if (!priv_p->p_wr_hook_invoked) {
		nid_log_warning("%s: do not have a previous wr hook successfully done", log_header);
		priv_p->p_need_rw_hook = 0;
		pthread_mutex_lock(&priv_p->p_op_lck);
		priv_p->p_doing_op = 0;
		*(priv_p->p_doing_job) = '\0';
		pthread_mutex_unlock(&priv_p->p_op_lck);
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	fp = popen(cmd, "r");
	if (fp) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			nid_log_debug("%s", buff);
		}
		rc = pclose(fp);
	}
	if (!rc) {
		priv_p->p_need_rw_hook = 0;
		priv_p->p_wr_hook_invoked = 0; // clear the wr_hook_done flag
	} else {
		priv_p->p_need_rw_hook = 1;
	}

out:
	if (do_it) {
		pthread_mutex_lock(&priv_p->p_op_lck);
		priv_p->p_doing_op = 0;
		*(priv_p->p_doing_job) = '\0';
		pthread_mutex_unlock(&priv_p->p_op_lck);
	} else {
		priv_p->p_need_rw_hook = 1;
	}
	nid_log_debug("%s: done, do_it:%d, rc:%d, %s done",
			log_header, do_it, rc, priv_p->p_devname);
	nid_log_debug("%s: a_op_put %s",
			log_header, asc_p->as_op->as_get_devname(asc_p));
	asc_p->as_op->as_op_put(asc_p);
}

static void
_do_up_hook_job(struct tp_jobheader *header)
{
	char *log_header = "_do_up_hook_job";
	struct asc_job *job = (struct asc_job *)header;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	char *cmd = job->cmd;
	FILE *fp;
	char buff[1024];
	int rc = -2;
	struct ascg_interface *ascg_p = job->ascg;

	nid_log_notice("%s: start, %s cmd:{%s}",
		log_header, priv_p->p_devname, cmd);

	if (!ascg_p->ag_op->ag_get_hook(ascg_p)) {
		nid_log_notice("%s: hook function is disabled", log_header);
		return;
	}

	fp = popen(cmd, "r");
	if (fp) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			nid_log_debug("%s", buff);
		}
		rc = pclose(fp);
	}
	nid_log_debug("%s: %s done, rc:%d", log_header, priv_p->p_devname, rc);
}

static void
_start_job_do(struct tp_jobheader *header)
{
	char *log_header = "_start_job_do";
	struct asc_job *job = (struct asc_job *)header;
	struct asc_interface *asc_p = job->asc;
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = job->ascg;
	struct ash_interface *ash_p = job->ash;
	struct ash_channel_info c_info;
	int rc, do_it = 0, got_it = 0;

	nid_log_debug("%s: create channel %s",
		log_header, priv_p->p_devname);

	if (!priv_p->p_need_start) {
		nid_log_notice("%s: do not need start job", log_header);
		asc_p->as_op->as_op_put(asc_p);
		return;
	}

	pthread_mutex_lock(&priv_p->p_op_lck);
	if (!priv_p->p_doing_op) {
		do_it = 1;
		priv_p->p_doing_op = 1;
		sprintf(priv_p->p_doing_job, "_start_job_do");
	}
	pthread_mutex_unlock(&priv_p->p_op_lck);
	if (!do_it) {
		nid_log_debug("%s: %s is running, give up, %s",
			log_header, priv_p->p_doing_job, priv_p->p_devname);
		goto out;
	}

	rc = ash_p->h_op->h_create_channel(ash_p, &c_info);
	if (rc) {
		ascg_p->ag_op->ag_failed_to_establish_channel(ascg_p, job->ascg_chan);
		nid_log_debug("%s: failed, retry later, %s",
			log_header, priv_p->p_devname);
		goto out;
	} else {
		priv_p->p_rsfd = c_info.i_rsfd;
		priv_p->p_dsfd = c_info.i_dsfd;
		priv_p->p_size = c_info.i_size;

		/*
		 * by default, the service will give us NID_ALEVEL_RDONLY alevel.
		 * we need upgrade access level if necessary
		 */
		if (priv_p->p_alevel > NID_ALEVEL_RDONLY)
			rc = asc_p->as_op->as_upgrade_alevel(asc_p);
		if (rc) {
			ascg_p->ag_op->ag_failed_to_establish_channel(ascg_p, job->ascg_chan);
			nid_log_debug("%s: failed, retry later, %s",
				log_header, priv_p->p_devname);
			close(priv_p->p_rsfd);
			priv_p->p_rsfd = -1;
			close(priv_p->p_dsfd);
			priv_p->p_dsfd = -1;
			priv_p->p_size = 0;
			goto out;
		}

		if ((priv_p->p_chan_type == NID_CHANNEL_TYPE_ASC_CODE) && !priv_p->p_established) {
			nid_log_debug("%s: calling as_xdev_init_device, %s",
				log_header, priv_p->p_devname);
			rc = asc_p->as_op->as_xdev_init_device(asc_p);
			if (rc) {
				nid_log_debug("%s: as_xdev_init_device failed, %s",
					log_header, priv_p->p_devname);
				goto out;
			}

			/*
			 * Waiting for the driver to finish init
			 */
			pthread_mutex_lock(&priv_p->p_drv_lck);
			while (!priv_p->p_established) {
				pthread_cond_wait(&priv_p->p_drv_cond, &priv_p->p_drv_lck);
				if(priv_p->p_drv_init_code == 1 || priv_p->p_drv_init_code == 2 || priv_p->p_drv_init_code == 3) {
					nid_log_warning("%s: failed to init driver, parameter wrong", log_header);
					assert(0);
				}
			}
			pthread_mutex_unlock(&priv_p->p_drv_lck);
		}

		nid_log_debug("%s: calling a_xdev_start_channel, %s",
			log_header, priv_p->p_devname);
		ascg_p->ag_op->ag_ioctl_lock(ascg_p);
		asc_p->as_op->as_xdev_start_channel(asc_p);
		if (priv_p->p_ioctl_fd < 0) {
			priv_p->p_ioctl_fd = open(AGENT_RESERVED_DEV, O_RDWR|O_CLOEXEC);
			assert(priv_p->p_ioctl_fd >= 0);
		}
		ioctl(priv_p->p_ioctl_fd, NID_START_CHANNEL, 0);
		ascg_p->ag_op->ag_ioctl_unlock(ascg_p);
		got_it = 1;
	}

out:
	if (do_it) {
		pthread_mutex_lock(&priv_p->p_op_lck);
		priv_p->p_doing_op = 0;
		*(priv_p->p_doing_job) = '\0';
		pthread_mutex_unlock(&priv_p->p_op_lck);
	}
	nid_log_debug("%s: a_op_put %s", log_header, priv_p->p_devname);

	if (got_it)
		priv_p->p_need_start = 0;
	else
		priv_p->p_need_start = 1;

	asc_p->as_op->as_op_put(asc_p);
}

#if 0
static int
_io_testing(int rsfd, int dsfd, uint64_t *d_sequence)
{
	struct nid_request write_req;
	int len = 4096;
	char wbuf[4096] = {'A'}, resp_buf[4096], *p;
	int i, to_read, nread, num = 100;
	struct nid_request *ir_p;

	for (i = 0; i < num; i++) {
		write_req.cmd = NID_REQ_WRITE;
		write_req.len = htonl(len);
		write_req.offset = htonl(0);
		write_req.dseq = htobe64(*d_sequence);
		*d_sequence += len;
		if (write(rsfd, &write_req, NID_SIZE_REQHDR) != NID_SIZE_REQHDR ||
		    write(dsfd, wbuf, len) != len) {
			nid_log_error("_io_testing (rsfd:%d, dsfd:%d): cannot write, errno:%d",
				rsfd, dsfd, errno);
			return -1;
		}
	}

	to_read = num * NID_SIZE_REQHDR;
	p = resp_buf;
	while (to_read) {
		nread = util_nw_read_n_timeout(rsfd, p, 4096, 1, 2);
		if (nread <= 0) {
			nid_log_error("_io_testing: cannot read");
			return -1;
		}
		to_read -= nread;
		p += nread;
	}
	ir_p = (struct nid_request *)resp_buf;
	if (ir_p->code)
		nid_log_warning("_io_testing: write error, errno:%d", ir_p->code);
	return 0;
}
#endif

static int
asc_housekeeping(struct asc_interface *asc_p)
{
	char *log_header = "asc_housekeeping";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_debug("%s: start %s", log_header, priv_p->p_devname);
	rc = dsm_p->sm_op->sm_housekeeping(dsm_p);
	if (rc < 0)
		dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_FAILURE);
	/*
	 * Always let the housekeeping routines to do op get/put by themselves.
	 */
#if 0
	if (rc <= 0) {
		nid_log_debug("%s: a_op_put %s", log_header, priv_p->p_devname);
		asc_p->as_op->as_op_put(asc_p);
	}
#endif
	return rc;
}

static void
asc_op_get(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	pthread_mutex_lock(&priv_p->p_op_lck);
	nid_log_debug("asc_op_get: %s, p_op_counter:%d",
		priv_p->p_devname, priv_p->p_op_counter);
	priv_p->p_op_counter++;
	pthread_mutex_unlock(&priv_p->p_op_lck);
}

static void
asc_op_put(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	pthread_mutex_lock(&priv_p->p_op_lck);
	nid_log_debug("asc_op_put: %s, p_op_counter:%d",
		priv_p->p_devname, priv_p->p_op_counter);
	priv_p->p_op_counter--;
	pthread_mutex_unlock(&priv_p->p_op_lck);
}

/*
 * Return
 * 	1: got the lock
 * 	0: did not get the lock
 */
static int
asc_ex_lock(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	int got_it = 1;
	pthread_mutex_lock(&priv_p->p_ex_lck);
	if (!priv_p->p_doing_ex)
		priv_p->p_doing_ex = 1;
	else
		got_it = 0;
	pthread_mutex_unlock(&priv_p->p_ex_lck);
	return got_it;
}

static void
asc_ex_unlock(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	pthread_mutex_lock(&priv_p->p_ex_lck);
	priv_p->p_doing_ex = 0;
	pthread_mutex_unlock(&priv_p->p_ex_lck);
}

static int
asc_op_busy(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	return priv_p->p_op_counter;
}

static char*
asc_get_uuid(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	return priv_p->p_uuid;
}

static char*
asc_get_devname(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	return priv_p->p_devname;
}

static int
asc_get_minor(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	return priv_p->p_chan_id;
}

static void
asc_get_stat(struct asc_interface *asc_p, struct asc_stat *stat_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	char msg_buf[128];
	int msg_len = 128;
	int fd;
	int rc;
	struct nid_message msg;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct stat buf;

	priv_p->p_stat.as_state = dsm_p->sm_op->sm_get_state(dsm_p);
	priv_p->p_stat.as_alevel = priv_p->p_alevel;
	memcpy(stat_p, &priv_p->p_stat, sizeof(*stat_p));

	rc = stat(priv_p->p_devname, &buf);
	if (rc) {
		nid_log_debug("asc_get_stat() %s not created", priv_p->p_devname);
		return;
	}

	fd = open(AGENT_RESERVED_DEV, O_RDWR|O_CLOEXEC);
	if (fd < 0) {
		nid_log_error("asc_get_stat() failed to open %s: (%d)%s",
			AGENT_RESERVED_DEV, errno, strerror(errno));
		return;
	}

	*(int *)msg_buf = priv_p->p_chan_id;
	rc = ioctl(fd, NID_GET_STAT, msg_buf);
	close(fd);
	if (rc) {
		nid_log_error("asc_get_stat() ioctl failed: (%d)%s", errno, strerror(errno));
		return;
	}

	mpk_p->m_op->m_decode(mpk_p, msg_buf, &msg_len, (struct nid_message_hdr *)&msg);

	stat_p->as_read_counter = msg.m_rcounter;
	stat_p->as_write_counter = msg.m_wcounter;
	stat_p->as_keepalive_counter = msg.m_kcounter;
	stat_p->as_read_resp_counter = msg.m_r_rcounter;
	stat_p->as_write_resp_counter = msg.m_r_wcounter;
	stat_p->as_keepalive_resp_counter = msg.m_r_kcounter;
	stat_p->as_recv_sequence = msg.m_recv_sequence;
	stat_p->as_wait_sequence = msg.m_wait_sequence;
	stat_p->as_ts = time(NULL) - priv_p->p_stat.as_ts;
}

static int
asc_upgrade_alevel(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct nid_request upgrade_req;
	int nwrite, to_read, nread;
	char *p;
//	struct asc_job *job;
	struct tp_jobheader *jh_p;
	struct tp_interface *tp_p = priv_p->p_tp;
//	struct pp2_interface *pp2_p = priv_p->p_pp2;

	nid_log_debug("asc_upgrade_alevel (%s): start ...", priv_p->p_devname);
	upgrade_req.cmd = NID_REQ_UPGRADE;
	nwrite = util_nw_write_timeout(priv_p->p_rsfd, (char *)&upgrade_req, NID_SIZE_REQHDR, 10);
	if (nwrite != NID_SIZE_REQHDR) {
		nid_log_error("asc_upgrade_alevel: write error, %s, errno:%d",
			priv_p->p_devname, errno);
		return -1;
	}

	to_read = NID_SIZE_REQHDR;
	p = (char *)&upgrade_req;
	while (to_read) {
		nread = util_nw_read_n_timeout(priv_p->p_rsfd, p, NID_SIZE_REQHDR, 10);
		if (nread <= 0) {
			nid_log_error("asc_upgrade_alevel: read error, %s, errno:%d",
				priv_p->p_devname, errno);
			return -1;
		}
		to_read -= nread;
		p += nread;
	}
	if (upgrade_req.code) {
		nid_log_error("asc_upgrade_alevel: the service side reject to upgrade, %s, code:%d",
			priv_p->p_devname, upgrade_req.code);
		if (priv_p->p_up_job != NULL && upgrade_req.code != -2) {
#if 0
			//job = x_calloc(1, sizeof(*job));
			job = (struct asc_job*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
			*job = *(priv_p->p_up_job);
			jh_p = (struct tp_jobheader *)job;
#else
			jh_p = (struct tp_jobheader *)priv_p->p_up_job;
#endif
			tp_p->t_op->t_delay_job_insert(tp_p, jh_p, 0);
		}
		return -1;
	}

	priv_p->p_alevel = NID_ALEVEL_RDWR;
	return 0;
}

static void
asc_cleanup(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ash_interface *ash_p = priv_p->p_ash;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct dsm_interface *dsm_p = priv_p->p_dsm;

	nid_log_info("asc_cleanup: %s", priv_p->p_devname);
	if (priv_p->p_rsfd >= 0) {
		close(priv_p->p_rsfd);
		priv_p->p_rsfd = -1;
	}
	if (priv_p->p_dsfd >= 0) {
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = 1;
	}
	if (priv_p->p_fd >= 0) {
		close(priv_p->p_fd);
		priv_p->p_fd = -1;
	}

	ash_p->h_op->h_cleanup(ash_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)ash_p);

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)dsm_p);

	while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)priv_p->p_start_job))
		sleep(1);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_start_job);

	if (priv_p->p_wr_job != NULL) {
		while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)priv_p->p_wr_job))
			sleep(1);
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_wr_job);
	}

	if (priv_p->p_rw_job != NULL) {
		while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)priv_p->p_rw_job))
			sleep(1);
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_rw_job);
	}

	if (priv_p->p_iw_job != NULL) {
		while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)priv_p->p_iw_job))
			sleep(1);
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_iw_job);
	}

	if (priv_p->p_up_job != NULL) {
		while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)priv_p->p_up_job))
			sleep(1);
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p->p_up_job);
	}

	pthread_mutex_destroy(&priv_p->p_op_lck);
	pthread_mutex_destroy(&priv_p->p_ex_lck);
	pthread_mutex_destroy(&priv_p->p_drv_lck);
	pthread_cond_destroy(&priv_p->p_drv_cond);

	pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
	asc_p->as_private = NULL;
}

static int
asc_xdev_init_device(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc, fd;

	nid_log_info("asc_xdev_init_device: start, %s, (size:%lu, blksize:%u) ...",
		priv_p->p_devname, priv_p->p_size, priv_p->p_blksize);

	fd = open(priv_p->p_devname, O_RDWR|O_CLOEXEC);
	if (fd < 0) {
		nid_log_error("asc_xdev_init_device: cannot open %s, errno:%d",
			priv_p->p_devname, errno);
		return -1;
	}
	priv_p->p_fd = fd;

	rc = dsm_p->sm_op->sm_xdev_init_device(dsm_p);
	if (rc) {
		nid_log_error("asc_xdev_init_device: cannot start_channel, %s",
			priv_p->p_devname);
		return rc;
	}

	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_SUCCESS);	// state: INIT->IWI
	return 0;
}

static int
asc_xdev_start_channel(struct asc_interface *asc_p)
{
	char *log_header = "asc_xdev_start_channel";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_info("%s: start, %s (rsfd:%d, dsfd:%d) ...",
		log_header, priv_p->p_devname, priv_p->p_rsfd, priv_p->p_dsfd);

	rc = dsm_p->sm_op->sm_xdev_start_channel(dsm_p);
	if (rc) {
		nid_log_error("%s: cannot start_channel %s",
			log_header, priv_p->p_devname);
		return rc;
	}

	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_SUCCESS);	// state: IWI -> IWS, or R -> RW
	return 0;
}

static int
asc_xdev_delete_device(struct asc_interface *asc_p)
{
	char *log_header = "asc_xdev_delete_device";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_info("%s: start, [%s, minor:%d] ...",
		log_header, priv_p->p_devname, priv_p->p_chan_id);

	rc = dsm_p->sm_op->sm_xdev_delete_device(dsm_p);
	if (rc) {
		nid_log_error("%s: cannot delete_device, %s",
			log_header, priv_p->p_devname);
		return rc;
	}

	// state: INIT|W|R -> D
	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_DELETE);
	return 0;
}

/*
 * Return:
 * 	 1: expecting rdev_ioerro to finish
 * 	-1: cannot do it at current state
 */
static int
asc_xdev_ioerror_device(struct asc_interface *asc_p)
{
	char *log_header = "asc_xdev_ioerror_device";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_warning("%s: start [%s, minor:%d] ...",
		log_header, priv_p->p_devname, priv_p->p_chan_id);

	rc = dsm_p->sm_op->sm_xdev_ioerror_device(dsm_p);
	if (rc < 0) {
		nid_log_error("%s: cannot ioerror, %s",
			log_header, priv_p->p_devname);
	}
	if (rc <= 0) {
		nid_log_debug("%s: a_op_put %s",
			log_header, asc_p->as_op->as_get_devname(asc_p));

		nid_log_warning("%s: failed [%s] ...",
			log_header, priv_p->p_devname);

		asc_p->as_op->as_ex_unlock(asc_p);
		asc_p->as_op->as_op_put(asc_p);

	}
	return rc;
}

/*
 * Return:
 * 	Always 0
 */
static int
asc_xdev_keepalive_channel(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;

	nid_log_debug("asc_xdev_keepalive_channel: start (chan_type:%s chan_id:%d) ...",
		priv_p->p_chan_typestr, priv_p->p_chan_id);
	dsm_p->sm_op->sm_xdev_keepalive_channel(dsm_p);
	return 0;
}

static int
asc_xdev_upgrade(struct asc_interface *asc_p, char force)
{
	char *log_header = "asc_xdev_upgrade";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct ascg_interface *ascg_p = priv_p->p_ascg;
	int chan_id = priv_p->p_chan_id;
	int chan_type = priv_p->p_chan_type;

	nid_log_info("%s: start (chan_type:%d, chan_id:%d, force:%d) ...",
		log_header, chan_type, chan_id, force);
	ascg_p->ag_op->ag_xdev_upgrade_channel(ascg_p, chan_type, chan_id, force);
	return 0;
}

static int
asc_rdev_keepalive(struct asc_interface *asc_p, uint64_t recv_req, uint64_t wait_seq, uint32_t status)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;
	nid_log_debug("asc_rdev_keepalive: start, %s", priv_p->p_devname);

	priv_p->p_srv_recv_seq = recv_req;
	priv_p->p_srv_wait_seq = wait_seq;
	priv_p->p_srv_status = status;
	rc = dsm_p->sm_op->sm_rdev_keepalive(dsm_p);
	if (rc)
		nid_log_error("asc_rdev_keepalive: cannot reset_keepalive, %s",
			priv_p->p_devname);
	return rc;
}

static int
asc_rdev_init_device(struct asc_interface *asc_p, uint8_t ret_code)
{
	char *log_header = "asc_rdev_init_device";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	int minor = priv_p->p_chan_id;
	int rc = 0;

	nid_log_notice("%s: start (minor:%d) ...", log_header, minor);
	pthread_mutex_lock(&priv_p->p_drv_lck);
	if (!priv_p->p_established) {
		priv_p->p_established = 1;
	} else {
		nid_log_error("%s: the device (minor:%d) has already been inited",
			log_header, minor);
		rc = -1;
	}
	priv_p->p_drv_init_code = ret_code;
	pthread_cond_broadcast(&priv_p->p_drv_cond);
	pthread_mutex_unlock(&priv_p->p_drv_lck);
	return rc;
}

static int
asc_rdev_delete_device(struct asc_interface *asc_p)
{
	char *log_header = "asc_rdev_delete_device";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_notice("%s: start, [%s, minor:%d] ...",
		log_header, priv_p->p_devname, priv_p->p_chan_id);
	rc = dsm_p->sm_op->sm_rdev_delete_device(dsm_p);
	if (rc) {
		nid_log_error("%s: cannot delete_device %s",
			log_header, priv_p->p_devname);
		return rc;
	}

	// state: D -> DSM_STATE_NO
	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_SUCCESS);

	// don't cleanup here, let ascg do it
	return 0;
}

static int
asc_rdev_start_channel(struct asc_interface *asc_p)
{
	char *log_header = "asc_rdev_start_channel";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_notice("%s: start (%s) ...",
		log_header, priv_p->p_devname);
	rc = dsm_p->sm_op->sm_rdev_start_channel(dsm_p);

	/* state: IWS|RW -> WORK */
	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_SUCCESS);
	if (!rc) {
		nid_log_debug("%s: a_op_put %s",
			log_header, priv_p->p_devname);
		asc_p->as_op->as_op_put(asc_p);
	}
	return 0;
}

/*
 * Return:
 * 	Always 0
 */
static int
asc_rdev_recover_channel(struct asc_interface *asc_p)
{
	char *log_header = "asc_rdev_recover_channel";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;

	nid_log_notice("%s: start (%s) ...", log_header, priv_p->p_devname);
	dsm_p->sm_op->sm_rdev_recover_channel(dsm_p);
	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_FAILURE);	// state: WORK -> RECOVER
	nid_log_debug("%s: a_op_put %s", log_header, priv_p->p_devname);
	asc_p->as_op->as_op_put(asc_p);
	return 0;
}

/*
 * Return:
 * 	 0: done completely
 * 	-1: cannot do it at current state
 */
static int
asc_rdev_ioerror_device(struct asc_interface *asc_p)
{
	char *log_header = "asc_rdev_ioerror_device";
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	int rc;

	nid_log_warning("%s: start [%s, minor:%d] ...",
		log_header, priv_p->p_devname, priv_p->p_chan_id);

	rc = dsm_p->sm_op->sm_rdev_ioerror_device(dsm_p);
	if (rc < 0)
		nid_log_error("%s: cannot ioerror_device, %s",
			log_header, priv_p->p_devname);

	asc_p->as_op->as_ex_unlock(asc_p);
	nid_log_warning("%s: a_op_put %s", log_header, priv_p->p_devname);
	asc_p->as_op->as_op_put(asc_p);
	return rc;
}

static int
asc_ok_delete_device(struct asc_interface *asc_p)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;
	struct dsm_interface *dsm_p = priv_p->p_dsm;
	return dsm_p->sm_op->sm_ok_delete_device(dsm_p);
}

static int
asc_rdev_upgrade(struct asc_interface *asc_p, char code)
{
	struct asc_private *priv_p = (struct asc_private *)asc_p->as_private;

	nid_log_debug("asc_rdev_upgrade: start, %s ...", priv_p->p_devname);
	if (code) {
		nid_log_error("asc_rdev_upgrade: the service side reject to upgrade %s, code:%d",
			priv_p->p_devname, code);
		return EBUSY; //EBUSY: 16  Device or resource busy
	}

	priv_p->p_alevel = NID_ALEVEL_RDWR;
	return 0;
}

struct asc_operations asc_op = {
	.as_housekeeping = asc_housekeeping,
	.as_op_get = asc_op_get,
	.as_op_put = asc_op_put,
	.as_ex_lock = asc_ex_lock,
	.as_ex_unlock = asc_ex_unlock,
	.as_op_busy = asc_op_busy,
	.as_get_uuid = asc_get_uuid,
	.as_get_devname = asc_get_devname,
	.as_cleanup = asc_cleanup,
	.as_get_stat = asc_get_stat,
	.as_get_minor = asc_get_minor,
	.as_upgrade_alevel = asc_upgrade_alevel,

	.as_xdev_init_device = asc_xdev_init_device,
	.as_xdev_start_channel = asc_xdev_start_channel,
	.as_xdev_delete_device = asc_xdev_delete_device,
	.as_xdev_ioerror_device = asc_xdev_ioerror_device,
	.as_xdev_keepalive_channel = asc_xdev_keepalive_channel,
	.as_xdev_upgrade = asc_xdev_upgrade,

	.as_rdev_recover_channel = asc_rdev_recover_channel,
	.as_rdev_keepalive = asc_rdev_keepalive,
	.as_rdev_init_device = asc_rdev_init_device,
	.as_rdev_start_channel = asc_rdev_start_channel,
	.as_rdev_delete_device = asc_rdev_delete_device,
	.as_rdev_ioerror_device = asc_rdev_ioerror_device,
	.as_rdev_upgrade = asc_rdev_upgrade,

	.as_ok_delete_device = asc_ok_delete_device,
};

int
asc_initialization(struct asc_interface *asc_p, struct asc_setup *setup)
{
	char *log_header = "asc_initialization";
	struct asc_private *priv_p;
	struct asc_stat *stat_p;
	struct ash_setup ash_setup;
	struct ash_interface *ash_p;
	struct asc_job *job;
	struct dsm_setup dsm_setup;
	struct dsm_interface *dsm_p;
	struct pp2_interface *pp2_p = setup->pp2;
	struct pp2_interface *dyn_pp2_p = setup->dyn_pp2;

	nid_log_info("%s: start (ip:%s, uuid:%s)...",
		log_header, setup->sip, setup->uuid);
	if (!setup->uuid)
		return -1;

	asc_p->as_op = &asc_op;
	//priv_p = x_calloc(1, sizeof(*priv_p));
	priv_p = (struct asc_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	asc_p->as_private = priv_p;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_dyn_pp2 = dyn_pp2_p;
	priv_p->p_chan_type = setup->type;
	if (priv_p->p_chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
		strcpy(priv_p->p_chan_typestr, "asc");
	} else {
		nid_log_error("%s: worng channel type (%d)", log_header, priv_p->p_chan_type);
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
		return -1;
	}

	//priv_p->p_blksize = 1024;
	priv_p->p_blksize = 4096;
	stat_p = &priv_p->p_stat;
	stat_p->as_ts = time(NULL);
	priv_p->p_mpk = setup->mpk;
	priv_p->p_tp = setup->tp;
	priv_p->p_ascg = setup->ascg;
	priv_p->p_ascg_chan = setup->ascg_chan;
	priv_p->p_alevel = NID_ALEVEL_INVALID;
	if (!strcmp(setup->access, "r"))
		priv_p->p_alevel = NID_ALEVEL_RDONLY;
	else if (!strcmp(setup->access, "rw"))
		priv_p->p_alevel = NID_ALEVEL_RDWR;

	strncpy(priv_p->p_uuid, setup->uuid, NID_MAX_UUID);
	stat_p->as_uuid = priv_p->p_uuid;
	stat_p->as_sip = priv_p->p_sip;
	strncpy(priv_p->p_devname, setup->devname, NID_MAX_DEVNAME);
	priv_p->p_chan_id = setup->chan_id;
	priv_p->p_fd = -1;
	priv_p->p_ioctl_fd = -1;
	priv_p->p_wr_timewait = setup->wr_timewait;

	if (setup->iw_hook)
		strncpy(priv_p->p_iw_hook, setup->iw_hook, NID_MAX_CMD);
	if (setup->wr_hook)
		strncpy(priv_p->p_wr_hook, setup->wr_hook, NID_MAX_CMD);
	if (setup->rw_hook)
		strncpy(priv_p->p_rw_hook, setup->rw_hook, NID_MAX_CMD);
	if (setup->up_hook)
		strncpy(priv_p->p_up_hook, setup->up_hook, NID_MAX_CMD);

	stat_p->as_devname = priv_p->p_devname;

	strncpy(priv_p->p_sip, setup->sip, NID_MAX_IP);

	ash_setup.uuid = priv_p->p_uuid;
	ash_setup.sip = priv_p->p_sip;
	ash_p = (struct ash_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*ash_p));
	ash_initialization(ash_p, &ash_setup);
	priv_p->p_ash = ash_p;
	pthread_mutex_init(&priv_p->p_op_lck, NULL);
	pthread_mutex_init(&priv_p->p_ex_lck, NULL);
	pthread_mutex_init(&priv_p->p_drv_lck, NULL);
	pthread_cond_init(&priv_p->p_drv_cond, NULL);

	job = (struct asc_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
	job->tp = priv_p->p_tp;
	job->ash = priv_p->p_ash;
	job->asc = asc_p;
	job->ascg = priv_p->p_ascg;
	job->ascg_chan = priv_p->p_ascg_chan;
	job->header.jh_do = _start_job_do;
	priv_p->p_start_job = job;
	priv_p->p_need_start = 1;

	if (priv_p->p_wr_hook[0]) {
		job = (struct asc_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
		job->tp = priv_p->p_tp;
		job->ash = priv_p->p_ash;
		job->asc = asc_p;
		job->ascg = priv_p->p_ascg;
		job->ascg_chan = priv_p->p_ascg_chan;
		job->header.jh_do = _do_wr_hook_job;
		job->cmd = priv_p->p_wr_hook;
		priv_p->p_wr_job = job;
		nid_log_debug("%s: uuid:%s, wr_job: {%s}", log_header, priv_p->p_uuid, job->cmd);
	}

	if (priv_p->p_iw_hook[0]) {
		job = (struct asc_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
		job->tp = priv_p->p_tp;
		job->ash = priv_p->p_ash;
		job->asc = asc_p;
		job->ascg = priv_p->p_ascg;
		job->ascg_chan = priv_p->p_ascg_chan;
		job->header.jh_do = _do_iw_hook_job;
		job->cmd = priv_p->p_iw_hook;
		priv_p->p_iw_job = job;
		nid_log_debug("%s: uuid:%s, iw_job: {%s}", log_header, priv_p->p_uuid, job->cmd);
	}

	if (priv_p->p_rw_hook[0]) {
		job = (struct asc_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
		job->tp = priv_p->p_tp;
		job->ash = priv_p->p_ash;
		job->asc = asc_p;
		job->ascg = priv_p->p_ascg;
		job->ascg_chan = priv_p->p_ascg_chan;
		job->header.jh_do = _do_rw_hook_job;
//		job->header.jh_free = asc_free_dyn_job;
//		job->header.jh_free = asc_free_job;
		job->cmd = priv_p->p_rw_hook;
		priv_p->p_rw_job = job;
		nid_log_debug("%s: uuid:%s, rw_job: {%s}", log_header, priv_p->p_uuid, job->cmd);
	}

	if (priv_p->p_up_hook[0]) {
		job = (struct asc_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
		job->tp = priv_p->p_tp;
		job->ash = priv_p->p_ash;
		job->asc = asc_p;
		job->ascg = priv_p->p_ascg;
		job->ascg_chan = priv_p->p_ascg_chan;
		job->header.jh_do = _do_up_hook_job;
//		job->header.jh_free = asc_free_job;
		job->cmd = priv_p->p_up_hook;
		priv_p->p_up_job = job;
		nid_log_debug("%s: uuid:%s, up_job: {%s}", log_header, priv_p->p_uuid, job->cmd);
	}

	dsm_p = (struct dsm_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*dsm_p));
	priv_p->p_dsm = dsm_p;
	dsm_setup.data = (void *)asc_p;
	dsm_initialization(dsm_p, &dsm_setup);
	dsm_p->sm_op->sm_state_transition(dsm_p, DSM_LA_SUCCESS);	// state: NO->INIT

	return 0;
}
