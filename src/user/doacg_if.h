/*
 * doacg_if.h
 * 	Interface of Delegation of Authority Guardian Module
 */
#ifndef NID_doacg_IF_H
#define NID_doacg_IF_H

struct doacg_interface;
struct umessage_doa_information;
struct doacg_operations {
	void*	(*dg_accept_new_channel)(struct doacg_interface *, int, uint8_t);
	void	(*dg_do_channel)(struct doacg_interface *, void *);
	void	(*dg_request)(struct doacg_interface *, struct umessage_doa_information *, uint8_t);
	void	(*dg_release)(struct doacg_interface *, struct umessage_doa_information *, uint8_t);
	void	(*dg_check)(struct doacg_interface *, struct umessage_doa_information *m, uint8_t);
};

struct doacg_interface {
	void			*dg_private;
	struct doacg_operations	*dg_op;
};

struct doacg_setup {
	struct umpk_interface		*umpk;
	struct scg_interface		*scg;
	struct list_head			all_doa_heads;
};

extern int doacg_initialization(struct doacg_interface *, struct doacg_setup *);
#endif
