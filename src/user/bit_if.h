/*
 * bit_if.h
 * 	Interface of Bit Module
 */
#ifndef NID_BIT_IF_H
#define NID_BIT_IF_H

#include <stdint.h>

struct bit_interface;
struct bit_operations {
	int	(*b_ctz32)(struct bit_interface *, int n);
	int	(*b_clz32)(struct bit_interface *, int n);
	int	(*b_ctz64)(struct bit_interface *, uint64_t n);
	int	(*b_clz64)(struct bit_interface *, uint64_t n);
	void	(*b_close)(struct bit_interface *);
};

struct bit_interface {
	void			*b_private;
	struct bit_operations	*b_op;
};

struct bit_setup {
};

extern int bit_initialization(struct bit_interface *, struct bit_setup *);
#endif
