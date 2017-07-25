/*
 * scg_if.h
 * 	Interface of Service Channel Guardian Module
 */

#ifndef _NID_SCG_IF_H
#define _NID_SCG_IF_H

#include <stdint.h>
#include "list.h"

struct list_head;
struct mpk_interface;
struct tp_interface;
struct ash_interface;
struct ini_interface;
struct scg_interface;
struct sac_interface;
struct sds_setup;
struct pp_setup;
struct scg_operations {
	void*			(*sg_accept_new_channel)(struct scg_interface *, int, char *);
	void			(*sg_do_channel)(struct scg_interface *, void *);
	void			(*sg_get_stat)(struct scg_interface *, struct list_head *);
	void			(*sg_reset_stat)(struct scg_interface *);
	void			(*sg_update)(struct scg_interface *);
	int			(*sg_upgrade_alevel)(struct scg_interface *, struct sac_interface *);
	int			(*sg_fupgrade_alevel)(struct scg_interface *, struct sac_interface *);
	int			(*sg_check_conn)(struct scg_interface *, char *);
	int			(*sg_bio_fast_flush)(struct scg_interface *, char *);
	int			(*sg_bio_stop_fast_flush)(struct scg_interface *, char *);
	void			(*sg_stop)(struct scg_interface *);
	void			(*sg_recover)(struct scg_interface *);
	void			(*sg_set_max_workers)(struct scg_interface *, uint16_t);
	int 			(*sg_bio_vec_start)(struct scg_interface *);
	int 			(*sg_bio_vec_stop)(struct scg_interface *);
	void			(*sg_bio_vec_stat)(struct scg_interface *, struct list_head *);
	void			(*sg_lock_dis_lck)(struct scg_interface *);
	void			(*sg_unlock_dis_lck)(struct scg_interface *);
};

struct scg_interface {
	void			*sg_private;
	struct scg_operations	*sg_op;
};

struct sacg_interface;
struct mpk_interface;
struct umpk_interface;
struct tpg_interface;
struct ini_interface;
struct ini_key_desc;
struct ini_section_desc;
struct scg_setup {
//	struct sacg_interface		*sacg;	// manage doa
	struct allocator_interface	*allocator;
	struct mpk_interface		*mpk;	//
	struct umpk_interface		*umpk;
	struct tpg_interface		*tpg;
	struct ini_interface		*ini;	// cfg
//	struct ini_key_desc		*server_keys;
	struct list_head		server_keys;
	struct pp2_interface		*pp2;
	struct lstn_interface		*lstn;
	struct txn_interface		*txn;
	struct mqtt_interface		*mqtt;
	int				support_smc;
	int				support_wcc;
	int				support_rcc;
	int				support_bwcc;
	int				support_crcc;
	int				support_sac;
	int				support_cds;
	int				support_sds;
	int				support_rio;
	int				support_bio;
	int				support_bwc;
	int				support_crc;
	int				support_sarw;
	int				support_sarwc;
	int				support_drw;
	int				support_drwc;
	int				support_mrw;
	int				support_mrwc;
	int                             support_twc;
	int                             support_twcc;
	int				support_sacc;
	int				support_sdsc;
	int				support_ppc;

	int				support_cxa;
	int				support_cxac;
	int				support_cxp;
	int				support_cxpc;
	int				support_cxt;

	int				support_dxa;
	int				support_dxac;
	int				support_dxp;
	int				support_dxpc;
	int				support_dxt;

	int				support_tpc;
	int				support_doac;

	int				support_inic;

	int				support_rept;
	int				support_reps;

	int				support_reptc;
	int				support_repsc;
};

extern int scg_initialization(struct scg_interface *, struct scg_setup *);
#endif
