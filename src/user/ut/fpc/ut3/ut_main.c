
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include "list.h"
#include "nid.h"
#include "fpc_if.h"

#define TEST_COUNT 10000
#define TEST_COUNT2 100

static struct fpc_setup setup;
static struct fpc_interface fpc;

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int i;

    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

void test_fpc_cal() {
	int i,j;
	char fp[128], fp2[128], buf[128];
	int wgt;
	unsigned char* (*algorithm_fun_p)(const unsigned char *, size_t, unsigned char *);
	switch (setup.fpc_algrm) {
	case FPC_MD5:
		algorithm_fun_p=MD5;
		break;
	case FPC_SHA224:
		algorithm_fun_p=SHA224;
		break;
	case FPC_SHA256:
		algorithm_fun_p=SHA256;
		break;
	}

	for (i = 0; i < TEST_COUNT2; i++) {
		wgt = rand() % NID_SIZE_FP_WGT;
		fpc.fpc_op->fpc_set_wgt(&fpc, wgt);
		for (j=0; j<TEST_COUNT; j++)
		{
			gen_random(buf, 32);
			algorithm_fun_p((unsigned char*)buf, fpc.fpc_op->fpc_get_fp_len(&fpc), (unsigned char*)fp2);
			fpc.fpc_op->fpc_calculate(&fpc, buf,  fpc.fpc_op->fpc_get_fp_len(&fpc), (unsigned char*)fp);

			assert(fpc.fpc_op->fpc_cmp(&fpc, fp, fp2) == 0);
			assert(fpc.fpc_op->fpc_get_wgt_from_fp(&fpc, fp) == wgt);
		}
	}

}

void test_fpc_get_fp_len() {
	switch (setup.fpc_algrm) {
	case FPC_MD5:
		assert(fpc.fpc_op->fpc_get_fp_len(&fpc) == 16);
		break;
	case FPC_SHA224:
		assert(fpc.fpc_op->fpc_get_fp_len(&fpc) == 28);
		break;
	case FPC_SHA256:
		assert(fpc.fpc_op->fpc_get_fp_len(&fpc) == 32);
		break;
	}
}

void test_fpc_get_and_set_wgt_from_fp() {
	int i;
	char fp[128];
	int wgt;
	for (i=0; i< TEST_COUNT; i++) {
		gen_random(fp, 32);
		wgt = rand() % NID_SIZE_FP_WGT;
		fpc.fpc_op->fpc_set_wgt_to_fp(&fpc, fp, wgt);
		assert(wgt == fpc.fpc_op->fpc_get_wgt_from_fp(&fpc, fp));
	}
}

int main(void) {
	setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&fpc, &setup);

	test_fpc_get_fp_len();
	test_fpc_cal();
	test_fpc_get_and_set_wgt_from_fp();

	printf("Test success!!!\n");

	return EXIT_SUCCESS;
}
