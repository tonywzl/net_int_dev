/*
 * bfp1_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP1_IF_H
#define NID_BFP1_IF_H

#include <stdint.h>

struct bfp1_interface;
struct pp_interface;
struct bfp1_operations {
	int 	(*bf_get_flush_num)(struct bfp1_interface *);
	int 	(*bf_update_water_mark)(struct bfp1_interface *, int, int);
};

struct bfp1_interface {
	void			*fp_private;
	struct bfp1_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct bfp1_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	int			low_water_mark;
	int			high_water_mark;
};

extern int bfp1_initialization(struct bfp1_interface *, struct bfp1_setup *);
#endif
