/*
 * twcg.c
 * 	Implementation of Write Through Cache Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid.h"
#include "ini_if.h"
#include "twcg_if.h"
#include "wc_if.h"
#include "twc_if.h"
#include "sdsg_if.h"
#include "ds_if.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "io_if.h"
#include "smc_if.h"

struct twcg_private {
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
	struct twc_setup		p_setup[TWCG_MAX_TWC];
	struct wc_interface		*p_wc[TWCG_MAX_TWC];
	int				p_counter;
};

struct wc_interface *
twcg_search_twc(struct twcg_interface *twcg_p, char *twc_uuid)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct wc_interface *ret_wc = NULL;
	int i;

	if (twc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid)) {
			ret_wc = priv_p->p_wc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_wc;
}

struct wc_interface *
twcg_search_and_create_twc(struct twcg_interface *twcg_p, char *twc_uuid)
{
	char *log_header = "twcg_search_and_create_twc";
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct ds_interface *ds_p = NULL;
	struct wc_interface *ret_wc = NULL;
	struct wc_setup wc_setup;
	struct tpg_interface *tpg_p = priv_p->p_tpg;
	char *tp_name;
	int i;

	if (twc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid)) {
			/* got a matched twc */
			if (!priv_p->p_wc[i]) {
				if (sdsg_p)
					ds_p = sdsg_p->dg_op->dg_search_by_wc_and_create_sds(sdsg_p, twc_uuid);
				if (!ds_p) {
					nid_log_error("%s: owner ds does not exist (wc_uuid:%s)",
							log_header, twc_uuid);
					break;
				}
				priv_p->p_wc[i] = x_calloc(1, sizeof(*priv_p->p_wc[i]));
				memset(&wc_setup, 0, sizeof(wc_setup));
				wc_setup.pp = ds_p->d_op->d_get_pp(ds_p);;
				wc_setup.srn = priv_p->p_srn;
				wc_setup.allocator = priv_p->p_allocator;
				wc_setup.lstn = priv_p->p_lstn;
				wc_setup.type = WC_TYPE_TWC;
				wc_setup.do_fp = twc_setup->do_fp;
				wc_setup.uuid = twc_setup->uuid;

				tp_name = twc_setup->tp_name;
				wc_setup.tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_name);

				wc_initialization(priv_p->p_wc[i], &wc_setup);
			}
			ret_wc = priv_p->p_wc[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_wc;
}

