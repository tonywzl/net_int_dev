#include "lstn_if.h"
#include "ini_if.h"

#define DESCRIPTION_TYPE		"Server global module."
#define DESCRIPTION_SUPPORT_BIO		"Whether support Buffer IO Module."
#define DESCRIPTION_SUPPORT_CDS		"Whether support Cycle Data Stream Module."
#define DESCRIPTION_SUPPORT_CRC		"Whether support CAS (Content-Addressed Storage) Read Cache Module."
#define DESCRIPTION_SUPPORT_CRCC	"Whether support CRC Channel Module."
#define DESCRIPTION_SUPPORT_DRC		"Whether support Distributed Read Cache Module."
#define DESCRIPTION_SUPPORT_DRW		"Whether support Device Read Write Module."
#define DESCRIPTION_SUPPORT_DRWC	"Whether support Device Read Write Channel Module."
#define DESCRIPTION_SUPPORT_DOAC	"Whether support Delegation of Authority Channel Module."
#define DESCRIPTION_SUPPORT_CXA		"Whether support Control Exchange Active Module."
#define DESCRIPTION_SUPPORT_CXAC	"Whether support Control Exchange Active Channel Module."
#define DESCRIPTION_SUPPORT_CXP		"Whether support Control Exchange Passive Module."
#define DESCRIPTION_SUPPORT_CXPC	"Whether support Control Exchange Passive Channel Module."
#define DESCRIPTION_SUPPORT_CXT		"Whether support Data Exchange Transfer Module."
#define DESCRIPTION_SUPPORT_DXA		"Whether support Data Exchange Active Module."
#define DESCRIPTION_SUPPORT_DXAC	"Whether support Data Exchange Active Channel Module."
#define DESCRIPTION_SUPPORT_DXP		"Whether support Data Exchange Passive Module."
#define DESCRIPTION_SUPPORT_DXPC	"Whether support Data Exchange Passive Channel Module."
#define DESCRIPTION_SUPPORT_DXT		"Whether support Data Exchange Transfer Module."
#define DESCRIPTION_SUPPORT_MRW		"Whether support Meta Server Read Write Module."
#define DESCRIPTION_SUPPORT_MRWC	"Whether support Meta Server Read Write Channel Module."
#define DESCRIPTION_SUPPORT_BWC		"Whether support Writeback Write Cache Module."
#define DESCRIPTION_SUPPORT_BWCC	"Whether support Writeback Write Cache Channel Module."
#define DESCRIPTION_SUPPORT_TP		"Whether support Thread Pool Module."
#define DESCRIPTION_SUPPORT_TWCC	"Whether support Write Through Cache Channel Module."
#define DESCRIPTION_SUPPORT_TWC		"Whether support Write Through Cache Module."
#define DESCRIPTION_SUPPORT_RCC		"Whether support Read Cache Channel Module."
#define DESCRIPTION_SUPPORT_RIO		"Whether support Resource IO Module."
#define DESCRIPTION_SUPPORT_SAC		"Whether support Server Agent Channel Module."
#define DESCRIPTION_SUPPORT_SACC	"Whether support Server Agent Channel Channel Module."
#define DESCRIPTION_SUPPORT_SDSC	"Whether support Split Data Stream Channel Module."
#define DESCRIPTION_SUPPORT_PPC		"Whether support Page Pool Channel Module."
#define DESCRIPTION_SUPPORT_SDS		"Whether support Split Data Stream Module."
#define DESCRIPTION_SUPPORT_SMC		"Whether support Server Manager Channel Module."
#define DESCRIPTION_SUPPORT_WCC		"Whether support Write Cache Channel Module."
#define DESCRIPTION_SUPPORT_TPC		"Whether support Thread Pool Channel Module."
#define DESCRIPTION_SUPPORT_INIC	"Whether support ini Channel Module."
#define DESCRIPTION_SUPPORT_RS		"Whether support Resource Module."
#define DESCRIPTION_SUPPORT_RSM		"Whether support Resource Stream Manager Module."
#define DESCRIPTION_SUPPORT_REPT	"Whether support Replication Target Module."
#define DESCRIPTION_SUPPORT_REPS	"Whether support Replication Source Module."
#define DESCRIPTION_SUPPORT_REPTC	"Whether support Replication Target Channel Module."
#define DESCRIPTION_SUPPORT_REPSC	"Whether support Replication Source Channel Module."

#define DESCRIPTION_LOG_LEVEL       "log level."

