#include "ms_intf.h"
#include "srn_if.h"
#include "list.h"
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

struct fp_cb_arg {
	struct list_head list;
	off_t voff;
	size_t len;
	Callback cbFunc;
	void *cbArg;
	void *fpBuf;
	bool *fpFound;
};

struct content_cb_arg {
	struct list_head list;
	struct iovec* vec;
	size_t count;
	void *fpBuf;
	Callback cbFunc;
	void *cbArg;
};

struct list_head fp_cb_head;
pthread_mutex_t	fp_cb_lck, *fpcb_lck = &fp_cb_lck;
pthread_cond_t fp_cb_cond, *fpcb_cond = &fp_cb_cond;

struct list_head content_cb_head;
pthread_mutex_t	content_cb_lck, *contentcb_lck = &content_cb_lck;
pthread_cond_t content_cb_cond, *contentcb_cond = &content_cb_cond;

static struct list_head offset_fp_list;
static struct list_head fp_content_list;
static volatile int stop = 0;

struct offset_fp {
	struct list_head 	list;
	off_t			off;
	char 			fp[32];
};

struct fp_content {
	struct list_head	list;
	char			fp[32];
	char			content[4096];
};

static char*
_print_p(char * fpbuf, char *bufstr) {
	int j;
	char *bufstrp = (char *)bufstr;
	for (j=0; j<32; j++) {
		sprintf(bufstrp, "%02x", fpbuf[j]);
		bufstrp += 2;
	}
	*bufstrp = (char)'\0';
	return bufstr;
}

static void
_insert_fp(off_t off, char *fp)
{
	struct offset_fp *of, *ofpre = (struct offset_fp *)&offset_fp_list;
	char found = 0;
	char fpbuf[128];

	list_for_each_entry(of, struct offset_fp, &offset_fp_list, list) {
		if (of->off == off) {
			found = 1;
			break;
		} else if (of->off > off) {
			break;
		}
		ofpre = of;
	}

	if (found) {
		printf("Update offset:%ld new fp:%s\n", of->off, _print_p(fp, fpbuf));
		memcpy(of->fp, fp, 32);
	} else {
		of = malloc(sizeof(*of));
		of->off = off;
		printf("Insert new offset:%ld new fp:%s\n", of->off, _print_p(fp, fpbuf));
		memcpy(of->fp, fp, 32);
		if (ofpre == (struct offset_fp *)&offset_fp_list) {
			list_add(&of->list, &offset_fp_list);
		} else {
			list_add(&of->list, &ofpre->list);
		}
	}
}

static void
_search_fp(off_t off, size_t len, char *fp, bool *fpmap)
{
	size_t fp_len = (len >> 12) * 32;
	struct offset_fp *of;
	char *fpbuf, fppbuf[128];
	off_t index, off_curent = off;
	memset(fp, 0 , fp_len);
	memset(fpmap, false, sizeof(bool) * (len >> 12));

	list_for_each_entry(of, struct offset_fp, &offset_fp_list, list) {
		if (of->off == off_curent && off_curent <= off+(off_t)len) {
			if (off_curent >= off+(off_t)len) {
				break;
			}
			index = (off_curent - off) >> 12;
			fpbuf = fp + index * 32;
			memcpy(fpbuf, of->fp, 32);
			fpmap[index] = true;
			printf("Found off:%ld fp:%s\n", off_curent, _print_p(fpbuf, fppbuf));
			off_curent += 4096;
			continue;
		} else if (of->off > off_curent) {
			while (off_curent != of->off && off_curent <= off+(off_t)len) {
				off_curent += 4096;
			}
			if (off_curent >= off+(off_t)len) {
				break;
			}
			assert(off_curent == of->off);
			index = (off_curent - off) >> 12;
			fpbuf = fp + (index) * 32;
			memcpy(fpbuf, of->fp, 32);
			fpmap[index] = true;
			printf("Found off:%ld fp:%s\n", off_curent, _print_p(fpbuf, fppbuf));
			off_curent += 4096;
		}
	}
}

