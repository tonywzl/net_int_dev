#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <glib.h>
#include <ctype.h>

#include "nid_log.h"
#include "lstn_if.h"
#include "ini_if.h"

struct ini_private {
	struct ini_key_desc	*p_key_desc;
	char			p_pathname[1024];
	struct list_head	p_key_set;
	struct list_head	p_section_head;		// list of all sections
	struct list_head	*p_next_pos;
};

void
__str_trim(char *str)
{
	char *tail, *head;
	if (str == NULL) return;
	for (tail = str + strlen( str ) - 1; tail >= str; tail --) {
		if (!isspace(*tail)) break;
	}
	tail[1] = '\0';
	for (head = str; head <= tail; head ++) {
		if (!isspace(*head)) break;
	}
	if (head != str) {
		memmove(str, head,(tail - head + 2) * sizeof(char));
	}
}

static int
ini_parse(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	char *fpath = priv_p->p_pathname;
	struct ini_section_content *the_sc;
	GKeyFile *gkey;
	GError *err = NULL;
	gchar **groups, **the_group;
	struct ini_key_desc *the_key;
	struct ini_key_content *the_kc;
	int rc = 0;
	gint ival;
	gint64 g64ival;
	gchar *sval;
	gboolean bval;
	struct list_node *lnp;

	nid_log_warning("ini_parse start ...");
	gkey = g_key_file_new();
	if (!g_key_file_load_from_file(gkey, fpath,
		G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &err)) {
		rc = -1;
		goto out;
	}
	groups = g_key_file_get_groups(gkey, NULL);
	the_group = groups;
	while (*the_group) {
		the_sc = x_calloc(1, sizeof(*the_sc));
		list_add_tail(&the_sc->s_list, &priv_p->p_section_head);
		strcpy(the_sc->s_name, *the_group);
		INIT_LIST_HEAD(&the_sc->s_head);
		nid_log_warning("ini_parse: pricessing the_group:%s", *the_group);

		/*
		 * find type of this section
		 */
		sval = g_key_file_get_string(gkey, *the_group, "type", &err);
		assert(sval);
		__str_trim(sval);

		/*
		 * Find key set of this type
		 */
		list_for_each_entry(lnp, struct list_node, &priv_p->p_key_set, ln_list) {
			the_key = (struct ini_key_desc *)lnp->ln_data;
			while (the_key->k_name) {
				if (!strcmp(the_key->k_name, "type")) {
					break;
				}
				the_key++;
			}
			assert(the_key->k_name);
			assert(the_key->k_value);
			if (!strcmp(sval, the_key->k_value)) {
				break;
			}
		}
		the_key = (struct ini_key_desc *)lnp->ln_data;

		//the_key = priv_p->p_key_desc;
		while (the_key && the_key->k_name) {
			nid_log_warning("ini_parse: k_name:%s", the_key->k_name);
			the_kc = x_calloc(1, sizeof(*the_kc));
			strcpy(the_kc->k_name, the_key->k_name);
			switch (the_key->k_type) {
			case KEY_TYPE_INT:
				the_kc->k_type = KEY_TYPE_INT;
				ival = g_key_file_get_integer(gkey, *the_group, the_key->k_name, &err);
				the_kc->k_value = x_malloc(sizeof(ival));
				if (!err) {
					 *((int *)the_kc->k_value) = ival;
				} else if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND && !the_key->k_required) {
					*((int *)the_kc->k_value) = *((gint *)the_key->k_value);
				} else {
					nid_log_error("ini_parse: [%s] has invalid %s value",
						*the_group, the_key->k_name);
					assert(0);
				}
				break;

			case KEY_TYPE_INT64:
				the_kc->k_type = KEY_TYPE_INT64;
				g64ival = g_key_file_get_int64(gkey, *the_group, the_key->k_name, &err);
				the_kc->k_value = x_malloc(sizeof(g64ival));
				if (!err) {
					*((gint64 *)the_kc->k_value) = g64ival;
				} else if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND && !the_key->k_required) {
					*((gint64 *)the_kc->k_value) = *((gint *)the_key->k_value);
				} else {
					nid_log_error("ini_parse: [%s] has invalid %s value",
						*the_group, the_key->k_name);
					assert(0);
				}
				break;

			case KEY_TYPE_STRING:
				nid_log_warning("doing %s:%s", *the_group, the_key->k_name);
				the_kc->k_type = KEY_TYPE_STRING;
				sval = g_key_file_get_string(gkey, *the_group, the_key->k_name, &err);
				__str_trim(sval);
				if (!err) {
					nid_log_warning("got string %s:%s:%s", *the_group, the_key->k_name, sval);
					the_kc->k_value = sval;
				} else if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND && !the_key->k_required) {
					nid_log_warning("can not got string %s:%s", *the_group, the_key->k_name);
					if (the_key->k_value) {
						the_kc->k_value = x_malloc(1024);
						strcpy(the_kc->k_value, the_key->k_value);
					}
				} else {
					nid_log_error("ini_parse: [%s] has invalid %s value",
						*the_group, the_key->k_name);
					assert(0);
				}
				break;

			case KEY_TYPE_BOOL:
				the_kc->k_type = KEY_TYPE_BOOL;
				bval = g_key_file_get_boolean(gkey, *the_group, the_key->k_name, &err);
				the_kc->k_value = x_malloc(sizeof(int));
				if (!err) {
					*((char *)the_kc->k_value) = (bval==TRUE) ? 1 : 0;
				} else if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND && !the_key->k_required) {
					*((char *)the_kc->k_value) = *((char *)the_key->k_value);
				} else if (err->code == G_KEY_FILE_ERROR_INVALID_VALUE && !the_key->k_required) {
					nid_log_error("ini_parse: [%s] has invalid %s value",
						*the_group, the_key->k_name);
					assert(0);
				}
				break;

			default:
				break;
			}

			if (err) {
				if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND && the_key->k_required) {
					nid_log_error("%s", err->message);
					g_error_free(err);
					err = NULL;
					rc = -1;
					goto out;
				}
				g_error_free(err);
				err = NULL;
			}
			list_add_tail(&the_kc->k_list, &the_sc->s_head);
			the_key++;
		}
		the_group++;
	}

