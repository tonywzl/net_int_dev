#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <errno.h>

#include "nid_log.h"
#include "sds_if.h"
#include "allocator_if.h"
#include "pp_if.h"

#define	SDS_BUF_REQ_NUM	1024

#define SDS_TEST_FILE	"//sds.dat"

static struct sds_setup sds_setup;
static struct sds_interface ut_sds;
static struct allocator_setup setup_alloc;
static struct pp_setup setup_pp;
static struct allocator_interface allocator_p;
static struct pp_interface      pp_p;

struct buf_info {
	char *buf;
	uint32_t	blen;
};

/*
** return a random integer in the interval
** [a, b]
*/
static uint32_t
rand_range(uint32_t a, uint32_t b) 
{
	static uint32_t g_is_first = 1;

	if (g_is_first) {
		g_is_first = 0;
		srand((unsigned int)time(NULL));
	}

	return (uint32_t)((double)rand() / ((RAND_MAX + 1.0) / (b - a + 1.0)) + a);
}

static int
get_data(char *buf, uint32_t buf_len)
{
	ssize_t dlen;
	int fd = open(SDS_TEST_FILE, O_RDONLY, 0x600);
	if (fd < 0) {
		nid_log_error("cannot open %s", SDS_TEST_FILE);
		return -1;
	}

	dlen = read(fd, buf, buf_len);
	close(fd);
	return dlen;
}

int
main()
{
	struct buf_info p[SDS_BUF_REQ_NUM];
	uint32_t to_read, run_cycle = 0;
	char *dbuf;
	int i, nread;

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

	dbuf = malloc(10*1024*1024);
	if(!dbuf) {
		nid_log_warning ("failed to malloc memory, errno = %d", errno);
	}

	memset(p, 0, sizeof(p));
	while(1) {
		for(i = 0; i < SDS_BUF_REQ_NUM; i++) {
			char *pbuf = ut_sds.d_op->d_get_buffer(&ut_sds, 0, &to_read, NULL);
			if(pbuf) {
				p[i].buf = pbuf;
				p[i].blen = rand_range(1, to_read);
				nread = get_data(dbuf, p[i].blen);
				if(nread < 0 || (uint32_t)nread != p[i].blen) {
					assert(0);
				}
				memcpy(p[i].buf, dbuf, nread);
				ut_sds.d_op->d_confirm(&ut_sds, 0, p[i].blen);
				nid_log_warning ("get %d memory %p of len %u", i, p[i].buf, p[i].blen);
			}
		}

		for(i = 0; i < SDS_BUF_REQ_NUM; i++) {
			if(p[i].buf) {
				nread = get_data(dbuf, p[i].blen);
				if(nread < 0 || (uint32_t)nread != p[i].blen) {
					assert(0);
				}
				if(memcmp(dbuf, p[i].buf, p[i].blen)) {
					assert(0);
				}
				ut_sds.d_op->d_put_buffer2(&ut_sds, 0, (void *)p[i].buf, p[i].blen, NULL);
				nid_log_warning ("put %d memory %p of len %u", i, p[i].buf, p[i].blen);
				p[i].buf = NULL;
				p[i].blen = 0;
			}
		}

		run_cycle++;
		if(run_cycle == 1000)
			break;
		sleep(1);
	}

	nid_log_info("sds ut module end...");
	nid_log_close();

	return 0;
}
