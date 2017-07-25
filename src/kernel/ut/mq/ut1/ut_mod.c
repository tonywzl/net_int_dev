#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "nid_log.h" 
#include "mq_if.h"

static int nr_consumers = 2;
static int nr_mnodes = 1024;
static int nr_loops = 1024;
static int exit_counter = 0;
static spinlock_t exit_lck;

module_param(nr_consumers, int, 0444);
MODULE_PARM_DESC(nr_consumers, "number of nr_consumers (default: 2)");
module_param(nr_mnodes, int, 0444);
MODULE_PARM_DESC(nr_mnodes, "number of message nodes (default: 1024)");
module_param(nr_loops, int, 0444);
MODULE_PARM_DESC(nr_mnodes, "number of loops ofr each comsumer (default: 1024)");

MODULE_DESCRIPTION("unit test of Message Link module");
MODULE_LICENSE("GPL");

struct mq_setup mq_setup; 
struct mq_interface ut_mq;

static int
consumer_thread(void *data)
{
	struct mq_interface *mq_p = (struct mq_interface *)data;
	struct mq_message_node *mn_p;
	int i;

	for (i = 0; i < nr_loops; i++) {
		while (!(mn_p = mq_p->m_op->m_get_free_mnode(mq_p)))
			msleep(100);

		mq_p->m_op->m_enqueue(mq_p, mn_p);
		mn_p = mq_p->m_op->m_dequeue(mq_p);
		mq_p->m_op->m_put_free_mnode(mq_p, mn_p);
	}
	nid_log_info("consumer_thread exit");
	spin_lock(&exit_lck);
	exit_counter++;
	spin_unlock(&exit_lck);
	return 0;
}

static int
ut_mod_init(void)
{
	struct task_struct *the_thread;
	int i;
	nid_log_info("this is mn ut module");
	mq_setup.size = nr_mnodes;
	mq_initialization(&ut_mq, &mq_setup);
	spin_lock_init(&exit_lck);
	for (i = 0; i < nr_consumers; i++) {
		the_thread = kthread_create(consumer_thread, &ut_mq, "%s", "mq_ut");
		wake_up_process(the_thread);
	}
	return 0;
}

static void 
ut_mod_exit(void)
{
	nid_log_info("mn ut module exit ...");
	while (exit_counter < nr_consumers)
		msleep(100);
	ut_mq.m_op->m_exit(&ut_mq);
}

module_init(ut_mod_init)
module_exit(ut_mod_exit)