static int d_support_bio = 0;		// default not support bio
static int d_support_cds = 1;		// default support cds
static int d_support_crc = 0;		// default not support crc: content read cache
static int d_support_crcc = 0;		// default not support crcc
static int d_support_drc = 0;		// default not support drc
static int d_support_drw = 1;		// default support drw
static int d_support_drwc = 1;		// default support drwc
static int d_support_cxa = 0;		// default not support cxa
static int d_support_cxac = 0;		// default not support cxac
static int d_support_cxp = 0;		// default not support cxp
static int d_support_cxpc = 0;		// default not support cxpc
static int d_support_cxt = 0;		// default not support cxt
static int d_support_dxa = 0;		// default not support dxa
static int d_support_dxac = 0;		// default not support dxac
static int d_support_dxp = 0;		// default not support dxp
static int d_support_dxpc = 0;		// default not support dxpc
static int d_support_dxt = 0;		// default not support dxt
static int d_support_doac = 0;		// default not support doac
static int d_support_mrw = 0;		// default not support mrw
static int d_support_mrwc = 0;		// default not support mrwc
static int d_support_bwc = 0;		// default not support bwc
static int d_support_bwcc = 0;		// default not support bwcc
static int d_support_rcc = 0;		// default not support rcc
static int d_support_rio = 1;		// default support rio
static int d_support_sac = 1;		// default support sac
static int d_support_sacc = 1;		// default support sacc
static int d_support_sdsc = 1;		// default support dsc
static int d_support_ppc = 1;		// default support ppc
static int d_support_sds = 1;		// default support sds
static int d_support_smc = 1;		// default support smc
static int d_support_wcc = 0;		// default not support wcc
static int d_support_tp = 1;		// default to support tp
static int d_support_twc = 0;		// default not to support twc
static int d_support_twcc = 0;		// default not to support twcc
static int d_support_tpc = 0;		// default not to support tpc
static int d_support_inic = 0;		// default not to support inic
static int d_support_rs = 0;		// default not to support rs
static int d_support_rsm = 0;		// default not to support rsm
static int d_support_rept = 0;		// default not to support rept
static int d_support_reps = 0;		// default not to support reps
static int d_support_reptc = 0;		// default not to support reptc
static int d_support_repsc = 0;		// default not to support repsc

#ifdef DEBUG_LEVEL
static int d_log_level = DEBUG_LEVEL;
#else
static int d_log_level = 4;
#endif