static void
_insert_content(char *fp, char *content)
{
	struct fp_content *of;
	char found = 0;
	char fppbuf[128];

	list_for_each_entry(of, struct fp_content, &fp_content_list, list) {
		if (memcmp(of->fp, fp, 32) == 0) {
			found = 1;
			break;
		}
	}

	if (found) {
		printf("Update fp:%s content:%02x\n", _print_p(fp, fppbuf), content[0]);
		memcpy(of->content, content, 4096);
	} else {
		of = malloc(sizeof(*of));
		printf("Insert fp:%s content:%02x\n", _print_p(fp, fppbuf), content[0]);
		memcpy(of->fp, fp, 32);
		memcpy(of->content, content, 4096);
		list_add(&of->list, &fp_content_list);
	}
}

static bool
_search_content(char *fp, char *content)
{
	struct fp_content *of;
	char fppbuf[128];

	list_for_each_entry(of, struct fp_content, &fp_content_list, list) {
		if (memcmp(of->fp, fp, 32) == 0) {
			memcpy(content, of->content, 4096);
			printf("Found fp:%s content:%02x\n", _print_p(fp, fppbuf), content[0]);
			return true;
		}
	}
	printf("Not found fp:%s content\n", _print_p(fp, fppbuf));
	return false;
}

MsRet MS_Write_Async(uint64_t vid,
                     uint64_t snapid,
                     void *buf,
                     off_t voff,
                     size_t len,
                     Callback cbFunc,
                     void *cbArg,
                     void *fpBuf,
                     IoFlag flag,
                     size_t *more) {
	(void)vid;(void)snapid; (void)flag;
	(void)more;(void)cbFunc;(void)cbArg;

	size_t length = 0;
	assert((len&(4096-1)) == 0);
	assert((voff&(4096-1)) == 0);

	char *buff = buf;
	char *fpbuf = fpBuf;
	off_t off = voff;

	printf("Write off: %ld Len:%lu\n", voff, len); fflush(stdout);
	while (length != len) {

		_insert_fp(off, fpbuf);
		_insert_content(fpbuf, buff);

		off += 4096;
		buff += 4096;
		length += 4096;
		fpbuf += 32;
	}

	return retOk;
}

MsRet MS_Writev_Async(uint64_t vid,
                      uint64_t snapid,
                      struct iovec *iovec,
                      off_t voff,
                      size_t count,
                      Callback cbFunc,
                      void *cbArg,
                      void *fpBuf,
                      IoFlag flag,
                      size_t *more) {
	(void)vid;(void)snapid;(void)iovec;(void)voff;(void)count;
	(void)cbFunc;(void)cbArg;(void)fpBuf;(void)flag;(void)more;

	assert(0);
	return retOk;
}

MsRet MS_Read_Async(uint64_t vid,
                    uint64_t snapid,
                    void *buf,
                    off_t voff,
                    size_t len,
                    Callback cbFunc,
                    void *cbArg,
                    IoFlag flag) {
	(void)vid;(void)snapid;(void)buf;(void)voff;(void)len;
	(void)cbFunc;(void)cbArg;(void)flag;
	assert(0);
	return retOk;
}

MsRet MS_Readv_Async(uint64_t vid,
                     uint64_t snapid,
                     struct iovec *iovec,
                     off_t voff,
                     size_t count,
                     Callback cbFunc,
                     void *cbArg,
                     IoFlag flag) {
	(void)vid;(void)snapid;(void)iovec;(void)voff;(void)count;
	(void)cbFunc;(void)cbArg;(void)flag;
	assert(0);
	return retFail;
}

static void*
MS_Read_Fp_Async_Thread(void *p) {

	struct fp_cb_arg *arg, *arg1;
	(void)p;
next:
	pthread_mutex_lock(fpcb_lck);
	while(list_empty(&fp_cb_head)) {
		pthread_cond_wait(fpcb_cond, fpcb_lck);
		if (stop) {
			pthread_mutex_unlock(fpcb_lck);
			return NULL;
		}
	}
	list_for_each_entry_safe(arg, arg1, struct fp_cb_arg, &fp_cb_head, list) {
		list_del(&arg->list);
		pthread_mutex_unlock(fpcb_lck);
		_search_fp(arg->voff, arg->len, arg->fpBuf, arg->fpFound);
		arg->cbFunc(0, arg->cbArg);
		free(arg);
		pthread_mutex_lock(fpcb_lck);
	}
	pthread_mutex_unlock(fpcb_lck);

	goto next;
	return NULL;

}

