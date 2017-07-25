#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "bwcg_if.h"
#include "ini_if.h"
#include "server.h"

static struct bwcg_setup bwcg_setup;
static struct bwcg_interface ut_bwcg;

int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;

	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.keys_desc = server_keys;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);

	nid_log_open();
	nid_log_info("bwcg ut module start ...");

	bwcg_setup.ini = &ini_p;
	bwcg_initialization(&ut_bwcg, &bwcg_setup);

	nid_log_info("bwcg ut module end...");
	nid_log_close();

	return 0;
}
