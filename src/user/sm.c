/*
 * sm.c
 * 	Implementation of Service Manager Module
 */
#include "nid_log.h"
#include "list.h"
#include "ini_if.h"
#include "sm_if.h"

struct sm_private {
	struct ini_interface	**p_ini;
};

static void
sm_display(struct sm_interface *sm_p)
{
	struct sm_private *priv_p = (struct sm_private *)sm_p->s_private;
	struct ini_interface *ini_p = *(priv_p->p_ini);
	struct ini_section_content *sc_p;
	struct ini_key_content *the_kc;

	ini_p->i_op->i_rollback(ini_p);
	nid_log_debug("sm_display: start ...");
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		nid_log_debug("[%s]", sc_p->s_name);
		list_for_each_entry(the_kc, struct ini_key_content, &sc_p->s_head, k_list) {
			ini_p->i_op->i_display_key(ini_p, the_kc);
		}
	}
	nid_log_debug("sm_display: end");
}

struct sm_operations sm_op = {
	.s_display = sm_display,
};

int
sm_initialization(struct sm_interface *sm_p, struct sm_setup *setup)
{
	struct sm_private *priv_p;

	nid_log_info("sm_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	sm_p->s_private = priv_p;
	priv_p->p_ini = setup->ini;
	sm_p->s_op = &sm_op;
	return 0;
}
