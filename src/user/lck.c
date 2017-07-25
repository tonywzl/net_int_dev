/*
 * lck.c
 * 	Implementation of  Lock Module
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nid_log.h"
#include "lck_if.h"
#include <malloc.h>

struct lck_private {
	pthread_mutex_t		p_mlck;
	pthread_cond_t		*p_cond;
};

static void
lck_rlock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	while (node->ln_wbusy || node->ln_ubusy) {
		pthread_cond_wait(cond, mlck);
	}
	node->ln_rcounter++;
	pthread_mutex_unlock(mlck);
}

static void
lck_runlock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	node->ln_rcounter--;
	if ((node->ln_rcounter == 0 && node->ln_wbusy) ||
	    (node->ln_rcounter == 1 && node->ln_ubusy)) {
		pthread_cond_broadcast(cond);
	}
	pthread_mutex_unlock(mlck);
}

static void
lck_wlock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	while (node->ln_wbusy) {
		pthread_cond_wait(cond, mlck);
	}
	node->ln_wbusy = 1;
	while (node->ln_rcounter) {
		pthread_cond_wait(cond, mlck);
	}
	pthread_mutex_unlock(mlck);
}

static void
lck_wunlock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	node->ln_wbusy = 0;
	pthread_cond_broadcast(cond);
	pthread_mutex_unlock(mlck);
}

/*
 * Only one thread can do uplock. The caller must be a read lock holder
 */
static void
lck_uplock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	assert(!node->ln_ubusy);
	node->ln_ubusy = 1;
	while (node->ln_rcounter > 1) {
		pthread_cond_wait(cond, mlck);
	}
	pthread_mutex_unlock(mlck);
}

static void
lck_downlock(struct lck_interface *lck_if, struct lck_node *node)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_mutex_lock(mlck);
	node->ln_ubusy = 0;
	pthread_cond_broadcast(cond);
	pthread_mutex_unlock(mlck);
}

static void
lck_destroy(struct lck_interface *lck_if)
{
	struct lck_private *priv_p = (struct lck_private *)lck_if->l_private;
	pthread_mutex_t *mlck = &priv_p->p_mlck;
	pthread_cond_t *cond = priv_p->p_cond;

	pthread_cond_destroy(cond);
	pthread_mutex_destroy(mlck);
	free(priv_p->p_cond);
	free((void *)priv_p);
}

struct lck_operations lck_op = {
	.l_rlock = lck_rlock,
	.l_runlock = lck_runlock,
	.l_wlock = lck_wlock,
	.l_wunlock = lck_wunlock,
	.l_uplock = lck_uplock,
	.l_downlock = lck_downlock,
	.l_destroy = lck_destroy,
};

int
lck_initialization(struct lck_interface *lck_p, struct lck_setup *setup)
{
	struct lck_private *priv_p;

	nid_log_info("lck_initialization start ...");
	assert(setup);
	lck_p->l_op = &lck_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	lck_p->l_private = priv_p;
	pthread_mutex_init(&priv_p->p_mlck, NULL);
	priv_p->p_cond = (pthread_cond_t*)memalign(4, sizeof(pthread_cond_t));
	pthread_cond_init(priv_p->p_cond, NULL);
	return 0;
}

int
lck_node_init(struct lck_node *ln_p)
{
	memset(ln_p, 0, sizeof(*ln_p));
	return 0;
}