out:
	g_key_file_free(gkey);
	if (!rc) {
		priv_p->p_next_pos = priv_p->p_section_head.next;
	}
	return rc;
}

static struct ini_section_content*
ini_search_section(struct ini_interface *ini_p, char *name)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct list_head *head = &priv_p->p_section_head;
	struct ini_section_content *the_sc, *target = NULL;

	list_for_each_entry(the_sc, struct ini_section_content, head, s_list) {
		if (!strcmp(name, the_sc->s_name)) {
			target = the_sc;
			break;
		}
	}
	return target;
}


static struct ini_section_content*
ini_search_section_by_key(struct ini_interface *ini_p, char *kname, void *kval)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct list_head *head = &priv_p->p_section_head;
	struct ini_section_content *the_sc, *target = NULL;
	struct ini_key_content *the_kc;

	list_for_each_entry(the_sc, struct ini_section_content, head, s_list) {
		list_for_each_entry(the_kc, struct ini_key_content, &the_sc->s_head, k_list) {
			if (!strcmp(kname, the_kc->k_name)) {
				switch (the_kc->k_type) {
				case KEY_TYPE_STRING:
					if (!strcmp(kval, the_kc->k_value))
						target = the_sc;
					break;

				case KEY_TYPE_INT:
					if (*(int *)the_kc->k_value == *(int *)kval)
						target = the_sc;
					break;

				default:
					break;
				}
			}
		}
		if (target)
			break;
	}

	return target;
}

static struct ini_key_content*
ini_search_key(struct ini_interface *ini_p, struct ini_section_content *sect_p, char *name)
{
	struct ini_key_content *the_kc, *target = NULL;
	struct list_head *head = &sect_p->s_head;

	assert(ini_p->i_private);
	list_for_each_entry(the_kc, struct ini_key_content, head, k_list) {
		if (!strcmp(name, the_kc->k_name)) {
			target = the_kc;
			break;
		}
	}
	return target;
}

