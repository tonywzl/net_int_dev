/*
 * bio_if.h
 * 	Interface of Buffer IO Module
 */
#ifndef NID_BIO_IF_H
#define NID_BIO_IF_H

struct bio_interface;
struct list_head;
struct io_chan_stat;
struct rw_interface;
struct io_vec_stat;
struct io_stat;
struct io_channel_info;
struct bio_operations {
	void*		(*b_create_channel)(struct bio_interface *, void *, struct io_channel_info *, char *, int *);
	void*		(*b_prepare_channel)(struct bio_interface *, struct io_channel_info *, char *);
	uint32_t	(*b_pread)(struct bio_interface *, void *, void *, size_t, off_t);
	void		(*b_read_list)(struct bio_interface *, void *, struct list_head *, int);
	void		(*b_write_list)(struct bio_interface *, void *, struct list_head *, int);
	void		(*b_trim_list)(struct bio_interface *, void *, struct list_head *, int);
	int		(*b_chan_inactive)(struct bio_interface *, void *);
	int		(*b_stop)(struct bio_interface *);
	int		(*b_start_page)(struct bio_interface *, void *, void *, int);
	int		(*b_end_page)(struct bio_interface *, void *, void *, int);
	int		(*b_get_chan_stat)(struct bio_interface *, void *, struct io_chan_stat *);
	int		(*b_get_vec_stat)(struct bio_interface *, void *, struct io_vec_stat *);
	int		(*b_get_stat)(struct bio_interface *, struct io_stat *);
	int		(*b_fast_flush)(struct bio_interface *, char *);
	int		(*b_stop_fast_flush)(struct bio_interface *, char *);
	int		(*b_vec_start)(struct bio_interface *);
	int		(*b_vec_stop)(struct bio_interface *);
	uint8_t		(*b_get_cutoff)(struct bio_interface *, void *);
	int		(*b_destroy_wc)(struct bio_interface *, char *);
};

struct bio_interface {
	void			*b_private;
	struct bio_operations	*b_op;
};

struct pp_interface;
struct tp_interface;
struct bio_setup {
};

extern int bio_initialization(struct bio_interface *, struct bio_setup *);
#endif
