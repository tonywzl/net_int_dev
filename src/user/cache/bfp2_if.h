/*
 * bfp2_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP2_IF_H
#define NID_BFP2_IF_H

#include <stdint.h>

struct bfp2_interface;
struct pp_interface;
struct bfp2_operations {
	int 	(*bf_get_flush_num)(struct bfp2_interface *);
};

struct bfp2_interface {
	void			*fp_private;
	struct bfp2_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct bfp2_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	double			load_ratio_max;
	double			load_ratio_min;
	double			load_ctrl_level;
	double			flush_delay_ctl;
};

extern int bfp2_initialization(struct bfp2_interface *, struct bfp2_setup *);
#endif