static int
twcg_add_twc(struct twcg_interface *twcg_p, char *twc_uuid, int do_fp, char *tp_name)
{
	char *log_header = "twcg_add_twc";
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct ds_interface *ds_p = NULL;
//	struct tpg_interface *tpg_p = priv_p->p_tpg;
	struct wc_interface *twc_p = NULL;
//	struct wc_setup wc_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid))
			goto out;

		if (empty_index != -1 && twc_setup->uuid == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
		if (sdsg_p)
			ds_p = sdsg_p->dg_op->dg_search_by_wc(sdsg_p, twc_uuid);
		if (!ds_p) {
			nid_log_error("%s: owner ds does not exist (wc_uuid:%s)",
					log_header, twc_uuid);
			goto out;
		}
//		priv_p->p_wc[empty_index] = x_calloc(1, sizeof(*priv_p->p_wc[empty_index]));
		twc_setup = &priv_p->p_setup[empty_index];
		strcpy(twc_setup->uuid, twc_uuid);
		strcpy(twc_setup->tp_name, tp_name);
/*		memset(&wc_setup, 0, sizeof(wc_setup));
		wc_setup.pp =  ds_p->d_op->d_get_pp(ds_p);
		wc_setup.srn = priv_p->p_srn;
		wc_setup.allocator = priv_p->p_allocator;
		wc_setup.lstn = priv_p->p_lstn;
		wc_setup.type = WC_TYPE_TWC;
		wc_setup.do_fp = do_fp;
		wc_setup.uuid = twc_setup->uuid;

		wc_setup.tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_name);
		wc_initialization(priv_p->p_wc[empty_index], &wc_setup);*/
		twc_setup->do_fp = do_fp;
		priv_p->p_counter++;

		pthread_mutex_unlock(&priv_p->p_lck);//give the lock to twcg_search_and_create_twc
		twc_p = twcg_search_and_create_twc(twcg_p, twc_uuid);
		pthread_mutex_lock(&priv_p->p_lck);//lock back

		if (!twc_p)
			goto out;
		rc = 0;
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

/* job that adds a twc */
struct twcg_recover_twc_job {
	struct tp_jobheader	j_header;
	struct wc_interface	*j_wc;
};

static void
__twcg_recover_twc(struct tp_jobheader *jh_p)
{
	struct twcg_recover_twc_job *job_p = (struct twcg_recover_twc_job *)jh_p;
	struct wc_interface *wc_p = job_p->j_wc;

	wc_p->wc_op->wc_recover(wc_p);
}

static void
__twcg_free_recover_twc(struct tp_jobheader *jh_p)
{
	free((void *)jh_p);
}

static int
twcg_recover_twc(struct twcg_interface *twcg_p, char *twc_uuid)
{
	char *log_header = "twcg_recover_twc";
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct wc_interface *wc_p;
	struct twcg_recover_twc_job *job_p;
	int i, recover_state = -1;

	nid_log_warning("%s: start(twc_uuid:%s)...", log_header, twc_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC && twc_setup->uuid[0] != '\0'; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				recover_state = wc_p->wc_op->wc_get_recover_state(wc_p);
				if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
					break;
				job_p = (struct twcg_recover_twc_job *)x_calloc(1, sizeof(*job_p));
				job_p->j_header.jh_do = __twcg_recover_twc;
				job_p->j_header.jh_free = __twcg_free_recover_twc;
				job_p->j_wc = wc_p;
				tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (recover_state == -1) {
		nid_log_warning("%s: twc:%s does not exist!", log_header, twc_uuid);
		return 1;
	}
	if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
		nid_log_warning("%s: twc:%s recovery already started!", log_header, twc_uuid);
	return 0;
}

static void
twcg_recover_all_twc(struct twcg_interface *twcg_p)
{
	char *log_header = "twcg_recover_twc";
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct tp_interface *tp_p = priv_p->p_gtp;
	struct wc_interface *wc_p;
	struct twcg_recover_twc_job *job_p;
	int i, recover_state;

	nid_log_warning("%s: start...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC && twc_setup->uuid[0] != '\0'; i++, twc_setup++) {
		wc_p = priv_p->p_wc[i];
		if (!wc_p)
			continue;
		recover_state = wc_p->wc_op->wc_get_recover_state(wc_p);
		if (recover_state == WC_RECOVER_DOING || recover_state == WC_RECOVER_DONE)
			continue;
		job_p = (struct twcg_recover_twc_job *)x_calloc(1, sizeof(*job_p));
		job_p->j_header.jh_do = __twcg_recover_twc;
		job_p->j_header.jh_free = __twcg_free_recover_twc;
		job_p->j_wc = wc_p;
		tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
twcg_info_stat(struct twcg_interface *twcg_p, char *twc_uuid, struct umessage_twc_information_resp_stat *stat_p)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct twc_interface *twc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC && twc_setup->uuid[0] != '\0'; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				twc_p = (struct twc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				twc_p->tw_op->tw_get_info_stat(twc_p, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
twcg_get_rw(struct twcg_interface *twcg_p, char *twc_uuid, char *res_uuid, struct twc_rw_stat *stat_p)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct twc_setup *twc_setup = &priv_p->p_setup[0];
	struct wc_interface *wc_p;
	struct twc_interface *twc_p;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC && twc_setup->uuid[0] != '\0'; i++, twc_setup++) {
		if (!strcmp(twc_setup->uuid, twc_uuid)) {
			wc_p = priv_p->p_wc[i];
			if (wc_p) {
				twc_p = (struct twc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
				twc_p->tw_op->tw_get_rw(twc_p, res_uuid, stat_p);
			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static struct twc_setup *
twcg_get_all_twc_setup(struct twcg_interface *twcg_p, int *num_twc)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;

	*num_twc = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

static int *
twcg_get_working_twc_index(struct twcg_interface *twcg_p, int *num_twc)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < TWCG_MAX_TWC; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_wc[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_twc = count;

	return ret;
}

void
twcg_get_all_twc_stat(struct twcg_interface *twcg_p, struct list_head *out_head)
{
	struct twcg_private *priv_p = (struct twcg_private *)twcg_p->wg_private;
	struct wc_interface *wc_p;
	struct twc_interface *twc_p;
	struct io_stat *io_stat;
	struct smc_stat_record *sr_p;
	int i;

	for (i = 0; i < TWCG_MAX_TWC; i++) {
		wc_p = priv_p->p_wc[i];
		if (wc_p) {
			twc_p = (struct twc_interface *)wc_p->wc_op->wc_get_cache_obj(wc_p);
			io_stat = calloc(1, sizeof(*io_stat));
			sr_p = calloc(1, sizeof(*sr_p));
			sr_p->r_type = NID_REQ_STAT_IO;
			sr_p->r_data = (void *)io_stat;
			list_add_tail(&sr_p->r_list, out_head);
			io_stat->s_io_type_bio = 1;
			twc_p->tw_op->tw_get_stat(twc_p, io_stat);
		}
	}
}

struct twcg_operations twcg_op = {
	.wg_search_twc = twcg_search_twc,
	.wg_search_and_create_twc = twcg_search_and_create_twc,
	.wg_add_twc = twcg_add_twc,
	.wg_recover_twc = twcg_recover_twc,
	.wg_recover_all_twc = twcg_recover_all_twc,
	.wg_get_info_stat = twcg_info_stat,
	.wg_get_rw = twcg_get_rw,
	.wg_get_all_twc_setup = twcg_get_all_twc_setup,
	.wg_get_working_twc_index = twcg_get_working_twc_index,
	.wg_get_all_twc_stat = twcg_get_all_twc_stat,
};

int
twcg_initialization(struct twcg_interface *twcg_p, struct twcg_setup *setup)
{
	char *log_header = "twcg_initialization";
	struct twcg_private *priv_p;
	struct tpg_interface *tpg_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type, *tp_name;
	struct twc_setup *twc_setup;
	int twc_counter = 0;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	twcg_p->wg_op = &twcg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	twcg_p->wg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_sdsg = setup->sdsg;
	priv_p->p_wtp = setup->wtp;
	priv_p->p_srn = setup->srn;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_lstn = setup->lstn;
	ini_p = *priv_p->p_ini;
	twc_setup = &priv_p->p_setup[0];

	tpg_p = setup->tpg;
	priv_p->p_gtp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, NID_GLOBAL_TP);
	priv_p->p_tpg = tpg_p;


	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "twc"))
			continue;

		memset(twc_setup, 0, sizeof(*twc_setup));
		strcpy(twc_setup->uuid, sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "do_fp");
		twc_setup->do_fp = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "tp_name");
		tp_name = (char *)(the_key->k_value);
		strcpy (twc_setup->tp_name, tp_name);

		twc_setup++;
		twc_counter++;
		assert(twc_counter <= TWCG_MAX_TWC);
	}
	priv_p->p_counter = twc_counter;
	return 0;
}
