/*
 * cxag.c
 * 	Implementation of Control Exchange Acive Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "nid_log.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "cdsg_if.h"
#include "sdsg_if.h"
#include "cxa_if.h"
#include "cxag_if.h"
#include "pp2_if.h"
#include "umpk_if.h"
#include "umpk_cxp_if.h"

struct ds_interface;
struct cxag_private {
	struct lstn_interface		*p_lstn;
	struct ini_interface		*p_ini;
	struct cxa_setup		*p_setup;
	struct cxtg_interface		*p_cxtg;
	struct ds_interface		*p_ds;
	int				p_counter;
	int				p_type;
	struct list_head		p_cxa_head;
	pthread_mutex_t			p_rs_mlck;
	struct pp2_interface		*p_pp2;
	struct umpk_interface		*p_umpk;
	char				p_dr_done;
	char				p_stop;
	int				p_busy;
};

#ifdef DO_HOUSEKEEPING
static void* _do_housekeeping(void *p);
#endif

static int
__cxa_type_mapping(char *cxa_type)
{

	if (!strcmp(cxa_type, "mrep") || !strcmp(cxa_type, "mreplication" ))
		return CXA_TYPE_MREPLICATION;
	else
		return CXA_TYPE_WRONG;
}

static struct cxa_interface *
cxag_search_cxa(struct cxag_interface *cxag_p, char *cxa_uuid)
{
	char *log_header = "cxag_search_cxa";
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct cxa_interface *cxa_p = NULL;
	struct list_node *lnp;
	int got_it = 0;
	char *chan_uuid;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxa_head, ln_list) {
		cxa_p = (struct cxa_interface *)lnp->ln_data;
		chan_uuid = cxa_p->cx_op->cx_get_chan_uuid(cxa_p);
		if(!strcmp(chan_uuid, cxa_uuid)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	if(!got_it)
		cxa_p = NULL;

	return cxa_p;
}

static struct cxa_interface *
cxag_create_channel(struct cxag_interface *cxag_p, char *cxa_uuid)
{
	char *log_header = "cxag_create_channel";
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct cxa_setup *cxa_setup = &priv_p->p_setup[0];
	struct cxa_interface *cxa_p = NULL;
	char *chan_uuid;
	struct list_node *lnp;
	int i, got_it = 0;
#ifdef DO_HOUSEKEEPING
	int start_housekeeping = 0;
#endif

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_counter; i++, cxa_setup++) {
		if (!strcmp(cxa_uuid, cxa_setup->uuid)) {
			nid_log_warning("%s: found match uuid: %s", log_header, cxa_uuid);
			break;
		}
	}
	if (i == priv_p->p_counter) {
		nid_log_warning("%s: did not found match uuid: %s", log_header, cxa_uuid);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_rs_mlck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxa_head, ln_list) {
		cxa_p = (struct cxa_interface *)lnp->ln_data;
		chan_uuid = cxa_p->cx_op->cx_get_chan_uuid(cxa_p);
		if(!strcmp(cxa_uuid, chan_uuid)) {
			got_it = 1;
			break;
		}
	}
	if(!got_it) {
		priv_p->p_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(got_it)
		goto out;
	/*
	 * create a new cxa
	 */
	cxa_p = calloc(1, sizeof(*cxa_p));
	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)cxa_p;
	cxa_setup->handle = (void *)lnp;
	cxa_setup->cxtg	= priv_p->p_cxtg;
	cxa_setup->ds = priv_p->p_ds;
	cxa_initialization(cxa_p, cxa_setup);

	pthread_mutex_lock(&priv_p->p_rs_mlck);
#ifdef DO_HOUSEKEEPING
	if(list_empty(&priv_p->p_cxa_head)) {
		start_housekeeping = 1;
	}
#endif
	list_add_tail(&lnp->ln_list, &priv_p->p_cxa_head);
	priv_p->p_busy = 0;
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

#ifdef DO_HOUSEKEEPING
	if(start_housekeeping) {
		pthread_attr_t attr;
		pthread_t thread_id;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, _do_housekeeping, cxag_p);
	}
#endif

out:
	return cxa_p;
}

