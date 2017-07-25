/*
 * tpg.c
 * 	Implementation of Thread Pool Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid.h"
#include "ini_if.h"
#include "list.h"
#include "tp_if.h"
#include "tpg_if.h"

struct tpg_channel{
	struct list_head	c_list;
	void			*c_data;
};

struct tpg_private {
	struct ini_interface		*p_ini;
	struct tp_setup			*p_setup;
	pthread_mutex_t			p_lck;
	int				p_counter;
	struct list_head		p_tp_head;
	struct pp2_interface 		*p_pp2;
};

static struct tp_interface *
_tpg_create_channel(struct tpg_interface *tpg_p, char *tp_name)
{

	char *log_header = "_tpg_create_channel";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_setup *tp_setup = &priv_p->p_setup[0];
	struct tp_interface *tp_p = NULL;
	struct tpg_channel *chan_p = NULL;
	int i;

	nid_log_warning("%s: start ...", log_header);
	for (i = 0; i < priv_p->p_counter; i++, tp_setup++) {
		if (!strcmp(tp_name, tp_setup->name))
			break;
	}
	if (i == priv_p->p_counter) {
		nid_log_error("%s: tp (%s) does not exist in configure file",
				log_header, tp_name);
		goto out;
	}

	/*
	 * create a new tp
	 */

	tp_p = x_calloc(1, sizeof(*tp_p));
	tp_initialization(tp_p, tp_setup);
	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = tp_p;
	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&chan_p->c_list, &priv_p->p_tp_head);
	pthread_mutex_unlock(&priv_p->p_lck);

out:
	return tp_p;
}


static struct tp_interface *
tpg_search_tp(struct tpg_interface *tpg_p, char *tp_name)
{
	char *log_header = "tpg_search_tp";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *ret_tp = NULL;
	struct tpg_channel *chan_p = NULL;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
		ret_tp = (struct tp_interface *)chan_p->c_data;
		target_name = ret_tp->t_op->t_get_name(ret_tp);
		if(!strcmp(target_name, tp_name)){
			got_it = 1;
			break;
		}

	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if(!got_it)
		ret_tp = NULL;

	return ret_tp;

}

static struct tp_interface **
tpg_get_all_tp(struct tpg_interface *tpg_p, int *tp_num)
{
	char *log_header = "tpg_get_all_tp";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *ret_tp = NULL;
	struct tp_interface **tps;
	struct tpg_channel *chan_p = NULL;
	int i = 0;

	nid_log_warning("%s: start ...", log_header);
	tps = x_calloc(priv_p->p_counter, sizeof(struct tp_interface *));

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
		ret_tp = (struct tp_interface *)chan_p->c_data;
		tps[i] = ret_tp;
		nid_log_warning("%s: %s", log_header, ret_tp->t_op->t_get_name(ret_tp));
		i++;
	}
	*tp_num = i;
	pthread_mutex_unlock(&priv_p->p_lck);
	

	return tps;

}

