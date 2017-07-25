/*
 * bwcg.c
 * 	Implementation of None-Memory Write Cache Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid.h"
#include "ini_if.h"
#include "bwcg_if.h"
#include "wc_if.h"
#include "bwc_if.h"
#include "bfp_if.h"
#include "io_if.h"
#include "sdsg_if.h"
#include "ds_if.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "smc_if.h"

struct bwcg_private {
	struct ini_interface		**p_ini;
	struct sdsg_interface		*p_sdsg;
	struct tp_interface		*p_wtp;
	struct srn_interface		*p_srn;
	struct allocator_interface	*p_allocator;
	struct lstn_interface		*p_lstn;
	struct io_interface		*p_io;
	struct tp_interface		*p_gtp;
	struct tpg_interface		*p_tpg;
	pthread_mutex_t			p_lck;
	struct bwc_setup		p_setup[BWCG_MAX_BWC];
	struct wc_interface		*p_wc[BWCG_MAX_BWC];
	struct mqtt_interface		*p_mqtt;
	int				p_counter;
};

struct wc_interface *
bwcg_search_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *ret_wc = NULL;
	int i;

	if (bwc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			ret_wc = priv_p->p_wc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_wc;
}

static void
__bwcg_start_post_initialize_bwc(struct bwcg_private *, struct wc_interface *);

struct wc_interface *
bwcg_search_and_create_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid)
{
	char *log_header = "bwcg_search_and_create_bwc";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct ds_interface *ds_p = NULL;
	struct wc_interface *ret_wc = NULL;
	struct tpg_interface *tpg_p = priv_p->p_tpg;
	char *tp_name;
	struct wc_setup wc_setup;
	int i;

	if (bwc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			/* got a matched bwc */
			if (!priv_p->p_wc[i]) {
				if (sdsg_p)
					ds_p = sdsg_p->dg_op->dg_search_by_wc_and_create_sds(sdsg_p, bwc_uuid);
				if (!ds_p) {
					nid_log_error("%s: owner ds does not exist (wc_uuid:%s)",
							log_header, bwc_uuid);
					break;
				}
				priv_p->p_wc[i] = x_calloc(1, sizeof(*priv_p->p_wc[i]));
				memset(&wc_setup, 0, sizeof(wc_setup));
				wc_setup.pp = ds_p->d_op->d_get_pp(ds_p);
				wc_setup.srn = priv_p->p_srn;
				wc_setup.allocator = priv_p->p_allocator;
				wc_setup.lstn = priv_p->p_lstn;
				wc_setup.type = WC_TYPE_NONE_MEMORY;
				wc_setup.do_fp = bwc_setup->do_fp;
				wc_setup.rw_sync = bwc_setup->rw_sync;
				wc_setup.bufdevice = bwc_setup->bufdevice;
				wc_setup.bufdevicesz = bwc_setup->bufdevicesz;
				wc_setup.two_step_read = bwc_setup->two_step_read;
				wc_setup.uuid = bwc_setup->uuid;
				wc_setup.bfp_type = bwc_setup->bfp_type;
				wc_setup.coalesce_ratio = bwc_setup->coalesce_ratio;
				wc_setup.load_ratio_max = bwc_setup->load_ratio_max;
				wc_setup.load_ratio_min = bwc_setup->load_ratio_min;
				wc_setup.load_ctrl_level = bwc_setup->load_ctrl_level;
				wc_setup.flush_delay_ctl = bwc_setup->flush_delay_ctl;
				wc_setup.throttle_ratio = bwc_setup->throttle_ratio;
				wc_setup.low_water_mark = bwc_setup->low_water_mark;
				wc_setup.high_water_mark = bwc_setup->high_water_mark;
				wc_setup.max_flush_size = bwc_setup->max_flush_size;
				wc_setup.ssd_mode = bwc_setup->ssd_mode;
				wc_setup.write_delay_first_level = bwc_setup->write_delay_first_level;
				wc_setup.write_delay_second_level = bwc_setup->write_delay_second_level;
				wc_setup.write_delay_first_level_max_us = bwc_setup->write_delay_first_level_max_us;
				wc_setup.write_delay_second_level_max_us = bwc_setup->write_delay_second_level_max_us;
				wc_setup.mqtt = priv_p->p_mqtt;

				tp_name = bwc_setup->tp_name;
				wc_setup.tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_name);
				if(!wc_setup.tp) {
					nid_log_error("%s: failed to create tp (%s) instance",
							log_header, tp_name);
					assert(0);
				}
				wc_initialization(priv_p->p_wc[i], &wc_setup);
				__bwcg_start_post_initialize_bwc(priv_p, priv_p->p_wc[i]);
			}
			ret_wc = priv_p->p_wc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_wc;
}