#ifdef DO_HOUSEKEEPING
static int
_do_dr_housekeeping(struct cxag_interface *cxag_p)
{
	char *log_header = "_do_dr_housekeeping";
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct cxa_interface *cxa_p;
	struct list_node *lnp;

	nid_log_debug("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_mlck);

	// do housekeeping on every dxa channel
	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxa_head, ln_list) {
		cxa_p = lnp->ln_data;
		nid_log_debug("%s: run housekeeping on channel %s", log_header, cxa_p->cx_op->cx_get_chan_uuid(cxa_p));
		cxa_p->cx_op->cx_housekeeping(cxa_p);
	}

	if (priv_p->p_stop || list_empty(&priv_p->p_cxa_head))
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
	struct cxag_interface *cxag_p = (struct cxag_interface *)p;
	int do_next;

	nid_log_debug("%s: start ...", log_header);
	while(1) {
		nid_log_debug("%s: run housekeeping ...", log_header);
		do_next = _do_dr_housekeeping(cxag_p);
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

static int
cxag_add_channel_setup(struct cxag_interface *cxag_p, char *uuid, char *ip, char *cxt_name) {
	char *log_header = "cxag_add_channel_setup";
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct cxa_setup *cxa_setup = &priv_p->p_setup[0];
	int i, ret = 0;

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_counter; i++, cxa_setup++) {
		if (!strcmp(uuid, cxa_setup->uuid)) {
			ret = -1;
			goto out;
		}

	}

	priv_p->p_setup = realloc(priv_p->p_setup, (priv_p->p_counter + 1)*sizeof(struct cxa_setup));
	priv_p->p_counter++;
	cxa_setup = &priv_p->p_setup[i];
	strcpy(cxa_setup->uuid, uuid);
	strcpy(cxa_setup->ip, ip);
	strcpy(cxa_setup->cxt_name, cxt_name);

out:
	return ret;
}

static int
cxag_drop_channel(struct cxag_interface *cxag_p, struct cxa_interface *cxa_p)
{
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_cxa_head, ln_list) {
		if(lnp->ln_data == (void *)cxa_p) {
			list_del(&lnp->ln_list);
			rc = 0;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(!rc) {
		cxa_p->cx_op->cx_cleanup(cxa_p);
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
	}

	return rc;
}

static int
cxag_callback(struct cxag_interface *cxag_p, struct cxa_interface *cxa_p, int cmd) {
	if(cmd == UMSG_CXP_CMD_DROP_CHANNEL) {
		return cxag_drop_channel(cxag_p, cxa_p);
	}

	return 0;
}

static struct cxa_setup *
cxag_get_all_cxa_setup(struct cxag_interface *cxag_p, int *num_cxa)
{
	char *log_header = "cxag_get_all_cxa_setup";
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;

	nid_log_warning("%s: start ...", log_header);
	*num_cxa = priv_p->p_counter;
	return priv_p->p_setup;
}

static char **
cxag_get_working_cxa_name(struct cxag_interface *cxag_p, int *num_cxa)
{
	struct cxag_private *priv_p = (struct cxag_private *)cxag_p->cxg_private;
	struct cxa_interface *cxa_p;
	struct list_node *ln_p;
	char **ret;
	int i = 0;

	ret = calloc(priv_p->p_counter, sizeof(*ret));
	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(ln_p, struct list_node, &priv_p->p_cxa_head, ln_list) {
		cxa_p = (struct cxa_interface *)ln_p->ln_data;
		ret[i] = cxa_p->cx_op->cx_get_chan_uuid(cxa_p);
		i++;
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	*num_cxa = i;

	return ret;
}

struct cxag_operations cxag_op = {
	.cxg_search_cxa = cxag_search_cxa,
	.cxg_create_channel = cxag_create_channel,
	.cxg_add_channel_setup = cxag_add_channel_setup,
	.cxg_drop_channel = cxag_drop_channel,
	.cxg_callback = cxag_callback,
	.cxg_get_all_cxa_setup = cxag_get_all_cxa_setup,
	.cxg_get_working_cxa_name = cxag_get_working_cxa_name,
};

int
cxag_initialization(struct cxag_interface *cxag_p, struct cxag_setup *setup)
{
	char *log_header = "cxag_initialization";
	struct cxag_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type, *cxa_type, *cxt_name;
	struct cxa_setup *cxa_setup;
	int cxa_counter = 0;

	nid_log_warning("%s: start ...", log_header);
	if (!setup->ini)
		return 0;	// this is jsut a ut

	cxag_p->cxg_op = &cxag_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxag_p->cxg_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_cxa_head);
	pthread_mutex_init(&priv_p->p_rs_mlck, NULL);

	priv_p->p_umpk = setup->umpk;
	priv_p->p_lstn = setup->lstn;
	priv_p->p_ini = setup->ini;
	priv_p->p_cxtg = setup->cxtg;
	priv_p->p_pp2 = setup->pp2;
	ini_p = priv_p->p_ini;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "cxa"))
			cxa_counter++;
	}

	if (!cxa_counter)
		return 0;
	priv_p->p_counter = cxa_counter;
	priv_p->p_setup = calloc(cxa_counter, sizeof(*priv_p->p_setup));
	cxa_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "cxa"))
			continue;

		strcpy(cxa_setup->uuid,  sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cxa_type");
		if(the_key) {
			cxa_type = (char *)(the_key->k_value);
			priv_p->p_type = __cxa_type_mapping(cxa_type);
		}

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cxt_name");
		if(!the_key) {
			nid_log_error("%s: cxt_name item not set in conf file", log_header);
			assert(0);
		}
		cxt_name = (char *)(the_key->k_value);
		strcpy(cxa_setup->cxt_name, cxt_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "ip");
		if(!the_key) {
			nid_log_error("%s: ip item not set in conf file", log_header);
			assert(0);
		}
		strcpy(cxa_setup->ip, the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "peer_uuid");
		if(!the_key) {
			nid_log_error("%s: peer_uuid item not set in conf file", log_header);
			assert(0);
		}
		strcpy(cxa_setup->peer_uuid, the_key->k_value);

		cxa_setup++;
	}

	return 0;
}
