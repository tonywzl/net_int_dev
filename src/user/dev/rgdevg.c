/*
 * rgdevg.c
 * 	Implementation of Regular Device Guardian  Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "list.h"
#include "rgdevg_if.h"
#include "rgdev_if.h"
#include "ini_if.h"

struct rgdevg_channel{
	struct list_head		c_list;
	void				*c_data;
};

struct rgdevg_private {
	struct ini_interface            *p_ini;
	pthread_mutex_t                 p_lck;
	struct rgdev_setup		*p_setup;
	struct list_head		p_dev_head;
	int				p_counter;
};

static void *
rgdevg_search_and_create(struct rgdevg_interface *rgdevg_p, char *dev_name)
{
	char *log_header = "rgdevg_search_and_create";
	struct rgdevg_private *priv_p = (struct rgdevg_private *)rgdevg_p->dg_private;
	struct rgdev_setup *rgdev_setup = &priv_p->p_setup[0];
	struct rgdev_interface *rgdev_p = NULL;
	struct rgdevg_channel *chan_p = NULL;
	char *target_name;
	int i, got_it = 0;


	nid_log_warning("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct rgdevg_channel, &priv_p->p_dev_head, c_list) {
		rgdev_p = (struct rgdev_interface *)chan_p->c_data;
		target_name = rgdev_p->d_op->d_get_name(rgdev_p);
		if (!strcmp(target_name, dev_name)) {
			got_it = 1;
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (got_it)
		goto out;

	/*
	 *  create a new rgdev
	 */
	for (i = 0; i < priv_p->p_counter; i++, rgdev_setup++) {
		if (!strcmp(dev_name, rgdev_setup->dev_name))
			break;
	}

	if (i == priv_p->p_counter){
		nid_log_warning("%s: did not found pre-defined matched device (%s)", log_header, dev_name);
		goto out;
	}

	rgdev_p = calloc(1, sizeof(*rgdev_p));
	rgdev_initialization(rgdev_p, rgdev_setup);
	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->c_data = rgdev_p;

	pthread_mutex_lock(&priv_p->p_lck);
	list_add_tail(&chan_p->c_list, &priv_p->p_dev_head);
	pthread_mutex_unlock(&priv_p->p_lck);

out:
	return rgdev_p;
}

struct rgdevg_operations rgdevg_op = {
	.dg_search_and_create = rgdevg_search_and_create,
};

int
rgdevg_initialization(struct rgdevg_interface *rgdevg_p, struct rgdevg_setup *setup)
{
	char *log_header = "rgdevg_initialization";
	struct rgdevg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct rgdev_setup *rgdev_setup;
	int rgdev_counter = 0;

	nid_log_warning("%s: start ...", log_header);

	rgdevg_p->dg_op = &rgdevg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rgdevg_p->dg_private = priv_p;

	priv_p->p_ini = setup->ini;
	INIT_LIST_HEAD(&priv_p->p_dev_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	ini_p= priv_p->p_ini;
	if(!ini_p) {
		nid_log_error("%s: no conf file setting", log_header);
		return 1;
	}

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (!strcmp(sect_type, "rgdev"))
			rgdev_counter++;
	}
	if (!rgdev_counter)
		return 0;

	priv_p->p_counter = rgdev_counter;
	priv_p->p_setup = calloc(rgdev_counter, sizeof(*priv_p->p_setup));
	rgdev_setup = priv_p->p_setup;

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "rgdev"))
			continue;

		strcpy(rgdev_setup->dev_name,  sc_p->s_name);
		rgdev_setup++;
	}
	return 0;
}
