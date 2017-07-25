/*
 * rdg.c
 * 	Implementation of Random Data Generator Module
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "nid_log.h"
#include "rdg_if.h"

struct rdg_private {
	off_t		p_start_off;
	uint64_t	p_range;
	uint32_t	p_maxlen;
	int		p_fd;
	uint64_t	p_next_rand;
};

/* RAND_MAX assumed to be 32767 */
static void
_rdg_rand(struct rdg_interface *rdg_p)
{
	struct rdg_private *priv_p = (struct rdg_private *)rdg_p->rd_private;
	priv_p->p_next_rand = priv_p->p_next_rand * 1103515245 + 12345;
}

static void
rdg_get_range(struct rdg_interface *rdg_p, off_t *start, uint32_t *len)
{
	struct rdg_private *priv_p = (struct rdg_private *)rdg_p->rd_private;
	uint64_t end_point, end_range, next_rand;

	_rdg_rand(rdg_p);
	next_rand = priv_p->p_next_rand;
	*start = (next_rand - priv_p->p_start_off) % priv_p->p_range + priv_p->p_start_off;

	_rdg_rand(rdg_p);
	next_rand = priv_p->p_next_rand;
	*len = next_rand % priv_p->p_maxlen;

	end_range = priv_p->p_start_off + priv_p->p_range - 1;
	end_point = *start + *len - 1;
	if (end_point > end_range)
		*len = (priv_p->p_start_off + priv_p->p_range - 1) - *start + 1;
}

static void
rdg_get_data(struct rdg_interface *rdg_p, uint32_t len, char *buf)
{
	struct rdg_private *priv_p = (struct rdg_private *)rdg_p->rd_private;
	assert(len <= priv_p->p_maxlen);
	read(priv_p->p_fd, buf, len);
}

struct rdg_operations rdg_op = {
	.rd_get_range = rdg_get_range,
	.rd_get_data = rdg_get_data,
};

int
rdg_initialization(struct rdg_interface *rdg_p, struct rdg_setup *setup)
{
	struct rdg_private *priv_p;
	struct timeval now;

	nid_log_info("rdg_initialization start ...");
	rdg_p->rd_op = &rdg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	rdg_p->rd_private = priv_p;
	priv_p->p_start_off = setup->start_off;
	priv_p->p_range = setup->range * (1UL << 30);
	priv_p->p_maxlen = setup->maxlen * (1U << 10);
	priv_p->p_fd = open("/dev/urandom", O_RDONLY);
	gettimeofday(&now, NULL);
	priv_p->p_next_rand = now.tv_sec;
	return 0;
}
