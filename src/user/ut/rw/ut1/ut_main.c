#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include  <sys/uio.h>

#include "nid_log.h"
#include "rw_if.h"
#include "allocator_if.h"
#include "fpc_if.h"
#include "fpn_if.h"


static struct allocator_interface   ut_al;
static struct fpn_interface         ut_fpn;
static struct rw_interface ut_rw;
static void *rw_handle;

static void
ut_rw_init_env ()
{
	struct allocator_setup ut_rc_al_setup;
	ut_rc_al_setup.a_role = ALLOCATOR_ROLE_SERVER;
	allocator_initialization(&ut_al, &ut_rc_al_setup);

	struct fpn_setup fpn_setup;
	fpn_setup.allocator = &ut_al;
	fpn_setup.fp_size = 32;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(&ut_fpn, &fpn_setup);

	struct rw_setup rw_setup;
	rw_setup.name = "rw_name";
	rw_setup.exportname = "/wc_device";
	rw_setup.allocator = &ut_al;
	rw_setup.fpn_p = &ut_fpn;
	rw_setup.type = RW_TYPE_DEVICE;
	rw_setup.device_provision = 0;
	rw_setup.simulate_async = 0;
	rw_setup.simulate_delay = 0;
	rw_setup.simulate_delay_min_gap = 0;
	rw_setup.simulate_delay_max_gap = 0;
	rw_setup.simulate_delay_time_us = 0;

	rw_initialization(&ut_rw, &rw_setup);
	rw_handle = ut_rw.rw_op->rw_create_worker(&ut_rw, "/wc_device", (char)1, (char)1, 4096, 0);
}

struct data_desc {
	off_t 	off;
	struct iovec *iov;
	int iov_cnt;
};

struct iovec testiov[] = {
		{
				.iov_base = "hello0#",
				.iov_len = 7,
		},
		{
				.iov_base = "hello01@@@",
				.iov_len = 10,
		},
		{
				.iov_base = "hello02#",
				.iov_len = 8,
		},
		{
				.iov_base = "hell3$",
				.iov_len = 6
		},
		{
				.iov_base = "hello",
				.iov_len = 5
		},
};

int testdata_cnt = 11;
struct data_desc testdata[] = {
		{
				.off = 4090,
				.iov = &testiov[0],
				.iov_cnt = 1,
		},
		{
				.off = 4090 + 7,
				.iov = &testiov[1],
				.iov_cnt = 1,
		},
		{
				.off = 4090 + 7 + 10,
				.iov = &testiov[2],
				.iov_cnt = 1,
		},
		{
				.off = 4090 + 7 + 10 + 8,
				.iov = &testiov[3],
				.iov_cnt = 1,
		},
		{
				.off = 4090 + 7 + 10 + 8 + 6,
				.iov = &testiov[4],
				.iov_cnt = 1,
		},
		{
				.off = 8050,
				.iov = testiov,
				.iov_cnt = 5

		},
		{
				.off = 8080,
				.iov = testiov,
				.iov_cnt = 5
		},
		{
				.off = 12094,
				.iov = testiov,
				.iov_cnt = 5
		},
		{
				.off = 16095,
				.iov = testiov,
				.iov_cnt = 5
		},
		{
				.off = 20096,
				.iov = testiov,
				.iov_cnt = 5
		},
		{
				.off = 25097,
				.iov = testiov,
				.iov_cnt = 5
		},
};


void
ut_do_rw_test()
{
	int i = 0, j;
	char *buf, buffer[1024], retbuffer[1024];
	size_t buffer_len;
	struct list_head fp_head;
	struct fp_node *fphd;
	INIT_LIST_HEAD(&fp_head);
	void *align_buf;
	size_t count_align;
	off_t offset_align;
	struct fpn_interface *fpn_p = ut_rw.rw_op->rw_get_fpn_p(&ut_rw);
	ssize_t ret;

	for (i = 0; i < testdata_cnt; i++) {
		buffer_len = 0;
		buf = buffer;
		for (j=0; j<testdata[i].iov_cnt; j++) {
			memcpy(buf, testdata[i].iov[j].iov_base, testdata[i].iov[j].iov_len);
			buffer_len += testdata[i].iov[j].iov_len;
			buf += testdata[i].iov[j].iov_len;
		}

		ret = ut_rw.rw_op->rw_pwritev_fp(&ut_rw, rw_handle, testdata[i].iov, testdata[i].iov_cnt, testdata[i].off, &fp_head, &align_buf, &count_align, &offset_align);
		printf("ROUND:%d :: writelen:%ld off:%ld len:%lu align_off:%ld, count_align:%lu\n", i, ret, testdata[i].off, buffer_len, offset_align, count_align);
		fflush(stdout);
		assert((size_t)ret == buffer_len);
		if (align_buf != NULL) free(align_buf);
		while (!list_empty(&fp_head)) {
			fphd = list_first_entry(&fp_head, struct fp_node, fp_list);
			list_del(&fphd->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fphd);
		}

		ret = ut_rw.rw_op->rw_pread_fp(&ut_rw, rw_handle, retbuffer, buffer_len, testdata[i].off, &fp_head, &align_buf, &count_align, &offset_align);
		printf("ROUND:%d, readlen:%ld off:%ld len:%lu align_off:%ld, count_align:%lu\n", i, ret, testdata[i].off, buffer_len, offset_align, count_align);
		buffer[buffer_len] = '\0'; retbuffer[buffer_len] = '\0';
		printf("B1:%s\n", buffer);
		printf("B2:%s\n", retbuffer);
		fflush(stdout);

		assert(memcmp(buffer, retbuffer, buffer_len) == 0);
		assert((size_t)ret == buffer_len);
		if (align_buf != NULL) free(align_buf);
		while (!list_empty(&fp_head)) {
			fphd = list_first_entry(&fp_head, struct fp_node, fp_list);
			list_del(&fphd->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fphd);
		}
	}

	ret = ut_rw.rw_op->rw_pread_fp(&ut_rw, rw_handle, retbuffer, buffer_len, 4090, &fp_head, &align_buf, &count_align, &offset_align);
	printf("ROUND:%d, readlen:%ld off:%ld len:%lu align_off:%ld, count_align:%lu\n", i, ret, testdata[i].off, buffer_len, offset_align, count_align);
	buffer[buffer_len] = '\0'; retbuffer[buffer_len] = '\0';
	printf("B1:%s\n", buffer);
	printf("B2:%s\n", retbuffer);
	fflush(stdout);

	assert(memcmp(buffer, retbuffer, buffer_len) == 0);
	assert((size_t)ret == buffer_len);
	if (align_buf != NULL) free(align_buf);
	while (!list_empty(&fp_head)) {
		fphd = list_first_entry(&fp_head, struct fp_node, fp_list);
		list_del(&fphd->fp_list);
		fpn_p->fp_op->fp_put_node(fpn_p, fphd);
	}
}

int
main()
{
	ut_rw_init_env();
	ut_do_rw_test();
	return 0;
}