struct ini_key_desc global_keys[] = {
	{"type",		KEY_TYPE_STRING,	"global",		1,	DESCRIPTION_TYPE},
	{"support_bio",		KEY_TYPE_INT,		&d_support_bio,		0,	DESCRIPTION_SUPPORT_BIO},
	{"support_cds",		KEY_TYPE_INT,		&d_support_cds,		0,	DESCRIPTION_SUPPORT_CDS},
	{"support_crc",		KEY_TYPE_INT,		&d_support_crc,		0,	DESCRIPTION_SUPPORT_CRC},
	{"support_crcc",	KEY_TYPE_INT,		&d_support_crcc,	0,	DESCRIPTION_SUPPORT_CRCC},
	{"support_drc",		KEY_TYPE_INT,		&d_support_drc,		0,	DESCRIPTION_SUPPORT_DRC},
	{"support_drw",		KEY_TYPE_INT,		&d_support_drw,		0,	DESCRIPTION_SUPPORT_DRW},
	{"support_drwc",	KEY_TYPE_INT,		&d_support_drwc,	0,	DESCRIPTION_SUPPORT_DRWC},
	{"support_doac",	KEY_TYPE_INT,		&d_support_doac,	0,	DESCRIPTION_SUPPORT_DOAC},
	{"support_cxa",		KEY_TYPE_INT,		&d_support_cxa,		0,	DESCRIPTION_SUPPORT_CXA},
	{"support_cxac",	KEY_TYPE_INT,		&d_support_cxac,	0,	DESCRIPTION_SUPPORT_CXAC},
	{"support_cxp",		KEY_TYPE_INT,		&d_support_cxp,		0,	DESCRIPTION_SUPPORT_CXP},
	{"support_cxpc",	KEY_TYPE_INT,		&d_support_cxpc,	0,	DESCRIPTION_SUPPORT_CXPC},
	{"support_cxt",		KEY_TYPE_INT,		&d_support_cxt,		0,	DESCRIPTION_SUPPORT_CXT},
	{"support_dxa",		KEY_TYPE_INT,		&d_support_dxa,		0,	DESCRIPTION_SUPPORT_DXA},
	{"support_dxac",	KEY_TYPE_INT,		&d_support_dxac,	0,	DESCRIPTION_SUPPORT_DXAC},
	{"support_dxp",		KEY_TYPE_INT,		&d_support_dxp,		0,	DESCRIPTION_SUPPORT_DXP},
	{"support_dxpc",	KEY_TYPE_INT,		&d_support_dxpc,	0,	DESCRIPTION_SUPPORT_DXPC},
	{"support_dxt",		KEY_TYPE_INT,		&d_support_dxt,		0,	DESCRIPTION_SUPPORT_DXT},
	{"support_mrw",		KEY_TYPE_INT,		&d_support_mrw,		0,	DESCRIPTION_SUPPORT_MRW},
	{"support_mrwc",	KEY_TYPE_INT,		&d_support_mrwc,	0,	DESCRIPTION_SUPPORT_MRWC},
	{"support_bwc",		KEY_TYPE_INT,		&d_support_bwc,		0,	DESCRIPTION_SUPPORT_BWC},
	{"support_bwcc",	KEY_TYPE_INT,		&d_support_bwcc,	0,	DESCRIPTION_SUPPORT_BWCC},
	{"support_tp",		KEY_TYPE_INT,		&d_support_tp,		0,	DESCRIPTION_SUPPORT_TP},
	{"support_twcc",        KEY_TYPE_INT,           &d_support_twcc,        0,	DESCRIPTION_SUPPORT_TWCC},
	{"support_twc",         KEY_TYPE_INT,           &d_support_twc,         0,	DESCRIPTION_SUPPORT_TWC},
	{"support_rcc",		KEY_TYPE_INT,		&d_support_rcc,		0,	DESCRIPTION_SUPPORT_RCC},
	{"support_rio",		KEY_TYPE_INT,		&d_support_rio,		0,	DESCRIPTION_SUPPORT_RIO},
	{"support_sac",		KEY_TYPE_INT,		&d_support_sac,		0,	DESCRIPTION_SUPPORT_SAC},
	{"support_sacc",	KEY_TYPE_INT,		&d_support_sacc,	0,	DESCRIPTION_SUPPORT_SACC},
	{"support_sdsc",	KEY_TYPE_INT,		&d_support_sdsc,	0,	DESCRIPTION_SUPPORT_SDSC},
	{"support_ppc",		KEY_TYPE_INT,		&d_support_ppc,		0,	DESCRIPTION_SUPPORT_PPC},
	{"support_sds",		KEY_TYPE_INT,		&d_support_sds,		0,	DESCRIPTION_SUPPORT_SDS},
	{"support_smc",		KEY_TYPE_INT,		&d_support_smc,		0,	DESCRIPTION_SUPPORT_SMC},
	{"support_wcc",		KEY_TYPE_INT,		&d_support_wcc,		0,	DESCRIPTION_SUPPORT_WCC},
	{"support_tpc",		KEY_TYPE_INT,		&d_support_tpc,		0,	DESCRIPTION_SUPPORT_TPC},
	{"support_inic",	KEY_TYPE_INT,		&d_support_inic,	0,	DESCRIPTION_SUPPORT_INIC},
	{"support_rs",		KEY_TYPE_INT,		&d_support_rs,		0,	DESCRIPTION_SUPPORT_RS},
	{"support_rsm",		KEY_TYPE_INT,		&d_support_rsm,		0,	DESCRIPTION_SUPPORT_RSM},
	{"support_rept",	KEY_TYPE_INT,		&d_support_rept,	0,	DESCRIPTION_SUPPORT_REPT},
	{"support_reps",	KEY_TYPE_INT,		&d_support_reps,	0,	DESCRIPTION_SUPPORT_REPS},
	{"support_reptc",	KEY_TYPE_INT,		&d_support_reptc,	0,	DESCRIPTION_SUPPORT_REPTC},
	{"support_repsc",	KEY_TYPE_INT,		&d_support_repsc,	0,	DESCRIPTION_SUPPORT_REPSC},
    {"log_level",       KEY_TYPE_INT,       &d_log_level,       0,  DESCRIPTION_LOG_LEVEL},
	{NULL,			KEY_TYPE_WRONG,		NULL,			0,	NULL}
};

void
conf_global_registration(struct list_head *key_set_head, struct lstn_interface *lstn_p)
{
	struct list_node *lnp;

	lnp = lstn_p->ln_op->ln_get_node(lstn_p);
	lnp->ln_data = (void *)global_keys;
	list_add_tail(&lnp->ln_list, key_set_head);
}
