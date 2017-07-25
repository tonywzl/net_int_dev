/*
 * dxpg.c
 * 	Implementation of Data Exchange Acive Guardian Module
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
#include "dxp_if.h"
#include "dxpg_if.h"
#include "umpk_dxa_if.h"
#include "allocator_if.h"

struct dxpg_private {
	struct ini_interface		*p_ini;
	struct lstn_interface		*p_lstn;
	struct dxp_setup		*p_setup;
	int				p_counter;
	int				p_type;
	struct umpk_interface		*p_umpk;
	struct list_head		p_dxp_head;
	pthread_mutex_t			p_rs_mlck;
	int				p_busy;
	struct dxtg_interface		*p_dxtg;
};

static int
__dxp_type_mapping(char *dxp_type)
{

	if (!strcmp(dxp_type, "mrep") || !strcmp(dxp_type, "mreplication" ))
		return DXP_TYPE_MREPLICATION;
	else
		return DXP_TYPE_WRONG;
}

static struct dxp_interface *
dxpg_create_channel(struct dxpg_interface *dxpg_p, int csfd, char *dxp_uuid)
{
	char *log_header = "dxpg_create_channel";
	struct dxpg_private *priv_p = (struct dxpg_private *)dxpg_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct dxp_setup *dxp_setup = priv_p->p_setup;
	struct dxp_interface *dxp_p;
	struct list_node *lnp;
	char *chan_uuid;
	int i, ret, got_it = 0;

	nid_log_warning("%s: start ...", log_header);

	for (i = 0; i < priv_p->p_counter; i++, dxp_setup++) {
		if (!strcmp(dxp_uuid, dxp_setup->uuid)) {
			nid_log_warning("%s: found match uuid: %s", log_header, dxp_uuid);
			break;
		}
	}
	if (i == priv_p->p_counter) {
		nid_log_warning("%s: did not found match uuid: %s", log_header, dxp_uuid);
		goto out;
	}

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	while(priv_p->p_busy) {
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
		usleep(10*1000);
		pthread_mutex_lock(&priv_p->p_rs_mlck);
	}

	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxp_head, ln_list) {
		dxp_p = (struct dxp_interface *)lnp->ln_data;
		chan_uuid = dxp_p->dx_op->dx_get_chan_uuid(dxp_p);
		if(!strcmp(dxp_uuid, chan_uuid)) {
			got_it = 1;
			break;
		}
	}
	if(!got_it) {
		priv_p->p_busy = 1;
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	if(got_it) {
		// when dxp exist, it means it's not first time that dxa connect to dxp side
		// it need wait dxa connect data port
		nid_log_warning("%s: found exist dxp uuid: %s", log_header, dxp_uuid);

		// re-bind dport
		if(dxp_p->dx_op->dx_init_dport(dxp_p)) {
			dxp_p = NULL;
			goto error_lbl;
		} else {
			dxp_p->dx_op->dx_bind_socket(dxp_p, csfd);
			goto wait_dconn;
		}
	}

	/*
	 * create a new dxp
	 */
	dxp_p = calloc(1, sizeof(*dxp_p));
	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)dxp_p;
	dxp_setup->csfd = csfd;
	dxp_setup->umpk = priv_p->p_umpk;
	dxp_setup->dxpg = dxpg_p;
	dxp_setup->dxtg = priv_p->p_dxtg;
	dxp_initialization(dxp_p, dxp_setup);

	nid_log_warning("%s: create dxp uuid: %s", log_header, dxp_uuid);

wait_dconn:
	ret = dxp_p->dx_op->dx_ack_dport(dxp_p, csfd);
	if(ret) {
		nid_log_warning("%s: failed to send back port info of dxp uuid: %s", log_header, dxp_uuid);
		goto error_lbl;
	}

	/* waiting for the active site to connect me. */
	ret = dxp_p->dx_op->dx_accept(dxp_p);
	if(ret < 0) {
		nid_log_warning("%s: failed to wait dxa side connect to %s", log_header, dxp_uuid);
		goto error_lbl;
	}

	// only need insert dxp to dxp list, when dxp is new create
	if(!got_it) {
		pthread_mutex_lock(&priv_p->p_rs_mlck);
		list_add_tail(&lnp->ln_list, &priv_p->p_dxp_head);
		priv_p->p_busy = 0;
		pthread_mutex_unlock(&priv_p->p_rs_mlck);
	}
	nid_log_warning("%s: create dxp uuid: %s success", log_header, dxp_uuid);
	goto out;

error_lbl:
	if(!got_it) {
		// destroy resource allocated here
		lstn_p->ln_op->ln_put_node(lstn_p, lnp);
		dxp_p->dx_op->dx_cleanup(dxp_p);
		free(dxp_p);
		dxp_p = NULL;
	}

out:
	return dxp_p;
}

