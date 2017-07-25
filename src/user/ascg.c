/*
 * ascg.c
 * 	Implementation of Agent Server Channel Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "agent.h"
#include "list.h"
#include "nid_log.h"
#include "nid.h"
#include "mpk_if.h"
#include "tp_if.h"
#include "ini_if.h"
#include "asc_if.h"
#include "amc_if.h"
#include "ash_if.h"
#include "mwl_if.h"
#include "mq_if.h"
#include "drv_if.h"
#include "ascg_if.h"
#include "acg_if.h"
#include "pp2_if.h"

struct ascg_private {
	struct drv_interface	*p_drv;
	struct mpk_interface	*p_mpk;
	struct tp_interface	*p_tp;
	struct ini_interface	*p_ini;
	struct mwl_interface	*p_mwl;
	struct mq_interface	*p_mq;			// housekeeping leftover queue
	struct list_head	p_asc_head;
	struct list_head	p_delete_asc_head;
	struct list_head	p_waiting_chan_head;
	struct list_head	p_agent_keys;
	struct pp2_interface	*p_pp2;
	struct pp2_interface	*p_dyn_pp2;
	struct acg_interface    *p_acg;
	pthread_mutex_t		p_ioctl_lck;
	pthread_mutex_t		p_list_lck;
	pthread_mutex_t		p_update_lck;
	pthread_mutex_t		p_chan_lck;
	pthread_cond_t		p_update_cond;
	pthread_cond_t		p_chan_cond;
	int			p_update_counter;
	uint8_t			p_keepalive_timer;
	char			p_stop;
	char			p_dr_done;
	uint32_t		p_hook_flag; //1: enable, 0: disable
};

struct ascg_channel {
	struct list_head	a_list;
	uint16_t		a_type;		// channel type
	void			*a_chan;	// data for this communication channel
	char			*a_uuid;
	int			a_chan_id;
	int			a_delay;
	int			a_delay_save;
	char			a_established;
};

#if 0
struct ascg_housekeeping_job {
	struct tp_jobheader	j_header;
	struct ascg_interface	*j_ascg;
	int			j_delay;
};
#else
struct ascg_housekeeping_job {
	struct tp_jobheader	j_header;
	struct ascg_interface	*j_ascg;
	int			j_delay;
	int			j_do_next;
};
#endif

static int
__delete_callback(struct ascg_interface *ascg_p, struct ascg_message *msg_p)
{
	char *log_header = "__delete_callback";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	int code = msg_p->m_owner_code;

	nid_log_debug("%s: start, code:%d", log_header, code);
	if (code >= 0) {
		close(code);
	} else if (code == -1) {
		pthread_mutex_lock(&priv_p->p_update_lck);
		nid_log_debug("%s: update_counter:%d", log_header, priv_p->p_update_counter);
		if (--priv_p->p_update_counter == 0)
			pthread_cond_signal(&priv_p->p_update_cond);
		pthread_mutex_unlock(&priv_p->p_update_lck);
	}
	return 0;
}

static int
_ioerror_callback(struct ascg_interface *ascg_p, struct ascg_message *msg_p)
{
	assert(ascg_p);
	int code = msg_p->m_owner_code;
	nid_log_warning("_ioerror_callback: start, code:%d", code);
	if (code >= 0)
		close(code);
	return 0;
}

static int
_upgrade_callback(struct ascg_interface *ascg_p, struct ascg_message *msg_p)
{
	char *log_header = "_upgrade_callback";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	int code = msg_p->m_owner_code;

	nid_log_debug("%s: code:%d", log_header, code);
	if (code >= 0) {
		close(code);
	} else if (code == -1) {
		pthread_mutex_lock(&priv_p->p_update_lck);
		nid_log_debug("%s: update_counter:%d", log_header, priv_p->p_update_counter);
		if (--priv_p->p_update_counter == 0)
			pthread_cond_signal(&priv_p->p_update_cond);
		pthread_mutex_unlock(&priv_p->p_update_lck);
	}
	return 0;
}

/*
 * the caller should hold p_list_lck
 */
static struct ascg_channel *
__search_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id, int from_delete)
{
	char *log_header = "__search_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct list_head *search_head;

	if (!from_delete)
		search_head = &priv_p->p_asc_head;
	else
		search_head = &priv_p->p_delete_asc_head;

	if (chan_type == NID_CHANNEL_TYPE_ASC_CODE) {
		list_for_each_entry(chan_p, struct ascg_channel, search_head, a_list) {
			asc_p = (struct asc_interface *)chan_p->a_chan;
			if (chan_id == asc_p->as_op->as_get_minor(asc_p))
				return chan_p;
		}
	}

	nid_log_debug("%s: failed. chan_id:%d", log_header, chan_id);
	return NULL;
}

/*
 * the caller should hold p_list_lck
 */
static struct ascg_channel *
__search_channel_by_minor(struct ascg_interface *ascg_p, int minor, int from_delete)
{
	char *log_header = "__search_channel_by_minor";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct list_head *search_head;

	if (!from_delete)
		search_head = &priv_p->p_asc_head;
	else
		search_head = &priv_p->p_delete_asc_head;

	list_for_each_entry(chan_p, struct ascg_channel, search_head, a_list) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		if (minor == asc_p->as_op->as_get_minor(asc_p))
			return chan_p;
	}
	nid_log_debug("%s: failed. minor:%d", log_header, minor);
	return NULL;
}

/*
 * the caller should hold p_list_lck
 */
static struct ascg_channel *
__search_channel_by_name(struct ascg_interface *ascg_p, char *devname, int from_delete)
{
	char *log_header = "__search_channel_by_name";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct list_head *search_head;

	if (!from_delete)
		search_head = &priv_p->p_asc_head;
	else
		search_head = &priv_p->p_delete_asc_head;

	list_for_each_entry(chan_p, struct ascg_channel, search_head, a_list) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		if (!strcmp(asc_p->as_op->as_get_devname(asc_p), devname))
			return chan_p;
	}
	nid_log_debug("%s: failed. devname:%s", log_header, devname);
	return NULL;
}

/*
 * the caller should hold p_list_lck
 */
static struct ascg_channel *
__search_channel_by_uuid(struct ascg_interface *ascg_p, char *uuid, int from_delete)
{
	char *log_header = "__search_channel_by_uuid";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct list_head *search_head;
	char *the_uuid;

	if (!from_delete)
		search_head = &priv_p->p_asc_head;
	else
		search_head = &priv_p->p_delete_asc_head;

	list_for_each_entry(chan_p, struct ascg_channel, search_head, a_list) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		the_uuid = asc_p->as_op->as_get_uuid(asc_p);
		if (!strcmp(the_uuid, uuid))
			return chan_p;
	}
	nid_log_notice("%s: failed. uuid:%s", log_header, uuid);
	return NULL;
}

