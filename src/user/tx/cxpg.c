/*
 * cxpg.c
 * 	Implementation of Control Exchange Acive Guardian Module
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>

#include "util_nw.h"
#include "nid_log.h"
#include "nid.h"
#include "ini_if.h"
#include "lstn_if.h"
#include "cxp_if.h"
#include "cxpg_if.h"

struct cxpg_private {
	struct ini_interface		*p_ini;
	struct lstn_interface		*p_lstn;
	struct cxp_setup		*p_setup;
	struct cxtg_interface		*p_cxtg;
	int				p_counter;
	int				p_type;
	int				p_dsfd;
	u_short				p_dport;
	struct umpk_interface		*p_umpk;
	struct list_head		p_cxp_head;
	pthread_mutex_t			p_rs_mlck;
	int				p_busy;
};

static int
__cxp_type_mapping(char *cxp_type)
{

	if (!strcmp(cxp_type, "mrep") || !strcmp(cxp_type, "mreplication" ))
		return CXP_TYPE_MREPLICATION;
	else
		return CXP_TYPE_WRONG;
}

static struct cxp_interface *
cxpg_create_channel(struct cxpg_interface *cxpg_p, int csfd, char *cxp_uuid)
{
	char *log_header = "cxpg_create_channel";
	struct cxpg_private *priv_p = (struct cxpg_private *)cxpg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct cxp_setup *cxp_setup = priv_p->p_setup;
	struct cxp_interface *cxp_p = NULL;
	struct list_node *lnp;
	char *chan_uuid;
	int i, got_it = 0;

	nid_log_warning("%s: start ...", log_header);

	for (i = 0; i < priv_p->p_counter; i++, cxp_setup++) {
		if (!strcmp(cxp_uuid, cxp_setup->uuid)) {
			nid_log_warning("%s: found match uuid: %s", log_header, cxp_uuid);
			break;
		}

	}
	if (i == priv_p->p_counter) {
		nid_log_warning("%s: did not found match uuid: %s", log_header, cxp_uuid);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_rs_mlck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxp_head, ln_list) {
		cxp_p = (struct cxp_interface *)lnp->ln_data;
		chan_uuid = cxp_p->cx_op->cx_get_chan_uuid(cxp_p);
		if(!strcmp(cxp_uuid, chan_uuid)) {
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
	 * create a new cxp
	 */
	cxp_p = calloc(1, sizeof(*cxp_p));
	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)cxp_p;
	cxp_setup->csfd = csfd;
	cxp_setup->handle = lnp;
	cxp_setup->cxtg = priv_p->p_cxtg;
	cxp_initialization(cxp_p, cxp_setup);

	pthread_mutex_unlock(&priv_p->p_rs_mlck);
	priv_p->p_busy = 0;
	list_add_tail(&lnp->ln_list, &priv_p->p_cxp_head);
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

out:

	return cxp_p;
}

static int
cxpg_add_channel_setup(struct cxpg_interface *cxpg_p, char *uuid, char *cxt_name) {
	char *log_header = "cxpg_add_channel_setup";
	struct cxpg_private *priv_p = (struct cxpg_private *)cxpg_p->cxg_private;
	struct cxp_setup *cxp_setup = &priv_p->p_setup[0];
	int i, ret = 0;

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_counter; i++, cxp_setup++) {
		if (!strcmp(uuid, cxp_setup->uuid)) {
			ret = -1;
			goto out;
		}

	}

	priv_p->p_setup = realloc(priv_p->p_setup, (priv_p->p_counter + 1)*sizeof(struct cxp_setup));
	priv_p->p_counter++;
	cxp_setup = &priv_p->p_setup[i];
	strcpy(cxp_setup->uuid, uuid);
	strcpy(cxp_setup->cxt_name, cxt_name);

out:
	return ret;
}

static int
cxpg_drop_channel(struct cxpg_interface *cxpg_p, struct cxp_interface *cxp_p)
{
	char *log_header = "cxpg_drop_channel";
	struct cxpg_private *priv_p = (struct cxpg_private *)cxpg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_cxp_head, ln_list) {
		if(lnp->ln_data == (void *)cxp_p) {
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
cxpg_drop_channel_by_uuid(struct cxpg_interface *cxpg_p, char *cxp_uuid)
{
	struct cxpg_private *priv_p = (struct cxpg_private *)cxpg_p->cxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;
	struct cxp_interface *cxp_p;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_cxp_head, ln_list) {
		cxp_p = (struct cxp_interface *)lnp->ln_data;
		if(!strcmp(cxp_p->cx_op->cx_get_chan_uuid(cxp_p), cxp_uuid)) {
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

static struct cxp_interface *
cxpg_get_cxp_by_uuid(struct cxpg_interface *cxpg_p, char *cxp_uuid) {
	struct cxpg_private *priv_p = (struct cxpg_private *)cxpg_p->cxg_private;
	struct cxp_interface *cxp_p = NULL, *cxp_p_it;
	struct list_node *lnp;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_cxp_head, ln_list) {
		cxp_p_it = (struct cxp_interface *)lnp->ln_data;
		if(!strcmp(cxp_p_it->cx_op->cx_get_chan_uuid(cxp_p_it), cxp_uuid)) {
			cxp_p = cxp_p_it;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return cxp_p;
}

struct cxpg_operations cxpg_op = {
	.cxg_create_channel = cxpg_create_channel,
	.cxg_add_channel_setup = cxpg_add_channel_setup,
	.cxg_drop_channel = cxpg_drop_channel,
	.cxg_drop_channel_by_uuid = cxpg_drop_channel_by_uuid,
	.cxg_get_cxp_by_uuid = cxpg_get_cxp_by_uuid,
};

int
cxpg_initialization(struct cxpg_interface *cxpg_p, struct cxpg_setup *setup)
{
	char *log_header = "cxpg_initialization";
	struct cxpg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type, *cxp_type, *cxt_name;
	struct cxp_setup *cxp_setup;
	int cxp_counter = 0;

	nid_log_warning("%s: start ...", log_header);
	if (!setup->ini)
		return 0;	// this is just a ut

	cxpg_p->cxg_op = &cxpg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	cxpg_p->cxg_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_cxp_head);
	pthread_mutex_init(&priv_p->p_rs_mlck, NULL);

	priv_p->p_lstn = setup->lstn;
	priv_p->p_ini = setup->ini;
	priv_p->p_cxtg = setup->cxtg;
	ini_p = priv_p->p_ini;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "cxp"))
			cxp_counter++;
	}

	if (!cxp_counter)
		return 0;
	priv_p->p_counter = cxp_counter;
	priv_p->p_setup = calloc(cxp_counter, sizeof(*priv_p->p_setup));
	cxp_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "cxp"))
			continue;

		strcpy(cxp_setup->uuid,  sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cxp_type");
		if(!the_key) {
			nid_log_error("%s: cxp_type item not set in conf file", log_header);
			assert(0);
		}
		cxp_type = (char *)(the_key->k_value);
		priv_p->p_type = __cxp_type_mapping(cxp_type);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "cxt_name");
		if(!the_key) {
			nid_log_error("%s: cxt_name item not set in conf file", log_header);
			assert(0) ;
		}
		cxt_name = (char *)(the_key->k_value);
		strcpy(cxp_setup->cxt_name, cxt_name);

		cxp_setup++;
	}

	return 0;
}
