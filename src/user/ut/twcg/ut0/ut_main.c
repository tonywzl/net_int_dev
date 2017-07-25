#include <string.h>

#include "nid.h"
#include "nid_log.h"
#include "twcg_if.h"
#include "ini_if.h"
#include "server.h"

static struct twcg_setup twcg_setup;
static struct twcg_interface ut_twcg;

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
	nid_log_info("twcg ut module start ...");

	twcg_setup.ini = &ini_p;
	twcg_initialization(&ut_twcg, &twcg_setup);

	nid_log_info("twcg ut module end...");
	nid_log_close();

	return 0;
}