static int
_create_device_prechecking(struct ascg_interface *ascg_p, char *devname, int devminor)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct stat st_buf;
	int major, minor, rc = 0;

	if (stat(devname, &st_buf) || ((st_buf.st_mode & S_IFMT) != S_IFBLK)) {
		nid_log_error("_create_device_prechecking: failed");
		return -1;
	}

	major = major(st_buf.st_rdev);
	if (major != NID_MAJOR) {
		nid_log_error("_create_device_prechecking: wrong major:%d",
			major);
		return -1;
	}

	minor = minor(st_buf.st_rdev);
	if (minor != devminor) {
		nid_log_error("_create_device_prechecking: wrong minor:%d, expected:%d",
			minor, devminor);
		return -1;
	}

	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_minor(ascg_p, devminor, 0);
	if (chan_p) {
		nid_log_error("_create_device_prechecking: minor:%d is occupied",
			devminor);
		rc = -1;
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);
	nid_log_debug("_create_device_prechecking: %s minor:%d rc:%d",
		devname, devminor, rc);
	return rc;
}

static struct ascg_channel*
__alloc_asc(struct ascg_interface *ascg_p, struct ini_interface *ini_p, struct ini_section_content *sc_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ini_key_content *kc_p;
	struct ascg_channel *chan_p;
	struct asc_setup asc_setup;
	struct asc_interface *asc_p;
	int rc;

	nid_log_debug("__alloc_asc: uuid:%s", sc_p->s_name);
	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->a_chan_id = -1;	// for "asc" type
	chan_p->a_uuid = sc_p->s_name;
	chan_p->a_type = NID_CTYPE_REG;
	memset(&asc_setup, 0, sizeof(asc_setup));
	asc_setup.pp2 = priv_p->p_pp2;
	asc_setup.dyn_pp2 = priv_p->p_dyn_pp2;
	asc_setup.type = NID_CHANNEL_TYPE_ASC_CODE;
	asc_setup.mpk = priv_p->p_mpk;
	asc_setup.tp = priv_p->p_tp;
	asc_setup.ascg = ascg_p;
	asc_setup.ascg_chan = (void *)chan_p;
	asc_setup.uuid = sc_p->s_name;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "devname");
	asc_setup.devname = (char*)kc_p->k_value;
	assert(strcmp(asc_setup.devname, AGENT_NONEXISTED_DEV));
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "minor");
	asc_setup.chan_id = *(int*)kc_p->k_value;
	assert(asc_setup.chan_id);	// minor 0 is reserved for agent
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "ip");
	asc_setup.sip = (char*)kc_p->k_value;
	assert(strcmp(asc_setup.sip, AGENT_NONEXISTED_IP));
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "access");
	asc_setup.access = (char*)kc_p->k_value;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "wr_timewait");
	asc_setup.wr_timewait = *(int*)kc_p->k_value;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "iw_hook");
	if (kc_p)
		asc_setup.iw_hook = (char*)kc_p->k_value;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "wr_hook");
	if (kc_p)
		asc_setup.wr_hook = (char*)kc_p->k_value;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "rw_hook");
	if (kc_p)
		asc_setup.rw_hook = (char*)kc_p->k_value;
	kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "up_hook");
	if (kc_p)
		asc_setup.up_hook = (char*)kc_p->k_value;
	rc = _create_device_prechecking(ascg_p, asc_setup.devname, asc_setup.chan_id);
	if (rc) {
		free(chan_p);
		return NULL;
	}

	asc_p = x_calloc(1, sizeof(*asc_p));
	chan_p->a_chan = (void *)asc_p;
	asc_initialization(asc_p, &asc_setup);
	pthread_mutex_lock(&priv_p->p_list_lck);
	if (!priv_p->p_stop) {
		list_add_tail(&chan_p->a_list, &priv_p->p_asc_head);
	} else {
		asc_p->as_op->as_cleanup(asc_p);
		free(asc_p);
		free(chan_p);
		chan_p = NULL;
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);
	return chan_p;
}

