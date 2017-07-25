/*
 * cxtg.c
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
#include "cxtg_if.h"
#include "cxt_if.h"
#include "tx_shared.h"


struct cxtg_private {
	struct ini_interface	*p_ini;
	struct lstn_interface		*p_lstn;
	struct txn_interface		*p_txn;
	struct pp2_interface		*p_pp2;
	struct cxt_setup		*p_setup;
	struct cdsg_interface		*p_cdsg;
	struct sdsg_interface		*p_sdsg;
	pthread_mutex_t			p_lck;
	int				p_counter;
	struct list_head		p_cxt_head;
	int				p_busy;
};


static struct cxt_interface *
cxtg_search_cxt(struct cxtg_interface *cxtg_p, char *cxt_name)
{
	char *log_header = "cxtg_search_cxt";
	struct cxtg_private *priv_p = (struct cxtg_private *)cxtg_p->cxg_private;
	struct cxt_interface *cxt_p = NULL;
	struct list_node *lnp;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxt_head, ln_list) {
		cxt_p = (struct cxt_interface *)lnp->ln_data;
		target_name = cxt_p->cx_op->cx_get_name(cxt_p);
		if(!strcmp(target_name, cxt_name)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if(!got_it)
		cxt_p = NULL;

	return cxt_p;

}

static struct cxt_interface *
cxtg_search_and_create_channel(struct cxtg_interface *cxtg_p, char *cxt_name, int csfd)
{
	char *log_header = "cxtg_search_and_create_channel";
	struct cxtg_private *priv_p = (struct cxtg_private *)cxtg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct txn_interface *txn_p = priv_p->p_txn;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct cxt_setup *cxt_setup = &priv_p->p_setup[0];
	struct cxt_interface *cxt_p = NULL;
	struct sdsg_interface *sdsg_p;
	struct cdsg_interface *cdsg_p;
	struct list_node *lnp;
	char *ds_name;
	int i;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);

	for (i = 0; i < priv_p->p_counter; i++, cxt_setup++) {
		if (!strcmp(cxt_name, cxt_setup->cxt_name)) {
			nid_log_warning("%s: found match cxt name: %s", log_header, cxt_name);
			break;
		}
	}
	if (i == priv_p->p_counter) {
		nid_log_warning("%s: did not found match cxt name: %s", log_header, cxt_name);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_lck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_lck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_lck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxt_head, ln_list) {
		cxt_p = (struct cxt_interface *)lnp->ln_data;
		target_name = cxt_p->cx_op->cx_get_name(cxt_p);
		if(!strcmp(target_name, cxt_name)) {
			got_it = 1;
			break;
		}
	}
	if(!got_it) {
		priv_p->p_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if(!got_it) {
		sdsg_p = priv_p->p_sdsg;
		cdsg_p = priv_p->p_cdsg;
		/*
		 * create a new cxt
		 */
		cxt_p = calloc(1, sizeof(*cxt_p));
		cxt_setup->sfd = csfd;
		cxt_setup->lstn = lstn_p;
		cxt_setup->txn = txn_p;
		cxt_setup->pp2 = pp2_p;

		// ds_name set in dxt_setup from conf file
		ds_name = cxt_setup->ds_name;
		if (sdsg_p) {
			cxt_setup->ds = sdsg_p->dg_op->dg_search_and_create_sds(sdsg_p, ds_name);
			if(cxt_setup->ds) {
				nid_log_debug("%s: success create sds instance %s", log_header, ds_name);
			} else {
				nid_log_error("%s: fail to create sds instance %s", log_header, ds_name);
			}
		}

		if (!cxt_setup->ds && cdsg_p) {
			cxt_setup->ds = cdsg_p->dg_op->dg_search_and_create_cds(cdsg_p, ds_name);
			if(cxt_setup->ds) {
				nid_log_debug("%s: success create cds instance %s", log_header, ds_name);
			} else {
				nid_log_error("%s: fail to create cds instance %s", log_header, ds_name);
			}
		}

		assert(cxt_setup->ds);
		cxt_setup->req_size = UMSG_TX_HEADER_LEN;
		cxt_initialization(cxt_p, cxt_setup);
		lnp = lstn_p->ln_op->ln_get_node(lstn_p);
		lnp->ln_data = (void *)cxt_p;
		cxt_setup->handle = lnp;

		pthread_mutex_lock(&priv_p->p_lck);
		list_add_tail(&lnp->ln_list, &priv_p->p_cxt_head);
		priv_p->p_busy = 0;
		pthread_mutex_unlock(&priv_p->p_lck);
	}

