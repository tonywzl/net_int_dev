/*
 * Write some data to MDS.
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ms_intf.h"

#define NS_PAGE_SIZE 4096
#define FP_SIZE 32
#define BITMAP_LEN 1024
#define CALLBACK_DELAY 2
#define WRITE_CALLBACK_DELAY 1
#define READ_CALLBACK_DELAY 1

static void
callback(int err, void* arg)
{
	static int i = 0;
	printf("callback start ...%d\n", i++);
	if (err != 0) {
		printf("callback error = %d\n", err);
	}
	assert(arg==NULL);
}

void
write_data()
{
	char voluuid[] = "3f3a8671-baaa-3db8-a0ff-023717636945";
	int count = 3;
	off_t voff = 0;
	int len = BITMAP_LEN*NS_PAGE_SIZE;

	struct iovec iov_data;
	iov_data.iov_len = count * NS_PAGE_SIZE;
	iov_data.iov_base = calloc(1, iov_data.iov_len);
	memcpy(iov_data.iov_base, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", 100);
	memcpy(((char*)iov_data.iov_base)+NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde", 50);
	memcpy(((char*)iov_data.iov_base)+2*NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdef", 10);
	char *fp = calloc(count, FP_SIZE);
	memcpy(fp, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*count);
	bool *fpToWrite = calloc(BITMAP_LEN, sizeof(bool));
	fpToWrite[0] = 1;
	fpToWrite[1] = 1;
	fpToWrite[2] = 1;


	struct iovec iov_data1;
	iov_data1.iov_len = count * NS_PAGE_SIZE;
	iov_data1.iov_base = calloc(1, iov_data1.iov_len);
	memcpy(iov_data1.iov_base, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", 100);
	memcpy(((char*)iov_data1.iov_base)+NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde", 50);
	memcpy(((char*)iov_data1.iov_base)+2*NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdef", 10);
	char *fp1 = calloc(count, FP_SIZE);
	memcpy(fp1, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*count);
	bool *fpToWrite1 = calloc(BITMAP_LEN, sizeof(bool));
	fpToWrite1[0] = 1;
	fpToWrite1[1] = 1;
	fpToWrite1[2] = 1;

	struct iovec iov_data2;
	iov_data2.iov_len = count * NS_PAGE_SIZE;
	iov_data2.iov_base = calloc(1, iov_data2.iov_len);
	memcpy(iov_data2.iov_base, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", 100);
	memcpy(((char*)iov_data2.iov_base)+NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcde", 50);
	memcpy(((char*)iov_data2.iov_base)+2*NS_PAGE_SIZE, "abcdefghijabcdefghijabcdefghijabcdef", 10);
	char *fp2 = calloc(count, FP_SIZE);
	memcpy(fp2, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*count);
	bool *fpToWrite2 = calloc(BITMAP_LEN, sizeof(bool));
	fpToWrite2[0] = 1;
	fpToWrite2[1] = 1;
	fpToWrite2[2] = 1;

	int rc = -1;
	rc = MS_Write_Data_Async(voluuid, &iov_data, 1, voff, len, (Callback)callback, NULL, fp, fpToWrite);
	printf("iov_data.iov_base = %s\n", (char*)iov_data.iov_base);
	printf("iov_data.iov_base1 = %s\n", ((char*)iov_data.iov_base)+NS_PAGE_SIZE);
	printf("iov_data.iov_base2 = %s\n", ((char*)iov_data.iov_base)+2*NS_PAGE_SIZE);
	printf("MS_Write_Data_Async return = %d\n", rc);
    //	sleep(WRITE_CALLBACK_DELAY);
	voff += len;
	rc = MS_Write_Data_Async(voluuid, &iov_data1, 1, voff, len, (Callback)callback, NULL, fp1, fpToWrite1);
	printf("MS_Write_Data_Async return = %d\n", rc);
    //	sleep(WRITE_CALLBACK_DELAY);
	voff += len;
	rc = MS_Write_Data_Async(voluuid, &iov_data2, 1, voff, len, (Callback)callback, NULL, fp2, fpToWrite2);
	printf("MS_Write_Data_Async return = %d\n", rc);
	sleep(WRITE_CALLBACK_DELAY);
}

void
read_data()
{
//	char voluuid[] = "3f3a8671-baaa-3db8-a0ff-023717636945";
	int count = 3;

	struct iovec iov_data;
	iov_data.iov_len = count * NS_PAGE_SIZE;
	iov_data.iov_base = calloc(1, iov_data.iov_len);
	char *fp = calloc(count, FP_SIZE);

	struct iovec iov_data1;
	iov_data1.iov_len = count * NS_PAGE_SIZE;
	iov_data1.iov_base = calloc(1, iov_data1.iov_len);
	char *fp1 = calloc(count, FP_SIZE);

	struct iovec iov_data2;
	iov_data2.iov_len = count * NS_PAGE_SIZE;
	iov_data2.iov_base = calloc(1, iov_data2.iov_len);
	char *fp2 = calloc(count, FP_SIZE);

	int rc = -1;
	memcpy(fp, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*count);
	rc = MS_Read_Data_Async(&iov_data, 1, fp, (Callback)callback, NULL, 0);
	printf("MS_Read_Data_Async return = %d\n", rc);
	printf("fp = %s\n", fp);

	memcpy(fp1, "765432109876543210987654321098765432109876543210987654321098765498765432109876543210987654321098", FP_SIZE*count);
	rc = MS_Read_Data_Async(&iov_data1, 1, fp1, (Callback)callback, NULL, 0);
	printf("MS_Read_Data_Async return = %d\n", rc);
	printf("fp = %s\n", fp1);

	memcpy(fp2, "543210987654321098765432109876549876543210987654321098765432109876543210987654321098765432109876", FP_SIZE*count);
	rc = MS_Read_Data_Async(&iov_data2, 1, fp2, (Callback)callback, NULL, 0);
	printf("MS_Read_Data_Async return = %d\n", rc);
	sleep(READ_CALLBACK_DELAY);
	printf("fp = %s\n", fp2);

	printf("iov_data.iov_base = %s\n", (char*)iov_data.iov_base);
	printf("iov_data.iov_base1 = %s\n", ((char*)iov_data.iov_base)+NS_PAGE_SIZE);
	printf("iov_data.iov_base2 = %s\n", ((char*)iov_data.iov_base)+2*NS_PAGE_SIZE);

	printf("iov_data1.iov_base = %s\n", (char*)iov_data1.iov_base);
	printf("iov_data1.iov_base1 = %s\n", ((char*)iov_data1.iov_base)+NS_PAGE_SIZE);
	printf("iov_data1.iov_base2 = %s\n", ((char*)iov_data1.iov_base)+2*NS_PAGE_SIZE);

	printf("iov_data2.iov_base = %s\n", (char*)iov_data2.iov_base);
	printf("iov_data2.iov_base1 = %s\n", ((char*)iov_data2.iov_base)+NS_PAGE_SIZE);
	printf("iov_data2.iov_base2 = %s\n", ((char*)iov_data2.iov_base)+2*NS_PAGE_SIZE);
}


int main()
{
	Ms_Start();

	write_data();

	read_data();

	Ms_Stop();

	return 0;
}