# if 0
static int
_do_dr_housekeeping(struct tp_jobheader *jheader)
{
	char *log_header = "_do_dr_housekeeping";
	struct ascg_housekeeping_job *job = (struct ascg_housekeeping_job *)jheader;
	struct ascg_interface *ascg_p = job->j_ascg;
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct mq_interface *mq_p = priv_p->p_mq;
	struct tp_interface *tp_p = priv_p->p_tp;
	struct ascg_channel *chan_p, *chan_p1;
	struct asc_interface *asc_p;
	struct ascg_message *msg_p;
	struct mq_message_node *mn_p;
	int i, mq_len;

	nid_log_debug("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_list_lck);
	if (!list_empty(&priv_p->p_asc_head))
		nid_log_debug("%s: doing p_asc_head ...", log_header);
	else
		nid_log_debug("%s: p_asc_head is empty", log_header);
	list_for_each_entry_safe(chan_p, chan_p1, struct ascg_channel, &priv_p->p_asc_head, a_list) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		nid_log_debug("%s: a_op_get %s", log_header, asc_p->as_op->as_get_devname(asc_p));
		asc_p->as_op->as_op_get(asc_p);
		asc_p->as_op->as_housekeeping(asc_p);
	}

	/*
	 * some job couldn't go through, re-try now
	 */
	mq_len = mq_p->m_op->m_len(mq_p);
	for (i = 0; i < mq_len; i++) {
		mn_p = mq_p->m_op->m_dequeue(mq_p);
		if (!mn_p)
			break;
		msg_p = (struct ascg_message *)mn_p->m_data;
		asc_p = msg_p->m_asc;
		if (asc_p->as_op->as_ex_lock(asc_p)) {
			switch (msg_p->m_req) {
			case NID_REQ_IOERROR_DEVICE:
				asc_p->as_op->as_xdev_ioerror_device(asc_p);
				// don't unlock here since ioerror not finished yet.
				// asc_rdev_ioerror_device should unlock
				break;
			default:
				break;
			}
			free(msg_p);
			mn_p->m_data = NULL;
			mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
		} else {
			// try in next housekeeping cycle
			nid_log_warning("%s: busy. Do it next cycle", log_header);
			mq_p->m_op->m_enqueue(mq_p, mn_p);
		}
	}

	if (priv_p->p_stop &&
	    list_empty(&priv_p->p_asc_head) &&
	    list_empty(&priv_p->p_delete_asc_head))
		priv_p->p_dr_done = 1;
	pthread_mutex_unlock(&priv_p->p_list_lck);

	if (!priv_p->p_dr_done) {
        	tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, job->j_delay);
        	return 1;
    	}
    	else {
		nid_log_debug("%s: done", log_header);
        	return 0;
    	}
}
#else
static void
_do_dr_housekeeping(struct tp_jobheader *jheader)
{
	char *log_header = "_do_dr_housekeeping";
	struct ascg_housekeeping_job *job = (struct ascg_housekeeping_job *)jheader;
	struct ascg_interface *ascg_p = job->j_ascg;
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct mq_interface *mq_p = priv_p->p_mq;
//	struct tp_interface *tp_p = priv_p->p_tp;
	struct ascg_channel *chan_p, *chan_p1;
	struct asc_interface *asc_p;
	struct ascg_message *msg_p;
	struct mq_message_node *mn_p;
	int i, mq_len;

	nid_log_debug("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_list_lck);
	if (!list_empty(&priv_p->p_asc_head))
		nid_log_debug("%s: doing p_asc_head ...", log_header);
	else
		nid_log_debug("%s: p_asc_head is empty", log_header);
	list_for_each_entry_safe(chan_p, chan_p1, struct ascg_channel, &priv_p->p_asc_head, a_list) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		nid_log_debug("%s: a_op_get %s", log_header, asc_p->as_op->as_get_devname(asc_p));
		/*
		 * Alawys let the housekeeping routines to do op get/put by themselves.
		 */
#if 0
		asc_p->as_op->as_op_get(asc_p);
#endif
		asc_p->as_op->as_housekeeping(asc_p);
	}

	/*
	 * some job couldn't go through, re-try now
	 */
	mq_len = mq_p->m_op->m_len(mq_p);
	for (i = 0; i < mq_len; i++) {
		mn_p = mq_p->m_op->m_dequeue(mq_p);
		if (!mn_p)
			break;
		msg_p = (struct ascg_message *)mn_p->m_data;
		asc_p = msg_p->m_asc;
		if (asc_p->as_op->as_ex_lock(asc_p)) {
			switch (msg_p->m_req) {
			case NID_REQ_IOERROR_DEVICE:
				asc_p->as_op->as_xdev_ioerror_device(asc_p);
				// don't unlock here since ioerror not finished yet.
				// asc_rdev_ioerror_device should unlock
				break;
			default:
				break;
			}
			free(msg_p);
			mn_p->m_data = NULL;
			mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
		} else {
			// try in next housekeeping cycle
			nid_log_warning("%s: busy. Do it next cycle", log_header);
			mq_p->m_op->m_enqueue(mq_p, mn_p);
		}
	}

	if (priv_p->p_stop &&
	    list_empty(&priv_p->p_asc_head) &&
	    list_empty(&priv_p->p_delete_asc_head))
		priv_p->p_dr_done = 1;
	pthread_mutex_unlock(&priv_p->p_list_lck);

	if (!priv_p->p_dr_done) {
        	job->j_do_next = 1;
    	} else {
		nid_log_debug("%s: done", log_header);
		job->j_do_next = 0;
    	}
}
#endif

#if 0
static void
_do_housekeeping(struct tp_jobheader *jheader)
{
	char *log_header = "_do_housekeeping";
	struct ascg_housekeeping_job *job = (struct ascg_housekeeping_job *)jheader;
	struct ascg_interface *ascg_p = job->j_ascg;
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
    	struct pp2_interface *pp2_p = priv_p->p_pp2;
    	int do_next;

	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	do_next = _do_dr_housekeeping(jheader);
    	if (!do_next) {
        	 pp2_p->pp2_op->pp2_put(pp2_p, (void *)job);
    	}
    	return;
}
#else
static void*
_do_housekeeping(void *p)
{
	char *log_header = "_do_housekeeping";
	struct ascg_housekeeping_job *job = (struct ascg_housekeeping_job *)p;
	struct ascg_interface *ascg_p = job->j_ascg;
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
    	struct pp2_interface *pp2_p = priv_p->p_pp2;
    	struct tp_interface *tp_p = priv_p->p_tp;

	nid_log_debug("%s: start ...", log_header);

do_next:
    	if (job->j_do_next) {
    		tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, job->j_delay);
    		while (tp_p->t_op->t_get_job_in_use(tp_p, (struct tp_jobheader *)job))
    			usleep(500000);
    		goto do_next;
    	} else {
    		pp2_p->pp2_op->pp2_put(pp2_p, (void *)job);
    	}

    	return NULL;
}
#endif

static struct amc_stat_record*
_create_asc_stat(struct ascg_channel *chan_p)
{
	struct asc_interface *asc_p;
	struct asc_stat *stat_p;
	struct amc_stat_record *sr_p;
	asc_p = (struct asc_interface *)chan_p->a_chan;
	stat_p = x_calloc(1, sizeof(*stat_p));
	sr_p = x_calloc(1, sizeof(*sr_p));
	sr_p->r_type = NID_REQ_STAT;
	sr_p->r_data = (void *)stat_p;
	asc_p->as_op->as_get_stat(asc_p, stat_p);
	return sr_p;
}

/*
 * Input:
 * 	chan_type: asc or channel type
 *	chan_id:
 *		minor of device chan_type is NID_CHANNEL_TYPE_ASC_CODE
 *		channel id if chan_type is NID_CHANNEL_TYPE_CHAN_CODE
 */
static void
__cleanup_mwl(struct ascg_interface *ascg_p, int chan_type, int chan_id)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct mwl_message_node *mn_p;
	struct ascg_message *msg_p;
	while ((mn_p = mwl_p->m_op->m_remove_next(mwl_p, chan_type, chan_id))) {
		msg_p = (struct ascg_message *)mn_p->m_data;
		if (msg_p->m_callback)
			msg_p->m_callback(ascg_p, msg_p);
		free(msg_p);
		mn_p->m_data = NULL;
		mwl_p->m_op->m_put_free_mnode(mwl_p, mn_p);
	}
}

static void
ascg_failed_to_establish_channel(struct ascg_interface *ascg_p, void *chan)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p = (struct ascg_channel *)chan;
	assert(priv_p);
	chan_p->a_delay = chan_p->a_delay_save;
	if (chan_p->a_delay < 60) {
		if (!chan_p->a_established)
			chan_p->a_delay += 5;
		else
			chan_p->a_delay = 1;
	}
	chan_p->a_delay_save = chan_p->a_delay;
}

static void
ascg_get_stat(struct ascg_interface *ascg_p, struct list_head *out_head)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ini_interface *ini_p = priv_p->p_ini;
	struct ascg_channel *chan_p;
	struct amc_stat_record *sr_p;

	nid_log_debug("ascg_get_stat: start...");
	ini_p->i_op->i_display(ini_p);
	pthread_mutex_lock(&priv_p->p_list_lck);
	list_for_each_entry(chan_p, struct ascg_channel, &priv_p->p_asc_head, a_list) {
		sr_p = _create_asc_stat(chan_p);
		list_add_tail(&sr_p->r_list, out_head);
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);
}

