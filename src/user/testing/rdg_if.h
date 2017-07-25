/*
 * rdg_if.h
 * 	Interface of Random Data Generator Module
 */
#ifndef NID_RDG_IF_H
#define NID_RDG_IF_H

#include <stdint.h>

struct rdg_interface;
struct rdg_operations {
	void		(*rd_get_range)(struct rdg_interface *, off_t *, uint32_t *);
	void		(*rd_get_data)(struct rdg_interface *, uint32_t, char *);
};

struct rdg_interface {
	void			*rd_private;
	struct rdg_operations	*rd_op;
};

struct rdg_setup {
	off_t		start_off;
	uint32_t	range;		// in G
	uint32_t	maxlen;		// in K
};

extern int rdg_initialization(struct rdg_interface *, struct rdg_setup *);
#endif
