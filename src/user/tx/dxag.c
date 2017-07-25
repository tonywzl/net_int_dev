/*
 * dxag.c
 * 	Implementation of Data Exchange Acive Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "nid.h"
#include "nid_log.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "dxa_if.h"
#include "dxag_if.h"
#include "pp2_if.h"
#include "umpk_if.h"
#include "pp2_if.h"
#include "allocator_if.h"
#include "tx_shared.h"

struct dxag_private {
	struct lstn_interface		*p_lstn;
	struct ini_interface		*p_ini;
	struct dxa_setup		*p_setup;
	int				p_counter;
	int				p_type;
	struct list_head		p_dxa_head;
	pthread_mutex_t			p_rs_mlck;
	struct pp2_interface		*p_pp2;
	struct dxtg_interface		*p_dxtg;
	char				p_stop;
	char				p_dr_done;
	struct umpk_interface		*p_umpk;
	int				p_busy;
};

static int
dxag_drop_channel(struct dxag_interface *, struct dxa_interface *);
/*
static int
__dxa_type_mapping(char *dxa_type)
{

	if (!strcmp(dxa_type, "mrep") || !strcmp(dxa_type, "mreplication" ))
		return DXA_TYPE_MREPLICATION;
	else
		return DXA_TYPE_WRONG;
}
*/

#ifdef DO_HOUSEKEEPING

static int
_do_dr_housekeeping(struct dxag_interface *dxag_p)
{
	char *log_header = "_do_dr_housekeeping";
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct dxa_interface *dxa_p;
	struct list_node *lnp;

	nid_log_debug("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_mlck);

	// do housekeeping on every dxa channel
	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxa_head, ln_list) {
		dxa_p = lnp->ln_data;
		nid_log_debug("%s: run housekeeping on channel %s", log_header, dxa_p->dx_op->dx_get_chan_uuid(dxa_p));
		dxa_p->dx_op->dx_housekeeping(dxa_p);
	}

	if (priv_p->p_stop || list_empty(&priv_p->p_dxa_head))
		priv_p->p_dr_done = 1;

	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if (!priv_p->p_dr_done) {
		return 1;
	} else {
		return 0;
	}
}

static void*
_do_housekeeping(void *p)
{
	char *log_header = "_do_housekeeping";
	struct dxag_interface *dxag_p = (struct dxag_interface *)p;
	int do_next;

	nid_log_debug("%s: start ...", log_header);
	while(1) {
		nid_log_debug("%s: run housekeeping ...", log_header);
		do_next = _do_dr_housekeeping(dxag_p);
		if (!do_next) {
			nid_log_debug("%s: done ...", log_header);
			break;
		}
		// do housekeeping every 1s
		sleep(1);
	}
	return NULL;
}

#endif

static struct dxa_interface *
dxag_create_channel(struct dxag_interface *dxag_p, char *dxa_uuid)
{
	char* log_header = "dxag_create_channel";
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct dxa_setup *dxa_setup = &priv_p->p_setup[0];
	struct dxa_interface *dxa_p = NULL;
	struct list_node *lnp;
	char *chan_uuid;
	int i, got_it = 0;
#ifdef DO_HOUSEKEEPING
	int start_housekeeping = 0;
#endif

	nid_log_warning("%s: uuid is %s", log_header, dxa_uuid);

	for (i = 0; i < priv_p->p_counter; i++, dxa_setup++) {
		if (!strcmp(dxa_uuid, dxa_setup->uuid)) {
			nid_log_warning("%s: found match uuid: %s", log_header, dxa_uuid);
			break;
		}
	}
	if (i == priv_p->p_counter){
		nid_log_warning("%s: did not found match uuid: %s", log_header, dxa_uuid);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_rs_mlck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxa_head, ln_list) {
		dxa_p = (struct dxa_interface *)lnp->ln_data;
		chan_uuid = dxa_p->dx_op->dx_get_chan_uuid(dxa_p);
		if(!strcmp(dxa_uuid, chan_uuid)) {
			got_it = 1;
			break;
		}
	}
	if(!got_it) {
		priv_p->p_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(!got_it) {
		/*
		 * create a new dxa
		 */
		dxa_p = calloc(1, sizeof(*dxa_p));
		lnp = lstn_p->ln_op->ln_get_node(lstn_p);
		lnp->ln_data = (void *)dxa_p;
		dxa_setup->handle = (void *)lnp;
		dxa_setup->dxag = dxag_p;
		dxa_setup->dxtg = priv_p->p_dxtg;
		dxa_setup->umpk = priv_p->p_umpk;
		dxa_initialization(dxa_p, dxa_setup);

		pthread_mutex_lock(&priv_p->p_rs_mlck);
#ifdef DO_HOUSEKEEPING
		if(list_empty(&priv_p->p_dxa_head)) {
			start_housekeeping = 1;
		}
#endif
		list_add_tail(&lnp->ln_list, &priv_p->p_dxa_head);
		priv_p->p_busy = 0;
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
	}

#ifdef DO_HOUSEKEEPING
	if(start_housekeeping) {
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, _do_housekeeping, dxag_p);
	}
#endif

out:
	return dxa_p;
}