MsRet MS_Read_Fp_Async(uint64_t vid,
                       uint64_t snapid,
                       off_t voff,
                       size_t len,
                       Callback cbFunc,
                       void *cbArg,
                       void *fpBuf,
                       bool *fpFound,
                       IoFlag flag) {
	(void)flag;(void)vid;(void)snapid;
	struct fp_cb_arg *arg = malloc(sizeof(*arg));
	arg->cbArg = cbArg;
	arg->voff = voff;
	arg->len = len;
	arg->cbFunc = cbFunc;
	arg->fpBuf = fpBuf;
	arg->fpFound = fpFound;
	pthread_mutex_lock(fpcb_lck);
	list_add_tail(&arg->list, &fp_cb_head);
	pthread_cond_broadcast(fpcb_cond);
	pthread_mutex_unlock(fpcb_lck);
	return retOk;
}

static void*
MS_Read_Data_Async_Thread(void *p) {

	struct content_cb_arg *arg, *arg1;
	size_t n;
	char *fpBuf;
	(void)p;
next:
	pthread_mutex_lock(contentcb_lck);
	while(list_empty(&content_cb_head)) {
		pthread_cond_wait(contentcb_cond, contentcb_lck);
		if (stop) {
			pthread_mutex_unlock(contentcb_lck);
			return NULL;
		}
	}
	list_for_each_entry_safe(arg, arg1, struct content_cb_arg, &content_cb_head, list) {
		list_del(&arg->list);
		pthread_mutex_unlock(contentcb_lck);
		n = 0;
		fpBuf = arg->fpBuf;
		for (n = 0; n < arg->count; n++) {
			assert(arg->vec[n].iov_len == 4096);
			_search_content(fpBuf, (char *)arg->vec[n].iov_base);
			fpBuf += 32;
		}
		arg->cbFunc(0, arg->cbArg);
		free(arg);
		pthread_mutex_lock(contentcb_lck);
	}
	pthread_mutex_unlock(contentcb_lck);

	goto next;
	return NULL;
}

MsRet MS_Read_Data_Async(struct iovec* vec,
                         size_t count,
                         void *fpBuf,
                         Callback cbFunc, 
                         void *cbArg,
                         IoFlag flag) {
	(void)flag;
	struct content_cb_arg *arg = malloc(sizeof(*arg));
	arg->cbArg = cbArg;
	arg->vec = vec;
	arg->count = count;
	arg->cbFunc = cbFunc;
	arg->fpBuf = fpBuf;
	pthread_mutex_lock(contentcb_lck);
	list_add_tail(&arg->list, &content_cb_head);
	pthread_cond_broadcast(contentcb_cond);
	pthread_mutex_unlock(contentcb_lck);
	return retOk;
}

MsRet MS_Flush(Callback cbFunc, void *cbArg) {
	(void)cbFunc; (void)cbArg;
    return retOk;
}

void Ms_Start() {
	INIT_LIST_HEAD(&fp_cb_head);
	INIT_LIST_HEAD(&content_cb_head);
	INIT_LIST_HEAD(&offset_fp_list);
	INIT_LIST_HEAD(&fp_content_list);
	pthread_mutex_init(fpcb_lck, NULL);
	pthread_mutex_init(contentcb_lck, NULL);

	pthread_cond_init(fpcb_cond, NULL);
	pthread_cond_init(contentcb_cond, NULL);

	pthread_t tid;
	pthread_attr_t attr, attr1;

	pthread_attr_init(&attr);
	pthread_attr_init(&attr1);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);

	pthread_create(&tid, &attr, MS_Read_Fp_Async_Thread, NULL);
	pthread_create(&tid, &attr1, MS_Read_Data_Async_Thread, NULL);
}

void Ms_Stop() {
	__sync_lock_test_and_set(&stop, 1);
}