static struct ini_key_content*
ini_search_key_from_section(struct ini_interface *ini_p, char *sname, char *kname)
{
	struct ini_section_content *sc_p;
	sc_p = ini_p->i_op->i_search_section(ini_p, sname);
	if (sc_p)
		return ini_p->i_op->i_search_key(ini_p, sc_p, kname);
	else
		return NULL;
}


/*
 * Algorithm:
 * 	compare two ini to find out difference
 * Input:
 * 	tgt_ini_p: old ini
 * 	new_ini_p: new ini
 * 	added_sect_p: sections added to tgt_ini, i.e not existing in tgt_ini, but existing in new_ini
 *	rm_sect_p: sections removed from tgt_ini, i.e existing in tgt_ini, but not in new_ini
 */
static void
ini_compare(struct ini_interface *tgt_ini_p, struct ini_interface *new_ini_p,
	struct ini_section_content **added_sc, struct ini_section_content **rm_sc)
{
	struct ini_private *priv_p1 = (struct ini_private *)tgt_ini_p->i_private;
	struct ini_private *priv_p2 = (struct ini_private *)new_ini_p->i_private;
	struct ini_section_content *the_sc1, *the_sc2;
	int found;

	list_for_each_entry(the_sc1, struct ini_section_content, &priv_p1->p_section_head, s_list) {
		found = 0;
		list_for_each_entry(the_sc2, struct ini_section_content, &priv_p2->p_section_head, s_list) {
			if (!strcmp(the_sc1->s_name, the_sc2->s_name)){
				found = 1;
				break;
			}
		}
		if (!found) {
			*rm_sc++ = the_sc1; /* the_sc1 should be removed from tgt_ini */
		}
	}
	*rm_sc = NULL;

	list_for_each_entry(the_sc2, struct ini_section_content, &priv_p2->p_section_head, s_list) {
		found = 0;
		list_for_each_entry(the_sc1, struct ini_section_content, &priv_p1->p_section_head, s_list) {
			if (!strcmp(the_sc2->s_name, the_sc1->s_name)){
				found = 1;
				break;
			}
		}
		if (!found) {
			*added_sc++ = the_sc2; /* the_sc2 should be added to tgt_ini */
		}
	}
	*added_sc = NULL;
}

static struct ini_section_content*
ini_add_section(struct ini_interface *ini_p, struct ini_section_content *sc_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct ini_section_content *the_sc, *target = NULL;
	struct list_head *head = &priv_p->p_section_head;
	char *name = sc_p->s_name;

	list_for_each_entry(the_sc, struct ini_section_content, head, s_list) {
		if (!strcmp(name, the_sc->s_name)) {
			target = the_sc;
			break;
		}
	}
	if (target) {
		nid_log_warning("ini_add_section: cannot add a duplicate section(%s)", name);
		target = NULL;	// nothing to add
	} else {
		list_add_tail(&sc_p->s_list, head);
		target = sc_p;	// add sc_p
	}

	return target;	// return what is added
}

static struct ini_section_content *
ini_remove_section(struct ini_interface *ini_p, char *name)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct ini_section_content *the_sc, *target = NULL;
	struct list_head *head = &priv_p->p_section_head;

	list_for_each_entry(the_sc, struct ini_section_content, head, s_list) {
		if (!strcmp(name, the_sc->s_name)) {
			target = the_sc;
			break;
		}
	}
	if (target) {
		list_del(&target->s_list);
	}
	return target;
}

static struct ini_section_content*
ini_next_section(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct ini_section_content *sc_p;

	if (priv_p->p_next_pos == &priv_p->p_section_head)
		return NULL;

	sc_p = list_entry(priv_p->p_next_pos, struct ini_section_content, s_list);
	priv_p->p_next_pos = priv_p->p_next_pos->next;
	return sc_p;
}

static void
ini_cleanup_section(struct ini_interface *ini_p, char *name)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct ini_section_content *the_sc, *target = NULL;
	struct list_head *head = &priv_p->p_section_head;
	struct ini_key_content *kc, *tmp;

	list_for_each_entry(the_sc, struct ini_section_content, head, s_list) {
		if (!strcmp(name, the_sc->s_name)) {
			target = the_sc;
			break;
		}
	}
	if (target) {
		list_for_each_entry_safe(kc, tmp, struct ini_key_content, &target->s_head, k_list) {
			if (kc->k_value != NULL) {
				free(kc->k_value);
			}
			list_del(&kc->k_list);
			free(kc);
		}
		list_del(&target->s_list);
		free(target);
	}
}

