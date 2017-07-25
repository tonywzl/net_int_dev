/*
 * doa_if.h
 * 	Interface of Delegation of Authority Module
 */

#ifndef NID_DOA_IF_H
#define NID_DOA_IF_H

#define DOA_MAX_ID	1024
#define DOA_CMD_REQUEST 1
#define DOA_CMD_CHECK 2
#define DOA_CMD_RELEASE 3

#define NID_DOA_OVERTIME 30

struct doa_interface;
struct umessage_doa_information;
struct doa_operations {
	void	(*d_get_info_stat)(struct doa_interface *, struct umessage_doa_information *);
	void	(*d_init_info)(struct doa_interface *, struct umessage_doa_information *);
	struct doa_interface *	(*d_compare)(struct doa_interface *, struct doa_interface *);
	char*	(*d_get_id)(struct doa_interface *);
	char*	(*d_get_owner)(struct doa_interface *);
	time_t	(*d_get_timestamp)(struct doa_interface *);
	void	(*d_free)(struct doa_interface *);
};

struct doa_interface {
	void			*d_private;
	struct doa_operations	*d_op;
};

struct doa_setup {
	struct doacg_interface	*doacg;
	struct umpk_interface	*umpk;
};

extern int doa_initialization(struct doa_interface *, struct doa_setup *);
#endif
