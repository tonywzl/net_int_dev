/*
 * bfp4_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP4_IF_H
#define NID_BFP4_IF_H

#include <stdint.h>

struct bfp4_interface;
struct pp_interface;
struct bfp4_operations {
	int 	(*bf_get_flush_num)(struct bfp4_interface *);
};

struct bfp4_interface {
	void			*fp_private;
	struct bfp4_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct bfp4_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	double			load_ratio_max;
	double			load_ratio_min;
	double			load_ctrl_level;
	double			flush_delay_ctl;
	double			throttle_ratio;
};

extern int bfp4_initialization(struct bfp4_interface *, struct bfp4_setup *);
#endif