/*
 * Return:
 * 	0: done comepletely
 */
static int
ascg_update(struct ascg_interface *ascg_p)
{
	char *log_header = "ascg_update";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ini_interface *ini_p = priv_p->p_ini;
	struct ini_interface new_ini;
	struct ini_setup setup;
	struct ini_section_content **sc_pp, *added_sc[1024], *rm_sc[1024];
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	char *devname;
	int found_it;
	struct ini_key_content *kc_p;
	char *section_type;

	nid_log_notice("%s: start ...", log_header);
	setup.path = NID_CONF_AGENT;
	//setup.keys_desc = agent_keys;
	setup.key_set = priv_p->p_agent_keys;
	ini_initialization(&new_ini, &setup);
	ini_p->i_op->i_parse(&new_ini);
	ini_p->i_op->i_compare(ini_p, &new_ini, added_sc, rm_sc);

	sc_pp = rm_sc;
	nid_log_debug("%s: display removed sections start: ...", log_header);
	pthread_mutex_lock(&priv_p->p_update_lck);
	priv_p->p_update_counter = 0;
	while (*sc_pp) {
		kc_p = ini_p->i_op->i_search_key(ini_p, *sc_pp, "type");
		section_type = (char*)kc_p->k_value;
		if (strcmp(section_type, AGENT_TYPE_ASC) ) {
			/* skip non asc section */
			sc_pp++;
			continue;
		}

		found_it = 0;
		pthread_mutex_lock(&priv_p->p_list_lck);
		list_for_each_entry(chan_p, struct ascg_channel, &priv_p->p_asc_head, a_list) {
			asc_p = (struct asc_interface *)chan_p->a_chan;
			if (!strcmp(asc_p->as_op->as_get_uuid(asc_p), (*sc_pp)->s_name)) {
				priv_p->p_update_counter++;
				devname = asc_p->as_op->as_get_devname(asc_p);
				found_it = 1;
				break;
			}
		}

		if (found_it) {
			while (ascg_p->ag_op->ag_delete_device(ascg_p, devname, -1, 0) == -2) {
				/*
				 * case 1:
				 * the channel is busy, we can not delete it right now
				 * But the asc has already been deleted from p_asc_head by
				 * a_xdev_delete_device to prevent from getting any new requests
				 * So we can sleep to wait for all existing operations to finish
				 * case 2:
				 * the channel in some non-deletable state, we just wait
				 */
				pthread_mutex_unlock(&priv_p->p_list_lck);
				sleep(1);
				pthread_mutex_lock(&priv_p->p_list_lck);
			}
		}
		pthread_mutex_unlock(&priv_p->p_list_lck);
		nid_log_debug("%s: uuid:%s, update_counter:%d", log_header, (*sc_pp)->s_name, priv_p->p_update_counter);
		sc_pp++;
	}
	nid_log_debug("%s: update_counter:%d", log_header, priv_p->p_update_counter);
	while (priv_p->p_update_counter) {
		pthread_cond_wait(&priv_p->p_update_cond, &priv_p->p_update_lck);
	}
	pthread_mutex_unlock(&priv_p->p_update_lck);
	nid_log_debug("%s: display removed sections end", log_header);

	nid_log_debug("%s: display added sections start:", log_header);
	sc_pp = added_sc;
	while (*sc_pp) {
		kc_p = ini_p->i_op->i_search_key(ini_p, *sc_pp, "type");
		section_type = (char*)kc_p->k_value;
		if (strcmp(section_type, AGENT_TYPE_ASC) ) {
			/* skip non asc section */
			sc_pp++;
			continue;
		}
		new_ini.i_op->i_remove_section(&new_ini, (*sc_pp)->s_name);
		nid_log_debug("%s: uuid:%s", log_header, (*sc_pp)->s_name);
		if (__alloc_asc(ascg_p, ini_p, *sc_pp))
			ini_p->i_op->i_add_section(ini_p, *sc_pp);
		sc_pp++;
	}
	nid_log_debug("%s: display added sections end", log_header);

	new_ini.i_op->i_cleanup(&new_ini);
	return 0;
}

/*
 * Input:
 * 	need_lck: 0 if the caller hold the list lock
 * 		  1 otherwise
 * Return:
 * 	 1: half done, completed asychronously
 * 	-1: failed
 * 	-2: failed, please re-try. This may happen if the caller hold the list_lck
 */
static int
ascg_delete_device(struct ascg_interface *ascg_p, char *devname, int sfd, int need_lock)
{
	char *log_header = "ascg_delete_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p = NULL;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct mwl_message_node *mn_p;
	struct ascg_message *msg_p;
	int op_counter;
	int rc = 1, found_it = 0;

	nid_log_notice("%s: delete %s ...", log_header, devname);

	if (need_lock)
		pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_name(ascg_p, devname, 0);
	if (chan_p) {
		/*
		 * remove it from the asc list,
		 * so nobody can send any request to this asc any more
		 */
		list_del(&chan_p->a_list);	// removed from p_asc_head
		list_add_tail(&chan_p->a_list, &priv_p->p_delete_asc_head);
	} else {
		list_for_each_entry(chan_p, struct ascg_channel, &priv_p->p_delete_asc_head, a_list) {
			asc_p = (struct asc_interface *)chan_p->a_chan;
			if (!strcmp(asc_p->as_op->as_get_devname(asc_p), devname)) {
				found_it = 1;
				break;
			}
		}
		if (!found_it) {
			nid_log_notice("%s: not found %s ", log_header, devname);
			rc = -1;
		} else {
			nid_log_notice("%s: re-try to delete %s", log_header, devname);
			// the reason we come here is that we failed to delete it last time due to op_busy
			// or in some status which are not allowed to delete
		}
	}
	if (need_lock)
		pthread_mutex_unlock(&priv_p->p_list_lck);

	if (rc < 0)
		return rc;

	asc_p = (struct asc_interface *)chan_p->a_chan;
	while ((op_counter = asc_p->as_op->as_op_busy(asc_p)) || !asc_p->as_op->as_ok_delete_device(asc_p)) {
		nid_log_info("%s: delete %s, op_counter:%d, need_lck:%d",
			log_header, devname, op_counter, need_lock);
		if (!need_lock) {
			/*
			 * if the caller hold the list lock, we shouldn't block here
			 * let the caller decide next step
			 */
			return -2;
		}

		/* othewise we can sleep here while we are not locking the list */
		sleep(1);
	}

	while (!(mn_p = mwl_p->m_op->m_get_free_mnode(mwl_p))) {
		nid_log_warning("%s: out of mnode", log_header);
		sleep(1);	// almost impossible
	}
	mn_p->m_type = NID_CHANNEL_TYPE_ASC_CODE;
	mn_p->m_id = asc_p->as_op->as_get_minor(asc_p);
	mn_p->m_req = NID_REQ_DELETE_DEVICE;
	if (!mn_p->m_data) {
		mn_p->m_data = (void *)x_calloc(1, sizeof(*msg_p));
		mn_p->m_container = mn_p->m_data;
		mn_p->m_free = free;
	}
	msg_p = (struct ascg_message *)mn_p->m_data;
	mn_p->m_data = (void *)msg_p;
	msg_p->m_asc = asc_p;
	msg_p->m_owner_code = sfd;
	msg_p->m_callback = __delete_callback;
	mwl_p->m_op->m_insert(mwl_p, mn_p);	// cannot be failed since delete request is unique
	asc_p->as_op->as_xdev_delete_device(asc_p);
	return rc;
}

