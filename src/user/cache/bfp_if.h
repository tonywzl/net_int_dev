/*
 * bfp_if.h
 * 	Interface of Bio Flushing Policy Module
 */
#ifndef NID_BFP_IF_H
#define NID_BFP_IF_H

#include <stdint.h>

#define BFP_POLICY_BFP1		1
#define BFP_POLICY_BFP2		2
#define BFP_POLICY_BFP3		3
#define BFP_POLICY_BFP4		4
#define BFP_POLICY_BFP5		5

#define LOAD_RATIO_MAX			0.95
#define LOAD_RATIO_MAX_DEFAULT		LOAD_RATIO_MAX
#define LOAD_RATIO_MIN			0.1
#define LOAD_RATIO_MIN_DEFAULT		LOAD_RATIO_MIN
#define LOAD_CTRL_LEVEL_MIN		0.1
#define LOAD_CTRL_LEVEL_MAX		0.95
#define LOAD_CTRL_LEVEL_DEFAULT		0.7
#define FLUSH_DELAY_CTL_DEFAULT		8.0
#define FLUSH_DELAY_CTL_MIN		0
#define FLUSH_DELAY_CTL_MAX		100.0
#define THROTTLE_RATIO_MAX		0.9
#define THROTTLE_RATIO_MIN		0.1
#define THROTTLE_RATIO_DEFAULT		THROTTLE_RATIO_MAX

struct bfp_interface;
struct pp_interface;
struct bfp_operations {
	int 	(*bf_get_flush_num)(struct bfp_interface *);
	int 	(*bf_update_water_mark)(struct bfp_interface *, int, int);
};

struct bfp_interface {
	void			*fp_private;
	struct bfp_operations	*fp_op;
};

struct bio_interface;
struct wc_interface;
struct pp_interface;
struct ini_interface;
struct bfp_setup {
	struct wc_interface	*wc;
	struct pp_interface	*pp;
	int			type;
	uint32_t		pagesz;
	ssize_t 		buffersz;
	char			*uuid;
	double			coalesce_ratio;
	double  		load_ratio_max;
	double  		load_ratio_min;
	double  		load_ctrl_level;
	double  		flush_delay_ctl;
	double			throttle_ratio;
	int			low_water_mark;
	int			high_water_mark;
};

extern int bfp_initialization(struct bfp_interface *, struct bfp_setup *);
#endif
