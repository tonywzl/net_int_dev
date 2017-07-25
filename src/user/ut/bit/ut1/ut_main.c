#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "nid_log.h"
#include "bit_if.h"

static struct bit_setup bit_setup;
static struct bit_interface ut_bit;

int
main()
{
	int count, n;

	nid_log_open();
	nid_log_info("bit ut module start ...");

	bit_initialization(&ut_bit, &bit_setup);

	n = 0xff;
	count = ut_bit.b_op->b_ctz32(&ut_bit, n);
	printf("0x%x ctz32 is %d\n", n, count);

	n = 0xff00;
	count = ut_bit.b_op->b_ctz32(&ut_bit, n);
	printf("0x%x ctz32 is %d\n", n, count);

	n = 0xff;
	count = ut_bit.b_op->b_clz32(&ut_bit, n);
	printf("0x%x clz32 is %d\n", n, count);

	n = 0xff00;
	count = ut_bit.b_op->b_clz32(&ut_bit, n);
	printf("0x%x clz32 is %d\n", n, count);

	n = 0xff;
	count = ut_bit.b_op->b_ctz64(&ut_bit, n);
	printf("0x%x ctz64 is %d\n", n, count);

	n = 0xff00;
	count = ut_bit.b_op->b_ctz64(&ut_bit, n);
	printf("0x%x ctz64 is %d\n", n, count);

	n = 0xff;
	count = ut_bit.b_op->b_clz64(&ut_bit, n);
	printf("0x%x clz64 is %d\n", n, count);

	n = 0xff00;
	count = ut_bit.b_op->b_clz64(&ut_bit, n);
	printf("0x%x clz64 is %d\n", n, count);

	nid_log_info("bit ut module end...");
	nid_log_close();

	return 0;
}
