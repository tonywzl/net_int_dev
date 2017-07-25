#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "sds_if.h"
#include "allocator_if.h"
#include "pp_if.h"

#define	SDS_BUF_REQ_NUM	1024

static struct sds_setup sds_setup;
static struct sds_interface ut_sds;
static struct allocator_setup setup_alloc;
static struct pp_setup setup_pp;
static struct allocator_interface allocator_p;
static struct pp_interface	pp_p;

struct buf_info {
	char *buf;
	uint32_t	blen;
};

int
main()
{
	struct buf_info p[SDS_BUF_REQ_NUM];
	uint32_t to_read;
	int i;
	to_read = 0;
	nid_log_open();
	nid_log_info("sds ut module start ...");

	setup_alloc.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&allocator_p, &setup_alloc);

	strcpy(setup_pp.pp_name, "itest_pp");
	setup_pp.allocator = &allocator_p;
	setup_pp.set_id = 1;
	setup_pp.page_size = 8;
	setup_pp.pool_size = SDS_BUF_REQ_NUM;
	pp_initialization(&pp_p, &setup_pp);

	sds_setup.pp = &pp_p;
	sds_initialization(&ut_sds, &sds_setup);

	ut_sds.d_op->d_create_worker(&ut_sds, NULL, 1, NULL);
	nid_log_info ("create worker finished");

	memset(p, 0, sizeof(p));
	for(i = 0; i < SDS_BUF_REQ_NUM; i++) {
		char *pbuf = ut_sds.d_op->d_get_buffer(&ut_sds, 0, &to_read, NULL);
		if(pbuf) {
			p[i].buf = pbuf;
			p[i].blen = to_read;
			ut_sds.d_op->d_confirm(&ut_sds, 0, to_read);
		}
	}
	nid_log_info("get buffer finished");
	for(i = 0; i < SDS_BUF_REQ_NUM; i ++) {
		if(p[i].buf) {
			ut_sds.d_op->d_put_buffer2(&ut_sds, 0, (void *)p[i].buf, p[i].blen, NULL);
		}
	}

	nid_log_info("put buffer finished");
	nid_log_info("sds ut module end...");
	nid_log_close();

	return 0;
}
