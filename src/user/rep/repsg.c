/*
 * repsg.c
 *	Imlementation of Replication Source Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "list.h"
#include "lstn_if.h"
#include "reps_if.h"
#include "repsg_if.h"
#include "dxag_if.h"
#include "tpg_if.h"

struct repsg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct repsg_private {
	struct ini_interface		*p_ini;
	struct tpg_interface		*p_tpg;
	struct allocator_interface	*p_allocator;
	struct umpk_interface		*p_umpk;
	struct dxag_interface		*p_dxag;
	struct reps_setup		*p_setup;
	pthread_mutex_t			p_lck;
	int				p_counter;
	struct list_head		p_rs_head;
};

static struct reps_interface *
repsg_search_rs(struct repsg_interface *rsg_p, char *rs_name)
{
	char *log_header = "rsg_search_rs";
	struct repsg_private *priv_p = (struct repsg_private *)rsg_p->rsg_private;
	struct reps_interface *ret_rs = NULL;
	struct repsg_channel *chan_p = NULL;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct repsg_channel, &priv_p->p_rs_head, c_list) {
		ret_rs = (struct reps_interface *)chan_p->c_data;
		target_name = ret_rs->rs_op->rs_get_name(ret_rs);
		if (!strcmp(target_name, rs_name)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (!got_it) {
		ret_rs = NULL;
	}

	return ret_rs;
}

static struct reps_interface *
repsg_search_and_create_rs(struct repsg_interface *rsg_p, char *rs_name)
{
	char *log_header = "rsg_search_and_create_rs";
	struct repsg_private *priv_p = (struct repsg_private *)rsg_p->rsg_private;
	struct reps_setup *rs_setup = &priv_p->p_setup[0];
	struct reps_interface *rs_p = NULL;
	struct repsg_channel *chan_p = NULL;
	int i, got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct repsg_channel, &priv_p->p_rs_head, c_list) {
		rs_p = (struct reps_interface *)chan_p->c_data;
		target_name = rs_p->rs_op->rs_get_name(rs_p);
		if(!strcmp(target_name, rs_name)){
			got_it = 1;
			break;
		}

	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if(got_it)
		goto out;

	rs_p = NULL;

	for (i = 0; i < priv_p->p_counter; i++, rs_setup++) {
		if (!strcmp(rs_name, rs_setup->rs_name))
			break;
	}
	if (i == priv_p->p_counter)
		goto out;

	/*
	 * create a new rs
	 */
	rs_p = calloc(1, sizeof(*rs_p));
	reps_initialization(rs_p, rs_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = rs_p;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&chan_p->c_list, &priv_p->p_rs_head);
	pthread_mutex_unlock(&priv_p->p_lck);

out:
	return rs_p;
}

struct reps_setup *
repsg_get_all_rs_setup(struct repsg_interface *rsg_p, int *num_rs)
{
	struct repsg_private *priv_p = (struct repsg_private *)rsg_p->rsg_private;

	*num_rs = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

char **
repsg_get_working_rs_name(struct repsg_interface *rsg_p, int *num_rs)
{
	struct repsg_private *priv_p = (struct repsg_private *)rsg_p->rsg_private;
	struct repsg_channel *chan_p;
	struct reps_interface *rs_p;
	char **ret;
	int i = 0;

	ret = calloc(priv_p->p_counter, sizeof(*ret));
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct repsg_channel, &priv_p->p_rs_head, c_list) {
		rs_p = (struct reps_interface *)chan_p->c_data;
		ret[i] = rs_p->rs_op->rs_get_name(rs_p);
		i++;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_rs = i;

	return ret;
}

struct repsg_operations repsg_op = {
	.rsg_search_and_create_rs = repsg_search_and_create_rs,
	.rsg_search_rs = repsg_search_rs,
	.rsg_get_all_rs_setup = repsg_get_all_rs_setup,
	.rsg_get_working_rs_name = repsg_get_working_rs_name,
};

int
repsg_initialization(struct repsg_interface *repsg_p, struct repsg_setup *setup)
{
	char *log_header = "repsg_initialization";
	struct repsg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct dxag_interface *dxag_p;
	struct tpg_interface *tpg_p;
	char *sect_type;
	struct reps_setup *rs_setup;
	int rs_counter = 0;

	nid_log_warning("%s: start ...", log_header);

	if (!setup->ini)
		return 0;	// this is jsut a ut

	repsg_p->rsg_op = &repsg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	repsg_p->rsg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_tpg = setup->tpg;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_dxag = setup->dxag;
	INIT_LIST_HEAD(&priv_p->p_rs_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	ini_p = priv_p->p_ini;
	dxag_p = priv_p->p_dxag;
	tpg_p = priv_p->p_tpg;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "reps"))
			rs_counter++;
	}

	if (!rs_counter)
		return 0;
	priv_p->p_counter = rs_counter;
	priv_p->p_setup = calloc(rs_counter, sizeof(*priv_p->p_setup));
	rs_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "reps"))
			continue;

		strcpy(rs_setup->rs_name,  sc_p->s_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "rept_name");
		assert(the_key);
		strcpy(rs_setup->rt_name, (char *)(the_key->k_value));
		assert(rs_setup->rt_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "dxa_name");
		assert(the_key);
		strcpy(rs_setup->dxa_name, (char *)(the_key->k_value));
		rs_setup->dxag_p = dxag_p;

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "voluuid");
		assert(the_key);
		strcpy(rs_setup->voluuid, (char *)(the_key->k_value));

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "vol_len");
		rs_setup->vol_len = *(size_t *)(the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "tp_name");
		assert(the_key);
		rs_setup->tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, (char *)(the_key->k_value));

		rs_setup->allocator = priv_p->p_allocator;
		rs_setup->umpk = priv_p->p_umpk;

		rs_setup++;
	}
	return 0;
}