static int
bwcg_update_water_mark(struct bwcg_interface *bwcg_p, char *bwc_uuid, int high_water_mark, int low_water_mark)
{
	char *log_header = "bwcg_update_water_mark";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i, rc = 0;

	nid_log_info("%s: ..start", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				if (!bwc_p)
					goto out;
				bwc_p->bw_op->bw_update_water_mark(bwc_p, low_water_mark, high_water_mark);
				bwc_setup->high_water_mark = high_water_mark;
				bwc_setup->low_water_mark = low_water_mark;
				rc = 1;

			}
			break;
		}
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static int
bwcg_update_delay_level(struct bwcg_interface *bwcg_p, char *bwc_uuid, struct bwc_delay_stat *delay_stat)
{
	char *log_header = "bwcg_update_delay_level";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i, rc = -1;

	nid_log_info("%s: ..start", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				if (!bwc_p)
					goto out;
				rc = bwc_p->bw_op->bw_update_delay_level(bwc_p, delay_stat);
				if (rc)
					goto out;
				bwc_setup->write_delay_first_level = delay_stat->write_delay_first_level;
				bwc_setup->write_delay_second_level = delay_stat->write_delay_second_level;
				bwc_setup->write_delay_first_level_max_us = delay_stat->write_delay_first_level_max_us;
				bwc_setup->write_delay_second_level_max_us = delay_stat->write_delay_second_level_max_us;
				rc = 0;
			}
			break;
		}
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static int
bwcg_add_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *bufdevice, int bufdevicesz,
		int rw_sync, int two_step_read, int do_fp, int bfp_type, double coalesce_ratio,
		double load_ratio_max, double load_ratio_min, double load_ctrl_level,
		double flush_delay_ctl, double throttle_ratio, char *tp_name,
		int max_flush_size, int ssd_mode, int write_delay_first_level, int write_delay_second_level,
		int high_water_mark, int low_water_mark)
{
	char *log_header = "bwcg_add_bwc";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct ds_interface *ds_p = NULL;
//	struct tpg_interface *tpg_p = priv_p->p_tpg;
	struct wc_interface *bwc_p = NULL;
//	struct wc_setup wc_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid))
			goto out;

		if (empty_index == -1 && bwc_setup->uuid[0] == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
		if (sdsg_p)
			ds_p = sdsg_p->dg_op->dg_search_by_wc(sdsg_p, bwc_uuid);
		if (!ds_p) {
			nid_log_error("%s: owner ds does not exist (wc_uuid:%s)",
					log_header, bwc_uuid);
			goto out;
		}
//		priv_p->p_wc[empty_index] = x_calloc(1, sizeof(*priv_p->p_wc[empty_index]));
		bwc_setup = &priv_p->p_setup[empty_index];
		strcpy(bwc_setup->uuid, bwc_uuid);
		strcpy(bwc_setup->bufdevice, bufdevice);
		strcpy(bwc_setup->tp_name, tp_name);
/*		memset(&wc_setup, 0, sizeof(wc_setup));
		wc_setup.pp =  ds_p->d_op->d_get_pp(ds_p);
		wc_setup.srn = priv_p->p_srn;
		wc_setup.allocator = priv_p->p_allocator;
		wc_setup.lstn = priv_p->p_lstn;
		wc_setup.type = WC_TYPE_NONE_MEMORY;
		wc_setup.do_fp = do_fp;
		wc_setup.rw_sync = rw_sync;
		wc_setup.bufdevice = bwc_setup->bufdevice;
		wc_setup.bufdevicesz = bufdevicesz;
		wc_setup.two_step_read = two_step_read;
		wc_setup.uuid = bwc_setup->uuid;
		wc_setup.bfp_type = bfp_type;
		wc_setup.coalesce_ratio = coalesce_ratio;
		wc_setup.load_ratio_max = load_ratio_max;
		wc_setup.load_ratio_min = load_ratio_min;
		wc_setup.load_ctrl_level = load_ctrl_level;
		wc_setup.flush_delay_ctl = flush_delay_ctl;
		wc_setup.throttle_ratio = throttle_ratio;

		wc_setup.tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_name);
		wc_initialization(priv_p->p_wc[empty_index], &wc_setup);
		__bwcg_start_post_initialize_bwc(priv_p, priv_p->p_wc[empty_index]);*/
		bwc_setup->do_fp = do_fp;
		bwc_setup->rw_sync = rw_sync;
		bwc_setup->bufdevicesz = bufdevicesz;
		bwc_setup->two_step_read = two_step_read;
		bwc_setup->bfp_type = bfp_type;
		bwc_setup->coalesce_ratio = coalesce_ratio;
		bwc_setup->load_ratio_max = load_ratio_max;
		bwc_setup->load_ratio_min = load_ratio_min;
		bwc_setup->load_ctrl_level = load_ctrl_level;
		bwc_setup->flush_delay_ctl = flush_delay_ctl;
		bwc_setup->throttle_ratio = throttle_ratio;
		bwc_setup->max_flush_size = max_flush_size;
		bwc_setup->ssd_mode = ssd_mode;
		bwc_setup->write_delay_first_level = write_delay_first_level;
		bwc_setup->write_delay_second_level = write_delay_second_level;
		// TODO add below two parameter to dynamic bwc add, set default by now.
		bwc_setup->write_delay_first_level_max_us = 200000;
		bwc_setup->write_delay_second_level_max_us = 500000;
		bwc_setup->high_water_mark = high_water_mark;
		bwc_setup->low_water_mark = low_water_mark;
		priv_p->p_counter++;

		pthread_mutex_unlock(&priv_p->p_lck);//give the lock the bwcg_search_and_create_bwc
		bwc_p = bwcg_search_and_create_bwc(bwcg_p, bwc_uuid);
		pthread_mutex_lock(&priv_p->p_lck);//lock back

		if (!bwc_p)
			goto out;
		rc = 0;
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

/* job that adds a bwc */
struct bwcg_recover_bwc_job {
	struct tp_jobheader	j_header;
	struct wc_interface	*j_wc;
};

static void
__bwcg_recover_bwc(struct tp_jobheader *jh_p)
{
	struct bwcg_recover_bwc_job *job_p = (struct bwcg_recover_bwc_job *)jh_p;
	struct wc_interface *wc_p = job_p->j_wc;

	wc_p->wc_op->wc_recover(wc_p);
}

static void
__bwcg_free_recover_bwc(struct tp_jobheader *jh_p)
{
	free((void *)jh_p);
}

static void
__bwcg_post_initialize_bwc(struct tp_jobheader *jh_p)
{
	struct bwcg_recover_bwc_job *job_p = (struct bwcg_recover_bwc_job *)jh_p;
	struct wc_interface *wc_p = job_p->j_wc;

	wc_p->wc_op->wc_post_initialization(wc_p);
}

static void
__bwcg_free_post_initialize_bwc(struct tp_jobheader *jh_p)
{
	free((void *)jh_p);
}

static int
bwcg_recover_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid)
{
	char *log_header = "bwcg_recover_bwc";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct wc_interface *wc_p;
	struct bwcg_recover_bwc_job *job_p;
	int i, recover_state = -1;

	nid_log_warning("%s: start(bwc_uuid:%s)...", log_header, bwc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				recover_state = wc_p->wc_op->wc_get_recover_state(wc_p);
				if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
					break;
				job_p = (struct bwcg_recover_bwc_job *)x_calloc(1, sizeof(*job_p));
				job_p->j_header.jh_do = __bwcg_recover_bwc;
				job_p->j_header.jh_free = __bwcg_free_recover_bwc;
				job_p->j_wc = wc_p;
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (recover_state == -1) {
		nid_log_warning("%s: bwc:%s does not exist!", log_header, bwc_uuid);
		return 1;
	}
	if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
		nid_log_warning("%s: bwc:%s recovery already started!", log_header, bwc_uuid);
	return 0;
}

static void
__bwcg_start_post_initialize_bwc(struct bwcg_private *priv_p, struct wc_interface *wc_p)
{
	char *log_header = "__bwcg_start_post_initialize_bwc";;
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct bwcg_recover_bwc_job *job_p;

	nid_log_warning("%s: start...", log_header);

	job_p = (struct bwcg_recover_bwc_job *)x_calloc(1, sizeof(*job_p));
	job_p->j_header.jh_do = __bwcg_post_initialize_bwc;
	job_p->j_header.jh_free = __bwcg_free_post_initialize_bwc;
	job_p->j_wc = wc_p;
	tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
}

static void
bwcg_recover_all_bwc(struct bwcg_interface *bwcg_p)
{
	char *log_header = "bwcg_recover_all_bwc";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct wc_interface *wc_p;
	struct bwcg_recover_bwc_job *job_p;
	int i, recover_state;

	nid_log_warning("%s: start...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		wc_p = priv_p->p_wc[i];
		if (!wc_p)
			continue;
		recover_state = wc_p->wc_op->wc_get_recover_state(wc_p);
		if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
			continue;
		job_p = (struct bwcg_recover_bwc_job *)x_calloc(1, sizeof(*job_p));
		job_p->j_header.jh_do = __bwcg_recover_bwc;
		job_p->j_header.jh_free = __bwcg_free_recover_bwc;
		job_p->j_wc = wc_p;
		tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static int
bwcg_get_recover_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid)
{
	char *log_header = "bwcg_get_recover_bwc";
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	int i, recover_state = -1;

	nid_log_warning("%s: start(bwc_uuid:%s)...", log_header, bwc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p)
				recover_state = wc_p->wc_op->wc_get_recover_state(wc_p);
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return recover_state;
}

static int
bwcg_remove_bwc(struct bwcg_interface *bwcg_p, char *bwc_uuid){
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct io_interface *io_p = priv_p->p_io;
	int i, rc;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			bwc_setup->uuid[0] = '\0';
			priv_p->p_wc[i] = NULL;
			priv_p->p_counter--;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	rc = io_p->io_op->io_destroy_wc(io_p, bwc_uuid);
	return rc;
}

static void
bwcg_info_stat(struct bwcg_interface *bwcg_p, char *bwc_uuid, struct umessage_bwc_information_resp_stat *stat_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_get_info_stat(bwc_p, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_info_list_stat(struct bwcg_interface *bwcg_p, char *bwc_uuid,char *res_uuid, struct umessage_bwc_information_list_resp_stat *stat_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_get_list_info(bwc_p, res_uuid, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_get_throughput(struct bwcg_interface *bwcg_p, char *bwc_uuid, struct bwc_throughput_stat *stat_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_get_throughput(bwc_p, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_get_rw(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid, struct bwc_rw_stat *stat_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_get_rw(bwc_p, res_uuid, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_get_delay(struct bwcg_interface *bwcg_p, char *bwc_uuid, struct bwc_delay_stat *stat_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_get_delay(bwc_p, stat_p);
				stat_p->write_delay_first_level_max_us = bwc_setup->write_delay_first_level_max_us;
				stat_p->write_delay_second_level_max_us = bwc_setup->write_delay_second_level_max_us;
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_reset_throughput(struct bwcg_interface *bwcg_p, char *bwc_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_reset_throughput(bwc_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_dropcache_start(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid, int do_sync)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_dropcache_start(bwc_p, res_uuid, do_sync);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_dropcache_stop(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_dropcache_stop(bwc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_fflush_start(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_fast_flush(bwc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_fflush_start_all(struct bwcg_interface *bwcg_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++) {
		wc_p = priv_p->p_wc[i];
		if (wc_p) {
			bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
			bwc_p->bw_op->bw_fast_flush(bwc_p, NULL);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static int
bwcg_fflush_get(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i, rc = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				rc = bwc_p->bw_op->bw_get_fast_flush(bwc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return rc;
}

static void
bwcg_fflush_stop(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				bwc_p->bw_op->bw_stop_fast_flush(bwc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
bwcg_fflush_stop_all(struct bwcg_interface *bwcg_p)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++) {
		wc_p = priv_p->p_wc[i];
		if (wc_p) {
			bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
			bwc_p->bw_op->bw_stop_fast_flush(bwc_p, NULL);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static int
bwcg_freeze_snapshot_stage1(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	int i, rc  = 1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				rc = wc_p->wc_op->wc_freeze_snapshot_stage1(wc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static int
bwcg_freeze_snapshot_stage2(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	int i, rc  = 1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				rc = wc_p->wc_op->wc_freeze_snapshot_stage2(wc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static int
bwcg_unfreeze_snapshot(struct bwcg_interface *bwcg_p, char *bwc_uuid, char *res_uuid)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct bwc_setup *bwc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	int i, rc = 1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++, bwc_setup++) {
		if (!strcmp(bwc_setup->uuid, bwc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				rc = wc_p->wc_op->wc_unfreeze_snapshot(wc_p, res_uuid);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static struct bwc_setup *
bwcg_get_all_bwc_setup(struct bwcg_interface *bwcg_p, int *num_bwc)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;

	*num_bwc = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

static int *
bwcg_get_working_bwc_index(struct bwcg_interface *bwcg_p, int *num_bwc)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_wc[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_bwc = count;

	return ret;
}

static struct bwc_interface **
bwcg_get_all_bwc(struct bwcg_interface *bwcg_p, int *num_bwc)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct wc_interface *wc_p;
	struct bwc_interface **ret_bwc;
	int i, counter = 0;

	ret_bwc = x_calloc(priv_p->p_counter, sizeof(*ret_bwc));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < BWCG_MAX_BWC; i++) {
		wc_p = priv_p->p_wc[i];
		if (wc_p) {
			ret_bwc[counter] = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
			counter++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_bwc = counter;
	return ret_bwc;
}

void
bwcg_get_all_bwc_stat(struct bwcg_interface *bwcg_p, struct list_head *out_head)
{
	struct bwcg_private *priv_p = (struct bwcg_private *)bwcg_p->wg_private;
	struct wc_interface *wc_p;
	struct bwc_interface *bwc_p;
	struct io_stat *io_stat;
	struct smc_stat_record *sr_p;
	int i;

	for (i = 0; i < BWCG_MAX_BWC; i++) {
		wc_p = priv_p->p_wc[i];
		if (wc_p) {
			bwc_p = (struct bwc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
			io_stat = calloc(1, sizeof(*io_stat));
			sr_p = calloc(1, sizeof(*sr_p));
			sr_p->r_type = NID_REQ_STAT_IO;
			sr_p->r_data = (void *)io_stat;
			list_add_tail(&sr_p->r_list, out_head);
			io_stat->s_io_type_bio = 1;
			bwc_p->bw_op->bw_get_stat(bwc_p, io_stat);
		}
	}
}


struct bwcg_operations bwcg_op = {
	.wg_search_bwc = bwcg_search_bwc,
	.wg_search_and_create_bwc = bwcg_search_and_create_bwc,
	.wg_add_bwc = bwcg_add_bwc,
	.wg_recover_bwc = bwcg_recover_bwc,
	.wg_recover_all_bwc = bwcg_recover_all_bwc,
	.wg_get_recover_bwc = bwcg_get_recover_bwc,
	.wg_remove_bwc = bwcg_remove_bwc,
	.wg_get_info_stat = bwcg_info_stat,
	.wg_get_throughput = bwcg_get_throughput,
	.wg_get_rw = bwcg_get_rw,
	.wg_get_delay = bwcg_get_delay,
	.wg_reset_throughput = bwcg_reset_throughput,
	.wg_dropcache_start = bwcg_dropcache_start,
	.wg_dropcache_stop = bwcg_dropcache_stop,
	.wg_fflush_start = bwcg_fflush_start,
	.wg_fflush_start_all = bwcg_fflush_start_all,
	.wg_fflush_get = bwcg_fflush_get,
	.wg_fflush_stop = bwcg_fflush_stop,
	.wg_fflush_stop_all = bwcg_fflush_stop_all,
	.wg_freeze_snapshot_stage1 = bwcg_freeze_snapshot_stage1,
	.wg_freeze_snapshot_stage2 = bwcg_freeze_snapshot_stage2,
	.wg_unfreeze_snapshot = bwcg_unfreeze_snapshot,
	.wg_get_all_bwc_setup = bwcg_get_all_bwc_setup,
	.wg_get_working_bwc_index = bwcg_get_working_bwc_index,
	.wg_update_water_mark = bwcg_update_water_mark,
	.wg_update_delay_level = bwcg_update_delay_level,
	.wg_get_all_bwc = bwcg_get_all_bwc,
	.wg_get_all_bwc_stat = bwcg_get_all_bwc_stat,
	.wg_info_list_stat = bwcg_info_list_stat,
};

struct bwcg_bfp_policy_param {
	double  coalesce_ratio;
	double  load_ratio_max;
	double  load_ratio_min;
	double  load_ctrl_level;
	double  flush_delay_ctl;
	double  throttle_ratio;
	int	low_water_mark;
	int	high_water_mark;
};

static void
__bwcg_bfp_get_param(struct ini_interface *ini_p, char *bwc_uuid,  struct bwcg_bfp_policy_param *param) {
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;

	memset(param, 0, sizeof(*param));
	param->throttle_ratio = 1.0;

	ini_p->i_op->i_rollback(ini_p);
	sc_p = ini_p->i_op->i_next_section(ini_p);      // the first section is [global]

	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "bwc")) {
			// it can have multiple bwc in system, it must check the channel uuid
			if(strcmp(sc_p->s_name, bwc_uuid)) continue;
			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "load_ctrl_level");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->load_ctrl_level = atof( (char *)(the_key->k_value));
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "load_ratio_max");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->load_ratio_max = atof( (char *)(the_key->k_value));
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "load_ratio_min");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->load_ratio_min = atof( (char *)(the_key->k_value));
			}


			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "flush_delay_ctl");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->flush_delay_ctl = atof( (char *)(the_key->k_value));
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "coalesce_ratio");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->coalesce_ratio = atof( (char *)(the_key->k_value));
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "throttle_ratio");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->throttle_ratio = atof( (char *)(the_key->k_value));
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "high_water_mark");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->high_water_mark = atoi( (char *)(the_key->k_value));
			} else {
				param->high_water_mark = 60;
			}

			the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "low_water_mark");
			if(the_key != NULL && the_key->k_value != NULL) {
				param->low_water_mark = atoi( (char *)(the_key->k_value));
			} else {
				param->low_water_mark = 40;
			}

			break;
        }
    }

	// check parameter value
	if(param->flush_delay_ctl < FLUSH_DELAY_CTL_MIN ||
		param->flush_delay_ctl > FLUSH_DELAY_CTL_MAX ) {
		param->flush_delay_ctl = FLUSH_DELAY_CTL_DEFAULT;
	}

	if(param->load_ctrl_level < LOAD_CTRL_LEVEL_MIN ||
	   param->load_ctrl_level > LOAD_CTRL_LEVEL_MAX ) {
		param->load_ctrl_level = LOAD_CTRL_LEVEL_DEFAULT;
	}

	if(param->load_ratio_min < LOAD_RATIO_MIN) {
		param->load_ratio_min = LOAD_RATIO_MIN;
	}


	if(param->load_ratio_max > LOAD_RATIO_MAX || param->load_ratio_max < param->load_ratio_min) {
		param->load_ratio_max = LOAD_RATIO_MAX;
	}

	if(param->coalesce_ratio > 1.0) {
		param->coalesce_ratio = 1.0;
	} else if(param->coalesce_ratio < 0.0) {
		param->coalesce_ratio = 0;
	}

	if(param->throttle_ratio > THROTTLE_RATIO_MAX ||
	   param->throttle_ratio < THROTTLE_RATIO_MIN) {
		 param->throttle_ratio = THROTTLE_RATIO_MAX;
	}

}

int
bwcg_initialization(struct bwcg_interface *bwcg_p, struct bwcg_setup *setup)
{
	char *log_header = "bwcg_initialization";
	struct bwcg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct tpg_interface *tpg_p;
	char *sect_type, *bfp_policy, *tp_name;
	struct bwc_setup *bwc_setup;
	int bwc_counter = 0;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	bwcg_p->wg_op = &bwcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bwcg_p->wg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_sdsg = setup->sdsg;
	priv_p->p_wtp = setup->wtp;
	priv_p->p_srn = setup->srn;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_io = setup->io;
	priv_p->p_mqtt = setup->mqtt;
	ini_p = *priv_p->p_ini;
	bwc_setup = &priv_p->p_setup[0];

	tpg_p = setup->tpg;
	priv_p->p_gtp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, NID_GLOBAL_TP);
	priv_p->p_tpg = tpg_p;

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "bwc"))
			continue;

		memset(bwc_setup, 0, sizeof(*bwc_setup));
		strcpy(bwc_setup->uuid, sc_p->s_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cache_device");
		strcpy(bwc_setup->bufdevice, (char *)(the_key->k_value));
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cache_size");
		bwc_setup->bufdevicesz = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "rw_sync");
		bwc_setup->rw_sync = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "two_step_read");
		bwc_setup->two_step_read = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "do_fp");
		bwc_setup->do_fp = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "flush_policy");
		bfp_policy = (char *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "tp_name");
		tp_name = (char *)(the_key->k_value);
		strcpy (bwc_setup->tp_name, tp_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "ssd_mode");
		bwc_setup->ssd_mode = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "max_flush_size");
		bwc_setup->max_flush_size = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "write_delay_first_level");
		bwc_setup->write_delay_first_level = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "write_delay_second_level");
		bwc_setup->write_delay_second_level = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "write_delay_first_level_max_us");
		bwc_setup->write_delay_first_level_max_us = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "write_delay_second_level_max_us");
		bwc_setup->write_delay_second_level_max_us = *(int *)(the_key->k_value);

		if(bfp_policy == NULL) {
			nid_log_notice("%s: no set flush policy, use bfp1", log_header);
			bwc_setup->bfp_type = BFP_POLICY_BFP1;
		} else if(strcmp(bfp_policy, "bfp5") == 0) {
			bwc_setup->bfp_type = BFP_POLICY_BFP5;

			struct bwcg_bfp_policy_param bfp_param;
			__bwcg_bfp_get_param(ini_p, sc_p->s_name,  &bfp_param);
			bwc_setup->coalesce_ratio = bfp_param.coalesce_ratio;
			bwc_setup->load_ratio_max = bfp_param.load_ratio_max;
			bwc_setup->load_ratio_min = bfp_param.load_ratio_min;
			bwc_setup->load_ctrl_level = bfp_param.load_ctrl_level;
			bwc_setup->flush_delay_ctl = bfp_param.flush_delay_ctl;
			bwc_setup->throttle_ratio = bfp_param.throttle_ratio;
		} else if(strcmp(bfp_policy, "bfp4") == 0) {
			bwc_setup->bfp_type = BFP_POLICY_BFP4;

			struct bwcg_bfp_policy_param bfp_param;
			__bwcg_bfp_get_param(ini_p, sc_p->s_name,  &bfp_param);
			bwc_setup->load_ratio_max = bfp_param.load_ratio_max;
			bwc_setup->load_ratio_min = bfp_param.load_ratio_min;
			bwc_setup->load_ctrl_level = bfp_param.load_ctrl_level;
			bwc_setup->flush_delay_ctl = bfp_param.flush_delay_ctl;
			bwc_setup->throttle_ratio = bfp_param.throttle_ratio;
		} else if(strcmp(bfp_policy, "bfp3") == 0) {
			bwc_setup->bfp_type = BFP_POLICY_BFP3;

			struct bwcg_bfp_policy_param bfp_param;
			__bwcg_bfp_get_param(ini_p, sc_p->s_name,  &bfp_param);
			bwc_setup->coalesce_ratio = bfp_param.coalesce_ratio;
			bwc_setup->load_ratio_max = bfp_param.load_ratio_max;
			bwc_setup->load_ratio_min = bfp_param.load_ratio_min;
			bwc_setup->load_ctrl_level = bfp_param.load_ctrl_level;
			bwc_setup->flush_delay_ctl = bfp_param.flush_delay_ctl;
		} else if(strcmp(bfp_policy, "bfp2") == 0) {
			bwc_setup->bfp_type = BFP_POLICY_BFP2;

			struct bwcg_bfp_policy_param bfp_param;
			__bwcg_bfp_get_param(ini_p, sc_p->s_name,  &bfp_param);
			bwc_setup->load_ratio_max = bfp_param.load_ratio_max;
			bwc_setup->load_ratio_min = bfp_param.load_ratio_min;
			bwc_setup->load_ctrl_level = bfp_param.load_ctrl_level;
			bwc_setup->flush_delay_ctl = bfp_param.flush_delay_ctl;
		} else if(strcmp(bfp_policy, "bfp1") == 0) {
			bwc_setup->bfp_type = BFP_POLICY_BFP1;
		} else {
			nid_log_notice("%s: invalid flush policy %s, use bfp1", log_header, bfp_policy);
			bwc_setup->bfp_type = BFP_POLICY_BFP1;
		}

		if(bwc_setup->bfp_type == BFP_POLICY_BFP1) {
			struct bwcg_bfp_policy_param bfp_param;
			__bwcg_bfp_get_param(ini_p, sc_p->s_name,  &bfp_param);
			bwc_setup->high_water_mark = bfp_param.high_water_mark;
			bwc_setup->low_water_mark = bfp_param.low_water_mark;
		}

		bwc_setup++;
		bwc_counter++;
		assert(bwc_counter <= BWCG_MAX_BWC);
	}
	priv_p->p_counter = bwc_counter;
	return 0;
}