/*
 * Return:
 * 	 1: half done, completed asychronously
 * 	-1: failed
 */
static int
ascg_ioerror_device(struct ascg_interface *ascg_p, char *devname, int sfd)
{
	char *log_header = "ascg_ioerror_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct mq_interface *mq_p = priv_p->p_mq;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct ascg_message *msg_p;
	struct mwl_message_node *mn_p;
	struct mq_message_node *qmn_p;
	int rc = 1;

	nid_log_warning("%s: start {%s} ...", log_header, devname);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_name(ascg_p, devname, 0);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		nid_log_debug("%s: a_op_get %s", log_header, asc_p->as_op->as_get_devname(asc_p));
		asc_p->as_op->as_op_get(asc_p);
	} else {
		nid_log_warning("%s: not found %s", log_header, devname);
		close(sfd);
		rc = -1;
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);

	if (rc > 0) {
		while (!(mn_p = mwl_p->m_op->m_get_free_mnode(mwl_p))) {
			nid_log_warning("%s: out of mnode", log_header);
			sleep(1);	// almost impossible
		}
		mn_p->m_type = NID_CHANNEL_TYPE_NONE_CODE;
		mn_p->m_id = asc_p->as_op->as_get_minor(asc_p);
		mn_p->m_req = NID_REQ_IOERROR_DEVICE;
		if (!mn_p->m_data) {
			mn_p->m_data = (void *)x_calloc(1, sizeof(*msg_p));
			mn_p->m_container = mn_p->m_data;
			mn_p->m_free = free;
		}
		msg_p = (struct ascg_message *)mn_p->m_data;
		msg_p->m_asc = asc_p;
		msg_p->m_owner_code = sfd;
		msg_p->m_callback = _ioerror_callback;
		mwl_p->m_op->m_insert(mwl_p, mn_p);

		if (asc_p->as_op->as_ex_lock(asc_p)) {
			asc_p->as_op->as_xdev_ioerror_device(asc_p);
		} else {
			nid_log_warning("%s: busy, %s", log_header, devname);
			while (!(qmn_p = mq_p->m_op->m_get_free_mnode(mq_p))) {
				nid_log_warning("%s: out of mnode", log_header);
				sleep(1);	// rarely happen
			}

			if (!qmn_p->m_data) {
				qmn_p->m_data = (void *)x_calloc(1, sizeof(*msg_p));
				qmn_p->m_container = qmn_p->m_data;
				qmn_p->m_free = free;
			}
			msg_p = (struct ascg_message *)qmn_p->m_data;
			msg_p->m_asc = asc_p;
			msg_p->m_req = NID_REQ_IOERROR_DEVICE;
			mq_p->m_op->m_enqueue(mq_p, qmn_p);
		}
	}
	return rc;
}