out:
	return cxt_p;
}

static int
cxtg_add_channel_setup(struct cxtg_interface *cxtg_p, char *cxt_name, char *ds_name, int req_size) {
	char *log_header = "cxag_add_channel_setup";
	struct cxtg_private *priv_p = (struct cxtg_private *)cxtg_p->cxg_private;
	struct cxt_setup *cxt_setup = &priv_p->p_setup[0];
	int i, ret = 0;

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_counter; i++, cxt_setup++) {
		if (!strcmp(cxt_name, cxt_setup->cxt_name)) {
			ret = -1;
			goto out;
		}

	}

	priv_p->p_setup = realloc(priv_p->p_setup, (priv_p->p_counter + 1)*sizeof(struct cxt_setup));
	priv_p->p_counter++;
	cxt_setup = &priv_p->p_setup[i];
	strcpy(cxt_setup->cxt_name, cxt_name);
	strcpy(cxt_setup->ds_name, ds_name);
	cxt_setup->req_size = req_size;

out:
	return ret;
}

static int
cxtg_drop_channel(struct cxtg_interface *cxtg_p, struct cxt_interface *cxt_p) {
	char *log_header = "cxtg_drop_channel";
	struct cxtg_private *priv_p = (struct cxtg_private *)cxtg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_cxt_head, ln_list) {
		if(lnp->ln_data == (void *)cxt_p) {
			list_del(&lnp->ln_list);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if(!rc) {
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

	return rc;
}


static void
cxtg_cleanup(struct cxtg_interface *cxtg_p) {
	char *log_header = "cxtg_cleanup";
	struct cxtg_private *priv_p = (struct cxtg_private *)cxtg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct cxt_interface *cxt_p = NULL;
	struct list_node *lnp, *lnp1;

	nid_log_warning("%s: start ...", log_header);

	// should we release lock after list_del and get lock again after call dx_cleanup
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_cxt_head, ln_list) {
		list_del(&lnp->ln_list);
		cxt_p = (struct cxt_interface *)lnp->ln_data;
		cxt_p->cx_op->cx_cleanup(cxt_p);
		free(cxt_p);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	free(priv_p);
}

struct cxtg_operations cxtg_op = {
	.cxg_search_and_create_channel = cxtg_search_and_create_channel,
	.cxg_search_cxt = cxtg_search_cxt,
	.cxg_add_channel_setup = cxtg_add_channel_setup,
	.cxg_drop_channel = cxtg_drop_channel,
	.cxg_cleanup = cxtg_cleanup,
};

int
cxtg_initialization(struct cxtg_interface *cxtg_p, struct cxtg_setup *setup)
{
	char *log_header = "cxtg_initialization";
	struct cxtg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct cxt_setup *cxt_setup;
	int cxt_counter = 0;
	char *ds_name;

	nid_log_warning("%s: start ...", log_header);

	if (!setup->ini)
		return 0;	// this is jsut a ut

	cxtg_p->cxg_op = &cxtg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxtg_p->cxg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_cdsg = setup->cdsg_p;
	priv_p->p_sdsg = setup->sdsg_p;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_txn = setup->txn;
	priv_p->p_pp2 = setup->pp2;

	INIT_LIST_HEAD(&priv_p->p_cxt_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	ini_p = priv_p->p_ini;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "cxt"))
			cxt_counter++;
	}

	if (!cxt_counter)
		return 0;
	priv_p->p_counter = cxt_counter;
	priv_p->p_setup = calloc(cxt_counter, sizeof(*priv_p->p_setup));
	cxt_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "cxt"))
			continue;

		strcpy(cxt_setup->cxt_name,  sc_p->s_name);

		// from cxt_conf.c, req_size is not forced
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "req_size");
		if(the_key) {
			cxt_setup->req_size = *(int *)(the_key->k_value);
		} else {
			cxt_setup->req_size = UMSG_TX_HEADER_LEN;
		}

		// from cxt_conf.c, ds_name is not forced
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "ds_name");
		if(the_key) {
			ds_name = (char *)(the_key->k_value);
			strcpy(cxt_setup->ds_name,  ds_name);
		}

		cxt_setup++;
	}
	return 0;
}
