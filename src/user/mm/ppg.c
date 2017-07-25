/*
 * ppg.c
 * 	Implementation of  ppg Module
 */

#include <stdlib.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "pp_if.h"
#include "ppg_if.h"

struct ppg_private {
	struct allocator_interface	*p_allocator;
	struct ini_interface		*p_ini;
	pthread_mutex_t			p_lck;
	struct pp_setup			p_setup[PPG_MAX_PP];
	struct pp_interface		*p_pp[PPG_MAX_PP];
	int				p_counter;
};

struct pp_interface *
ppg_search_pp(struct ppg_interface *ppg_p, char *pp_name)
{
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;
	struct pp_setup *pp_setup = &priv_p->p_setup[0];
	struct pp_interface *ret_pp = NULL;
	int i;

	if (pp_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < PPG_MAX_PP; i++, pp_setup++) {
		if (!strcmp(pp_setup->pp_name, pp_name)) {
			ret_pp = priv_p->p_pp[i];
			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_pp;
}

/*
 * Used for initialization from configuration.
 */
struct pp_interface *
ppg_search_and_create_pp(struct ppg_interface *ppg_p, char *pp_name, int set_id)
{
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;
	struct pp_setup *pp_setup = &priv_p->p_setup[0];
	struct pp_interface *ret_pp = NULL;
	int i;


	if (pp_name[0] == '\0')
		return NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < PPG_MAX_PP; i++, pp_setup++) {
		if (!strcmp(pp_setup->pp_name, pp_name)) {
			/* got a matched pp_setup */
			if (!priv_p->p_pp[i]) {
				priv_p->p_pp[i] = x_calloc(1, sizeof(*priv_p->p_pp[i]));
				pp_setup->set_id = set_id;
				pp_initialization(priv_p->p_pp[i], pp_setup);
			}	
			ret_pp = priv_p->p_pp[i];
			break;	
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	return ret_pp;
}

/*
 * Used for online adding.
 */
static int
ppg_add_pp(struct ppg_interface *ppg_p, char *pp_name, int set_id,
		uint32_t pool_size, uint32_t page_size)
{
	char *log_header = "ppg_add_pp";
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;
	struct pp_setup *pp_setup = &priv_p->p_setup[0];
	int i, rc = 1, empty_index = -1;

	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < PPG_MAX_PP; i++, pp_setup++) {
		if (!strcmp(pp_setup->pp_name, pp_name)) {
			nid_log_warning("%s: pp (%s) already exists", log_header, pp_name);
			goto out;
		}
		if (empty_index == -1 && pp_setup->pp_name[0] == '\0') {
			empty_index = i;
			nid_log_warning("%s: index %d is assgined to pp (%s)", log_header, empty_index, pp_name);
		}
	}

	if (empty_index != -1) {
		priv_p->p_pp[empty_index] = x_calloc(1, sizeof(*priv_p->p_pp[empty_index]));
		pp_setup = &priv_p->p_setup[empty_index];
		strcpy(pp_setup->pp_name, pp_name);
		pp_setup->allocator = priv_p->p_allocator;
		pp_setup->set_id = set_id;
		pp_setup->pool_size = pool_size;
		pp_setup->page_size = page_size;
		pp_initialization(priv_p->p_pp[empty_index], pp_setup);
		priv_p->p_counter++;
		rc = 0;
	} else {
		nid_log_warning("%s: pp number reaches the limitation", log_header);
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);

	return rc;
}

static int
ppg_delete_pp(struct ppg_interface *ppg_p, char *pp_name)
{
	char *log_header = "ppg_delete_pp";
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;
	struct pp_setup *pp_setup = &priv_p->p_setup[0];
	struct pp_interface *pp_p = NULL;
	int i, got_setup = 0;

	nid_log_warning("%s: start (name:%s) ...", log_header, pp_name);
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < PPG_MAX_PP; i++, pp_setup++) {
		if (!strcmp(pp_setup->pp_name, pp_name)) {
			got_setup = 1;
			pp_setup->pp_name[0] = '\0';
			assert(priv_p->p_counter);
			priv_p->p_counter--;

			pp_p = priv_p->p_pp[i];
			priv_p->p_pp[i] = NULL;

			break;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	if (pp_p)
		pp_p->pp_op->pp_destroy(pp_p);
	else if (got_setup)
		nid_log_warning("%s: pp does not exist, setup removed (name:%s)", log_header, pp_name);
	else
		nid_log_warning("%s: no pp matched (name:%s)", log_header, pp_name);

	nid_log_warning("%s: end (name:%s) ...", log_header, pp_name);
	return 0;
}

struct pp_interface **
ppg_get_all_pp(struct ppg_interface *ppg_p, int *len)
{
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;

	*len = priv_p->p_counter;
	return priv_p->p_pp;
}

static struct pp_setup *
ppg_get_all_pp_setup(struct ppg_interface *ppg_p, int *num_pp)
{
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;

	*num_pp = priv_p->p_counter;
	return &priv_p->p_setup[0];
}

static int *
ppg_get_working_pp_index(struct ppg_interface *ppg_p, int *num_pp)
{
	struct ppg_private *priv_p = (struct ppg_private *)ppg_p->pg_private;
	int *ret;
	int count = 0, i;

	ret = x_calloc(priv_p->p_counter, sizeof(int));
	pthread_mutex_lock(&priv_p->p_lck);
	for (i = 0; i < PPG_MAX_PP; i++) {
		if (count == priv_p->p_counter )
			break;
		if (priv_p->p_pp[i]) {
			ret[count] = i;
			count++;
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	*num_pp = count;

	return ret;
}

struct ppg_operations ppg_op = {
	.pg_search_pp = ppg_search_pp,
	.pg_search_and_create_pp = ppg_search_and_create_pp,
	.pg_add_pp = ppg_add_pp,
	.pg_delete_pp = ppg_delete_pp,
	.pg_get_all_pp = ppg_get_all_pp,
	.pg_get_all_pp_setup = ppg_get_all_pp_setup,
	.pg_get_working_pp_index = ppg_get_working_pp_index,
};

int
ppg_initialization(struct ppg_interface *ppg_p, struct ppg_setup *setup)
{
	char *log_header = "ppg_initialization";
	struct ppg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *sect_type;
	struct pp_setup *pp_setup;
	int pp_counter = 0;

	nid_log_info("%s: start ...", log_header);
	ppg_p->pg_op = &ppg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	ppg_p->pg_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_ini = setup->ini;
	ini_p = priv_p->p_ini;
	pp_setup = &priv_p->p_setup[0];

	pthread_mutex_init(&priv_p->p_lck, NULL);

	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "pp"))
			continue;

		memset(pp_setup, 0, sizeof(*pp_setup));
		strcpy(pp_setup->pp_name, sc_p->s_name);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "page_size");
		pp_setup->page_size = *(int *)(the_key->k_value);
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "pool_size");
		pp_setup->pool_size = *(int *)(the_key->k_value);
		pp_setup->allocator = priv_p->p_allocator;

		pp_setup++;
		pp_counter++;
		assert(pp_counter <= PPG_MAX_PP);
	}
	priv_p->p_counter = pp_counter;
	return 0;
}
