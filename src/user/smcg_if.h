/*
 * smcg_if.h
 * 	Interface of Server Manager Channel Guardian Module
 */
#ifndef NID_SMCG_IF_H
#define NID_SMCG_IF_H

struct smcg_interface;
struct list_head;
struct smcg_operations {
	void*	(*smg_accept_new_channel)(struct smcg_interface *, int, char *);
	void	(*smg_do_channel)(struct smcg_interface *, void *);
	void	(*smg_get_stat)(struct smcg_interface *, struct list_head *);
	void	(*smg_reset_stat)(struct smcg_interface *);
	void	(*smg_update)(struct smcg_interface *);
	int	(*smg_check_conn)(struct smcg_interface *, char *);
	void	(*smg_stop)(struct smcg_interface *);
	void	(*smg_bio_fast_flush)(struct smcg_interface *, char *);
	void	(*smg_bio_stop_fast_flush)(struct smcg_interface *, char *);
	void	(*smg_bio_vec_start)(struct smcg_interface *);
	void	(*smg_bio_vec_stop)(struct smcg_interface *);
	void	(*smg_bio_vec_stat)(struct smcg_interface *, struct list_head *);
	void	(*smg_mem_size)(struct smcg_interface *);
};

struct smcg_interface {
	void			*smg_private;
	struct smcg_operations	*smg_op;
};

struct scg_interface;
struct mpk_interface;
struct smcg_setup {
	struct scg_interface	*scg;
	struct mpk_interface	*mpk;
	struct allocator_interface *alloc;
	struct ini_interface	*ini;
	struct mqtt_interface	*mqtt;
};

extern int smcg_initialization(struct smcg_interface *, struct smcg_setup *);
#endif
