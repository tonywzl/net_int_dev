#include <string.h>
#include "nid_log.h"
#include "ini_if.h"
#include "rsg_if.h"
#include "nid.h"

static struct rsg_setup rsg_setup;
static struct rsg_interface ut_rsg;

struct rs_interface;
struct rs_setup;

struct ini_interface;
struct ini_setup;
int
ini_initialization(struct ini_interface *ini_p, struct ini_setup *setup)
{
	assert(ini_p && setup);
	return 0;
}

struct rsun_interface;
struct rsun_setup;
int
rsun_initialization(struct rsun_interface *rsun_p, struct rsun_setup *setup)
{
	assert(rsun_p && setup);
	return 0;

}

int
rs_initialization(struct rs_interface *rs_p, struct rs_setup *setup)
{
	assert(rs_p && setup);
	return 0;
}

void *
rs_dwc_unit_mblock_parser(struct rs_interface *rs_p, void *mblock)
{
	assert(rs_p && mblock);
	return  NULL;
}

int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct list_head conf_key;

	INIT_LIST_HEAD(&conf_key);
	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.key_set = conf_key;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);

	nid_log_open();
	nid_log_info("rsg ut module start ...");

	rsg_setup.ini = ini_p;
	rsg_initialization(&ut_rsg, &rsg_setup);

	nid_log_info("rsg ut module end ...");
	nid_log_close();

	return 0;
}
