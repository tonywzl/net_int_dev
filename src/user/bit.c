/*
 * bit.c
 * 	Implementation of  BIT Module
 */

#include <stdlib.h>
#include <stdint.h>
#include "nid_log.h"
#include "bit_if.h"

#define	BIT_TWOBYTES_SIZE	(1<<16)

struct bit_private {
	uint8_t		p_clz32_tbl[BIT_TWOBYTES_SIZE];
	uint8_t		p_ctz32_tbl[BIT_TWOBYTES_SIZE];
};

static int
bit_clz32(struct bit_interface *bit_p, int n)
{
	struct bit_private *priv_p = (struct bit_private *)bit_p->b_private; 
	int count = 0;

	if (!n)
		return 32;

	if ((n & 0xFFFF0000)) {
		n >>= 16;
		count = priv_p->p_clz32_tbl[n] - 16;	
	} else {
		count = priv_p->p_clz32_tbl[n];
	}
	return count;
}

static int
bit_ctz32(struct bit_interface *bit_p, int n)
{
	struct bit_private *priv_p = (struct bit_private *)bit_p->b_private; 
	int count = 0;

	if (!n)
		return 32;

	if (!(n & 0x0000FFFF)) {
		count += 16;
		n >>= 16;
	}
	count += priv_p->p_ctz32_tbl[n];	
	return count;
}

static int
bit_clz64(struct bit_interface *bit_p, uint64_t n)
{
	int count = 0;

	if (!(n & 0xFFFFFFFF00000000))
		count = 32;
	else
		n >>= 32;
	count += bit_clz32(bit_p, n);
	return count;
}

static int
bit_ctz64(struct bit_interface *bit_p, uint64_t n)
{
	int count = 0;

	if (!(n & 0x00000000FFFFFFFF)) {
		count = 32;
		n >>= 32;
	}
	count += bit_ctz32(bit_p, n);
	return count;
}

static void 
bit_close(struct bit_interface *bit_p)
{
	char *log_header = "bit_close";
	struct bit_private *priv_p = (struct bit_private *)bit_p->b_private; 
	nid_log_notice("%s: start ...", log_header);
	if (priv_p)
		free(priv_p);
	bit_p->b_private = NULL;
}


struct bit_operations bit_op = {
	.b_clz32 = bit_clz32,
	.b_ctz32 = bit_ctz32,
	.b_clz64 = bit_clz64,
	.b_ctz64 = bit_ctz64,
	.b_close = bit_close,
};

int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
	struct bit_private *priv_p;
	int i, n, count, onebit;

	nid_log_info("bit_initialization start ...");
	assert(setup);
	bit_p->b_op = &bit_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bit_p->b_private = priv_p;

	priv_p->p_clz32_tbl[0] = 32;
	for (i = 1; i < BIT_TWOBYTES_SIZE; i++) {
		count = 0;
		n = i;
		onebit = 0x80000000;
		while (!(n & onebit)) {
			count++;
			onebit >>= 1;
		}
		priv_p->p_clz32_tbl[i] = count;
	}

	priv_p->p_ctz32_tbl[0] = 32;
	for (i = 1; i < BIT_TWOBYTES_SIZE; i++) {
		count = 0;
		n = i;
		while (!(n & 1)) {
			count++;
			n >>= 1;
		}
		priv_p->p_ctz32_tbl[i] = count;
	}

	return 0;
}
