#include <string.h>
#include "nid_log.h"
#include "ini_if.h"
#include "rsmg_if.h"
#include "rsm_if.h"
#include "allocator_if.h"
#include "nid.h"

static struct rsmg_setup rsmg_setup;
static struct rsmg_interface ut_rsmg;

int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct allocator_interface allocator_p;
	struct list_head conf_key;

	INIT_LIST_HEAD(&conf_key);
	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_SERVICE;
	ini_setup.key_set = conf_key;
	ini_p = calloc(1, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);

	nid_log_open();
	nid_log_info("rsmg ut module start ...");

	rsmg_setup.ini = ini_p;
	rsmg_setup.allocator = &allocator_p;
	rsmg_initialization(&ut_rsmg, &rsmg_setup);

	nid_log_info("rsmg ut module end ...");
	nid_log_close();

	return 0;
}
