/*
 * Interface of Cycle Data Stream Module
 */

#ifndef _CDS_H
#define _CDS_H

#include <stdint.h>
#include "nid_shared.h"

struct cds_interface;
struct cds_operations {
	int		(*d_create_worker)(struct cds_interface *);
	char*		(*d_get_buffer)(struct cds_interface *, int, uint32_t *);
	void		(*d_confirm)(struct cds_interface *, int, uint32_t);
	void		(*d_put_buffer)(struct cds_interface *, int, uint64_t, uint32_t);
	void		(*d_put_buffer2)(struct cds_interface *, int, void *, uint32_t);
	int		(*d_ready)(struct cds_interface *, int, uint64_t);
	char*		(*d_position)(struct cds_interface *, int, uint64_t);
	char*		(*d_position_length)(struct cds_interface *, int, uint64_t, uint32_t *);
	uint64_t	(*d_sequence)(struct cds_interface *, int);	// return current sequence nuber
	int		(*d_sequence_in_row)(struct cds_interface *, uint64_t, uint64_t);
	uint32_t	(*d_get_pagesz)(struct cds_interface *);
	void		(*d_cleanup)(struct cds_interface *, int);
	void		(*d_stop)(struct cds_interface *, int);
};

struct cds_interface {
	void			*d_private;
	struct cds_operations	*d_op;
};

struct cds_setup {
	char	cds_name[NID_MAX_DSNAME];
	uint8_t	pagenrshift;
	uint8_t	pageszshift;
};

extern int cds_initialization(struct cds_interface *, struct cds_setup *);

#endif
