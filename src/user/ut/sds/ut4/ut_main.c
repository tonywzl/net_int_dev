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

#define SDS_DSEQ_MAX		8*1024*1024*1024L
#define SDS_REQ_SIZE_MAX	8*1024*1024
#define SDS_REQ_SIZE_MIN	1
#define SDS_BUF_REQ_NUM		1024

#define SDS_TEST_FILE	"//sds.dat"

#define	ACTION_GEN_DATA	1
#define	ACTION_GEN_CTRL	2
#define	ACTION_GET_CTRL 3

static struct sds_setup sds_setup;
static struct sds_interface ut_sds;
static struct allocator_setup setup_alloc;
static struct pp_setup setup_pp;
static struct allocator_interface allocator_p;
static struct pp_interface      pp_p;

static int ctrl_ds, data_ds;

struct req_info {
        uint64_t seq;
        uint64_t dseq;
        uint32_t dlen;
        uint8_t dummy[12];
}__attribute__ ((__packed__));

#define MSG_CTRL_SIZE sizeof(struct req_info)

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
gen_req(struct req_info *req_p)
{
	static uint64_t req_seq = 0;
	static uint64_t req_dseq = 0;
	uint32_t dlen;

	dlen = rand_range(SDS_REQ_SIZE_MIN, SDS_REQ_SIZE_MAX);
	req_p->dlen = dlen;
	req_p->dseq = req_dseq;
	req_p->seq = req_seq;
	req_seq++;
	req_dseq += dlen;
	if(req_dseq >= SDS_DSEQ_MAX) {
		return -1;
	} else {
		return 0;
	}
}

static int
get_data(off_t doff, char *buf, uint32_t buf_len)
{
	ssize_t dlen = -1;

	int fd = open(SDS_TEST_FILE, O_RDONLY, 0x600);
	if (fd < 0) {
		nid_log_error("cannot open %s", SDS_TEST_FILE);
		return -1;
	}
	off_t rc = lseek(fd, doff, SEEK_SET);
	if(rc == -1) {
		nid_log_error("failed to seek file with errno: %d", errno);
		goto out;
	}
	dlen = read(fd, buf, buf_len);
	if((uint32_t)dlen != buf_len) {
		nid_log_error("failed to read file with errno: %d", errno);
		goto out;
	}

out:
	close(fd);
	return dlen;
}

static int32_t
gen_action(void)
{
	return rand_range(ACTION_GEN_DATA, ACTION_GET_CTRL);
}

