/*
 * dxtg.c
 * 	Implementation of Control Exchange Transfer Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "nid_log.h"
#include "ini_if.h"
#include "list.h"
#include "lstn_if.h"
#include "ds_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "dxtg_if.h"
#include "dxt_if.h"
#include "tx_shared.h"


struct dxtg_private {
	struct ini_interface	*p_ini;
	struct lstn_interface		*p_lstn;
	struct pp2_interface		*p_pp2;
	struct txn_interface		*p_txn;
	struct sdsg_interface		*p_sdsg;
	struct cdsg_interface		*p_cdsg;
	struct dxt_setup		*p_setup;
	pthread_mutex_t			p_lck;
	int				p_counter;
	struct list_head		p_dxt_head;
	int				p_busy;
};


static struct dxt_interface *
dxtg_search_dxt(struct dxtg_interface *dxtg_p, char *dxt_name)
{
	char *log_header = "dxtg_search_dxt";
	struct dxtg_private *priv_p = (struct dxtg_private *)dxtg_p->dxg_private;
	struct dxt_interface *dxt_p = NULL;
	struct list_node *lnp;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxt_head, ln_list) {
		dxt_p = (struct dxt_interface *)lnp->ln_data;
		target_name = dxt_p->dx_op->dx_get_name(dxt_p);
		if(!strcmp(target_name, dxt_name)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if(!got_it)
		dxt_p = NULL;

	return dxt_p;

}

static struct dxt_interface *
dxtg_search_and_create_channel(struct dxtg_interface *dxtg_p, char *dxt_name, int csfd, int dsfd)
{
	char *log_header = "dxtg_search_and_create_channel";
	struct dxtg_private *priv_p = (struct dxtg_private *)dxtg_p->dxg_private;
	struct dxt_setup *dxt_setup = &priv_p->p_setup[0];
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct dxt_interface *dxt_p = NULL;
	struct sdsg_interface *sdsg_p;
	struct cdsg_interface *cdsg_p;
	struct list_node *lnp;
	char *ds_name, *target_name;
	int i, got_it = 0;

	nid_log_warning("%s: start ...", log_header);

	for (i = 0; i < priv_p->p_counter; i++, dxt_setup++) {
		if (!strcmp(dxt_name, dxt_setup->dxt_name)) {
			nid_log_warning("%s: found match dxt name: %s", log_header, dxt_name);
			break;
		}
	}
	if (i == priv_p->p_counter) {
		nid_log_warning("%s: did not found match dxt name: %s", log_header, dxt_name);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_lck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxt_head, ln_list) {
		dxt_p = (struct dxt_interface *)lnp->ln_data;
		target_name = dxt_p->dx_op->dx_get_name(dxt_p);
		if(!strcmp(target_name, dxt_name)) {
			got_it = 1;
			break;
		}
	}
	if(!got_it) {
		priv_p->p_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if(!got_it) {
		/*
		 * create a new dxt
		 */
		sdsg_p = priv_p->p_sdsg;
		cdsg_p = priv_p->p_cdsg;

		dxt_p = calloc(1, sizeof(*dxt_p));
		dxt_setup->csfd = csfd;
		dxt_setup->dsfd = dsfd;
		dxt_setup->lstn = priv_p->p_lstn;
		dxt_setup->pp2 = priv_p->p_pp2;
		dxt_setup->txn = priv_p->p_txn;

		// ds_name set in dxt_setup from conf file
		ds_name = dxt_setup->ds_name;
		if (sdsg_p) {
			dxt_setup->ds = sdsg_p->dg_op->dg_search_and_create_sds(sdsg_p, ds_name);
			if(dxt_setup->ds) {
				nid_log_debug("%s: success create sds instance %s", log_header, ds_name);
			} else {
				nid_log_error("%s: fail to create sds instance %s", log_header, ds_name);
			}
		}

		if (!dxt_setup->ds && cdsg_p) {
			dxt_setup->ds = cdsg_p->dg_op->dg_search_and_create_cds(cdsg_p, ds_name);
			if(dxt_setup->ds) {
				nid_log_debug("%s: success create cds instance %s", log_header, ds_name);
			} else {
				nid_log_error("%s: fail to create cds instance %s", log_header, ds_name);
			}
		}

		assert(dxt_setup->ds);
		dxt_setup->req_size = UMSG_TX_HEADER_LEN;
		dxt_initialization(dxt_p, dxt_setup);
		lnp = lstn_p->ln_op->ln_get_node(lstn_p);
		lnp->ln_data = (void *)dxt_p;
		dxt_setup->handle = lnp;

		pthread_mutex_lock(&priv_p->p_lck);
		list_add_tail(&lnp->ln_list, &priv_p->p_dxt_head);
		priv_p->p_busy = 0;
		pthread_mutex_unlock(&priv_p->p_lck);
	}

