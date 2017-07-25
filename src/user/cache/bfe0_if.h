/*
 * bfe0_if.h
 * 	Interface of BIO0 Flush Engine Module
 */
#ifndef NID_BFE0_IF_H
#define NID_BFE0_IF_H

#include <bfe_if.h>

struct io_vec_stat;
struct pp_page_node;
struct data_description_node;
struct bfe0_operations {
	struct bfe_operations super;
	// Add new bfe0 operation here, for bfe0 only.
};

struct bfe0_setup {
	struct bfe_setup 	super;
	char 			*bufdevice;
	int8_t			ssd_mode;
};

extern int bfe0_initialization(struct bfe_interface *, struct bfe0_setup *);

#endif