int
main()
{
	char *p;
	uint32_t to_read, dlen, stop_gen_req = 0;
	uint64_t doffset = 0;
	char *pbuf;
	struct list_head req_in_head;
	struct list_element *node_p, *node1_p;
	int rc;

	to_read = 0;
	nid_log_open();
	nid_log_info("sds ut module start ...");

	INIT_LIST_HEAD(&req_in_head);
	p = calloc(1, SDS_REQ_SIZE_MAX);
	assert(p != NULL);

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

	ctrl_ds = ut_sds.d_op->d_create_worker(&ut_sds, NULL, 1, NULL);
	data_ds = ut_sds.d_op->d_create_worker(&ut_sds, NULL, 1, NULL);
	assert(ctrl_ds >= 0 && data_ds >= 0);
	nid_log_warning ("create worker finished");

	while(1) {
		struct req_info *req_p;
		uint32_t do_action = gen_action();

		switch(do_action) {
		case ACTION_GEN_DATA:
			// simulate receive data by data stream
			pbuf = ut_sds.d_op->d_get_buffer(&ut_sds, data_ds, &to_read, NULL);
			assert(pbuf != NULL);
			dlen = rand_range(SDS_REQ_SIZE_MIN, SDS_REQ_SIZE_MAX);
			if(to_read < dlen) {
				dlen = to_read;
			}
			if(doffset + dlen <= SDS_DSEQ_MAX) {
				rc = get_data(doffset, pbuf, dlen);
				if(rc == -1 || (uint32_t)rc != dlen) {
					nid_log_warning ("faile to read test file");
					assert(0);
				}
				ut_sds.d_op->d_confirm(&ut_sds, data_ds, dlen);
				doffset += dlen;
			}

			break;

		case ACTION_GEN_CTRL:
			// simulate receive ctrol message by ctrl stream
			if(!stop_gen_req) {
				pbuf = ut_sds.d_op->d_get_buffer(&ut_sds, ctrl_ds, &to_read, NULL);
				assert(pbuf != NULL);
				assert(to_read >= MSG_CTRL_SIZE);
				dlen = MSG_CTRL_SIZE;
				req_p = (struct req_info *)pbuf;
				rc = gen_req(req_p);
				if(rc == -1) {
					stop_gen_req = 1;
					nid_log_warning ("error in generate control message");
					continue;
				}
				nid_log_warning ("receive %lu control message with dseq %lu, dlen %u",
						req_p->seq, req_p->dseq, req_p->dlen);
				ut_sds.d_op->d_confirm(&ut_sds, ctrl_ds, dlen);

				// add req to control message queue
				node_p = calloc(1, sizeof(*node_p));
				node_p->data = pbuf;
				list_add_tail(&node_p->list, &req_in_head);
			}
			break;
		case ACTION_GET_CTRL:
			// simulate process control message
			if(list_empty(&req_in_head)) {
				if(stop_gen_req) {
					goto cleanup;
				}
				continue;
			}

			node_p = list_first_entry(&req_in_head, struct list_element, list);
			req_p = node_p->data;
			rc = ut_sds.d_op->d_ready(&ut_sds, data_ds, req_p->dseq + req_p->dlen);
			if(!rc) {
				nid_log_warning (" %lu control message data dseq %lu, dlen %u is not ready",
						req_p->seq, req_p->dseq, req_p->dlen);
				continue;
			}
			nid_log_warning (" %lu control message data dseq %lu, dlen %u is ready",
					req_p->seq, req_p->dseq, req_p->dlen);
			// delete control message from req queue
			list_del(&node_p->list);
			free(node_p);
			// get data associated with control message
			dlen = req_p->dlen;
			pbuf = ut_sds.d_op->d_position_length(&ut_sds, data_ds, req_p->dseq, &dlen);
			assert(pbuf != NULL);
			if(dlen != req_p->dlen) {
				rc = get_data(req_p->dseq, p, dlen);
				if(rc == -1 || (uint32_t)rc != dlen) {
					nid_log_warning ("faile to read test file");
					assert(0);
				}
				rc = bcmp(pbuf, p, dlen);
				if(rc) {
					nid_log_warning ("sds data currupt");
					assert(0);
				}
				ut_sds.d_op->d_put_buffer2(&ut_sds, data_ds, pbuf, dlen, NULL);

				uint64_t doff = req_p->dseq + dlen;
				dlen = req_p->dlen - dlen;
				rc = get_data(doff, p, dlen);
				if(rc == -1 || (uint32_t)rc != dlen) {
					nid_log_warning ("faile to read test file");
					assert(0);
				}
				pbuf = ut_sds.d_op->d_position_length(&ut_sds, data_ds, doff, &dlen);
				assert(pbuf != NULL);
				rc = bcmp(pbuf, p, dlen);
				if(rc) {
					nid_log_warning ("sds data currupt");
					assert(0);
				}
				ut_sds.d_op->d_put_buffer2(&ut_sds, data_ds, pbuf, dlen, NULL);
			} else {
				rc = get_data(req_p->dseq, p, dlen);
				if(rc == -1 || (uint32_t)rc != dlen) {
					nid_log_warning ("faile to read test file");
					assert(0);
				}
				rc = bcmp(pbuf, p, dlen);
				if(rc) {
					nid_log_warning ("sds data currupt");
					assert(0);
				}
				ut_sds.d_op->d_put_buffer2(&ut_sds, data_ds, pbuf, dlen, NULL);
			}
			break;
		}
	}

cleanup:
	list_for_each_entry_safe(node_p, node1_p, struct list_element, &req_in_head, list) {
		list_del(&node_p->list);
		free(node_p);
	}

	ut_sds.d_op->d_cleanup(&ut_sds, data_ds, NULL);
	ut_sds.d_op->d_cleanup(&ut_sds, ctrl_ds, NULL);
	nid_log_info("sds ut module end...");
	nid_log_close();

	return 0;
}
