#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "cds_if.h"

static struct cds_setup cds_setup;
static struct cds_interface ut_cds;
//static struct io_interface ut_io;

int
main()
{
	char *p[10];
	uint32_t to_read;
	int i;
	to_read = 0;
	nid_log_open();
	nid_log_info("cds ut module start ...");


	strcpy(cds_setup.cds_name, "itest_pp");
	cds_setup.pagenrshift = 2;
	cds_setup.pageszshift = 23;

	cds_initialization(&ut_cds, &cds_setup);

	ut_cds.d_op->d_create_worker(&ut_cds);
	nid_log_error ("create worker finished");

	for(i = 0; i < 8; i++){
		p[i] = ut_cds.d_op->d_get_buffer(&ut_cds, 0, &to_read );
		nid_log_error("get buffer finished");
		ut_cds.d_op->d_confirm(&ut_cds, 0, 4194304);
	}
	for(i = 0; i < 8; i ++){
		ut_cds.d_op->d_put_buffer2(&ut_cds, 0, (void *)p[i], 4194304);

		nid_log_error("put buffer finished");
	}
	nid_log_info("cds ut module end...");
	nid_log_close();

	return 0;
}
