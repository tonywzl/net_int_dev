/*
 * server_conf.c
 */

#include "ini_if.h"
#include "lstn_if.h"
#include "server.h"
#include "assert.h"

void
server_generate_conf_tmplate(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	conf_global_registration(key_set_head, lstn_p);
	conf_cds_registration(key_set_head, lstn_p);
	conf_crc_registration(key_set_head, lstn_p);
	conf_cxa_registration(key_set_head, lstn_p);
	conf_cxp_registration(key_set_head, lstn_p);
	conf_cxt_registration(key_set_head, lstn_p);
	conf_dxa_registration(key_set_head, lstn_p);
	conf_dxp_registration(key_set_head, lstn_p);
	conf_dxt_registration(key_set_head, lstn_p);
	conf_bwc_registration(key_set_head, lstn_p);
	conf_pp_registration(key_set_head, lstn_p);
	conf_sac_registration(key_set_head, lstn_p);
	conf_scg_registration(key_set_head, lstn_p);
	conf_sds_registration(key_set_head, lstn_p);
	conf_tp_registration(key_set_head, lstn_p);
	conf_twc_registration(key_set_head, lstn_p);

	/*
	 * dev registration
	 */
	conf_rgdev_registration(key_set_head, lstn_p);

	/*
	 * rw registration
	 */
	conf_mrw_registration(key_set_head, lstn_p);
	conf_drw_registration(key_set_head, lstn_p);
	conf_sarw_registration(key_set_head, lstn_p);

	/*
	 * rep registration
	 */
	conf_rept_registration(key_set_head, lstn_p);
	conf_reps_registration(key_set_head, lstn_p);
}
