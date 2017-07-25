/*
 * ini_if.h
 * 	Interface of Ini Module
 */

#ifndef _INI_IF_H
#define _INI_IF_H

#define first_intend "\t"

#include "list.h"

typedef enum {
	KEY_TYPE_INT,
	KEY_TYPE_INT64,
	KEY_TYPE_STRING,
	KEY_TYPE_BOOL,
	KEY_TYPE_WRONG,
} ini_type;

struct ini_key_desc {
	char			*k_name;
	ini_type		k_type;
	void			*k_value;
	char			k_required;
	char			*k_description;
};

struct ini_key_content {
	struct list_head	k_list;
	char			k_name[1024];
	ini_type		k_type;
	void			*k_value;
};

struct ini_section_content {
	struct list_head	s_list;
	struct list_head	s_head;		// list head of ini_key_type
	char			s_name[1024];
};

struct ini_interface;
struct ini_operations {
	int				(*i_parse)(struct ini_interface *);
	struct ini_section_content*	(*i_search_section)(struct ini_interface *, char *);
	struct ini_section_content*	(*i_search_section_by_key)(struct ini_interface *, char *, void *);
	struct ini_key_content*		(*i_search_key)(struct ini_interface *, struct ini_section_content *, char *);
	struct ini_key_content*		(*i_search_key_from_section)(struct ini_interface *, char *, char *);
	void				(*i_compare)(struct ini_interface *, struct ini_interface *,
						struct ini_section_content **, struct ini_section_content **);
	void				(*i_display)(struct ini_interface *);
	void				(*i_display_key)(struct ini_interface *, struct ini_key_content *);
	struct ini_section_content*	(*i_add_section)(struct ini_interface *, struct ini_section_content *);
	struct ini_section_content*	(*i_remove_section)(struct ini_interface *, char *);
	struct ini_section_content*	(*i_next_section)(struct ini_interface *);
	void				(*i_rollback)(struct ini_interface *);
	void				(*i_cleanup)(struct ini_interface *);
	void				(*i_cleanup_section)(struct ini_interface *, char *);
	struct list_head*		(*i_get_template_key_list)(struct ini_interface *);
	struct list_head*		(*i_get_conf_key_list)(struct ini_interface *);
};

struct ini_interface {
	void			*i_private;
	struct ini_operations	*i_op;
};

struct ini_setup {
	char 			*path;
//	struct ini_key_desc	*keys_desc;
	struct list_head	key_set;
};

extern int ini_initialization(struct ini_interface *, struct ini_setup *);
#endif