out:
	return dxt_p;
}

static void
dxtg_cleanup(struct dxtg_interface *dxtg_p) {
	char *log_header = "dxtg_cleanup";
	struct dxtg_private *priv_p = (struct dxtg_private *)dxtg_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct dxt_interface *dxt_p = NULL;
	struct list_node *lnp, *lnp1;

	nid_log_warning("%s: start ...", log_header);

	// should we release lock after list_del and get lock again after call dx_cleanup
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxt_head, ln_list) {
		list_del(&lnp->ln_list);
		dxt_p = (struct dxt_interface *)lnp->ln_data;
		dxt_p->dx_op->dx_cleanup(dxt_p);
		free(dxt_p);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	free(priv_p);
}

static int
dxtg_drop_channel(struct dxtg_interface *dxtg_p, struct dxt_interface *dxt_p) {
	char *log_header = "dxtg_drop_channel";
	struct dxtg_private *priv_p = (struct dxtg_private *)dxtg_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	nid_log_warning("%s: start ...", log_header);

	// here can only drop dxt when dxt created
	if(!dxt_p) {
		rc = 0;
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxt_head, ln_list) {
		if(lnp->ln_data == (void *)dxt_p) {
			list_del(&lnp->ln_list);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if(!rc) {
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

out:
	return rc;
}

struct dxtg_operations dxtg_op = {
	.dxg_search_and_create_channel = dxtg_search_and_create_channel,
	.dxg_search_dxt = dxtg_search_dxt,
	.dxg_drop_channel = dxtg_drop_channel,
	.dxg_cleanup = dxtg_cleanup,
};

int
dxtg_initialization(struct dxtg_interface *dxtg_p, struct dxtg_setup *setup)
{
	char *log_header = "dxtg_initialization";
	struct dxtg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct dxt_setup *dxt_setup;
	int dxt_counter = 0;
	char *ds_name;

	nid_log_warning("%s: start ...", log_header);


	if (!setup->ini)
		return 0;	// this is jsut a ut

	dxtg_p->dxg_op = &dxtg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxtg_p->dxg_private = priv_p;


	priv_p->p_ini = setup->ini;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_txn = setup->txn;
	INIT_LIST_HEAD(&priv_p->p_dxt_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	ini_p = priv_p->p_ini;
	priv_p->p_cdsg = setup->cdsg_p;
	priv_p->p_sdsg = setup->sdsg_p;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "dxt"))
			dxt_counter++;
	}

	if (!dxt_counter)
		return 0;
	priv_p->p_counter = dxt_counter;
	priv_p->p_setup = calloc(dxt_counter, sizeof(*priv_p->p_setup));
	dxt_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "dxt"))
			continue;

		strcpy(dxt_setup->dxt_name,  sc_p->s_name);

		// from dxt_conf.c, req_size is not forced
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "req_size");
		if(the_key) {
			dxt_setup->req_size = *(int *)(the_key->k_value);
		} else {
			dxt_setup->req_size = UMSG_TX_HEADER_LEN;
		}

		// from dxt_conf.c, ds_name is not forced
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "ds_name");
		if(the_key) {
			ds_name = (char *)(the_key->k_value);
			strcpy(dxt_setup->ds_name, ds_name);
		}

		dxt_setup++;
	}
	return 0;
}
