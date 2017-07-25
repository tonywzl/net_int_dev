/*
 * pp2_if.h
 *  Interface of Page Pool2 Module
 */

#ifndef NID_PP2_IF_H
#define NID_PP2_IF_H

#include "stdint.h"

struct pp2_interface;
struct pp2_operations {
	void*	(*pp2_get)(struct pp2_interface *, uint32_t);
	void*	(*pp2_get_zero)(struct pp2_interface *, uint32_t);
	void*	(*pp2_get_nowait)(struct pp2_interface *, uint32_t);
	void*	(*pp2_get_forcibly)(struct pp2_interface *, uint32_t);
	void	(*pp2_put)(struct pp2_interface *, void *);
	void	(*pp2_display)(struct pp2_interface *);
	void	(*pp2_cleanup)(struct pp2_interface *);
};

struct pp2_interface {
	void			*pp2_private;
	struct pp2_operations	*pp2_op;
};

struct allocator_interface;
struct pp2_setup {
	char				*name;
	struct allocator_interface	*allocator;
	int				set_id;
	uint32_t			page_size;
	uint32_t			pool_size;	// If set to 0, the pool is allowed to extend infinitely
	int 				get_zero;	// flag for whether to zero the buffer before get
	int				put_zero;	// flag for whether to zero the buffer after put
};

extern int pp2_initialization(struct pp2_interface *, struct pp2_setup *);
#endif
