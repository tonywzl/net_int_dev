#ifndef _NID_SERVER_H
#define _NID_SERVER_H

#include "ini_if.h"

struct lstn_interface;

extern void conf_global_registration(struct list_head *, struct lstn_interface *);
extern void conf_cds_registration(struct list_head *, struct lstn_interface *);
extern void conf_crc_registration(struct list_head *, struct lstn_interface *);
extern void conf_cxa_registration(struct list_head *, struct lstn_interface *);
extern void conf_cxp_registration(struct list_head *, struct lstn_interface *);
extern void conf_cxt_registration(struct list_head *, struct lstn_interface *);
extern void conf_drc_registration(struct list_head *, struct lstn_interface *);
extern void conf_dxa_registration(struct list_head *, struct lstn_interface *);
extern void conf_dxp_registration(struct list_head *, struct lstn_interface *);
extern void conf_dxt_registration(struct list_head *, struct lstn_interface *);
extern void conf_bwc_registration(struct list_head *, struct lstn_interface *);
extern void conf_pp_registration(struct list_head *, struct lstn_interface *);
extern void conf_sac_registration(struct list_head *, struct lstn_interface *);
extern void conf_scg_registration(struct list_head *, struct lstn_interface *);
extern void conf_sds_registration(struct list_head *, struct lstn_interface *);
extern void conf_tp_registration(struct list_head *, struct lstn_interface *);
extern void conf_twc_registration(struct list_head *, struct lstn_interface *);
extern void conf_rgdev_registration(struct list_head *, struct lstn_interface *);
extern void conf_drw_registration(struct list_head *, struct lstn_interface *);
extern void conf_mrw_registration(struct list_head *, struct lstn_interface *);
extern void conf_sarw_registration(struct list_head *, struct lstn_interface *);
extern void conf_rsa_registration(struct list_head *, struct lstn_interface *);
extern void conf_rsm_registration(struct list_head *, struct lstn_interface *);
extern void conf_rs_registration(struct list_head *, struct lstn_interface *);
extern void conf_rept_registration(struct list_head *, struct lstn_interface *);
extern void conf_reps_registration(struct list_head *, struct lstn_interface *);

extern void server_generate_conf_tmplate(struct list_head*, struct lstn_interface *);

#endif
