/*
 * crcg.c
 * 	Implementation of CAS (Content-Addressed Storage) Read Cache Module Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "ini_if.h"
#include "allocator_if.h"
#include "ppg_if.h"
#include "crc_if.h"
#include "crcg_if.h"
#include "rc_if.h"
#include "umpk_crc_if.h"
#include "list.h"
#include "tpg_if.h"
#include "tp_if.h"

struct crcg_private {
	struct ini_interface		**p_ini;
	struct ppg_interface		*p_ppg;
	struct allocator_interface	*p_allocator;
	struct fpn_interface		*p_fpn;
	struct brn_interface		*p_brn;
	struct srn_interface		*p_srn;
	struct tp_interface		*p_gtp;
	struct tpg_interface		*p_tpg;
	pthread_mutex_t			p_lck;
	struct crc_setup		p_setup[CRCG_MAX_CRC];
	struct rc_interface		*p_rc[CRCG_MAX_CRC];
	int				p_counter;
};

struct rc_interface *
crcg_search_crc(struct crcg_interface *crcg_p, char *crc_uuid)
{
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *ret_crc = NULL;
	int i;

	if (crc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			ret_crc = priv_p->p_rc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_crc;
}

static void
__crcg_start_recover_crc(struct crcg_private *, struct rc_interface *);

struct rc_interface *
crcg_search_and_create_crc(struct crcg_interface *crcg_p, char *crc_uuid)
{
	char *log_header = "crcg_search_and_create_crc";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	struct pp_interface *pp_p;
	struct rc_interface *ret_crc = NULL;
	struct rc_setup rc_setup;
	int i;

	if (crc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			if (!priv_p->p_rc[i]) {
				pp_p = ppg_p->pg_op->pg_search_and_create_pp(ppg_p,
						crc_setup->pp_name, ALLOCATOR_SET_CRC_PP);
				if (!pp_p) {
					nid_log_error("%s: according pp does not exist(pp_name:%s)",
							log_header, crc_setup->pp_name);
					break;
				}
				priv_p->p_rc[i] = x_calloc(1, sizeof(*priv_p->p_rc[i]));
				memset(&rc_setup, 0, sizeof(rc_setup));
				rc_setup.type = RC_TYPE_CAS;
				rc_setup.fpn = priv_p->p_fpn;
				rc_setup.srn = priv_p->p_srn;
				rc_setup.pp_name = crc_setup->pp_name;
				rc_setup.pp = pp_p;
				rc_setup.allocator = priv_p->p_allocator;
				rc_setup.brn = priv_p->p_brn;
				rc_setup.uuid = crc_setup->uuid;
				rc_setup.cachedev = crc_setup->cachedev;
				rc_setup.cachedevsz = crc_setup->cachedevsz;
				rc_initialization(priv_p->p_rc[i], &rc_setup);
				__crcg_start_recover_crc(priv_p, priv_p->p_rc[i]);
//				priv_p->p_counter++;
			}
			ret_crc = priv_p->p_rc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_crc;
}

static int
crcg_add_crc(struct crcg_interface *crcg_p, char *crc_uuid, char *pp_name, char *cache_device, int cache_size)
{
	char *log_header = "crcg_add_crc";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
//	struct ppg_interface *ppg_p = priv_p->p_ppg;
//	struct pp_interface *pp_p;
	struct rc_interface *rc_p = NULL;
//	struct rc_setup rc_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			nid_log_error("%s: crc:%s already exists.",
					log_header, crc_uuid);
			goto out;
		}
		if (empty_index == -1 && crc_setup->uuid[0] == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
//		pp_p = ppg_p->pg_op->pg_search_pp(ppg_p, pp_name);
//		if (!pp_p) {
//			nid_log_error("%s: according pp does not exist(pp_name:%s)",
//					log_header, crc_setup->pp_name);
//			goto out;
//		}
//		priv_p->p_rc[empty_index] = x_calloc(1, sizeof(*priv_p->p_rc[i]));
		crc_setup = &priv_p->p_setup[empty_index];
		strcpy(crc_setup->uuid, crc_uuid);
		strcpy(crc_setup->pp_name, pp_name);
		strcpy(crc_setup->cachedev, cache_device);
/*		memset(&rc_setup, 0, sizeof(rc_setup));
		rc_setup.type = RC_TYPE_CAS;
		rc_setup.fpn = priv_p->p_fpn;
		rc_setup.srn = priv_p->p_srn;
		rc_setup.pp_name = crc_setup->pp_name;
		rc_setup.pp = pp_p;
		rc_setup.allocator = priv_p->p_allocator;
		rc_setup.brn = priv_p->p_brn;
		rc_setup.uuid = crc_setup->uuid;
		rc_setup.cachedev =  crc_setup->cachedev;
		rc_setup.cachedevsz = cache_size;
		rc_initialization(priv_p->p_rc[empty_index], &rc_setup);
		__crcg_start_recover_crc(priv_p, priv_p->p_rc[empty_index]);*/
		crc_setup->cachedevsz = cache_size;
		priv_p->p_counter++;

		pthread_mutex_unlock(&priv_p->p_lck);//give the lock to crcg_search_and_create_crc
		rc_p = crcg_search_and_create_crc(crcg_p, crc_uuid);
		pthread_mutex_lock(&priv_p->p_lck);//lock back

		if (!rc_p)
			goto out;
		rc = 0;
	} else {
		nid_log_error("%s: too many crc.", log_header);
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

/* job that recovers an crc */
struct crcg_recover_crc_job {
	struct tp_jobheader	j_header;
	struct rc_interface	*j_rc;
};

static void
__crcg_recover_crc(struct tp_jobheader *jh_p)
{
	struct crcg_recover_crc_job *job_p = (struct crcg_recover_crc_job *)jh_p;
	struct rc_interface *rc_p = job_p->j_rc;

	rc_p->rc_op->rc_set_recover_state(rc_p, RC_RECOVER_DOING);
	rc_p->rc_op->rc_recover(rc_p);
	rc_p->rc_op->rc_set_recover_state(rc_p, RC_RECOVER_DONE);
}

static void
__crcg_free_recover_crc(struct tp_jobheader *jh_p)
{
	free((void *)jh_p);
}

static void
__crcg_start_recover_crc(struct crcg_private *priv_p, struct rc_interface *rc_p)
{
	char *log_header = "__crcg_start_recover_crc";
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct crcg_recover_crc_job *job_p;

	nid_log_warning("%s: start ...", log_header);

	job_p = (struct crcg_recover_crc_job *)x_calloc(1, sizeof(*job_p));
	job_p->j_header.jh_do = __crcg_recover_crc;
	job_p->j_header.jh_free = __crcg_free_recover_crc;
	job_p->j_rc = rc_p;
	tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
}

static void
crcg_dropcache_start(struct crcg_interface *crcg_p, char *crc_uuid, char *chan_uuid, int do_sync)
{
	char *log_header = "crcg_dropcache_start";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;

	nid_log_warning("%s: start (crc_uuid:%s, chan_uuid:%s, sync:%d) ...",
			log_header, crc_uuid, chan_uuid, do_sync);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				crc_p->cr_op->cr_dropcache_start(crc_p, chan_uuid, do_sync);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
crcg_set_fp_wgt(struct crcg_interface *crcg_p, char *crc_uuid, int fp_wgt)
{
	char *log_header = "crcg_setwgt";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;

	nid_log_warning("%s: start (crc_uuid:%s, fp_wgt:%d) ...",
			log_header, crc_uuid, fp_wgt);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				crc_p->cr_op->cr_set_fp_wgt(crc_p, fp_wgt);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
crcg_dropcache_stop(struct crcg_interface *crcg_p, char *crc_uuid, char *chan_uuid)
{
	char *log_header = "crcg_dropcache_stop";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;

	nid_log_warning("%s: start (crc_uuid:%s, chan_uuid:%s) ...",
			log_header, crc_uuid, chan_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				crc_p->cr_op->cr_dropcache_stop(crc_p, chan_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static uint64_t
crcg_freespace(struct crcg_interface *crcg_p, char *crc_uuid)
{
	char *log_header = "crcg_freespace";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	uint64_t freespace = 0;

	nid_log_warning("%s: start (crc_uuid:%s) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				freespace = crc_p->cr_op->cr_freespace(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return freespace;
}

static int*
crcg_sp_heads_size(struct crcg_interface *crcg_p, char *crc_uuid)
{
	char *log_header = "crcg_freespace";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	int *sp_heads_size;

	nid_log_warning("%s: start (crc_uuid:%s) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				sp_heads_size = crc_p->cr_op->cr_sp_heads_size(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return sp_heads_size;
}

static void
crcg_cse_hit(struct crcg_interface *crcg_p, char *crc_uuid, int *hit_counter, int *unhit_counter)
{
	char *log_header = "crcg_cse_hit";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;

	nid_log_warning("%s: start (crc_uuid:%s, ) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				crc_p->cr_op->cr_cse_hit(crc_p, hit_counter, unhit_counter);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static unsigned char
crcg_check_fp(struct crcg_interface *crcg_p,  char *crc_uuid)
{
	char *log_header = "crcg_check_fp";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	unsigned char rc = 1;

	nid_log_warning("%s: start (crc_uuid:%s, ) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				rc = crc_p->cr_op->cr_check_fp(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static struct umessage_crc_information_resp_freespace_dist *
crcg_freespace_dist(struct crcg_interface *crcg_p,  char *crc_uuid)
{
	char *log_header = "crcg_freespace_dist";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	struct umessage_crc_information_resp_freespace_dist *ret_dist = NULL;

	nid_log_warning("%s: start (crc_uuid:%s, ) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				ret_dist = crc_p->cr_op->cr_freespace_dist(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return ret_dist;
}

static int
crcg_get_nse_stat(struct crcg_interface *crcg_p, char *crc_uuid, char *chan_uuid, struct nse_stat *stat)
{
	char *log_header = "crcg_information_nse_counter";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i, rc = -1;

	nid_log_warning("%s: start (crc_uuid:%s, chan_uuid:%s) ...",
			log_header, crc_uuid, chan_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				rc = crc_p->cr_op->cr_get_nse_stat(crc_p, chan_uuid, stat);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static void *
crcg_nse_traverse_start(struct crcg_interface *crcg_p, char *crc_uuid, char *chan_uuid)
{
	char *log_header = "crcg_nse_traverse_start";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	void *chan_handle = NULL;

	nid_log_warning("%s: start (crc_uuid:%s, ) ...", log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				chan_handle = crc_p->cr_op->cr_nse_traverse_start(crc_p, chan_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return chan_handle;
}

static int
crcg_nse_traverse_next(struct crcg_interface *crcg_p, char *crc_uuid,
		void *chan_handle, char *fp_buf, uint64_t *block_index)
{
	char *log_header = "crcg_nse_traverse_next";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i, rc = -1;

	nid_log_warning("%s: start (crc_uuid:%s) ...",
			log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				rc = crc_p->cr_op->cr_nse_traverse_next(crc_p, chan_handle, fp_buf, block_index);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static void
crcg_nse_traverse_stop(struct crcg_interface *crcg_p, char *crc_uuid, void *chan_handle)
{
	char *log_header = "crcg_nse_traverse_stop";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;

	nid_log_warning("%s: start (crc_uuid:%s) ...",
			log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				crc_p->cr_op->cr_nse_traverse_stop(crc_p, chan_handle);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static struct list_head *
crcg_dsbmp_rtree_list(struct crcg_interface *crcg_p, char *crc_uuid)
{
	char *log_header = "crcg_dsbmp_rtree_list";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	struct list_head *list_head = NULL;

	nid_log_warning("%s: start (crc_uuid:%s) ...",
			log_header, crc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				list_head = crc_p->cr_op->cr_dsbmp_rtree_list(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return list_head;
}

static struct dsrec_stat
crcg_get_dsrec_stat(struct crcg_interface *crcg_p, char *crc_uuid)
{
	char *log_header = "crcg_get_dsrec_stat";
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	struct crc_setup *crc_setup = &priv_p->p_setup[0];
	struct rc_interface *rc_p;
	struct crc_interface *crc_p;
	int i;
	struct dsrec_stat dsrec_stat;

	nid_log_warning("%s: start (crc_uuid:%s) ...",
			log_header, crc_uuid);
	memset((void *)&dsrec_stat, 0, sizeof(dsrec_stat));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++, crc_setup++) {
		if (!strcmp(crc_setup->uuid, crc_uuid)) {
			rc_p = priv_p->p_rc[i];
			if (rc_p) {
				crc_p = rc_p->rc_op->rc_get_crc(rc_p);
				dsrec_stat = crc_p->cr_op->cr_get_dsrec_stat(crc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return dsrec_stat;
}

static struct crc_setup *
crcg_get_all_crc_setup(struct crcg_interface *crcg_p, int *num_crc)
{
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;

	*num_crc = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

static int *
crcg_get_working_crc_index(struct crcg_interface *crcg_p, int *num_crc)
{
	struct crcg_private *priv_p = (struct crcg_private *)crcg_p->rg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CRCG_MAX_CRC; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_rc[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_crc = count;

	return ret;
}

struct crcg_operations crcg_op = {
	.rg_search_crc = crcg_search_crc,
	.rg_search_and_create_crc = crcg_search_and_create_crc,
	.rg_add_crc = crcg_add_crc,
	.rg_dropcache_start = crcg_dropcache_start,
	.rg_dropcache_stop = crcg_dropcache_stop,
	.rg_set_fp_wgt = crcg_set_fp_wgt,
	.rg_freespace = crcg_freespace,
	.rg_sp_heads_size = crcg_sp_heads_size,
	.rg_cse_hit = crcg_cse_hit,
	.rg_check_fp = crcg_check_fp,
	.rg_freespace_dist = crcg_freespace_dist,
	.rg_get_nse_stat = crcg_get_nse_stat,
	.rg_nse_traverse_start = crcg_nse_traverse_start,
	.rg_nse_traverse_next = crcg_nse_traverse_next,
	.rg_nse_traverse_stop = crcg_nse_traverse_stop,
	.rg_dsbmp_rtree_list = crcg_dsbmp_rtree_list,
	.rg_get_dsrec_stat = crcg_get_dsrec_stat,
	.rg_get_all_crc_setup = crcg_get_all_crc_setup,
	.rg_get_working_crc_index = crcg_get_working_crc_index,
};

int
crcg_initialization(struct crcg_interface *crcg_p, struct crcg_setup *setup)
{
	char *log_header = "crcg_initialization";
	struct crcg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct crc_setup *crc_setup;
	int crc_counter = 0;

	nid_log_info("%s: start ...", log_header);

	assert(setup);
	crcg_p->rg_op = &crcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	crcg_p->rg_private = priv_p;

	priv_p->p_ini = setup->ini;
	ini_p = *priv_p->p_ini;
	priv_p->p_ppg = setup->ppg;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_brn = setup->brn;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_srn = setup->srn;
	priv_p->p_tpg = setup->tpg;
	priv_p->p_gtp = priv_p->p_tpg->tg_op->tpg_search_and_create_tp(priv_p->p_tpg, NID_GLOBAL_TP);

	crc_setup = &priv_p->p_setup[0];
	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "crc"))
			continue;

		memset(crc_setup, 0, sizeof(*crc_setup));
		strcpy(crc_setup->uuid, sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cache_device");
		strcpy(crc_setup->cachedev, (char *)(the_key->k_value));
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cache_size");
		crc_setup->cachedevsz = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "pp_name");
		strcpy(crc_setup->pp_name, (char *)(the_key->k_value));

		crc_setup++;
		crc_counter++;
		assert(crc_counter <= CRCG_MAX_CRC);
	}
	priv_p->p_counter = crc_counter;
	return 0;
}
