
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fpc_if.h"

int main(void) {
	struct fpc_interface fpc;
	struct fpc_setup setup;
	char buf[4096];
	int i;
	unsigned char ret[SHA256_DIGEST_LENGTH];

	setup.fpc_algrm = FPC_SHA256;
	fpc_initialization(&fpc, &setup);

	for (i=0; i<4096; i++) {
		buf[i] = (char) rand();
	}
	fpc.fpc_op->fpc_calculate(&fpc, buf, 4096, ret);

	for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
		printf("%02x", ret[i]);
	}
	printf("\n");

	for (i=0; i<4096; i++) {
		buf[i] = (char)rand();
	}
	fpc.fpc_op->fpc_calculate(&fpc, buf, 4096, ret);

	for (i=0; i<SHA256_DIGEST_LENGTH; i++) {
		printf("%02x", ret[i]);
	}
	printf("\n");

	fpc.fpc_op->fpc_destroy(&fpc);
	printf("Test success!!!");
	return EXIT_SUCCESS;
}
