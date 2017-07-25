/*
 * drwg.c
 * 	Implementation of Device Read Write Guardian Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "ini_if.h"
#include "drw_if.h"
#include "drwg_if.h"
#include "rw_if.h"

struct drwg_private {
	struct allocator_interface	*p_allocator;
	struct fpn_interface		*p_fpn;
	struct ini_interface		**p_ini;
	pthread_mutex_t			p_lck;
	struct drw_setup		p_setup[DRWG_MAX_DRW];
	struct rw_interface		*p_rw[DRWG_MAX_DRW];
	int				p_counter;
};

struct rw_interface *
drwg_search_drw(struct drwg_interface *drwg_p, char *drw_name)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	struct drw_setup *drw_setup = &priv_p->p_setup[0];
	struct rw_interface *ret_rw = NULL;
	int i;

	if (drw_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup++) {
		if (!strcmp(drw_setup->name, drw_name)) {
			ret_rw = priv_p->p_rw[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_rw;
}

struct rw_interface *
drwg_search_and_create_drw(struct drwg_interface *drwg_p, char *drw_name)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	struct drw_setup *drw_setup = &priv_p->p_setup[0];
	struct rw_interface *ret_rw = NULL;
	struct rw_setup rw_setup;
	int i;

	if (drw_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup++) {
		if (!strcmp(drw_setup->name, drw_name)) {
			if (!priv_p->p_rw[i]) {
				priv_p->p_rw[i] = x_calloc(1, sizeof(*priv_p->p_rw[i]));
				memset(&rw_setup, 0, sizeof(rw_setup));
				rw_setup.type = RW_TYPE_DEVICE;
				rw_setup.fpn_p = priv_p->p_fpn;
				rw_setup.allocator = priv_p->p_allocator;
				rw_setup.name = drw_setup->name;
				rw_setup.exportname = drw_setup->exportname;
				rw_setup.device_provision = drw_setup->device_provision;
				rw_setup.simulate_async = drw_setup->simulate_async;
				rw_setup.simulate_delay = drw_setup->simulate_delay;
				rw_setup.simulate_delay_max_gap = drw_setup->simulate_delay_max_gap;
				rw_setup.simulate_delay_min_gap = drw_setup->simulate_delay_min_gap;
				rw_setup.simulate_delay_time_us = drw_setup->simulate_delay_time_us;
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
drwg_add_drw(struct drwg_interface *drwg_p, char *drw_name, char *exportname, int device_provision)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	struct drw_setup *drw_setup = &priv_p->p_setup[0];
	struct rw_setup rw_setup;
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup++) {
		if (!strcmp(drw_setup->name, drw_name))
			goto out;

		if (empty_index == -1 && drw_setup->name[0] == '\0')
			empty_index = i;
	}

	if (empty_index != -1) {
		priv_p->p_rw[empty_index] = x_calloc(1, sizeof(priv_p->p_rw[empty_index]));
		drw_setup = &priv_p->p_setup[empty_index];
		strcpy(drw_setup->name, drw_name);
		strcpy(drw_setup->exportname, exportname);
		drw_setup->device_provision = device_provision;
		memset(&rw_setup, 0, sizeof(rw_setup));
		rw_setup.type = RW_TYPE_DEVICE;
		rw_setup.fpn_p = priv_p->p_fpn;
		rw_setup.allocator = priv_p->p_allocator;
		rw_setup.name = drw_setup->name;
		rw_setup.exportname = drw_setup->exportname;
		rw_setup.device_provision = drw_setup->device_provision;
		rw_initialization(priv_p->p_rw[empty_index], &rw_setup);
		priv_p->p_counter++;
		rc = 0;
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static int
drwg_delete_drw(struct drwg_interface *drwg_p, char *drw_name)
{
	char *log_header = "drwg_delete_drw";
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	struct drw_setup *drw_setup = &priv_p->p_setup[0];
	struct rw_interface *rw_p = NULL;
	int i, got_setup = 0;

	nid_log_warning("%s: start (name:%s) ...", log_header, drw_name);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup++) {
		if (!strcmp(drw_setup->name, drw_name)) {
			got_setup = 1;
			drw_setup->name[0] = '\0';
			assert(priv_p->p_counter);
			priv_p->p_counter--;

			rw_p = priv_p->p_rw[i];
			priv_p->p_rw[i] = NULL;
			
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (rw_p)
		rw_p->rw_op->rw_destroy(rw_p);
	else if (got_setup)
		nid_log_warning("%s: drw does not exist, setup removed (name:%s)", log_header, drw_name);
	else
		nid_log_warning("%s: no drw matched (name:%s)", log_header, drw_name);

	nid_log_warning("%s: end (name:%s) ...", log_header, drw_name);
	return 0;
}



static int
drwg_do_device_ready(struct drwg_interface *drwg_p, char *drw_name)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	struct drw_setup *drw_setup = &priv_p->p_setup[0];
	struct rw_interface *rw_p = NULL;
	struct drw_interface *drw_p;
	int i, rc = 1;

	if (drw_name[0] == '\0')
		return 1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++, drw_setup++) {
		if (!strcmp(drw_setup->name, drw_name)) {
			rw_p = priv_p->p_rw[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (rw_p) {
		drw_p = rw_p->rw_op->rw_get_rw_obj(rw_p);
		drw_p->rw_op->rw_do_device_ready(drw_p);
		rc = 0;
	}

	return rc;
}

static struct drw_setup *
drwg_get_all_drw_setup(struct drwg_interface *drwg_p, int *num_drw)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;

	*num_drw = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

int *
drwg_get_working_drw_index(struct drwg_interface *drwg_p, int *num_drw)
{
	struct drwg_private *priv_p = (struct drwg_private *)drwg_p->rwg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));	
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < DRWG_MAX_DRW; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_rw[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_drw = count;

	return ret;
}

struct drwg_operations drwg_op = {
	.rwg_search_drw = drwg_search_drw,
	.rwg_search_and_create_drw = drwg_search_and_create_drw,
	.rwg_add_drw = drwg_add_drw,
	.rwg_delete_drw = drwg_delete_drw,
	.rwg_do_device_ready = drwg_do_device_ready,
	.rwg_get_all_drw_setup = drwg_get_all_drw_setup,
	.rwg_get_working_drw_index = drwg_get_working_drw_index,
};

int
drwg_initialization(struct drwg_interface *drwg_p, struct drwg_setup *setup)
{
	char *log_header = "drwg_initialization";
	struct drwg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct drw_setup *drw_setup;
	char *sect_type;
	int drw_counter = 0;

	nid_log_warning("%s: start ...", log_header);

	drwg_p->rwg_op = &drwg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	drwg_p->rwg_private = priv_p;

	priv_p->p_allocator= setup->allocator;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_ini = setup->ini;
	ini_p = *(priv_p->p_ini);
	drw_setup = &priv_p->p_setup[0];

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "drw"))
			continue;

		memset(drw_setup, 0, sizeof(*drw_setup));
		strcpy(drw_setup->name, sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "exportname");
		strcpy(drw_setup->exportname, (char *)(the_key->k_value));
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "device_provision");
		drw_setup->device_provision = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "simulate_async");
		drw_setup->simulate_async = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "simulate_delay");
		drw_setup->simulate_delay = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "simulate_delay_max_gap");
		drw_setup->simulate_delay_max_gap = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "simulate_delay_min_gap");
		drw_setup->simulate_delay_min_gap = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "simulate_delay_time_us");
		drw_setup->simulate_delay_time_us = *(int *)(the_key->k_value);

		drw_setup++;
		drw_counter++;
	}
	priv_p->p_counter = drw_counter;

	return 0;
}
