/*
 * sds_if.h
 * 	Interface of Split Data Stream Module
 */
#ifndef NID_SDS_IF_H
#define NID_SDS_IF_H

#include <stdint.h>
#include "nid_shared.h"

struct sds_interface;
struct pp_page_node;
struct io_interface;
struct sds_operations {
	int			(*d_create_worker)(struct sds_interface *, void *, int, struct io_interface *);
	char*			(*d_get_buffer)(struct sds_interface *, int, uint32_t *, struct io_interface *);
	void			(*d_confirm)(struct sds_interface *, int, uint32_t);
	void			(*d_put_buffer)(struct sds_interface *, int, uint64_t, uint32_t, struct io_interface *);
	void			(*d_put_buffer2)(struct sds_interface *, int, void *, uint32_t, struct io_interface *);
	void			(*d_drop_page)(struct sds_interface *, int, void *);
	int			(*d_ready)(struct sds_interface *, int, uint64_t);
	char*			(*d_position)(struct sds_interface *, int, uint64_t);
	char*			(*d_position_length)(struct sds_interface *, int, uint64_t, uint32_t *);
	struct pp_page_node*	(*d_page_node)(struct sds_interface *, int, uint64_t);
	uint64_t		(*d_sequence)(struct sds_interface *, int);
	int			(*d_sequence_in_row)(struct sds_interface *, uint64_t, uint64_t);
	uint32_t		(*d_get_pagesz)(struct sds_interface *);
	void			(*d_stop)(struct sds_interface *, int);
	void			(*d_cleanup)(struct sds_interface *, int, struct io_interface *);
	void			(*d_destroy)(struct sds_interface *);
};

struct sds_interface {
	void			*d_private;
	struct sds_operations	*d_op;
};

struct pp_interface;
struct sds_setup {
	char			sds_name[NID_MAX_DSNAME];
	char			pp_name[NID_MAX_PPNAME];
	char			wc_name[NID_MAX_UUID];
	char			rc_name[NID_MAX_UUID];
	struct pp_interface	*pp;
};

extern int sds_initialization(struct sds_interface *, struct sds_setup *);
#endif
