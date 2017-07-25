/*
 * io_if.h
 * 	Interface of IO Module
 */
#ifndef NID_IO_IF_H
#define NID_IO_IF_H

#include <stdint.h>
#include "list.h"

#define IO_MAX_WORKERS		64
#define IO_TYPE_RESOURCE	1
#define IO_TYPE_BUFFER		2
#define IO_TYPE_MAX		3

struct io_channel_info {
	void	*io_rw;
	void	*io_rw_handle;
	void	*io_wc;
	void	*io_wc_handle;
	void	*io_rc;
	void	*io_rc_handle;
	void	*io_sdsg;
	void	*io_cdsg;
	char	*io_ds_name;
};

struct io_chan_stat {
	uint32_t	s_rtree_nseg;
	uint32_t	s_rtree_segsz;
	uint32_t	s_rtree_nfree;
	uint32_t	s_rtree_nused;
	uint32_t	s_btn_nseg;
	uint32_t	s_btn_segsz;
	uint32_t	s_btn_nfree;
	uint32_t	s_btn_nused;
};

struct io_stat {
	uint8_t			s_stat;
	uint8_t			s_io_type_bio;
	uint8_t			s_io_type_rio;
	uint16_t		s_block_occupied;
	uint16_t		s_flush_nblocks;
	uint16_t		s_cur_write_block;
	uint16_t		s_cur_flush_block;
	uint64_t		s_seq_flushed;
	uint64_t		s_rec_seq_flushed;
	uint64_t		s_seq_assigned;
	uint32_t		s_buf_avail;
	struct list_head	s_inactive_head;
};


struct io_vec_stat {
	char			*s_chan_uuid;
	uint8_t			s_stat;
	uint8_t			s_io_type_bio;
	uint8_t			s_io_type_rio;
	uint32_t		s_flush_num;
	uint64_t		s_vec_num;
	uint64_t		s_write_size;
	//struct list_head	s_inactive_head;
};

struct list_head;
struct io_interface;
struct rw_interface;
struct io_channel_info;
struct io_operations {
	void*		(*io_create_worker)(struct io_interface *, void *, struct io_channel_info *, char *, int *);
	void*		(*io_prepare_worker)(struct io_interface *, struct io_channel_info *io_chan, char *);
	int		(*io_get_type)(struct io_interface *);
	ssize_t 	(*io_pread)(struct io_interface *, void *, void *, size_t, off_t);
	ssize_t		(*io_pwrite)(struct io_interface *, void *, void *, size_t, off_t);
	ssize_t		(*io_trim)(struct io_interface *, void *, off_t, size_t);
	void		(*io_write_list)(struct io_interface *, void *, struct list_head *, int);
	void		(*io_read_list)(struct io_interface *, void *, struct list_head *, int);
	void		(*io_trim_list)(struct io_interface *, void *, struct list_head *, int);
	int		(*io_close)(struct io_interface *, void *);
	int		(*io_stop)(struct io_interface *);
	void		(*io_start_page)(struct io_interface *, void *, void *, int);
	void		(*io_end_page)(struct io_interface *, void *, void *, int);
	void		(*io_get_chan_stat)(struct io_interface *, void *, struct io_chan_stat *);
	void		(*io_get_stat)(struct io_interface *, struct io_stat *);
	int		(*io_fast_flush)(struct io_interface *, char *);
	int		(*io_stop_fast_flush)(struct io_interface *, char *);
	int     	(*io_vec_start)(struct io_interface *);
	int     	(*io_vec_stop)(struct io_interface *);
	void    	(*io_get_vec_stat)(struct io_interface *, void *, struct io_vec_stat *);
	uint8_t		(*io_get_cutoff)(struct io_interface *, void *);
	int		(*io_destroy_wc)(struct io_interface *io_p, char *wc_uuid);
};

struct io_interface {
	void			*io_private;
	struct io_operations	*io_op;
};

struct wc_interface;
struct tp_interface;
struct ini_interface;
struct pp_interface;
struct io_setup {
	int			io_type;
	struct tp_interface	*tp;
	struct ini_interface	**ini;
	struct pp_interface	*pool;
};

extern int io_initialization(struct io_interface *, struct io_setup *);
#endif
