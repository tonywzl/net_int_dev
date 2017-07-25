#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "server.h"
#include "ini_if.h"
#include "cc_if.h"
#include "ds_if.h"
#include "pp_if.h"
#include "io_if.h"
#include "wc_if.h"
#include "rc_if.h"
#include "rw_if.h"
#include "srn_if.h"
#include "sac_if.h"
#include "scg_if.h"

static struct scg_interface ut_scg;
static struct scg_setup ut_setup;


struct ini_key_desc* service_keys_ary[] = {
	NULL,
};

struct ini_section_desc service_sections[] = {
	{NULL,		-1,	NULL},
};

struct sacg_interface;
struct sacg_setup;
int
sacg_initialization(struct sacg_interface *sacg_p, struct sacg_setup *setup)
{	
	assert(sacg_p && setup);
	return 0;
}

int
ini_initialization(struct ini_interface *ini_p, struct ini_setup *setup)
{	
	nid_log_debug("ini_initialization start");
	assert(ini_p && setup);
	return 0;
}

struct tp_interface;
struct tp_setup;
int
tp_initialization(struct tp_interface *tp_p, struct tp_setup *setup)
{	
	nid_log_debug("tp_initialization start");
	assert(tp_p && setup);
	return 0;
}

struct brn_interface;
struct brn_setup;
int
brn_initialization(struct brn_interface *brn_p, struct brn_setup *setup)
{	
	assert(brn_p && setup);
	return 0;
}

int
srn_initialization(struct srn_interface *srn_p, struct srn_setup *setup)
{	
	assert(srn_p && setup);
	return 0;
}

struct fpn_interface;
struct fpn_setup;
int
fpn_initialization(struct fpn_interface *fpn_p, struct fpn_setup *setup)
{
	assert(fpn_p && setup);
	return 0;
}

int
ds_initialization(struct ds_interface *ds_p, struct ds_setup *setup)
{	
	nid_log_debug("ds_initialization start");
	assert(ds_p && setup);
	return 0;
}

int
pp_initialization(struct pp_interface *pp_p, struct pp_setup *setup)
{
	assert(pp_p && setup);
	return 0;
}

int
io_initialization(struct io_interface *io_p, struct io_setup *setup)
{
	assert(io_p && setup);
	return 0;
}

int
wc_initialization(struct wc_interface *wc_p, struct wc_setup *setup)
{
	assert(wc_p && setup);
	return 0;
}

int
rc_initialization(struct rc_interface *rc_p, struct rc_setup *setup)
{
	assert(rc_p && setup);
	return 0;
}

int
rw_initialization(struct rw_interface *rw_p, struct rw_setup *setup)
{
	assert(rw_p && setup);
	return 0;
}

int
sac_initialization(struct sac_interface *sac_p, struct sac_setup *setup)
{
	nid_log_debug("sac_initialization start");
	if (!sac_p || !setup)
		return -1;
	return 0;
}

int
cc_initialization(struct cc_interface *cc_p, struct cc_setup *setup)
{
	nid_log_debug("cc_initialization start");
	if (!cc_p || !setup)
		return -1;
	return 0;
}

struct smc_interface;
struct smc_setup;
int
smc_initialization(struct smc_interface *smc_p, struct smc_setup *setup)
{
	nid_log_debug("smc_initialization start");
	if (!smc_p || !setup)
		return -1;
	return 0;
}

struct smcg_interface;
struct smcg_setup;
int
smcg_initialization(struct smcg_interface *smcg_p, struct smcg_setup *setup)
{
	assert (smcg_p && !setup);
	return 0;
}

struct wccg_interface;
struct wccg_setup;
int
wccg_initialization(struct wccg_interface *wccg_p, struct wccg_setup *setup)
{
	assert (wccg_p && !setup);
	return 0;
}

struct bwccg_interface;
struct bwccg_setup;
int
bwccg_initialization(struct bwccg_interface *bwccg_p, struct bwccg_setup *setup)
{
	assert(bwccg_p && !setup);
	return 0;
}

struct rccg_interface;
struct rccg_setup;
int
rccg_initialization(struct rccg_interface *rccg_p, struct rccg_setup *setup)
{
	assert (rccg_p && !setup);
	return 0;
}

struct crccg_interface;
struct crccg_setup;
int
crccg_initialization(struct crccg_interface *crccg_p, struct crccg_setup *setup)
{
	assert(crccg_p && !setup);
	return 0;
}

struct doag_interface;
struct doag_setup;
int
doag_initialization(struct doag_interface *doag_p, struct doag_setup *setup)
{
	assert (doag_p && !setup);
	return 0;
}

struct doacg_interface;
struct doacg_setup;
int
doacg_initialization(struct doacg_interface *doacg_p, struct doacg_setup *setup)
{
	assert (doacg_p && !setup);
	return 0;
}

struct snapg_interface;
struct snapg_setup;
int
snapg_initialization(struct snapg_interface *snapg_p, struct snapg_setup *setup)
{
	assert (snapg_p && !setup);
	return 0;
}

struct lstn_interface;
struct lstn_setup;
int
lstn_initialization(struct lstn_interface *lstn_p, struct lstn_setup *setup)
{
	assert (lstn_p && !setup);
	return 0;
}

int
main()
{
	struct hs_interface;

	nid_log_open();
	nid_log_info("scg start ...");

	scg_initialization(&ut_scg, &ut_setup);

	nid_log_info("scg end...");
	nid_log_close();

	return 0;
}
