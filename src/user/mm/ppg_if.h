/*
 * ppg_if.h
 * 	Interface of Page Pool Guardian Module
 */
#ifndef NID_PPG_IF_H
#define NID_PPG_IF_H

#include <stdint.h>

#define PPG_MAX_PP	128

struct ppg_interface;
struct ppg_operations {
	struct pp_interface*	(*pg_search_pp)(struct ppg_interface *, char *);
	struct pp_interface*	(*pg_search_and_create_pp)(struct ppg_interface *, char *, int);
	int			(*pg_add_pp)(struct ppg_interface *, char *, int, uint32_t, uint32_t);
	int			(*pg_delete_pp)(struct ppg_interface *, char *);
	struct pp_interface**	(*pg_get_all_pp)(struct ppg_interface *, int *);
	struct pp_setup*	(*pg_get_all_pp_setup)(struct ppg_interface *, int *);
	int*			(*pg_get_working_pp_index)(struct ppg_interface *, int *);
};

struct ppg_interface {
	void			*pg_private;
	struct ppg_operations	*pg_op;
};

struct ini_interface;
struct allocator_interface;
struct ppg_setup {
	struct allocator_interface	*allocator;
	struct ini_interface		*ini;
};

extern int ppg_initialization(struct ppg_interface *, struct ppg_setup *);
#endif
