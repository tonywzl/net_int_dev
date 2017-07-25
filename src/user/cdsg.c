/*
 * cdsg.c
 * 	Implementation of cdsg Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "cds_if.h"
#include "cdsg_if.h"
#include "ppg_if.h"
#include "ds_if.h"

struct cdsg_private {
	struct allocator_interface	*p_allocator;
	struct ini_interface		*p_ini;
	struct ppg_interface		*p_ppg;
	pthread_mutex_t			p_lck;
	struct cds_setup		p_setup[CDSG_MAX_CDS];
	struct ds_interface		*p_ds[CDSG_MAX_CDS];
	int				p_counter;
};

struct ds_interface *
cdsg_search_cds(struct cdsg_interface *cdsg_p, char *cds_name)
{
	struct cdsg_private *priv_p = (struct cdsg_private *)cdsg_p->dg_private;
	struct cds_setup *cds_setup = &priv_p->p_setup[0];
	struct ds_interface *ret_cds = NULL;
	int i;

	if (cds_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CDSG_MAX_CDS; i++, cds_setup++) {
		if (!strcmp(cds_setup->cds_name, cds_name)) {
			ret_cds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_cds;
}

/*
 * Used for initialization from configuration.
 */
struct ds_interface *
cdsg_search_and_create_cds(struct cdsg_interface *cdsg_p, char *cds_name)
{
	struct cdsg_private *priv_p = (struct cdsg_private *)cdsg_p->dg_private;
	struct cds_setup *cds_setup = &priv_p->p_setup[0];
	struct ds_interface *ret_cds = NULL;
	struct ds_setup ds_setup;
	int i;

	if (cds_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CDSG_MAX_CDS; i++, cds_setup++) {
		if (!strcmp(cds_setup->cds_name, cds_name)) {
			/* got a matched cds_setup */
			if (!priv_p->p_ds[i]) {
				priv_p->p_ds[i] = x_calloc(1, sizeof(*priv_p->p_ds[i]));
				memset(&ds_setup, 0, sizeof(ds_setup));
				ds_setup.type = DS_TYPE_CYCLE;
				ds_setup.name = cds_setup->cds_name;
				ds_setup.pagenrshift = cds_setup->pagenrshift;
				ds_setup.pageszshift = cds_setup->pageszshift;
				ds_initialization(priv_p->p_ds[i], &ds_setup);
			}
			ret_cds = priv_p->p_ds[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_cds;
}

/*
 * Used for online adding.
 */
static int
cdsg_add_cds(struct cdsg_interface *cdsg_p,
		char *cds_name, uint8_t pagenrshift, uint8_t pageszshift)
{
	struct cdsg_private *priv_p = (struct cdsg_private *)cdsg_p->dg_private;
	struct cds_setup *cds_setup = &priv_p->p_setup[0];
	struct ds_setup ds_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < CDSG_MAX_CDS; i++, cds_setup++) {
		if (!strcmp(cds_setup->cds_name, cds_name))
			goto out;

		if (empty_index == -1 && cds_setup->cds_name[0] == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
		priv_p->p_ds[empty_index] = x_calloc(1, sizeof(*priv_p->p_ds[empty_index]));
		cds_setup = &priv_p->p_setup[empty_index];
		strcpy(cds_setup->cds_name, cds_name);
		cds_setup->pagenrshift = pagenrshift;
		cds_setup->pageszshift = pageszshift;
		memset(&ds_setup, 0, sizeof(ds_setup));
		ds_setup.type = DS_TYPE_CYCLE;
		ds_setup.name = cds_setup->cds_name;
		ds_setup.pagenrshift = cds_setup->pagenrshift;
		ds_setup.pageszshift = cds_setup->pageszshift;
		ds_initialization(priv_p->p_ds[empty_index], &ds_setup);
		priv_p->p_counter++;
		rc = 0;
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

struct cdsg_operations cdsg_op = {
	.dg_search_cds = cdsg_search_cds,
	.dg_search_and_create_cds = cdsg_search_and_create_cds,
	.dg_add_cds = cdsg_add_cds,
};

int
cdsg_initialization(struct cdsg_interface *cdsg_p, struct cdsg_setup *setup)
{
	char *log_header = "cdsg_initialization";
	struct cdsg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct cds_setup *cds_setup;
	int cds_counter = 0;

	nid_log_info("%s: start ...", log_header);
	cdsg_p->dg_op = &cdsg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	cdsg_p->dg_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_ini = setup->ini;
	ini_p = priv_p->p_ini;
	cds_setup = &priv_p->p_setup[0];

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "cds"))
			continue;

		memset(cds_setup, 0, sizeof(*cds_setup));
		strcpy(cds_setup->cds_name, sc_p->s_name);

		cds_setup->pagenrshift = 2;	// TODO: calculate "page_nr"
		cds_setup->pageszshift = 23;	// TODO: calculate "page_size"

		cds_setup++;
		cds_counter++;
		assert(cds_counter <= CDSG_MAX_CDS);
	}
	priv_p->p_counter = cds_counter;
	return 0;
}