static struct tp_interface *
tpg_search_and_create_tp(struct tpg_interface *tpg_p, char *tp_name)
{
	char *log_header = "tpg_search_and_create_tp";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *ret_tp = NULL;
	struct tpg_channel *chan_p = NULL;
	int got_it = 0;
	char *target_name;

	nid_log_warning("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_lck);

	list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
		ret_tp = (struct tp_interface *)chan_p->c_data;
		target_name = ret_tp->t_op->t_get_name(ret_tp);
		if(!strcmp(target_name, tp_name)){
			got_it = 1;
			nid_log_debug("%s: found matched tp_name:%s", log_header, tp_name);
			break;
		}

	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if( got_it)
		goto out;

	ret_tp = _tpg_create_channel(tpg_p, tp_name);

out:
	return ret_tp;
}

static int
tpg_add_tp(struct tpg_interface *tpg_p, char *tp_name, int num_workers)
{
	char *log_header = "tpg_add_tp";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *tp_p;
	struct tp_setup *new_setup_p, *tp_setup;
	int rc = -1, i, new_index;

	nid_log_warning("%s: start ...", log_header);

	tp_setup = priv_p->p_setup;
	for (i = 0; i < priv_p->p_counter; i++, tp_setup++) {
		if (!strcmp(tp_name, tp_setup->name)) {
			nid_log_warning("%s: tp %s alread exist", log_header, tp_name);
			goto out;
		}
	}

	pthread_mutex_lock(&priv_p->p_lck);
	new_index = priv_p->p_counter;
	new_setup_p  = realloc(priv_p->p_setup, (priv_p->p_counter + 1) * sizeof(*priv_p->p_setup));
	if(new_setup_p == NULL)
		goto out;
	priv_p->p_setup = new_setup_p;

	new_setup_p[new_index].pp2 = priv_p->p_pp2;
	strcpy(new_setup_p[new_index].name, tp_name);
	new_setup_p[new_index].max_workers = num_workers;
	new_setup_p[new_index].min_workers = num_workers;
	new_setup_p[new_index].extend = num_workers;
	new_setup_p[new_index].delay = 0;

	priv_p->p_counter++;
	pthread_mutex_unlock(&priv_p->p_lck);

	tp_p = _tpg_create_channel(tpg_p, tp_name);

	if (tp_p != NULL)
		rc = 0;

out:
	return rc;
}

static int
tpg_delete_tp(struct tpg_interface *tpg_p, char *tp_name)
{
	char *log_header = "tpg_delete_tp";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_setup *tp_setup = priv_p->p_setup;
	struct tp_interface *tp_p, *got_tp = NULL;
	struct tpg_channel *chan_p;
	int i, got_setup = 0;

	nid_log_warning("%s: start (name:%s) ...", log_header, tp_name);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < priv_p->p_counter; i++, tp_setup++) {
		if (!strcmp(tp_setup->name, tp_name)) {
			got_setup = 1;
			if (i != priv_p->p_counter - 1)
				memmove(tp_setup, tp_setup + 1, sizeof(*tp_setup) * (priv_p->p_counter - i - 1));
			assert(priv_p->p_counter);
			priv_p->p_counter--;

			list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
				tp_p = (struct tp_interface *)chan_p->c_data;
				if(!strcmp(tp_p->t_op->t_get_name(tp_p), tp_name)){
					got_tp = tp_p;
					list_del(&chan_p->c_list);
					break;
				}

			}
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (got_tp)
		got_tp->t_op->t_destroy(tp_p);
	else if(got_setup)
		nid_log_warning("%s: tp does not exist, setup removed (name:%s)", log_header, tp_name);
	else
		nid_log_warning("%s: no tp matched (name:%s)", log_header, tp_name);

	nid_log_warning("%s: end (name:%s) ...", log_header, tp_name);
	return 0;
}

static struct tp_setup *
tpg_get_all_tp_setup(struct tpg_interface *tpg_p, int *num_tp)
{
	char *log_header = "tpg_get_all_tp_setup";
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;

	nid_log_warning("%s: start ...", log_header);
	*num_tp = priv_p->p_counter;
	return priv_p->p_setup;
}

static char **
tpg_get_working_tp_name(struct tpg_interface *tpg_p, int *num_tp)
{
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *tp_p;
	struct tpg_channel *chan_p;
	char **ret;
	int i = 0;

	ret = x_calloc(priv_p->p_counter, sizeof(*ret));
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
		tp_p = (struct tp_interface *)chan_p->c_data;
		ret[i] = tp_p->t_op->t_get_name(tp_p);
		i++;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_tp = i;

	return ret;
}

static void
tpg_cleanup_tp(struct tpg_interface *tpg_p)
{
	struct tpg_private *priv_p = (struct tpg_private *)tpg_p->tg_private;
	struct tp_interface *tp_p;
	struct tpg_channel *chan_p;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct tpg_channel, &priv_p->p_tp_head, c_list) {
		tp_p = (struct tp_interface *)chan_p->c_data;
		tp_p->t_op->t_destroy(tp_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

struct tpg_operations tpg_op = {
	.tpg_search_tp = tpg_search_tp,
	.tpg_search_and_create_tp = tpg_search_and_create_tp,
	.tpg_add_tp = tpg_add_tp,
	.tpg_delete_tp = tpg_delete_tp,
	.tpg_get_all_tp = tpg_get_all_tp,
	.tpg_get_all_tp_setup = tpg_get_all_tp_setup,
	.tpg_get_working_tp_name = tpg_get_working_tp_name,
	.tpg_cleanup_tp = tpg_cleanup_tp,
};

int
tpg_initialization(struct tpg_interface *tpg_p, struct tpg_setup *setup)
{
	char *log_header = "tpg_initialization";
	struct tpg_private *priv_p;
	struct tp_setup *tp_setup;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	char *global_tp_name = NID_GLOBAL_TP;
	char *nw_tp_name = NID_NW_TP;
	int tp_counter = 2;


	nid_log_info("%s: start ...", log_header);
	assert(setup);

	tpg_p->tg_op = &tpg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	tpg_p->tg_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_tp_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);

	priv_p->p_pp2 = setup->pp2;

	priv_p->p_ini = setup->ini;
	ini_p = priv_p->p_ini;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "tp"))
			tp_counter++;
	}

	priv_p->p_counter = tp_counter;
	nid_log_warning("%s: %d tp founded", log_header, tp_counter);
	priv_p->p_setup = x_calloc(tp_counter, sizeof(*priv_p->p_setup));
	tp_setup = priv_p->p_setup;
	strcpy(tp_setup->name, global_tp_name);
	tp_setup->max_workers = 0;
	tp_setup->min_workers = 10;
	tp_setup->extend = 2;
	tp_setup->delay = 0;
	tp_setup->pp2 = setup->pp2;
	tp_setup++;

	strcpy(tp_setup->name, nw_tp_name);
	tp_setup->max_workers = 4;
	tp_setup->min_workers = 4;
	tp_setup->extend = 4;
	tp_setup->delay = 0;
	tp_setup->pp2 = setup->pp2;
	tp_setup++;

	if (tp_counter == 1)
		return 0;
	ini_p->i_op->i_rollback(ini_p);

	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "tp")){
			continue;
		}

		memset(tp_setup, 0, sizeof(*tp_setup));
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "num_workers");
		tp_setup->max_workers = *(int *)(the_key->k_value);
		tp_setup->min_workers = tp_setup->max_workers;
		tp_setup->extend = tp_setup->max_workers;

		strcpy(tp_setup->name, sc_p->s_name);
		tp_setup->pp2 = setup->pp2;
		tp_setup++;
	}


	return 0;
}