static void
ascg_stop(struct ascg_interface *ascg_p)
{
	char *log_header = "ascg_stop";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct acg_interface *acg_p;
	char *devname;
	struct nid_message nid_msg;
	int len;

	nid_log_notice("%s: start ...", log_header);
	/*
	 * we cannot stop agent right now. Instead we leave this task to the driver
	 * 1> we must guarantee no new channel could be established any more
	 * 2> delete all existing resource channels
	 * 3> stop the driver. Then the driver will send stop message to the agent.
	 */
	pthread_mutex_lock(&priv_p->p_list_lck);
	priv_p->p_stop = 1;	// so no new channel could be established
	while (!list_empty(&priv_p->p_asc_head)) {
		chan_p = list_first_entry(&priv_p->p_asc_head, struct ascg_channel, a_list);
		asc_p = (struct asc_interface *)chan_p->a_chan;
		if (chan_p->a_chan_id < 0) {
			devname = asc_p->as_op->as_get_devname(asc_p);
			while (ascg_p->ag_op->ag_delete_device(ascg_p, devname, -2, 0) == -2) {
				pthread_mutex_unlock(&priv_p->p_list_lck);
				sleep(1);
				pthread_mutex_lock(&priv_p->p_list_lck);
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);

	while (!priv_p->p_dr_done) {
		nid_log_info("%s: waiting to be done", log_header);
		sleep(1);
	}

	/*
	 * Inform the driver to stop
	 */
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_AGENT_STOP;
	len = 32;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
	nid_log_debug("%s: sending NID_REQ_AGENT_STOP(%d) to the driver, len:%d",
		log_header, nid_msg.m_req, len);

	// make acg donot accept new connection any more
	acg_p = priv_p->p_acg;
	acg_p->ag_op->ag_set_stop(acg_p);

	nid_log_info("ascg_stop: done");
}

static void
ascg_dispaly(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct asc_description *ad_p;

	nid_log_debug("ascg_dispaly:");
	nid_log_debug("agent server channels:");
	list_for_each_entry(ad_p, struct asc_description, &priv_p->p_asc_head, as_list) {
		nid_log_debug("      [%s]", ad_p->as_uuid);
		nid_log_debug("      ip:%s", ad_p->as_ip);
	}
}

static struct ini_interface*
ascg_get_ini(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	return priv_p->p_ini;
}

static void
ascg_check_driver(struct ascg_interface *ascg_p, int sfd)
{
	char *log_header = "ascg_check_driver";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_notice("%s: start ...", log_header);
	/*
	 * Send check driver command
	 */
	memset(&nid_msg, 0 , sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHECK_DRIVER;
	nid_msg.m_sfd = sfd;
	nid_msg.m_has_sfd = 1;
	len = 32;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
	nid_log_debug("%s: sending NID_REQ_CHECK_DRIVER(%d) to the driver, len:%d",
		log_header, nid_msg.m_req, len);

	nid_log_info("acg_check_driver: done");
}

static void
ascg_xdev_init_device(struct ascg_interface *ascg_p, int minor, uint64_t size, uint32_t blksize)
{
	char *log_header = "ascg_xdev_init_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_info("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_INIT_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	nid_msg.m_chan_type = NID_CHANNEL_TYPE_ASC_CODE;
	nid_msg.m_has_chan_type= 1;
	nid_msg.m_size = size;
	nid_msg.m_has_size = 1;
	nid_msg.m_blksize = blksize;
	nid_msg.m_has_blksize = 1;
	len = 128 + NID_MAX_UUID;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ascg_xdev_start_channel(struct ascg_interface *ascg_p, int the_id, uint8_t chan_type, int rsfd, int dsfd)
{
	char *log_header = "ascg_xdev_start_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_info("%s: the_id:%d", log_header, the_id);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_START;
	nid_msg.m_chan_type = chan_type;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = the_id;
	nid_msg.m_has_chan_id = 1;
	nid_msg.m_rsfd = rsfd;
	nid_msg.m_has_rsfd = 1;
	nid_msg.m_dsfd = dsfd;
	nid_msg.m_has_dsfd = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ascg_xdev_upgrade_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id, int force)
{
	char *log_header = "ascg_xdev_upgrade_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_info("%s: chan_type:%d, chan_id:%d, force:%d",
		log_header, chan_type, chan_id, force);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_UPGRADE;
	nid_msg.m_chan_type = chan_type;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = chan_id;
	nid_msg.m_has_chan_id = 1;
	nid_msg.m_upgrade_force = force;
	nid_msg.m_has_upgrade_force = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static int
ascg_upgrade_device(struct ascg_interface *ascg_p, char *devname, int sfd, char force)
{
	char *log_header = "ascg_upgrade_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct mwl_message_node *mn_p;
	struct ascg_message *msg_p;

	nid_log_debug("%s: start (%s) ...", log_header, devname);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_name(ascg_p, devname, 0);
	if (chan_p == NULL) {
		pthread_mutex_unlock(&priv_p->p_list_lck);
		return ENODEV; //ENODEV: 19  No such device
	}
	asc_p = (struct asc_interface *)chan_p->a_chan;
	asc_p->as_op->as_xdev_upgrade(asc_p, force);

	while (!(mn_p = mwl_p->m_op->m_get_free_mnode(mwl_p))) {
		nid_log_warning("%s: out of mnode", log_header);
		sleep(1);	// almost impossible
	}
	mn_p->m_type = NID_CHANNEL_TYPE_ASC_CODE;
	mn_p->m_id = asc_p->as_op->as_get_minor(asc_p);
	pthread_mutex_unlock(&priv_p->p_list_lck);

	mn_p->m_req = NID_REQ_UPGRADE;
	if (!mn_p->m_data) {
		mn_p->m_data = (void *)x_calloc(1, sizeof(*msg_p));
		mn_p->m_container = mn_p->m_data;
		mn_p->m_free = free;
	}
	msg_p = (struct ascg_message *)mn_p->m_data;
	mn_p->m_data = (void *)msg_p;
	msg_p->m_asc = asc_p;
	msg_p->m_owner_code = sfd;
	msg_p->m_callback = _upgrade_callback;
	mwl_p->m_op->m_insert(mwl_p, mn_p);	// cannot be failed since delete request is unique

	return 0;
}

static void
ascg_xdev_recover_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id)
{
	char *log_header = "ascg_xdev_recover_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_info("%s: (chan_type:%d chan_id:%d)", log_header, chan_type, chan_id);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_RECOVER;
	nid_msg.m_chan_type = chan_type;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = chan_id;
	nid_msg.m_has_chan_id = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ascg_xdev_delete_device(struct ascg_interface *ascg_p, int minor)
{
	char *log_header = "ascg_xdev_delete_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_notice("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_DELETE_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	len = 32;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ascg_xdev_keepalive_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id)
{
	char *log_header = "ascg_xdev_keepalive_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_debug("%s: start (chan_type:%d, chan_id:%d) ...", log_header, chan_type, chan_id);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHANNEL;
	nid_msg.m_req_code = NID_CODE_CHANNEL_KEEPALIVE;
	nid_msg.m_chan_type = chan_type;
	nid_msg.m_has_chan_type = 1;
	nid_msg.m_chan_id = chan_id;
	nid_msg.m_has_chan_id = 1;
	len = 128;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static int
ascg_rdev_init_device(struct ascg_interface *ascg_p, int minor, uint8_t ret_code)
{
	char *log_header = "ascg_rdev_init_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	int rc = 0;

	nid_log_notice("%s: init minor:%d", log_header, minor);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_minor(ascg_p, minor, 0);
	if (!chan_p) {
		rc = -1;
		goto out;
	}

	asc_p = (struct asc_interface *)chan_p->a_chan;
	rc = asc_p->as_op->as_rdev_init_device(asc_p, ret_code);
	if (rc) {
		nid_log_error("%s: failed, minor:%d is not waiting to init",
			log_header, minor);
		rc = -1;
		goto out;
	}

out:
	pthread_mutex_unlock(&priv_p->p_list_lck);
	return rc;
}

static void
ascg_rdev_keepalive(struct ascg_interface *ascg_p, int chan_type, int chan_id, uint64_t recv_seq, uint64_t wait_seq, uint32_t status)
{
	char *log_header = "ascg_rdev_keepalive";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;

	nid_log_debug("%s: got keepalive for chan_tupe:%d, chan_id:%d",
		log_header, chan_type, chan_id);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel(ascg_p, chan_type, chan_id, 0);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		asc_p->as_op->as_rdev_keepalive(asc_p, recv_seq, wait_seq, status);
	} else {
		nid_log_warning("%s: obsoleted, chan_tupe:%d, chan_id:%d",
			log_header, chan_type, chan_id);
	}
	pthread_mutex_unlock(&priv_p->p_list_lck);
}

static int
ascg_rdev_ioerror_device(struct ascg_interface *ascg_p, int minor, char code)
{
	char *log_header = "ascg_rdev_ioerror_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct mwl_message_node *mn_p;
	struct ascg_message *msg_p;
	int rc = 0;

	nid_log_warning("%s: minor:%d", log_header, minor);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_minor(ascg_p, minor, 0);
	pthread_mutex_unlock(&priv_p->p_list_lck);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		asc_p->as_op->as_rdev_ioerror_device(asc_p);
		mn_p = mwl_p->m_op->m_remove(mwl_p, NID_CHANNEL_TYPE_NONE_CODE, minor, NID_REQ_IOERROR_DEVICE);
		if (mn_p) {
			msg_p = (struct ascg_message *)mn_p->m_data;
			write(msg_p->m_owner_code, &code, 1);
			nid_log_debug("%s: callback, minor:%d", log_header, minor);
			msg_p->m_callback(ascg_p, msg_p);
			free(msg_p);
			mn_p->m_data = NULL;
			mwl_p->m_op->m_put_free_mnode(mwl_p, mn_p);
		}
	} else {
		rc = -1;
		nid_log_warning("%s: not found minor:%d", log_header, minor);
	}

	return rc;
}

static int
ascg_rdev_delete_device(struct ascg_interface *ascg_p, int minor)
{
	char *log_header = "ascg_rdev_delete_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	int rc = 0;
	struct ini_interface *ini_p = priv_p->p_ini;
	struct ini_section_content *sc_p;

	nid_log_notice("%s: delete minor:%d", log_header, minor);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_minor(ascg_p, minor, 1);
	if (!chan_p) {
		rc = -1;
		goto out;
	}

	asc_p = (struct asc_interface *)chan_p->a_chan;
	rc = asc_p->as_op->as_rdev_delete_device(asc_p);
	if (rc) {
		nid_log_error("%s: failed, minor:%d is not waiting to delete",
			log_header, minor);
		rc = -1;
		goto out;
	}

	asc_p->as_op->as_cleanup(asc_p);
	free(asc_p);
	list_del(&chan_p->a_list);	// removed from p_delete_asc_head
	free(chan_p);
	sc_p = ini_p->i_op->i_search_section_by_key(ini_p, "minor", (void *)&minor);
	if (sc_p) {
		ini_p->i_op->i_cleanup_section(ini_p, sc_p->s_name);
	} else {
		nid_log_debug("%s: i_remove_section cannot find sc, minor:%d", log_header, minor);
	}

	__cleanup_mwl(ascg_p, NID_CHANNEL_TYPE_ASC_CODE, minor);

out:
	pthread_mutex_unlock(&priv_p->p_list_lck);
	return rc;
}

static int
ascg_rdev_recover_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id)
{
	char *log_header = "ascg_rdev_recover_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	int rc = 0;

	nid_log_notice("%s: recovering chan_type:%d, chan_id:%d", log_header, chan_type, chan_id);
	if (chan_type != NID_CHANNEL_TYPE_ASC_CODE) {
		nid_log_notice("%s: got wrong chan_type:%d, chan_id:%d", log_header, chan_type, chan_id);
		return -1;
	}

	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel(ascg_p, chan_type, chan_id, 0);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		nid_log_debug("%s: a_op_get %s", log_header, asc_p->as_op->as_get_devname(asc_p));
		asc_p->as_op->as_op_get(asc_p);
		chan_p->a_delay = 0;
		goto out;
	}

	rc = -1;
out:
	pthread_mutex_unlock(&priv_p->p_list_lck);
	if (!rc) {
		__cleanup_mwl(ascg_p, chan_type, chan_id);
		asc_p->as_op->as_rdev_recover_channel(asc_p);
	}
	return rc;
}

/*
 * Called when the agent got NID_REQ_CHAN_ESTABLISHED from the driver
 */
static int
ascg_rdev_start_channel(struct ascg_interface *ascg_p, int chan_type, int chan_id)
{
	char *log_header = "ascg_rdev_start_channel";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p = NULL;
	int rc = 0;

	nid_log_notice("%s: established (chan_type:%d, chan_id:%d)",
		log_header, chan_type, chan_id);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel(ascg_p, chan_type, chan_id, 0);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		asc_p->as_op->as_op_get(asc_p);
		goto out;
	} else {
		chan_p = __search_channel(ascg_p, chan_type, chan_id, 1);
		if (chan_p) {
			asc_p = (struct asc_interface *)chan_p->a_chan;
			asc_p->as_op->as_op_get(asc_p);
			goto out;
		}
	}
	rc = -1;
out:
	pthread_mutex_unlock(&priv_p->p_list_lck);
	if (!rc)
		asc_p->as_op->as_rdev_start_channel(asc_p);
	return rc;
}

