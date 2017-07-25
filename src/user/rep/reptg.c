/*
 * reptg.c
 *	Imlementation of Replication Target Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "list.h"
#include "lstn_if.h"
#include "rept_if.h"
#include "reptg_if.h"
#include "dxpg_if.h"
#include "tpg_if.h"

struct reptg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct reptg_private {
	struct ini_interface		*p_ini;
	struct tpg_interface		*p_tpg;
	struct allocator_interface	*p_allocator;
	struct dxpg_interface           *p_dxpg;
	struct rept_setup		*p_setup;
	pthread_mutex_t			p_lck;
	int				p_counter;
	struct list_head		p_rt_head;
};

static struct rept_interface *
reptg_search_rt(struct reptg_interface *rtg_p, char *rt_name)
{
	char *log_header = "rtg_search_rt";
	struct reptg_private *priv_p = (struct reptg_private *)rtg_p->rtg_private;
	struct rept_interface *ret_rt = NULL;
	struct reptg_channel *chan_p = NULL;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct reptg_channel, &priv_p->p_rt_head, c_list) {
		ret_rt = (struct rept_interface *)chan_p->c_data;
		target_name = ret_rt->rt_op->rt_get_name(ret_rt);
		if (!strcmp(target_name, rt_name)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (!got_it) {
		ret_rt = NULL;
	}

	return ret_rt;
}

static struct rept_interface *
reptg_search_and_create_rt(struct reptg_interface *rtg_p, char *rt_name)
{
	char *log_header = "rtg_search_and_create_rt";
	struct reptg_private *priv_p = (struct reptg_private *)rtg_p->rtg_private;
	struct rept_setup *rt_setup = &priv_p->p_setup[0];
	struct rept_interface *rt_p = NULL;
	struct reptg_channel *chan_p = NULL;
	int i, got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct reptg_channel, &priv_p->p_rt_head, c_list) {
		rt_p = (struct rept_interface *)chan_p->c_data;
		target_name = rt_p->rt_op->rt_get_name(rt_p);
		if(!strcmp(target_name, rt_name)){
			got_it = 1;
			break;
		}

	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if(got_it)
		goto out;

	rt_p = NULL;

	for (i = 0; i < priv_p->p_counter; i++, rt_setup++) {
		if (!strcmp(rt_name, rt_setup->rt_name))
			break;
	}
	if (i == priv_p->p_counter)
		goto out;

	/*
	 * create a new rt
	 */
	rt_p = calloc(1, sizeof(*rt_p));
	rept_initialization(rt_p, rt_setup);

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = rt_p;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&chan_p->c_list, &priv_p->p_rt_head);
	pthread_mutex_unlock(&priv_p->p_lck);

out:
	return rt_p;
}

struct rept_setup *
reptg_get_all_rt_setup(struct reptg_interface *rtg_p, int *num_rt)
{
	struct reptg_private *priv_p = (struct reptg_private *)rtg_p->rtg_private;

	*num_rt = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

char **
reptg_get_working_rt_name(struct reptg_interface *rtg_p, int *num_rt)
{
	struct reptg_private *priv_p = (struct reptg_private *)rtg_p->rtg_private;
	struct reptg_channel *chan_p;
	struct rept_interface *rt_p;
	char **ret;
	int i = 0;

	ret = calloc(priv_p->p_counter, sizeof(*ret));
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct reptg_channel, &priv_p->p_rt_head, c_list) {
		rt_p = (struct rept_interface *)chan_p->c_data;
		ret[i] = rt_p->rt_op->rt_get_name(rt_p);
		i++;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_rt = i;

	return ret;
}

struct reptg_operations reptg_op = {
	.rtg_search_and_create_rt = reptg_search_and_create_rt,
	.rtg_search_rt = reptg_search_rt,
	.rtg_get_all_rt_setup = reptg_get_all_rt_setup,
	.rtg_get_working_rt_name = reptg_get_working_rt_name,
};

int
reptg_initialization(struct reptg_interface *reptg_p, struct reptg_setup *setup)
{
	char *log_header = "reptg_initialization";
	struct reptg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct dxpg_interface *dxpg_p;
	struct tpg_interface *tpg_p;
	char *sect_type;
	struct rept_setup *rt_setup;
	int rt_counter = 0;

	nid_log_warning("%s: start ...", log_header);

	if (!setup->ini)
		return 0;	// this is jsut a ut

	reptg_p->rtg_op = &reptg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	reptg_p->rtg_private = priv_p;

	priv_p->p_ini = setup->ini;
	priv_p->p_tpg = setup->tpg;
	priv_p->p_allocator = setup->allocator;
	priv_p->p_dxpg = setup->dxpg;
	INIT_LIST_HEAD(&priv_p->p_rt_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	ini_p = priv_p->p_ini;
	dxpg_p = priv_p->p_dxpg;
	tpg_p = priv_p->p_tpg;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "rept"))
			rt_counter++;
	}

	if (!rt_counter)
		return 0;
	priv_p->p_counter = rt_counter;
	priv_p->p_setup = calloc(rt_counter, sizeof(*priv_p->p_setup));
	rt_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "rept"))
			continue;

		strcpy(rt_setup->rt_name,  sc_p->s_name);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "dxp_name");
		assert(the_key);
		strcpy(rt_setup->dxp_name, (char *)(the_key->k_value));
		rt_setup->dxpg_p = dxpg_p;

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "voluuid");
		assert(the_key);
		strcpy(rt_setup->voluuid, (char *)(the_key->k_value));

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "vol_len");
		rt_setup->vol_len = *(size_t *)(the_key->k_value);

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "tp_name");
		assert(the_key);
		rt_setup->tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, (char *)(the_key->k_value));

		rt_setup->allocator = priv_p->p_allocator;

		rt_setup++;
	}
	return 0;
}
