/*
 * Interface of Data Stream Module
 */

#ifndef _DS_H
#define _DS_H

#include <stdint.h>

#define DS_TYPE_NONE	0
#define DS_TYPE_CYCLE	1
#define DS_TYPE_SPLIT	2
#define DS_TYPE_MAX	3
#define	DS_MAX_WORKERS	1024

struct ds_interface;
struct pp_page_node;
struct io_interface;
struct pp_interface;
struct ds_operations {
	char*			(*d_get_buffer)(struct ds_interface *, int, uint32_t *, struct io_interface *);
	void			(*d_confirm)(struct ds_interface *, int,  uint32_t);
	void			(*d_put_buffer)(struct ds_interface *, int, uint64_t, uint32_t, struct io_interface *);
	void			(*d_put_buffer2)(struct ds_interface *, int, void *, uint32_t, struct io_interface *);
	int			(*d_ready)(struct ds_interface *, int, uint64_t);
	char*			(*d_position)(struct ds_interface *, int, uint64_t);
	char*			(*d_position_length)(struct ds_interface *, int, uint64_t, uint32_t *);
	struct pp_page_node*	(*d_page_node)(struct ds_interface *, int, uint64_t);
	uint64_t		(*d_sequence)(struct ds_interface *, int);	// return current sequence nuber
	int			(*d_sequence_in_row)(struct ds_interface *, int, uint64_t, uint64_t);
	uint32_t		(*d_get_pagesz)(struct ds_interface *, int);
	void			(*d_drop_page)(struct ds_interface *, int, void *);
	void			(*d_cleanup)(struct ds_interface *);
	void			(*d_stop)(struct ds_interface *, int);
	void			(*d_destroy)(struct ds_interface *);
	int			(*d_create_worker)(struct ds_interface *, void *, int, struct io_interface *);
	void			(*d_release_worker)(struct ds_interface *ds_p, int chan_index, struct io_interface *);
	char*			(*d_get_name)(struct ds_interface *ds_p);
	char*			(*d_get_wc_name)(struct ds_interface *ds_p);
	char*			(*d_get_rc_name)(struct ds_interface *ds_p);
	struct pp_interface*	(*d_get_pp)(struct ds_interface *ds_p);
	int			(*d_get_type)(struct ds_interface *ds_p);
	void			(*d_set_wc)(struct ds_interface *ds_p, char *);
};

struct ds_interface {
	void			*d_private;
	struct ds_operations	*d_op;
};

struct ds_setup {
	char	*name;
	char	*wc_name;
	char	*rc_name;
	int	type;
	void	*pool;
	uint8_t pagenrshift;
	uint8_t pageszshift;
};

extern int ds_initialization(struct ds_interface *, struct ds_setup *);

#endif
