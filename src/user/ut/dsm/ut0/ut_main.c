#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
#include "dsm_if.h"

static struct dsm_setup sw_setup;
static struct dsm_interface ut_nw;

int asc_sm_i_housekeeping(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_w_housekeeping(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_r_housekeeping(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rw_housekeeping(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_iw_housekeeping(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_xdev_delete_device(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_xdev_init_device(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_xdev_start_device(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_xdev_ioerror_device(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_xdev_keepalive(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_keepalive(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_chan_established(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_delete_device(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_recover_channel(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_iw_chan_established(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_rw_chan_established(void *data)
{	
	assert(data);
	return 0;
}

int asc_sm_rdev_ioerror_device(void *data)
{	
	assert(data);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("sm ut module start ...");

	dsm_initialization(&ut_nw, &sw_setup);

	nid_log_info("sm ut module end...");
	nid_log_close();

	return 0;
}
