/*
 * sdsg.c
 * 	Implementation of sdsg Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "allocator_if.h"
#include "sds_if.h"
#include "sdsg_if.h"
#include "ppg_if.h"
#include "ds_if.h"

struct sdsg_private {
	struct allocator_interface	*p_allocator;
	struct ini_interface		*p_ini;
	struct ppg_interface		*p_ppg;
	pthread_mutex_t			p_lck;
	struct sds_setup		p_setup[SDSG_MAX_SDS];
	struct ds_interface		*p_ds[SDSG_MAX_SDS];
	int				p_counter;
};

struct ds_interface *
sdsg_search_sds(struct sdsg_interface *sdsg_p, char *sds_name)
{
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ds_interface *ret_sds = NULL;
	int i;

	if (sds_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->sds_name, sds_name)) {
			ret_sds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_sds;
}

struct ds_interface *
sdsg_search_by_wc(struct sdsg_interface *sdsg_p, char *wc_uuid)
{
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ds_interface *ret_sds = NULL;
	int i;

	if (wc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->wc_name, wc_uuid)) {
			ret_sds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_sds;
}

/*
 * Used for initialization from configuration.
 */
struct ds_interface *
sdsg_search_by_wc_and_create_sds(struct sdsg_interface *sdsg_p, char *wc_uuid)
{
	char *log_header = "sdsg_search_by_wc_and_create_sds";
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	struct pp_interface *pp_p;
	struct ds_interface *ret_sds = NULL;
	struct ds_setup ds_setup;
	int i;

	if (wc_uuid[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->wc_name, wc_uuid)) {
			/* got a matched sds_setup */
			if (!priv_p->p_ds[i]) {
				pp_p = ppg_p->pg_op->pg_search_and_create_pp(ppg_p,
						sds_setup->pp_name, ALLOCATOR_SET_SDS_PP);
				if (!pp_p) {
					nid_log_error("%s: according pp does not exist(pp_name:%s)",
							log_header, sds_setup->pp_name);
					break;
				}
				priv_p->p_ds[i] = x_calloc(1, sizeof(*priv_p->p_ds[i]));
				memset(&ds_setup, 0, sizeof(ds_setup));
				ds_setup.type = DS_TYPE_SPLIT;
				ds_setup.pool = (void *)pp_p;
				ds_setup.name = sds_setup->sds_name;
				ds_setup.wc_name = sds_setup->wc_name;
				ds_setup.rc_name = sds_setup->rc_name;
				ds_initialization(priv_p->p_ds[i], &ds_setup);
			}
			ret_sds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_sds;
}

/*
 * Used for initialization from configuration.
 */
struct ds_interface *
sdsg_search_and_create_sds(struct sdsg_interface *sdsg_p, char *sds_name)
{
	char *log_header = "sdsg_search_and_create_sds";
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	struct pp_interface *pp_p;
	struct ds_interface *ret_sds = NULL;
	struct ds_setup ds_setup;
	int i;

	if (sds_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->sds_name, sds_name)) {
			/* got a matched sds_setup */
			if (!priv_p->p_ds[i]) {
				pp_p = ppg_p->pg_op->pg_search_and_create_pp(ppg_p,
						sds_setup->pp_name, ALLOCATOR_SET_SDS_PP);
				if (!pp_p) {
					nid_log_error("%s: according pp does not exist(pp_name:%s)",
							log_header, sds_setup->pp_name);
					break;
				}
				priv_p->p_ds[i] = x_calloc(1, sizeof(*priv_p->p_ds[i]));
				memset(&ds_setup, 0, sizeof(ds_setup));
				ds_setup.type = DS_TYPE_SPLIT;
				ds_setup.pool = (void *)pp_p;
				ds_setup.name = sds_setup->sds_name;
				ds_setup.wc_name = sds_setup->wc_name;
				ds_setup.rc_name = sds_setup->rc_name;
				ds_initialization(priv_p->p_ds[i], &ds_setup);
			}
			ret_sds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_sds;
}

/*
 * Used for online adding, required pp ought to have been created.
 */
static int
sdsg_add_sds(struct sdsg_interface *sdsg_p,
		char *sds_name, char *pp_name, char *wc_uuid, char *rc_uuid)
{
	char *log_header = "sdsg_add_sds";
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	struct pp_interface *pp_p;
	struct ds_setup ds_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->sds_name, sds_name)) {
			nid_log_warning("%s: sds (%s) already exists", log_header, sds_name);
			goto out;
		}

		if (empty_index == -1 && sds_setup->sds_name[0] == '\0') {
			empty_index = i;
			nid_log_warning("%s: index %d is assgined to sds (%s)", log_header, empty_index, sds_name);
		}
	}

	if (empty_index != -1) {
		pp_p = ppg_p->pg_op->pg_search_pp(ppg_p, pp_name);
		if (!pp_p) {
			nid_log_warning("%s: according pp does not exist(pp_name:%s)",
					log_header, pp_name);
			goto out;
		}
		priv_p->p_ds[empty_index] = x_calloc(1, sizeof(*priv_p->p_ds[empty_index]));
		sds_setup = &priv_p->p_setup[empty_index];
		strcpy(sds_setup->sds_name, sds_name);
		strcpy(sds_setup->pp_name, pp_name);
		strcpy(sds_setup->wc_name, wc_uuid);
		strcpy(sds_setup->rc_name,  rc_uuid);
		memset(&ds_setup, 0, sizeof(ds_setup));
		ds_setup.type = DS_TYPE_SPLIT;
		ds_setup.pool = (void *)pp_p;
		ds_setup.name = sds_setup->sds_name;
		ds_setup.wc_name = sds_setup->wc_name;
		ds_setup.rc_name = sds_setup->rc_name;
		ds_initialization(priv_p->p_ds[empty_index], &ds_setup);
		priv_p->p_counter++;
		rc = 0;
	} else {
		nid_log_warning("%s: sds number reaches the limitation", log_header);
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static int
sdsg_delete_sds(struct sdsg_interface *sdsg_p, char *sds_name)
{
	char *log_header = "sdsg_delete_sds";
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ds_interface *ds_p = NULL;
	int i, got_setup = 0;

	nid_log_warning("%s: start (name:%s) ...", log_header, sds_name);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++, sds_setup++) {
		if (!strcmp(sds_setup->sds_name, sds_name)) {
			got_setup = 1;
			sds_setup->sds_name[0] = '\0';
			assert(priv_p->p_counter);
			priv_p->p_counter--;

			ds_p = priv_p->p_ds[i];
			priv_p->p_ds[i] = NULL;

			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (ds_p)
		ds_p->d_op->d_destroy(ds_p);
	else if (got_setup)
		nid_log_warning("%s: sds does not exist, setup removed (name:%s)", log_header, sds_name);
	else
		nid_log_warning("%s: no sds matched (name:%s)", log_header, sds_name);

	nid_log_warning("%s: end (name:%s) ...", log_header, sds_name);
	return 0;
}

/*
 * Used for online wc switch.
 */
static int
sdsg_switch_sds_wc(struct sdsg_interface *sdsg_p, char *sds_name, char *wc_uuid)
{
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	struct sds_setup *sds_setup = &priv_p->p_setup[0];
	struct ds_interface *ds_p;
	int i, rc = 1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS && sds_setup->sds_name[0] != '\0'; i++, sds_setup++) {
		if (!strcmp(sds_setup->sds_name, sds_name)) {
			ds_p = priv_p->p_ds[i];
			ds_p->d_op->d_set_wc(ds_p, wc_uuid);
			strcpy(sds_setup->wc_name, wc_uuid);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static struct sds_setup *
sdsg_get_all_sds_setup(struct sdsg_interface *sdsg_p, int *num_sds)
{
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;

	*num_sds = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

static int *
sdsg_get_working_sds_index(struct sdsg_interface *sdsg_p, int *num_sds)
{
	struct sdsg_private *priv_p = (struct sdsg_private *)sdsg_p->dg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));	
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < SDSG_MAX_SDS; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_ds[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_sds = count;

	return ret;
}


struct sdsg_operations sdsg_op = {
	.dg_search_sds = sdsg_search_sds,
	.dg_search_by_wc = sdsg_search_by_wc,
	.dg_search_by_wc_and_create_sds = sdsg_search_by_wc_and_create_sds,
	.dg_search_and_create_sds = sdsg_search_and_create_sds,
	.dg_add_sds = sdsg_add_sds,
	.dg_delete_sds = sdsg_delete_sds,
	.dg_switch_sds_wc = sdsg_switch_sds_wc,
	.dg_get_all_sds_setup = sdsg_get_all_sds_setup,
	.dg_get_working_sds_index = sdsg_get_working_sds_index,
};

int
sdsg_initialization(struct sdsg_interface *sdsg_p, struct sdsg_setup *setup)
{
	char *log_header = "sdsg_initialization";
	struct sdsg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct sds_setup *sds_setup;
	int sds_counter = 0;

	nid_log_info("%s: start ...", log_header);
	sdsg_p->dg_op = &sdsg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	sdsg_p->dg_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_ini = setup->ini;
	priv_p->p_ppg = setup->ppg;
	ini_p = priv_p->p_ini;
	sds_setup = &priv_p->p_setup[0];

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "sds"))
			continue;

		memset(sds_setup, 0, sizeof(*sds_setup));
		strcpy(sds_setup->sds_name, sc_p->s_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "pp_name");
		strcpy(sds_setup->pp_name, (char *)(the_key->k_value));

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "wc_uuid");
		if (strcmp((char *)(the_key->k_value), "wrong_name"))
			strcpy(sds_setup->wc_name, (char *)(the_key->k_value));

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "rc_uuid");
		if (strcmp((char *)(the_key->k_value), "wrong_name"))
			strcpy(sds_setup->rc_name, (char *)(the_key->k_value));

		sds_setup++;
		sds_counter++;
		assert(sds_counter <= SDSG_MAX_SDS);
	}
	priv_p->p_counter = sds_counter;
	return 0;
}
