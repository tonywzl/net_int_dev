/*
 * bfp5_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP5_IF_H
#define NID_BFP5_IF_H

#include <stdint.h>

struct bfp5_interface;
struct pp_interface;
struct bfp5_operations {
	int 	(*bf_get_flush_num)(struct bfp5_interface *);
};

struct bfp5_interface {
	void			*fp_private;
	struct bfp5_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct bfp5_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	double			coalesce_ratio;
	double			load_ratio_max;
	double			load_ratio_min;
	double			load_ctrl_level;
	double			flush_delay_ctl;
	double			throttle_ratio;
};

extern int bfp5_initialization(struct bfp5_interface *, struct bfp5_setup *);
#endif
