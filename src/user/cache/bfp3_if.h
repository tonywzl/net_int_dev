/*
 * bfp3_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP3_IF_H
#define NID_BFP3_IF_H

#include <stdint.h>

struct bfp3_interface;
struct pp_interface;
struct bfp3_operations {
	int 	(*bf_get_flush_num)(struct bfp3_interface *);
};

struct bfp3_interface {
	void			*fp_private;
	struct bfp3_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct bfp3_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	double			coalesce_ratio;
	double			load_ratio_max;
	double			load_ratio_min;
	double			load_ctrl_level;
	double			flush_delay_ctl;
};

extern int bfp3_initialization(struct bfp3_interface *, struct bfp3_setup *);
#endif
