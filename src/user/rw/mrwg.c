/*
 * mrwg.c
 * 	Implementation of Meta Server Read Write Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "mrw_if.h"
#include "mrwg_if.h"
#include "rw_if.h"

struct mrwg_private {
	struct allocator_interface	*p_allocator;
	struct fpn_interface		*p_fpn;
	struct ini_interface		**p_ini;
	pthread_mutex_t			p_lck;
	struct mrw_setup		p_setup[MRWG_MAX_MRW];
	struct rw_interface		*p_rw[MRWG_MAX_MRW];
	int				p_counter;
};

struct rw_interface *
mrwg_search_mrw(struct mrwg_interface *mrwg_p, char *mrw_name)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;
	struct mrw_setup *mrw_setup = &priv_p->p_setup[0];
	struct rw_interface *ret_rw = NULL;
	int i;

	if (mrw_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < MRWG_MAX_MRW; i++, mrw_setup++) {
		if (!strcmp(mrw_setup->name, mrw_name)) {
			ret_rw = priv_p->p_rw[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_rw;
}

struct rw_interface *
mrwg_search_and_create_mrw(struct mrwg_interface *mrwg_p, char *mrw_name)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;
	struct mrw_setup *mrw_setup = &priv_p->p_setup[0];
	struct rw_interface *ret_rw = NULL;
	struct rw_setup rw_setup;
	int i;

	if (mrw_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < MRWG_MAX_MRW; i++, mrw_setup++) {
		if (!strcmp(mrw_setup->name, mrw_name)) {
			if (!priv_p->p_rw[i]) {
				priv_p->p_rw[i] = x_calloc(1, sizeof(priv_p->p_rw[i]));
				memset(&rw_setup, 0, sizeof(rw_setup));
				rw_setup.type = RW_TYPE_MSERVER;
				rw_setup.fpn_p = priv_p->p_fpn;
				rw_setup.allocator = priv_p->p_allocator;
				rw_setup.name = mrw_setup->name;
				rw_initialization(priv_p->p_rw[i], &rw_setup);
			}
			ret_rw = priv_p->p_rw[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_rw;
}

static int
mrwg_add_mrw(struct mrwg_interface *mrwg_p, char *mrw_name)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;
	struct mrw_setup *mrw_setup = &priv_p->p_setup[0];
	struct rw_setup rw_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < MRWG_MAX_MRW; i++, mrw_setup++) {
		if (!strcmp(mrw_setup->name, mrw_name))
			goto out;

		if (empty_index == -1 && mrw_setup->name[0] == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
		priv_p->p_rw[empty_index] = x_calloc(1, sizeof(priv_p->p_rw[empty_index]));
		mrw_setup = &priv_p->p_setup[empty_index];
		strcpy(mrw_setup->name, mrw_name);
		memset(&rw_setup, 0, sizeof(rw_setup));
		rw_setup.type = RW_TYPE_MSERVER;
		rw_setup.fpn_p = priv_p->p_fpn;
		rw_setup.allocator = priv_p->p_allocator;
		rw_setup.name = mrw_setup->name;
		rw_initialization(priv_p->p_rw[empty_index], &rw_setup);
		priv_p->p_counter++;
		rc = 0;
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static void
mrwg_get_mrw_status(struct mrwg_interface *mrwg_p, char *mrw_name, struct mrw_stat *info)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;
	struct mrw_setup *mrw_setup = &priv_p->p_setup[0];
	struct rw_interface *rw_p;
	struct mrw_interface *mrw_p = NULL;
	int i;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < MRWG_MAX_MRW; i++, mrw_setup++) {
		if (!strcmp(mrw_setup->name, mrw_name)) {
			rw_p = priv_p->p_rw[i];
			if (rw_p) {
				mrw_p = rw_p->rw_op->rw_get_rw_obj(rw_p);
				mrw_p->rw_op->rw_get_status(mrw_p, info);
			}
			break;
		}

		if (!strcmp(mrw_setup->name, mrw_name))
			break;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static struct mrw_setup *
mrwg_get_all_mrw_setup(struct mrwg_interface *mrwg_p, int *num_mrw)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;

	*num_mrw = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

int *
mrwg_get_working_mrw_index(struct mrwg_interface *mrwg_p, int *num_mrw)
{
	struct mrwg_private *priv_p = (struct mrwg_private *)mrwg_p->rwg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));	
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < MRWG_MAX_MRW; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_rw[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_mrw = count;

	return ret;
}

struct mrwg_operations mrwg_op = {
	.rwg_search_mrw = mrwg_search_mrw,
	.rwg_search_and_create_mrw = mrwg_search_and_create_mrw,
	.rwg_add_mrw = mrwg_add_mrw,
	.rwg_get_mrw_status = mrwg_get_mrw_status,
	.rwg_get_all_mrw_setup = mrwg_get_all_mrw_setup,
	.rwg_get_working_mrw_index = mrwg_get_working_mrw_index,
};

int
mrwg_initialization(struct mrwg_interface *mrwg_p, struct mrwg_setup *setup)
{
	char *log_header = "mrwg_initialization";
	struct mrwg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct mrw_setup *mrw_setup;
	char *sect_type;
	int mrw_counter = 0;

	nid_log_warning("%s: start ...", log_header);

	mrwg_p->rwg_op = &mrwg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mrwg_p->rwg_private = priv_p;

	priv_p->p_allocator= setup->allocator;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_ini = setup->ini;
	ini_p = *(priv_p->p_ini);
	mrw_setup = &priv_p->p_setup[0];

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "mrw"))
			continue;

		memset(mrw_setup, 0, sizeof(*mrw_setup));
		strcpy(mrw_setup->name, sc_p->s_name);

		mrw_setup++;
		mrw_counter++;
	}
	priv_p->p_counter = mrw_counter;

	return 0;
}
