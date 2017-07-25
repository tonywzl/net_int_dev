#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "io_if.h"
#include "rio_if.h"
#include "bio_if.h"

static struct io_setup io_setup;
static struct io_interface ut_io;

int
rio_initialization(struct rio_interface *rio_p, struct rio_setup *setup)
{
	assert(rio_p && setup);
	return 0;
}

int
bio_initialization(struct bio_interface *bio_p, struct bio_setup *setup)
{
	assert(bio_p && setup);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("io ut module start ...");

	io_initialization(&ut_io, &io_setup);

	nid_log_info("io ut module end...");
	nid_log_close();

	return 0;
}