static void
ini_rollback(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	priv_p->p_next_pos = priv_p->p_section_head.next;
}

void
ini_cleanup(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct list_head *sc_head = &priv_p->p_section_head;
	struct ini_section_content *sc, *st;
	struct list_head *kc_head;
	struct ini_key_content *kc, *kt;

	list_for_each_entry_safe(sc, st, struct ini_section_content, sc_head, s_list) {
		kc_head = &sc->s_head;
		list_for_each_entry_safe(kc, kt, struct ini_key_content, kc_head, k_list) {
			if (kc->k_value != NULL) {
				free(kc->k_value);
			}
			list_del(&kc->k_list);
			free(kc);
		}
		list_del(&sc->s_list);
		free(sc);
	}

	free(priv_p);
}

static void
ini_display_key(struct ini_interface *ini_p, struct ini_key_content *the_kc)
{
	assert(ini_p);
	switch (the_kc->k_type) {
	case KEY_TYPE_INT:
		nid_log_debug("%s = %d", the_kc->k_name, *((int32_t *)the_kc->k_value));
		break;
	case KEY_TYPE_INT64:
		nid_log_debug("%s = %ld", the_kc->k_name, *((int64_t *)the_kc->k_value));
		break;
	case KEY_TYPE_STRING:
		nid_log_debug("%s = %s", the_kc->k_name, (char *)the_kc->k_value);
		break;
	case KEY_TYPE_BOOL:
		nid_log_debug("%s = %s", the_kc->k_name, *((char *)the_kc->k_value) ? "true" : "false");
		break;
	default:
		break;
	}
}

static void
ini_display(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;
	struct ini_section_content *the_sc;
	struct ini_key_content *the_kc;

	nid_log_debug("--------config display start------------(%s)", priv_p->p_pathname);

	list_for_each_entry(the_sc, struct ini_section_content, &priv_p->p_section_head, s_list) {
		nid_log_debug("[%s]", the_sc->s_name);
		list_for_each_entry(the_kc, struct ini_key_content, &(the_sc->s_head), k_list) {
			ini_display_key(ini_p, the_kc);
		}
	}
	nid_log_debug("--------config display end------------");
}

static struct list_head *
ini_get_template_key_list(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;

	(&priv_p->p_key_set)->prev->next = &priv_p->p_key_set;
	return &priv_p->p_key_set;
}


static struct list_head *
ini_get_conf_key_list(struct ini_interface *ini_p)
{
	struct ini_private *priv_p = (struct ini_private *)ini_p->i_private;

	(&priv_p->p_section_head)->prev->next = &priv_p->p_section_head;
	return &priv_p->p_section_head;
}

struct ini_operations ini_op = {
	.i_parse = ini_parse,
	.i_search_section = ini_search_section,
	.i_search_section_by_key = ini_search_section_by_key,
	.i_search_key = ini_search_key,
	.i_search_key_from_section = ini_search_key_from_section,
	.i_compare = ini_compare,
	.i_display = ini_display,
	.i_display_key = ini_display_key,
	.i_add_section = ini_add_section,
	.i_remove_section = ini_remove_section,
	.i_next_section = ini_next_section,
	.i_rollback = ini_rollback,
	.i_cleanup = ini_cleanup,
	.i_cleanup_section = ini_cleanup_section,
	.i_get_template_key_list = ini_get_template_key_list,
	.i_get_conf_key_list = ini_get_conf_key_list,
};

int
ini_initialization(struct ini_interface *ini_p, struct ini_setup *setup)
{
	char *log_header = "ini_initialization";
	struct ini_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	if (!setup)
		return 1;

	ini_p->i_op = &ini_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	ini_p->i_private = priv_p;

	//INIT_LIST_HEAD(&priv_p->p_key_set);
	priv_p->p_key_set = setup->key_set;

	//priv_p->p_key_desc = setup->keys_desc;
	strcpy(priv_p->p_pathname, setup->path);
	INIT_LIST_HEAD(&priv_p->p_section_head);
	return 0;
}
