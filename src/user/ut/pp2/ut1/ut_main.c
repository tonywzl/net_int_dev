#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "pp2_if.h"

static struct pp2_setup pp2_setup;
static struct pp2_interface ut_pp2;
static struct allocator_setup allocator_setup;
static struct allocator_interface allocator;
int nid_log_level = LOG_WARNING;
char *log_header = "pp2 ut1";

static void *
_get_thread(void *p)
{
	struct pp2_interface *pp2_p = (struct pp2_interface *)p;
	int r;
	int approach;
	uint32_t size;
	void *buf = NULL;

	for (;;) {
		r = rand() % (8388608/4);
		approach = r % 3;
		size = ((uint32_t)r) % ((pp2_setup.page_size << 20) - 128);

		nid_log_warning("ut_main: size is %u", size);
		switch (approach) {
		case 0:
			buf = pp2_p->pp2_op->pp2_get(pp2_p, size);
			break;

		case 1:
			buf = pp2_p->pp2_op->pp2_get_nowait(pp2_p, size);
			break;

		case 2:
			buf = pp2_p->pp2_op->pp2_get_forcibly(pp2_p, size);
			break;
		}

		usleep(10000);

		if (buf) {
			pp2_p->pp2_op->pp2_put(pp2_p, buf);
		}
	}
	return NULL;
}

int
main()
{
	pthread_t thread_id;
	pthread_attr_t attr;
	int ret, i;
#ifdef DEBUG_LEVEL
	nid_log_level = DEBUG_LEVEL;
#endif
	nid_log_open();
	nid_log_info("%s module start ...", log_header);

	allocator_setup.a_role = 1;
	allocator_initialization(&allocator, &allocator_setup);
	pp2_setup.allocator = &allocator;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 128;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_setup.set_id = 1;
	pp2_initialization(&ut_pp2, &pp2_setup);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for (i = 0; i < 2; i++) {
		ret = pthread_create(&thread_id, &attr, _get_thread, (void*)&ut_pp2);
		if (ret) {
			nid_log_error("%s thread creation: pthread_create error %d\n",
					log_header, ret);
			goto out;
		}
	}

	usleep(600000000);
	nid_log_info("%s module successfully lasted for 600 seconds...", log_header);

out:
	nid_log_info("%s module end...", log_header);
	nid_log_close();

	return 0;
}