static struct dxp_interface *
dxpg_get_dxp_by_uuid(struct dxpg_interface *dxpg_p, char *dxp_uuid) {
	struct dxpg_private *priv_p = (struct dxpg_private *)dxpg_p->dxg_private;
	struct dxp_interface *dxp_p = NULL, *dxp_p_it;
	struct list_node *lnp;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry(lnp, struct list_node, &priv_p->p_dxp_head, ln_list) {
		dxp_p_it = (struct dxp_interface *)lnp->ln_data;
		if(!strcmp(dxp_p_it->dx_op->dx_get_chan_uuid(dxp_p_it), dxp_uuid)) {
			dxp_p = dxp_p_it;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_mlck);

	return dxp_p;
}

static int
dxpg_add_channel_setup(struct dxpg_interface *dxpg_p, char *uuid, char *dxt_name) {
        char *log_header = "dxpg_add_channel_setup";
        struct dxpg_private *priv_p = (struct dxpg_private *)dxpg_p->dxg_private;
        struct dxp_setup *dxp_setup = &priv_p->p_setup[0];
        int i, ret = 0;

        nid_log_warning("%s: start ...", log_header);
        for (i = 0; i < priv_p->p_counter; i++, dxp_setup++) {
                if (!strcmp(uuid, dxp_setup->uuid)) {
                        ret = -1;
                        goto out;
                }

        }

        priv_p->p_setup = realloc(priv_p->p_setup, (priv_p->p_counter + 1)*sizeof(struct dxp_setup));
        priv_p->p_counter++;
        dxp_setup = &priv_p->p_setup[i];
        strcpy(dxp_setup->uuid, uuid);
        strcpy(dxp_setup->dxt_name, dxt_name);

out:
        return ret;
}

static int
dxpg_drop_channel(struct dxpg_interface *dxpg_p, struct dxp_interface *dxp_p)
{
	char *log_header = "dxpg_drop_channel";
	struct dxpg_private *priv_p = (struct dxpg_private *)dxpg_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxp_head, ln_list) {
		if(lnp->ln_data == (void *)dxp_p) {
			nid_log_warning("%s: delete dxp: %p ...", log_header, dxp_p);
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
dxpg_drop_channel_by_uuid(struct dxpg_interface *dxpg_p, char *dxp_uuid)
{
	struct dxpg_private *priv_p = (struct dxpg_private *)dxpg_p->dxg_private;
	struct lstn_interface *lstn_p = priv_p->p_lstn;
	struct list_node *lnp, *lnp1;
	int rc = 1;
	struct dxp_interface *dxp_p;

	pthread_mutex_lock(&priv_p->p_rs_mlck);
	list_for_each_entry_safe(lnp, lnp1, struct list_node, &priv_p->p_dxp_head, ln_list) {
		dxp_p = (struct dxp_interface *)lnp->ln_data;
		if(!strcmp(dxp_p->dx_op->dx_get_chan_uuid(dxp_p), dxp_uuid)) {
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

struct dxpg_operations dxpg_op = {
	.dxg_create_channel = dxpg_create_channel,
	.dxg_add_channel_setup = dxpg_add_channel_setup,
	.dxg_drop_channel = dxpg_drop_channel,
	.dxg_drop_channel_by_uuid = dxpg_drop_channel_by_uuid,
	.dxg_get_dxp_by_uuid = dxpg_get_dxp_by_uuid,
};

int
dxpg_initialization(struct dxpg_interface *dxpg_p, struct dxpg_setup *setup)
{
	char *log_header = "dxpg_initialization";
	struct dxpg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type, *dxp_type, *dxt_name;
	struct dxp_setup *dxp_setup;
	int dxp_counter = 0;

	nid_log_warning("%s: start ...", log_header);
	if (!setup->ini)
		return 0;	// this is jsut a ut

	dxpg_p->dxg_op = &dxpg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dxpg_p->dxg_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_dxp_head);

	priv_p->p_lstn = setup->lstn;
	priv_p->p_ini = setup->ini;
	ini_p = priv_p->p_ini;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxtg = setup->dxtg;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "dxp"))
			dxp_counter++;
	}

	if (!dxp_counter)
		return 0;

	priv_p->p_counter = dxp_counter;
	priv_p->p_setup = calloc(dxp_counter, sizeof(*priv_p->p_setup));
	dxp_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "dxp"))
			continue;

		// setup dxp uuid
		strcpy(dxp_setup->uuid,  sc_p->s_name);

		// setup dxa side uuid
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "peer_uuid");
		if(!the_key) {
			nid_log_error("%s: no peer_uuid item in conf file", log_header);
			assert(0);
		}
		strcpy(dxp_setup->peer_uuid, the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "dxp_type");
		if(!the_key) {
			nid_log_error("%s: no dxp_type item in conf file", log_header);
			assert(0);
		}
		dxp_type = (char *)(the_key->k_value);
		priv_p->p_type = __dxp_type_mapping(dxp_type);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "dxt_name");
		if(!the_key) {
			nid_log_error("%s: no dxt_name item in conf file", log_header);
			assert(0);
		}
		dxt_name = (char *)(the_key->k_value);
		strcpy(dxp_setup->dxt_name, dxt_name);
		dxp_setup->umpk = priv_p->p_umpk;

		dxp_setup++;
	}

	pthread_mutex_init(&priv_p->p_rs_mlck, NULL);

	return 0;
}
