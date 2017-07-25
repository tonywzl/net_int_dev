#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "nid_log.h"
#include "io_if.h"
#include "rio_if.h"
#include "bio_if.h"
#include "bfe_if.h"
#include "bse_if.h"
#include "pp_if.h"

static struct io_setup io_setup;
static struct io_interface ut_io;

int
main()
{
	struct pp_interface *pp_p;
	struct pp_setup pp_setup;
	int chan_index;
	uint32_t poolsz, pagesz;

	nid_log_open();
	nid_log_info("io ut module start ...");

	pp_p = calloc(1, sizeof(*pp_p));
	pp_setup.page_size = 4;
	pp_setup.pool_size = 256;
	pp_initialization(pp_p, &pp_setup);

	io_setup.io_type = IO_TYPE_BUFFER;
	io_setup.pool = pp_p;
	io_initialization(&ut_io, &io_setup);

	chan_index = ut_io.io_op->io_create_worker(&ut_io, "exportname", 1);
	poolsz = ut_io.io_op->io_get_poolsz(&ut_io);
	pagesz = ut_io.io_op->io_get_pagesz(&ut_io);
	nid_log_debug("poolsz:%d, pagesz:%d, chan_index:%d", poolsz, pagesz, chan_index);

	chan_index = ut_io.io_op->io_create_worker(&ut_io, "exportname", 1);
	poolsz = ut_io.io_op->io_get_poolsz(&ut_io);
	nid_log_debug("poolsz:%d, ipagesz:%d, chan_index:%d", poolsz, pagesz, chan_index);

	nid_log_info("io ut module end...");
	nid_log_close();

	return 0;
}
