#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "nid_log.h"
#include "nid.h"
#include "dsrec_if.h"
#include "fpc_if.h"
#include "cdn_if.h"
#include "allocator_if.h"


#define RC_SIZE 1000
#define TEST_TOTAL_SIZE 100000
#define TEST_COUNT 100

static struct dsrec_setup dsrec_setup;
static struct dsrec_interface ut_dsrec;
static struct fpc_setup fpc_setup;
static struct fpc_interface ut_fpc;
static struct allocator_setup alloc_setup;
static struct allocator_interface ut_alloc;
static struct cdn_setup cdn_setup;
static struct cdn_interface ut_cdn;

static struct dsrec_interface tmp_dsrec;
static struct dsrec_stat expect_dsrec_stat;



struct dsrec_private_clone {
	struct rc_interface	*p_rc;
	struct cdn_interface	*p_cdn;
	struct blksn_interface	*p_blksn;
	struct dsmgr_interface	*p_dsmgr;
	struct fpc_interface	*p_fpc;
	struct list_head	p_space_head;				// in space order
	struct list_head	p_to_reclaim_head;			// nodes to reclaim
	struct list_head	p_reclaiming_head;			// nodes being reclaimed
	struct list_head	p_lru_head[NID_SIZE_FP_WGT];		// in time order and different weight
	uint64_t		p_rc_size;				// the whole read cache size
	struct dsrec_stat	p_dsrec_stat;				// statistics info of dsrec.
	pthread_mutex_t		p_list_lck;
	pthread_cond_t		p_list_cond;
};

int
dsrec_initialization_clone(struct dsrec_interface *dsrec_p, struct dsrec_setup *setup)
{
	struct dsrec_private_clone *priv_p;
	uint8_t fp_wgt = 0;

	assert(setup);
	dsrec_p->sr_op = tmp_dsrec.sr_op;
	priv_p = calloc(1, sizeof(*priv_p));
	dsrec_p->sr_private = priv_p;

	priv_p->p_cdn = setup->cdn;
	priv_p->p_blksn = setup->blksn;
	priv_p->p_dsmgr = setup->dsmgr;
	priv_p->p_rc = setup->rc;
	priv_p->p_rc_size = setup->rc_size;
	priv_p->p_fpc = setup->fpc;
	priv_p->p_dsrec_stat.p_lru_max = (priv_p->p_rc_size * 7) / 10;
	INIT_LIST_HEAD(&priv_p->p_space_head);
	INIT_LIST_HEAD(&priv_p->p_to_reclaim_head);
	INIT_LIST_HEAD(&priv_p->p_reclaiming_head);
	for (fp_wgt=0; fp_wgt<NID_SIZE_FP_WGT; fp_wgt++){
		INIT_LIST_HEAD(&priv_p->p_lru_head[fp_wgt]);
	}

	pthread_mutex_init(&priv_p->p_list_lck, NULL);
	pthread_cond_init(&priv_p->p_list_cond, NULL);

	return 0;
}

void
init_data(struct list_head *reclaim_head) {
	int i;
	struct content_description_node *cdn_p;
	char fp[32];

	for (i=0; i<TEST_TOTAL_SIZE; i++) {
		cdn_p = ut_cdn.cn_op->cn_get_node(&ut_cdn);
		cdn_p->cn_data = i;
		sprintf(fp, "test_fp_%d", i);

		ut_fpc.fpc_op->fpc_set_wgt(&ut_fpc, rand()%NID_SIZE_FP_WGT);
		ut_fpc.fpc_op->fpc_calculate(&ut_fpc, fp, strlen(fp), (unsigned char*)cdn_p->cn_fp);
		list_add_tail(&cdn_p->cn_plist, reclaim_head);
	}
}

void
insert_data(struct list_head *reclaim_head) {
	struct content_description_node *cdn_p;
	int lru_max = ( RC_SIZE * 7 ) / 10, lru_cur=expect_dsrec_stat.p_lru_total_nr;
	struct dsrec_stat real_dsrec_stat;
	real_dsrec_stat.p_lru_max =  expect_dsrec_stat.p_lru_max = lru_max;

	uint8_t fp_wgt;
	int i;

	list_for_each_entry(cdn_p, struct content_description_node, reclaim_head, cn_plist) {
		fp_wgt = ut_fpc.fpc_op->fpc_get_wgt_from_fp(&ut_fpc, cdn_p->cn_fp);
		ut_dsrec.sr_op->sr_insert_block(&ut_dsrec, cdn_p);
		expect_dsrec_stat.p_lru_nr[fp_wgt]++;
		expect_dsrec_stat.p_lru_total_nr++;

		lru_cur++;
		if (lru_cur == lru_max) {
			/* lru_head is full. */
			for (i=0; i< NID_SIZE_FP_WGT; i++) {
				if (expect_dsrec_stat.p_lru_nr[i] > 0) {
					expect_dsrec_stat.p_lru_nr[i]--;
					break;
				}
			}
			expect_dsrec_stat.p_lru_total_nr--;
			expect_dsrec_stat.p_rec_nr++;

			lru_cur--;
		}

		/* verify real_dsrec_stat according to expect_dsrec_stat. */
		real_dsrec_stat = ut_dsrec.sr_op->sr_get_dsrec_stat(&ut_dsrec);
		for (i=0; i<NID_SIZE_FP_WGT; i++) {
			assert(real_dsrec_stat.p_lru_nr[i] == expect_dsrec_stat.p_lru_nr[i]);
		}
		assert(real_dsrec_stat.p_lru_total_nr == expect_dsrec_stat.p_lru_total_nr);
		assert(real_dsrec_stat.p_lru_max == expect_dsrec_stat.p_lru_max);
		assert(real_dsrec_stat.p_rec_nr == expect_dsrec_stat.p_rec_nr);

	}
}

int
main()
{
	printf("dsrec ut module start...\n");
	dsrec_initialization(&tmp_dsrec,&dsrec_setup);
	allocator_initialization(&ut_alloc, &alloc_setup);

	cdn_setup.fp_size = 32;
	cdn_setup.allocator = &ut_alloc;
	cdn_setup.set_id = 0;
	cdn_setup.seg_size = 0;
	cdn_initialization( &ut_cdn, &cdn_setup );

	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&ut_fpc, &fpc_setup);

	dsrec_setup.cdn = NULL;
	dsrec_setup.rc_size = RC_SIZE;
	dsrec_setup.rc = NULL;
	dsrec_setup.blksn = NULL;
	dsrec_setup.dsmgr = NULL;
	dsrec_setup.fpc = &ut_fpc;
	dsrec_initialization_clone(&ut_dsrec, &dsrec_setup);
	struct list_head reclaim_head;
	INIT_LIST_HEAD(&reclaim_head);
	int i;

	init_data(&reclaim_head);

	for (i=0; i<TEST_COUNT; i++) {
		insert_data(&reclaim_head);
	}

	printf("dsrec ut module end...\n");

	return 0;
}