static void
ascg_ioctl_lock(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	pthread_mutex_lock(&priv_p->p_ioctl_lck);
}

static void
ascg_ioctl_unlock(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	pthread_mutex_unlock(&priv_p->p_ioctl_lck);
}

static void
ascg_rdev_upgrade(struct ascg_interface *ascg_p, int chan_type, int chan_id, char code)
{
	char *log_header = "ascg_rdev_upgrade";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p;
	struct mwl_interface *mwl_p = priv_p->p_mwl;
	struct mwl_message_node *mn_p;
	struct ascg_message *msg_p;

	nid_log_notice("%s: chan_type:%d, chan_id:%d, code: %d",
		log_header, chan_type, chan_id, code);
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel(ascg_p, chan_type, chan_id, 0);
	if (chan_p) {
		asc_p = (struct asc_interface *)chan_p->a_chan;
		code = asc_p->as_op->as_rdev_upgrade(asc_p, code);
	} else {
		nid_log_warning("%s: obsoleted, chan_type:%d, chan_id:%d",
			log_header, chan_type, chan_id);
		code = ENODEV;
	}

	mn_p = mwl_p->m_op->m_remove(mwl_p, chan_type, chan_id, NID_REQ_UPGRADE);
	if (mn_p) {
		msg_p = (struct ascg_message *)mn_p->m_data;
		nid_log_debug("%s: callback, chan_type:%d, chan_id:%d",
			log_header, chan_type, chan_id);
		if (msg_p->m_owner_code > 0) {
			write(msg_p->m_owner_code, &code, 1);
		}
		msg_p->m_callback(ascg_p, msg_p);
		free(msg_p);
		mn_p->m_data = NULL;
		mwl_p->m_op->m_put_free_mnode(mwl_p, mn_p);
	}

	pthread_mutex_unlock(&priv_p->p_list_lck);
}

static struct asc_interface *
ascg_get_asc_by_uuid(struct ascg_interface *ascg_p, char *uuid)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	struct asc_interface *asc_p = NULL;
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_uuid(ascg_p, uuid, 0);
	pthread_mutex_unlock(&priv_p->p_list_lck);
	if (chan_p)
		asc_p = (struct asc_interface *)chan_p->a_chan;
	return asc_p;
}

static int
ascg_get_chan_id(struct ascg_interface *ascg_p, char *chan_uuid)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct ascg_channel *chan_p;
	pthread_mutex_lock(&priv_p->p_list_lck);
	chan_p = __search_channel_by_uuid(ascg_p, chan_uuid, 0);
	pthread_mutex_unlock(&priv_p->p_list_lck);
	if (chan_p) {
		assert(chan_p->a_chan_id >= 0);
		return chan_p->a_chan_id;
	}
	return -1;
}

static void
ascg_set_hook(struct ascg_interface *ascg_p, uint32_t cmd)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;

	nid_log_notice("ascg_set_hook: hook_flag %d -> %d", priv_p->p_hook_flag, cmd);
	priv_p->p_hook_flag = cmd;
}

static uint32_t
ascg_get_hook(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;

	nid_log_debug("ascg_get_hook: hook_flag %d", priv_p->p_hook_flag);
	return priv_p->p_hook_flag;
}