static struct dxa_interface *
dxag_get_dxa_by_uuid(struct dxag_interface *dxag_p, char *dxa_uuid) {
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct dxa_interface *dxa_p = NULL, *dxa_p_it;
	struct list_node *lnp;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxa_head, ln_list) {
		dxa_p_it = (struct dxa_interface *)lnp->ln_data;
		if(!strcmp(dxa_p_it->dx_op->dx_get_chan_uuid(dxa_p_it), dxa_uuid)) {
			dxa_p = dxa_p_it;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return dxa_p;
}


static int
dxag_drop_channel(struct dxag_interface *dxag_p, struct dxa_interface *dxa_p)
{
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxa_head, ln_list) {
		if(lnp->ln_data == (void *)dxa_p) {
			list_del(&lnp->ln_list);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(!rc) {
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

	return rc;
}


static int
dxag_drop_channel_by_uuid(struct dxag_interface *dxag_p, char *dxa_uuid)
{
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;
	struct dxa_interface *dxa_p;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxa_head, ln_list) {
		dxa_p = (struct dxa_interface *)lnp->ln_data;
		if(!strcmp(dxa_p->dx_op->dx_get_chan_uuid(dxa_p), dxa_uuid)) {
			list_del(&lnp->ln_list);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(!rc) {
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

	return rc;
}

static struct dxa_setup *
dxag_get_all_dxa_setup(struct dxag_interface *dxag_p, int *num_dxa)
{
	char *log_header = "dxag_get_all_dxa_setup";
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;

	nid_log_warning("%s: start ...", log_header);
	*num_dxa = priv_p->p_counter;
	return priv_p->p_setup;
}

static char **
dxag_get_working_dxa_name(struct dxag_interface *dxag_p, int *num_dxa)
{
	struct dxag_private *priv_p = (struct dxag_private *)dxag_p->dxg_private;
	struct dxa_interface *dxa_p;
	struct list_node *ln_p;
	char **ret;
	int i = 0;

	ret = calloc(priv_p->p_counter, sizeof(*ret));
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(ln_p, struct list_node, &priv_p->p_dxa_head, ln_list) {
		dxa_p = (struct dxa_interface *)ln_p->ln_data;
		ret[i] = dxa_p->dx_op->dx_get_chan_uuid(dxa_p);
		i++;
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	*num_dxa = i;

	return ret;
}

struct dxag_operations dxag_op = {
	.dxg_create_channel = dxag_create_channel,
	.dxg_drop_channel = dxag_drop_channel,
	.dxg_drop_channel_by_uuid = dxag_drop_channel_by_uuid,
	.dxg_get_dxa_by_uuid = dxag_get_dxa_by_uuid,
	.dxg_get_all_dxa_setup = dxag_get_all_dxa_setup,
	.dxg_get_working_dxa_name = dxag_get_working_dxa_name,
};

int
dxag_initialization(struct dxag_interface *dxag_p, struct dxag_setup *setup)
{
	char *log_header = "dxag_initialization";
	struct dxag_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct dxa_setup *dxa_setup;
	int dxa_counter = 0;

	nid_log_warning("%s: start ...", log_header);
	if (!setup->ini)
		return 0;       // this is just a ut

	dxag_p->dxg_op = &dxag_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxag_p->dxg_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_dxa_head);
	pthread_mutex_init(&priv_p->p_rs_mlck, NULL);

	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxtg = setup->dxtg;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_ini = setup->ini;
	ini_p = priv_p->p_ini;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "dxa"))
			dxa_counter++;
	}

	if (!dxa_counter)
		return 0;
	priv_p->p_counter = dxa_counter;
	nid_log_warning("%s: %d dxa founded",log_header, dxa_counter);
	priv_p->p_setup = calloc(dxa_counter, sizeof(*priv_p->p_setup));
	dxa_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "dxa")){
			continue;
		}

		// setup dxa uuid
		strcpy(dxa_setup->uuid,  sc_p->s_name);

		// setup dxp side uuid
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "peer_uuid");
		if(!the_key) {
			nid_log_error("%s: no peer_uuid item in conf file", log_header);
			assert(0);
		}
		strcpy(dxa_setup->peer_uuid, the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "dxt_name");
		if(!the_key) {
			nid_log_error("%s: no dxt_name item in conf file", log_header);
			assert(0);
		}
		strcpy(dxa_setup->dxt_name, the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "ip");
		if(!the_key) {
			nid_log_error("%s: no ip item in conf file", log_header);
			assert(0);
		}
		strcpy(dxa_setup->ip, the_key->k_value);
		dxa_setup->umpk = priv_p->p_umpk;

		dxa_setup++;
	}

	return 0;
}