static void
ascg_xdev_ioerror_device(struct ascg_interface *ascg_p, int minor)
{
	char *log_header = "ascg_xdev_ioerror_device";
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct drv_interface *drv_p = priv_p->p_drv;
	struct nid_message nid_msg;
	int len;

	nid_log_warning("%s: minor:%d", log_header, minor);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_IOERROR_DEVICE;
	nid_msg.m_devminor = minor;
	nid_msg.m_has_devminor = 1;
	len = 32;
	drv_p->dr_op->dr_send_message(drv_p, &nid_msg, &len);
}

static void
ascg_cleanup(struct ascg_interface *ascg_p)
{
	struct ascg_private *priv_p = (struct ascg_private *)ascg_p->ag_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct mq_interface *mq_p = priv_p->p_mq;
	struct mwl_interface *mwl_p = priv_p->p_mwl;

	mwl_p->m_op->m_cleanup(mwl_p);
	pp2_p->pp2_op->pp2_put(pp2_p, priv_p->p_mwl);

	mq_p->m_op->m_cleanup(mq_p);
	pp2_p->pp2_op->pp2_put(pp2_p, priv_p->p_mq);

	pp2_p->pp2_op->pp2_put(pp2_p, priv_p);
	ascg_p->ag_private = NULL;
}

struct ascg_operations ascg_op = {
	.ag_ioctl_lock = ascg_ioctl_lock,
	.ag_ioctl_unlock = ascg_ioctl_unlock,
	.ag_failed_to_establish_channel = ascg_failed_to_establish_channel,
	.ag_display = ascg_dispaly,
	.ag_get_stat = ascg_get_stat,
	.ag_update = ascg_update,
	.ag_delete_device = ascg_delete_device,
	.ag_upgrade_device = ascg_upgrade_device,
	.ag_ioerror_device = ascg_ioerror_device,
	.ag_stop = ascg_stop,
	.ag_get_ini = ascg_get_ini,
	.ag_check_driver = ascg_check_driver,

	.ag_xdev_init_device = ascg_xdev_init_device,
	.ag_xdev_start_channel = ascg_xdev_start_channel,
	.ag_xdev_upgrade_channel = ascg_xdev_upgrade_channel,
	.ag_xdev_recover_channel = ascg_xdev_recover_channel,
	.ag_xdev_delete_device = ascg_xdev_delete_device,
	.ag_xdev_keepalive_channel = ascg_xdev_keepalive_channel,
	.ag_xdev_ioerror_device = ascg_xdev_ioerror_device,

	.ag_rdev_init_device = ascg_rdev_init_device,
	.ag_rdev_start_channel = ascg_rdev_start_channel,
	.ag_rdev_keepalive = ascg_rdev_keepalive,
	.ag_rdev_ioerror_device = ascg_rdev_ioerror_device,
	.ag_rdev_delete_device = ascg_rdev_delete_device,
	.ag_rdev_recover_channel = ascg_rdev_recover_channel,
	.ag_rdev_upgrade = ascg_rdev_upgrade,

	.ag_get_asc_by_uuid = ascg_get_asc_by_uuid,
	.ag_get_chan_id = ascg_get_chan_id,
	.ag_set_hook = ascg_set_hook,
	.ag_get_hook = ascg_get_hook,
	.ag_cleanup = ascg_cleanup,
};

int
ascg_initialization(struct ascg_interface *ascg_p, struct ascg_setup *setup)
{
	char *log_header = "ascg_initialization";
	struct ascg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
//	struct tp_interface *tp_p;
	struct mwl_interface *mwl_p;
	struct mwl_setup mwl_setup;
	struct mq_interface *mq_p;
	struct mq_setup mq_setup;
	struct ascg_housekeeping_job *job;
	struct ini_key_content *kc_p;
	struct pp2_interface *pp2_p = setup->pp2;
	char *section_type;
	pthread_attr_t attr;
	pthread_t thread_id;

	nid_log_notice("%s: start ...", log_header);
	ascg_p->ag_op = &ascg_op;
	priv_p = (struct ascg_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	ascg_p->ag_private = priv_p;
	priv_p->p_hook_flag = 1;
	priv_p->p_ini = setup->ini;
	if (!priv_p->p_ini)
		return 0;	// this is just a test program
	priv_p->p_drv = setup->drv;
	priv_p->p_mpk = setup->mpk;
	priv_p->p_tp = setup->tp;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_dyn_pp2 = setup->dyn_pp2;
	priv_p->p_agent_keys = setup->agent_keys;
	priv_p->p_acg = setup->acg;
	pthread_mutex_init(&priv_p->p_ioctl_lck, NULL);
	pthread_mutex_init(&priv_p->p_list_lck, NULL);
	pthread_mutex_init(&priv_p->p_update_lck, NULL);
	pthread_mutex_init(&priv_p->p_chan_lck, NULL);
	pthread_cond_init(&priv_p->p_update_cond, NULL);
	pthread_cond_init(&priv_p->p_chan_cond, NULL);

	mwl_p = (struct mwl_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*mwl_p));
	priv_p->p_mwl = mwl_p;
	mwl_setup.size = 1024;
	mwl_setup.pp2 = pp2_p;
	mwl_initialization(mwl_p, &mwl_setup);

	mq_p = (struct mq_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*mq_p));
	priv_p->p_mq = mq_p;
	mq_setup.size = 1024;
	mq_setup.pp2 = pp2_p;
	mq_initialization(mq_p, &mq_setup);

	/*
	 * For agent-service channels
	 */
	INIT_LIST_HEAD(&priv_p->p_asc_head);
	INIT_LIST_HEAD(&priv_p->p_delete_asc_head);
	INIT_LIST_HEAD(&priv_p->p_waiting_chan_head);
	ini_p = priv_p->p_ini;
	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		kc_p = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		section_type = (char*)kc_p->k_value;
		if (!strcmp(section_type, AGENT_TYPE_ASC))
			__alloc_asc(ascg_p, ini_p, sc_p);
		else
			continue;
	}

#if 0
	tp_p = priv_p->p_tp;
	job = (struct ascg_housekeeping_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
	job->j_ascg = ascg_p;
	job->j_delay = 1;
	job->j_header.jh_do = _do_housekeeping;
	tp_p->t_op->t_delay_job_insert(tp_p, (struct tp_jobheader *)job, job->j_delay);
#else
	job = (struct ascg_housekeeping_job *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*job));
	job->j_ascg = ascg_p;
	job->j_delay = 1;
	job->j_do_next = 1;
	job->j_header.jh_do = _do_dr_housekeeping;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _do_housekeeping, (void *)job);
#endif

	nid_log_notice("%s: done", log_header);
	return 0;
}
