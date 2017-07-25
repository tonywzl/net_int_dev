#include "bwc.h"

#if 0
static void
__print_dev_header(int fd, struct bwc_dev_header *dhp, char *log_header)
{
	char _magic[2 * BWC_MAGICSZ];
	off_t cur_off;
	_magic[BWC_MAGICSZ] = 0;
	memcpy(_magic, dhp->header_magic, BWC_MAGICSZ);
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_notice("%s: _magic:{%s}, cur_off:%ld, dh_counter:%d",
		log_header, _magic, cur_off, dhp->dh_counter);
}

static void
__print_dev_trailer(int fd, struct bwc_dev_trailer *dtp, char *log_header)
{
	char _magic[2 * BWC_MAGICSZ];
	off_t cur_off;
	_magic[BWC_MAGICSZ] = 0;
	memcpy(_magic, dtp->trailer_magic, BWC_MAGICSZ);
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_notice("%s: _magic:{%s}, cur_off:%ld, dt_offset:%ld, dt_my_seq:%lu, dt_flushed_seq:%lu",
		log_header, _magic, cur_off, dtp->dt_offset, dtp->dt_my_seq,  dtp->dt_flushed_seq);
}

static void
__print_seg_trailer(int fd, struct bwc_seg_trailer *tp, char *log_header)
{
	char _magic[2 * BWC_MAGICSZ];
	off_t cur_off;
	_magic[BWC_MAGICSZ] = 0;
	memcpy(_magic, tp->trailer_magic, BWC_MAGICSZ);
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_notice("%s: trailer_magic:{%s}, cur_off:%ld, header_len:%u, data_len:%u, my_seq:%lu, flushed_seq:%lu",
		log_header, _magic, cur_off, tp->header_len, tp->data_len, tp->my_seq, tp->flushed_seq);
}

static void
__print_seg_header(int fd, struct bwc_seg_header *hp, char *log_header)
{
	char _magic[2 * BWC_MAGICSZ];
	off_t cur_off;
	_magic[BWC_MAGICSZ] = 0;
	memcpy(_magic, hp->header_magic, BWC_MAGICSZ);
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_debug("%s: header_magic:{%s}, cur_off:%ld, header_len:%u, data_len:%u, my_seq:%lu, flushed_seq:%lu",
		log_header, _magic, cur_off, hp->header_len, hp->data_len, hp->my_seq, hp->flushed_seq);
}

#else

static void
__print_dev_header(int fd, struct bwc_dev_header *dhp, char *log_header)
{
	char dev_header_magic[BWC_MAGICSZ + 1];
	char dev_header_ver[BWC_VERSZ + 1];
	time_t seg_header_magic;
	time_t seg_trailer_magic;
	off_t cur_off;

	dev_header_magic[BWC_MAGICSZ] = 0;
	dev_header_ver[BWC_VERSZ] = 0;
	memcpy(dev_header_magic, dhp->header_magic, BWC_MAGICSZ);
	memcpy(dev_header_ver, dhp->header_ver, BWC_VERSZ);
	memcpy(&seg_header_magic, dhp->dh_seg_header_magic, sizeof(time_t));
	memcpy(&seg_trailer_magic, dhp->dh_seg_trailer_magic, sizeof(time_t));
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_warning("%s: dev_header_magic:{%s}, dev_header_version:{%s}, seg_header_magic:{%ld}, seg_trailer_magic:{%ld}, cur_off:%ld, dh_counter:%d",
		log_header, dev_header_magic,dev_header_ver, seg_header_magic, seg_trailer_magic, cur_off, dhp->dh_counter);
}

static void
__print_dev_trailer(int fd, struct bwc_dev_trailer *dtp, char *log_header)
{
	char _magic[BWC_MAGICSZ + 1];
	char _ver[BWC_VERSZ + 1];
	off_t cur_off;
	_magic[BWC_MAGICSZ] = 0;
	_ver[BWC_VERSZ] = 0;
	memcpy(_magic, dtp->trailer_magic, BWC_MAGICSZ);
	memcpy(_ver, dtp->trailer_ver, BWC_VERSZ);
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_notice("%s: dev_trailer_magic:{%s}, dev_tailer_verison:{%s}, cur_off:%ld, dt_offset:%ld, dt_my_seq:%lu, dt_flushed_seq:%lu",
		log_header, _magic, _ver, cur_off, dtp->dt_offset, dtp->dt_my_seq,  dtp->dt_flushed_seq);
}

static void
__print_seg_trailer(int fd, struct bwc_seg_trailer *tp, char *log_header)
{
	time_t _magic;
	off_t cur_off;
	memcpy(&_magic, tp->trailer_magic, sizeof(time_t));
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_notice("%s: seg_trailer_magic:{%ld}, cur_off:%ld, header_len:%u, data_len:%u, my_seq:%lu, flushed_seq:%lu, wc_offset:%lu",
		log_header, _magic, cur_off, tp->header_len, tp->data_len, tp->my_seq, tp->flushed_seq, tp->wc_offset);
}

static void
__print_seg_header(int fd, struct bwc_seg_header *hp, char *log_header)
{
	time_t _magic;
	off_t cur_off;
	memcpy(&_magic, hp->header_magic, sizeof(time_t));
	cur_off = lseek(fd, 0, SEEK_CUR);
	nid_log_debug("%s: seg_header_magic:{%ld}, cur_off:%ld, header_len:%u, data_len:%u, my_seq:%lu, flushed_seq:%lu, wc_offset:%lu",
		log_header, _magic, cur_off, hp->header_len, hp->data_len, hp->my_seq, hp->flushed_seq, hp->wc_offset);
}

#endif

struct bwc_chain *
__get_inactive_chain_by_id(struct bwc_private *priv_p, uint16_t target_id)
{
	struct bwc_chain *chain_p;
	int got_it = 0;
	/*
	 * Always need lock, as this is called in recovery thread,
	 * so will not impact the performance by this lock,
	 * and also make sure thread safe if use async recovery in future.
	 */
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		if (target_id == chain_p->c_id) {
			got_it = 1;
			break;
		}
	}

	pthread_mutex_unlock(&priv_p->p_lck);

	if (!got_it)
		return NULL;
	return chain_p;
}

void
__bwc_start_page(struct bwc_chain *chain_p, void *page_p, int do_buffer)
{
	struct bfe_interface *bfe_p = &chain_p->c_bfe;
	struct pp_page_node *pnp = (struct pp_page_node *)page_p;

	if (do_buffer) {
		bfe_p->bf_op->bf_flush_page(bfe_p, pnp);
	}
}

static void
__bufdevice_gen_seg_magic(struct bwc_private *priv_p)
{
	char *log_header = "__bufdevice_gen_seg_magic";
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	time_t tick, tick_rev;

	nid_log_warning("%s: start ...", log_header);

	tick = time(NULL);
	memcpy(dhp->dh_seg_header_magic, &tick,  sizeof(time_t));

	tick_rev = ~tick;
	memcpy(dhp->dh_seg_trailer_magic, &tick_rev,  sizeof(time_t));

	nid_log_warning("%s: seg_header_magic:{%ld}, seg_trailer_magic:{%ld}",
			log_header, tick, tick_rev);

	__bufdevice_update_header(priv_p);
}

int
__bufdevice_rebuild_dev_header(struct bwc_private *priv_p)
{
	char *log_header = "__bufdevice_rebuild_dev_header";
	int fd = priv_p->p_fhandle;
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	struct bwc_dev_obj_header *dh_obj;
	char *p;
	int i;
	ssize_t nread;
	char ver[BWC_VERSZ + 1];

	nid_log_warning("%s: start ...", log_header);
	lseek(fd, priv_p->p_mdz_size, SEEK_SET);	// move to hdz
	nread = __read_n(fd, dhp, BWC_DEV_HEADERSZ);		// read device header
	assert(nread == BWC_DEV_HEADERSZ);
	__print_dev_header(fd, dhp, log_header);
	if (memcmp(dhp->header_magic, BWC_DEV_HEADER_MAGIC, BWC_MAGICSZ)) {
		nid_log_warning("%s: dev_header_magic checking failed", log_header);
		__bufdevice_gen_seg_magic(priv_p);
		return -1;
	} else {
		nid_log_warning("%s: dev_header_magic checking passed, obj counter:%d",
			log_header, dhp->dh_counter);
	}

	ver[BWC_VERSZ] = '\0';
	memcpy(ver, dhp->header_ver, BWC_VERSZ);
	if (memcmp(dhp->header_ver, BWC_DEV_HEADER_VER, BWC_VERSZ)) {
		nid_log_warning("%s: dev_header_version not matching, binary version:{%s}, dev_header_version:{%s}",
				log_header, BWC_DEV_HEADER_VER, ver);
		if (__bwc_ver_cmp_check(dhp->header_ver, BWC_DEV_HEADER_VER)) {
			nid_log_error("%s: dev_header_version not compatible.",
					log_header);
			return -2;
		}
	} else {
		nid_log_warning ("%s: dev_header_version checking passed, version:{%s}",
			log_header, ver);
	}
	p = (char *)dhp + sizeof(*dhp);
	dh_obj = (struct bwc_dev_obj_header *)p;
	for (i = 0; i < dhp->dh_counter; i++) {
		priv_p->p_resources[dh_obj->obj_id] = calloc(1, dh_obj->obj_len + 1);
		p += sizeof(*dh_obj);
		memcpy(priv_p->p_resources[dh_obj->obj_id], p, dh_obj->obj_len);
		nid_log_warning("%s: got resource:{%s}, len:%u, id:%u",
			log_header, priv_p->p_resources[dh_obj->obj_id], dh_obj->obj_len, dh_obj->obj_id);
		p += dh_obj->obj_len;
		dh_obj = (struct bwc_dev_obj_header *)p;
	}

	return 0;
}

void
__bufdevice_update_header(struct bwc_private *priv_p)
{
	char *log_header = "__bufdevice_update_header";
	int fd = priv_p->p_fhandle;
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	struct bwc_dev_obj_header dev_obj, *dh_obj = &dev_obj;
	char *p;
	int i, len = 0, aligned_len;
	off_t cur_off;
	ssize_t nwritten;

	nid_log_notice("%s: start ...", log_header);
	memcpy(dhp->header_magic, BWC_DEV_HEADER_MAGIC, BWC_MAGICSZ);
	memcpy(dhp->header_ver, BWC_DEV_HEADER_VER, BWC_VERSZ);
	dhp->dh_counter = 0;
	p = (char *)dhp + sizeof(*dhp);
	for (i = 0; i < BWC_MAX_RESOURCE; i++) {
		if (priv_p->p_resources[i]) {
			dh_obj->obj_type = 0;
			dh_obj->obj_id = i;
			dh_obj->obj_len = strlen(priv_p->p_resources[i]);
			memcpy(p, dh_obj, sizeof(*dh_obj));
			p += sizeof(*dh_obj);
			len += sizeof(*dh_obj);
			memcpy(p, priv_p->p_resources[i], dh_obj->obj_len);
			p += dh_obj->obj_len;
			len += dh_obj->obj_len;
			dhp->dh_counter++;
			nid_log_notice("%s: updating {%s}, id:%d, len:%d",
				log_header, priv_p->p_resources[i], i, dh_obj->obj_len);
		}
	}
	dhp->dh_data_size = len;
	len += sizeof(*dhp);

	cur_off = lseek(fd, 0, SEEK_CUR);	// remember current position
	lseek(fd, priv_p->p_mdz_size, SEEK_SET);			// seek to the start position
	aligned_len = __round_up(len, priv_p->p_dio_alignment);
	nwritten = __write_n(fd, dhp, aligned_len);		// wirte the dev header
	assert(nwritten == aligned_len);
	lseek(fd, cur_off, SEEK_SET);		// recover position
	//__print_dev_header(fd, dhp, log_header);
	nid_log_notice("%s: counter:%d, len:%d...", log_header, dhp->dh_counter, len);
}

/*
 * the bufdevice pointer to pos immedately after the end of the segment to be scanned
 * Notice:
 * 	the device file pointer will point to immediately after the segment
 * 	if return success.
 */
static char *debug_p;

static int
__bwc_seg_checking_rev(struct bwc_private *priv_p, struct bwc_seg_header *scan_header, struct bwc_seg_trailer *scan_trailer,
		struct bwc_rb_node **nnpp, off_t *seg_trailer_off, struct list_head *recover_buf_head, uint64_t target_seq, uint64_t *my_first)
{
	char *log_header = "__bwc_seg_checking_rev";
	int fd = priv_p->p_fhandle;
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	struct bwc_seg_header *hp;
	struct bwc_seg_trailer *tp;
	struct bwc_req_header *rhp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp = *nnpp;
	struct bwc_rb_node *nnp2 = NULL;
	char *buf = nnp->buf;
	uint32_t dlen, left, buf_left, req_nr_left;
	uint64_t my_seq, flushed_seq, pre_seq = 0;
	int rc = 0;
	ssize_t nread;

	tp = (struct bwc_seg_trailer *)buf;
	__print_seg_trailer(fd, tp, log_header);
	if (memcmp(tp->trailer_magic, dhp->dh_seg_trailer_magic, BWC_MAGICSZ)) {
		rc = 11;	// trailer magic dosen't match
		goto out;
	}

	if (tp->data_len > priv_p->p_segsz - (BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ)) {
		rc = 12;	//  data_len is too large
		goto out;
	}
	memcpy(scan_trailer, tp, sizeof(*tp));

	nnp2 = rbn_p->nn_op->nn_get_node(rbn_p);
	buf = nnp2->buf;
	debug_p = (char *)nnp2;
	assert(!(buf <= debug_p && (buf + BWC_RBN_BUFSZ) >= debug_p));
	(*seg_trailer_off) -= tp->data_len + tp->header_len + BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
	lseek(fd, *seg_trailer_off, SEEK_SET);
	nread = __read_n(fd, (void *)buf, BWC_RECOVER_MAXREAD);
	assert(nread == BWC_RECOVER_MAXREAD);
	tp = (struct bwc_seg_trailer *)buf;
	hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);
	if (memcmp(hp->header_magic, dhp->dh_seg_header_magic, BWC_MAGICSZ)) {
		__print_seg_trailer(fd, tp, log_header);
		__print_seg_header(fd, hp, log_header);
		rc = 1;	// header magic dosen't match
		goto out;
	} else {
		dlen = hp->header_len + hp->data_len;
		if (dlen > priv_p->p_segsz - (BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ)) {
			rc = 2;	// content len is too large
			goto out;
		}
 		my_seq = hp->my_seq;
		flushed_seq = hp->flushed_seq;
		if (my_seq < flushed_seq) {
			rc = 3;	// seq not reasonable
			goto out;
		}
	}
	memcpy(scan_header, hp, sizeof(*hp));
	*my_first = __my_first_seq(hp);

	/*
	 * Now the seq header checking has been passed.
	 * go through requst headers to do sanity checking
	 */
	buf_left = BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ - BWC_SEG_HEADERSZ;
	rhp = (struct bwc_req_header *)(buf + BWC_SEG_TRAILERSZ + BWC_SEG_HEADERSZ);
	left = scan_header->data_len;
	req_nr_left = scan_header->req_nr;

req_check:
	while (req_nr_left) {
		if (pre_seq && (rhp->my_seq != pre_seq + 1)) {
			rc = 4;	// req header corrupted
			goto out;
		}
		pre_seq = rhp->my_seq;
		left -=	rhp->data_len;
		req_nr_left--;
		buf_left -= BWC_REQ_HEADERSZ;
		if (!buf_left)
			break;
		rhp = (struct bwc_req_header *)((char *)rhp + BWC_REQ_HEADERSZ);
	}
	if (req_nr_left) {
		/*
		 * There are unprocessed req headers when all buf processed.
		 * Read BWC_RECOVER_MAXREAD more into buf.
		 */
		buf += BWC_RECOVER_MAXREAD;
		assert(buf <= nnp2->buf + BWC_RBN_BUFSZ - BWC_RECOVER_MAXREAD); // total read can't exceed the buf size
		nread = __read_n(fd, (void *)buf, BWC_RECOVER_MAXREAD);
		assert(nread == BWC_RECOVER_MAXREAD);
		buf_left = BWC_RECOVER_MAXREAD;
		rhp = (struct bwc_req_header *)buf;
		goto req_check;
	}

	if (pre_seq && pre_seq != my_seq) {
		rc = 5;
		goto out;
	}
	if (left) {
		rc = 6;
		goto out;
	}

	/*
	 * Now the header checking was passed.
	 * Let's checking the trailer part
	 */
	if (hp->header_len != scan_trailer->header_len) {
		rc = 12;
		goto out;
	} else if (hp->data_len != scan_trailer->data_len) {
		rc = 13;	// data_len doesn't match the header
		goto out;
	} else if (my_seq != scan_trailer->my_seq) {
		rc = 14;	// my_seq doesn't match the header
		goto out;
	} else if (flushed_seq != scan_trailer->flushed_seq) {
		rc = 15;	// flushed_seq doesn't match the header
		goto out;
	}

out:
	if (!rc && (*my_first > target_seq)) {
		list_add(&nnp2->nn_list, recover_buf_head);
	} else {
		nid_log_notice("%s: rc:%d, my_first%lu, target_seq%lu",
				log_header, rc, *my_first, target_seq);
		if (nnp2)
			rbn_p->nn_op->nn_put_node(rbn_p, nnp2);
	}
	*nnpp = nnp2;
	return rc;
}

/*
 * Input:
 * 	dtp: should pointer to a buffer not less than BWC_DEV_TRAILERSZ
 * Return:
 * 	0: good device trailer
 */
int
__bwc_dev_trailer_checking(struct bwc_private *priv_p, struct bwc_dev_trailer *dtp)
{
	char *log_header = "__bwc_dev_trailer_checking";
	int fd = priv_p->p_fhandle;
	struct bwc_seg_trailer *tp = priv_p->p_stp;
	struct bwc_seg_header *hp = priv_p->p_shp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp = NULL;
	char *buf;
	off_t seg_off;
	int rc = 0;
	ssize_t nread;
	char ver[BWC_VERSZ + 1];

	lseek(fd, priv_p->p_bufdevicesz - BWC_DEV_TRAILERSZ, SEEK_SET);	// the start point of the trailer
	nread = __read_n(fd, dtp, BWC_DEV_TRAILERSZ);
	assert(nread == BWC_DEV_TRAILERSZ);
	__print_dev_trailer(fd, dtp, log_header);
	if (memcmp(dtp->trailer_magic, BWC_DEV_TRAILER_MAGIC, BWC_MAGICSZ)) {
		rc = 1;
		goto out;
	} else if (memcmp(dtp->trailer_ver, BWC_DEV_TRAILER_VER, BWC_VERSZ)) {
		ver[BWC_VERSZ] = '\0';
		memcpy(ver, dtp->trailer_ver, BWC_VERSZ);
		nid_log_warning("%s: dev_trailer_verion not matching, binary version:{%s}, trailer_verion:{%s}",
				log_header, BWC_DEV_MDZ_VER, ver);
		if (__bwc_ver_cmp_check(dtp->trailer_ver, BWC_DEV_TRAILER_VER)) {
			nid_log_error("%s: dev_trailer_verion not compatible.",
					log_header);
			rc = 2;
			goto out;
		}

	} else if (dtp->dt_offset < 0 ||
	           dtp->dt_offset >= priv_p->p_bufdevicesz - BWC_DEV_TRAILERSZ) {
		rc = 3;
		goto out;
	}

	lseek(fd, dtp->dt_offset - BWC_SEG_TRAILERSZ, SEEK_SET);
	nread = __read_n(fd, tp, BWC_SEG_TRAILERSZ);
	assert(nread == BWC_SEG_TRAILERSZ);
	if (tp->my_seq != dtp->dt_my_seq) {
		__print_seg_trailer(fd, tp, log_header);
		rc = 21;
		goto out;
	}

	if (tp->data_len > priv_p->p_segsz -(BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ)) {
		__print_seg_trailer(fd, tp, log_header);
		rc = 22;
		goto out;
	}

	nnp = rbn_p->nn_op->nn_get_node(rbn_p);
	buf = nnp->buf;
	seg_off = dtp->dt_offset - (BWC_SEG_TRAILERSZ + tp->header_len + tp->data_len + BWC_SEG_HEADERSZ),
	lseek(fd, seg_off, SEEK_SET);
	nread = __read_n(fd, (void *)(buf + BWC_SEG_TRAILERSZ), BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
	assert(nread == BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
	if (__bwc_seg_checking(priv_p, hp, tp, &nnp, &seg_off, NULL, 0)) {
		__print_seg_trailer(fd, tp, log_header);
		__print_seg_header(fd, hp, log_header);
		rc = 23;
		goto out;
	}

	/*
	 * Don't check dt_flushed_seq. it may be not the same as (tp->flushed_seq).
	 */
out:
	if (nnp)
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	if (rc)
		nid_log_warning("%s: rc:%d", log_header, rc);
	return 0;
}

/*
 * backword scanning to find out the target segment
 * Input:
 * 	target_seq:
 * 		>0: looking for the first segment whose seq <= target_seq.
 * 		0:  use the flushed_seq stored in the dev_trailer as target seq
 * output:
 * 	start_off:
 * 	start_seq: the start segment my_seg
 * 	dtp:
 */
static int
__bwc_backward_scan(struct bwc_private *priv_p, uint64_t target_seq,
	off_t *start_off, uint64_t *start_seq, struct bwc_dev_trailer *dtp, struct list_head *recover_buf_head)
{
	char *log_header = "__bwc_backward_scan";
	int fd = priv_p->p_fhandle;
	struct bwc_seg_header *hp = priv_p->p_shp;
	struct bwc_seg_trailer *tp = priv_p->p_stp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp, *nnp_bak = NULL;
	char *buf;
	uint64_t my_first;
	int rc;
	off_t seg_trailer_off;
	uint32_t backward_scan_counter = 0;
	ssize_t nread;

	rc = __bwc_dev_trailer_checking(priv_p, dtp);	// make sure the dev_trailer is valid
	if (rc)
		goto out;
	if (!target_seq)
		target_seq = dtp->dt_flushed_seq;

	nnp = rbn_p->nn_op->nn_get_node(rbn_p);
	nnp_bak = nnp;
	debug_p = (char *)nnp_bak;
	buf = nnp->buf;
	seg_trailer_off = dtp->dt_offset - BWC_SEG_TRAILERSZ;
	lseek(fd, seg_trailer_off, SEEK_SET);		// pointer to immediately after the end of last segment
	nread = __read_n(fd, (void *)buf, BWC_SEG_TRAILERSZ);
	assert(nread == BWC_SEG_TRAILERSZ);

backward_next:
	backward_scan_counter++;
	rc = __bwc_seg_checking_rev(priv_p, hp, tp, &nnp, &seg_trailer_off, recover_buf_head, target_seq, &my_first);
	if (!rc && my_first > target_seq) {
		*start_seq = my_first;
		*start_off = seg_trailer_off + BWC_SEG_TRAILERSZ;
		goto backward_next;
	}
out:
	nid_log_warning("%s: backward_scan_counter:%u", log_header, backward_scan_counter);
	if (nnp_bak)
		rbn_p->nn_op->nn_put_node(rbn_p, nnp_bak);
	return rc;
}

/*
 * Output:
 * 	start_seq: the start segment my_seq
 */
int
__bwc_scan_bufdevice(struct bwc_private *priv_p, off_t *start_off, uint64_t *start_seq,
	uint64_t *flushed_seq, uint64_t *final_seq, struct bwc_dev_trailer *dtp, struct list_head *recover_buf_head)
{
	char *log_header = "__bwc_scan_bufdevice";
	int fd = priv_p->p_fhandle;
	struct bwc_seg_header *scan_header = priv_p->p_shp;;
	struct bwc_seg_trailer *scan_trailer = priv_p->p_stp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp = NULL;
	char *buf;
	uint64_t final_flushed_seq = 0, cur_seq = 0, cur_flushed_seq = 0;
	int first_corrupted = 0, seg_corrupted = 0;
	int rc = 0;
	off_t seg_off;
	uint32_t forward_scan_counter = 0;
	ssize_t nread;

	nid_log_warning("%s: start ...", log_header);

	/*
	 * checking the segment at the start point of the bufdevice
	 */
	nnp = rbn_p->nn_op->nn_get_node(rbn_p);
	buf = nnp->buf;
	seg_off = priv_p->p_first_seg_off;
	lseek(fd, seg_off, SEEK_SET);	// move to the start point of the first segment
	nread = __read_n(fd, (void *)(buf + BWC_SEG_TRAILERSZ), BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
	assert(nread == BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);

	first_corrupted = __bwc_seg_checking(priv_p, scan_header, scan_trailer, &nnp, &seg_off, recover_buf_head, cur_seq);
	if (first_corrupted) {
		/*
		 * the first segment is corrupted
		 */
		nid_log_warning("%s: the first segment is corrupted(%d)",
			log_header, first_corrupted);
	} else {
		*start_off = priv_p->p_first_seg_off;
		*start_seq = __my_first_seq(scan_header);	// seq of the first req in the seqment
	}

	if (!first_corrupted) {
		/*
		 * forward (clockwise) scanning to find out the final valid segment
		 */
		cur_seq = scan_header->my_seq;
		cur_flushed_seq = scan_header->flushed_seq;

		/*
		 * Now the file pointer to start position of the second segment
		 */
forward_next:
		forward_scan_counter++;
		seg_corrupted = __bwc_seg_checking(priv_p, scan_header, scan_trailer, &nnp, &seg_off, recover_buf_head, cur_seq);
		if (!seg_corrupted && scan_header->my_seq > cur_seq) {
			cur_seq = scan_header->my_seq;
			cur_flushed_seq = scan_header->flushed_seq;
			nid_log_debug("%s: cur_seq:%lu, cur_flushed_seq:%lu",
				log_header, cur_seq, cur_flushed_seq);
			goto forward_next;
		} else {
			nid_log_warning("%s: seg_corrupted:%d, scan_header->my_seq:%lu, cur_seq:%lu, forward_scan_counter:%u",
				log_header, seg_corrupted, scan_header->my_seq, cur_seq, forward_scan_counter);
		}

		/*
		 * we got the last segment
		 */
		final_flushed_seq = cur_flushed_seq;
		*final_seq = cur_seq;
		if (!((final_flushed_seq == 0) && (*start_seq == 1)) && (final_flushed_seq < *start_seq)) {
			/*
			 * backward (counterclockwise) scanning to find out the segment
			 * with final_flushed_seq less or equal to its seq
			 */
			nid_log_warning("%s: need __bwc_backward_scan, start_seq:%lu, final_flushed_seq:%lu",
				log_header, *start_seq, final_flushed_seq);
			rc = __bwc_backward_scan(priv_p, final_flushed_seq, start_off, start_seq, dtp, recover_buf_head);
		}
	} else {
		/*
		 * backward (counterclockwise) scanning
		 */
		nid_log_warning("%s: need __bwc_backward_scan", log_header);
		rc = __bwc_backward_scan(priv_p, 0, start_off, start_seq, dtp, recover_buf_head);
		if (!rc) {
			final_flushed_seq = dtp->dt_flushed_seq;
			*final_seq = dtp->dt_my_seq;
		}
	}

	if (nnp)
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	*flushed_seq = final_flushed_seq;
	return rc;
}

/*
 * Requirment:
 * 	the file pointer to the start point of the segment to check
 *
 * Return:
 * 	0: good segment
 * 	1: header magic dosen't match
 * 	2: seq not reasonable
 * 	3: flushed_Seq is not reasonable
 * 	4: a request header corrupted
 * 	5:
 * 	11:trailer magic dosen't match
 * 	12:trailer data_len doesn't match the header
 * 	13:trailer my_seq doesn't match the header
 * 	14:trailer flushed_seq doesn't match the header
 *
 * Notice:
 * 	the device file pointer will point to immediately after the segment
 * 	if return success.
 */
int
__bwc_seg_checking(struct bwc_private *priv_p, struct bwc_seg_header *scan_header, struct bwc_seg_trailer *scan_trailer,
	struct bwc_rb_node **nnpp, off_t *seg_off, struct list_head *recover_buf_head, uint64_t cur_seq)
{
	char *log_header = "__bwc_seg_checking";
	int fd = priv_p->p_fhandle;
	struct bwc_dev_header *dhp = priv_p->p_dhp;
	struct bwc_seg_header *hp;
	struct bwc_seg_trailer *tp;
	struct bwc_req_header *rhp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp = *nnpp;
	struct bwc_rb_node *nnp2 = NULL;
	char *buf = nnp->buf;
	uint32_t left, buf_left, req_nr_left;
	uint64_t my_seq, flushed_seq, pre_seq = 0;
	int rc = 0;
	ssize_t nread;

	/*
	 * check the header first
	 */
	hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);
	if (memcmp(hp->header_magic, dhp->dh_seg_header_magic, BWC_MAGICSZ)) {
		__print_seg_header(fd, hp, log_header);
		rc = 1;	// header magic dosen't match
		goto out;
	} else {
		if (hp->data_len > priv_p->p_segsz - (BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ)) {
			__print_seg_header(fd, hp, log_header);
			rc = 2;	// data_len is too large
			goto out;
		}
 		my_seq = hp->my_seq;
		flushed_seq = hp->flushed_seq;
		if (my_seq < flushed_seq) {
			__print_seg_header(fd, hp, log_header);
			rc = 3;	// seq not reasonable
			goto out;
		}
	}
	memcpy((void *)scan_header, (const void *)hp, sizeof(*hp));

	/*
	 * Now the seq header checking has been passed.
	 * go through request headers to do sanity checking
	 */
	buf_left = BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ - BWC_SEG_HEADERSZ;
	rhp = (struct bwc_req_header *)(buf + BWC_SEG_TRAILERSZ + BWC_SEG_HEADERSZ);
	left = scan_header->data_len;
	req_nr_left = scan_header->req_nr;

req_check:
	while (req_nr_left) {
		if (pre_seq && (rhp->my_seq != pre_seq + 1)) {
			rc = 4;	// req header corrupted
			goto out;
		}
		pre_seq = rhp->my_seq;
		left -=	rhp->data_len;
		req_nr_left--;
		buf_left -= BWC_REQ_HEADERSZ;
		if (!buf_left)
			break;
		rhp = (struct bwc_req_header *)((char *)rhp + BWC_REQ_HEADERSZ);
	}
	if (req_nr_left) {
		/*
		 * There are unprocessed req headers when all buf processed.
		 * Read BWC_RECOVER_MAXREAD more into buf.
		 */
		buf += BWC_RECOVER_MAXREAD;
		assert(buf <= nnp->buf + BWC_RBN_BUFSZ - BWC_RECOVER_MAXREAD); // total read can't exceed the buf size
		nread = __read_n(fd, (void *)buf, BWC_RECOVER_MAXREAD);
		assert(nread == BWC_RECOVER_MAXREAD);
		buf_left = BWC_RECOVER_MAXREAD;
		rhp = (struct bwc_req_header *)buf;
		goto req_check;
	}

	if (pre_seq && pre_seq != my_seq) {
		rc = 5;
		goto out;
	}
	if (left) {
		rc = 6;
		goto out;
	}

	/*
	 * Let's checking the trailer part
	 */
	nnp2 = rbn_p->nn_op->nn_get_node(rbn_p);
	buf = nnp2->buf;
	(*seg_off) += BWC_SEG_HEADERSZ + hp->header_len + hp->data_len;
	lseek(fd, *seg_off, SEEK_SET);
	nread =	__read_n(fd, (void *)buf, BWC_RECOVER_MAXREAD);
	assert(nread == BWC_RECOVER_MAXREAD);
	tp = (struct bwc_seg_trailer *)buf;
	if (memcmp(tp->trailer_magic, dhp->dh_seg_trailer_magic, BWC_MAGICSZ)) {
		__print_seg_trailer(fd, tp, log_header);
		__print_seg_header(fd, hp, log_header);
//		nid_log_warning("%s: trailer magic error,magic:{%s} dlen:%u, data_len:%u, my_seq:%lu, flushed_seq:%lu",
//			log_header, tp->trailer_magic, hp->data_len, tp->data_len, tp->my_seq, tp->flushed_seq);
		//assert(0);
		rc = 11;		// trailer magic dosen't match
		goto out;
	} else {
		if (hp->data_len!= tp->data_len) {
			rc = 12;	// data_len doesn't match the header
			assert(0);
			goto out;
		} else if (my_seq != tp->my_seq) {
			rc = 13;	// my_seq doesn't match the header
			goto out;
		} else if (flushed_seq != tp->flushed_seq) {
			rc = 14;	// flushed_seq doesn't match the header
			goto out;
		} else if (hp->header_len != tp->header_len) {
			rc = 15;	// header_len doesn't match the header
			goto out;
		}
	}
	memcpy((void *)scan_trailer, (const void *)tp, sizeof(*tp));
	(*seg_off) += BWC_SEG_TRAILERSZ;

out:
	if (recover_buf_head && !rc && (scan_header->my_seq > cur_seq)) {
		list_add_tail(&nnp->nn_list, recover_buf_head);
	} else {
		nid_log_warning("%s: recover_buf_head:%p, rc:%d, my_seq:%lu, cur_seq:%lu",
				log_header, recover_buf_head, rc, scan_header->my_seq, cur_seq);
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	}
	*nnpp = nnp2;
	return rc;
}

/*
 * Input:
 * 	seg_content: seg data pointer
 * 	seg_hp: seg header pointer
 */
static int
__bwc_seg_restore(struct bwc_private *priv_p, struct bwc_seg_header *seg_hp,
		struct bwc_req_header *req_hp, off_t doff, uint64_t pre_seq)
{
	char *log_header = "__bwc_seg_restore";
	struct pp_interface *pp_p = priv_p->p_pp;
	struct bwc_chain *chain_p = NULL;
	struct ddn_interface *ddn_p;
	struct data_description_node *np;
	struct pp_page_node *pnp;
	uint32_t pagesz = priv_p->p_pagesz;
	uint16_t cur_id = 0;
	uint32_t i;


	for (i = 0; i < seg_hp->req_nr; i++) {
		if (req_hp->my_seq <= pre_seq){
	//		nid_log_warning("%s: overlap seq:%lu, pre_seq:%lu", log_header, req_hp->my_seq, pre_seq);
			req_hp = (struct bwc_req_header *)((char *)req_hp + BWC_REQ_HEADERSZ);        // pointer to next req header
			continue;
		}
		if (!req_hp->data_len) {
			req_hp = (struct bwc_req_header *)((char *)req_hp + BWC_REQ_HEADERSZ);	// pointer to next req header
			continue;
		}
		if (req_hp->resource_id != cur_id) {
			chain_p = __get_inactive_chain_by_id(priv_p, req_hp->resource_id);
			if (!chain_p) {
				nid_log_error("%s: cannot find matched chan id (%u)",
					log_header, req_hp->resource_id);
				assert(0);
				return -1;
			}
			cur_id = chain_p->c_id;
		}
		assert(chain_p);

		pnp = chain_p->c_page;
		if (!pnp) {
			if (priv_p->p_ssd_mode) {
				chain_p->c_page = pp_p->pp_op->pp_get_node_nondata(pp_p);
			} else {
				chain_p->c_page = pp_p->pp_op->pp_get_node_forcibly(pp_p);
			}
			pnp = chain_p->c_page;
			chain_p->c_pos = pnp->pp_page;
			pnp->pp_start_seq = 0;
			pnp->pp_occupied = 0;
			pnp->pp_flushed = 0;
			pnp->pp_where = 0;
			INIT_LIST_HEAD(&pnp->pp_flush_head);
			INIT_LIST_HEAD(&pnp->pp_release_head);
			lck_node_init(&pnp->pp_lnode);
		}

		if (pnp->pp_occupied + req_hp->data_len > pagesz) {
			uint32_t wasted_space;

			/*
			 * this page does not have space to save data from this request
			 * If there is some space left unoccupied, we can cheat the flushing thread
			 * by adding the left size to the occupied size, so it's ready to flush.
			 * Of course, this added part will never be flushed to the bufdevice,
			 * so also add this cheating size to flushed size.
			 */
			assert(!pnp->pp_flushed);
			wasted_space = pagesz - pnp->pp_occupied;
			pnp->pp_flushed += wasted_space;	// wasted space, never flushed
			pnp->pp_occupied += wasted_space;	// wasted space
			__bwc_start_page(chain_p, pnp, 1);

			/* start a new page */
			if (priv_p->p_ssd_mode) {
				chain_p->c_page = pp_p->pp_op->pp_get_node_nondata(pp_p);
			} else {
				chain_p->c_page = pp_p->pp_op->pp_get_node_forcibly(pp_p);
			};
			pnp = chain_p->c_page;
			chain_p->c_pos = pnp->pp_page;
			pnp->pp_start_seq = 0;
			pnp->pp_occupied = 0;
			pnp->pp_flushed = 0;
			pnp->pp_where = 0;
			INIT_LIST_HEAD(&pnp->pp_flush_head);
			INIT_LIST_HEAD(&pnp->pp_release_head);
			lck_node_init(&pnp->pp_lnode);
		}

		ddn_p = &chain_p->c_ddn;
		np = ddn_p->d_op->d_get_node(ddn_p);
		np->d_page_node = pnp;
		if (req_hp->wc_offset){
			np->d_pos = req_hp->wc_offset;
		} else {
			np->d_pos = doff; //chain_p->c_pos;
		}
		np->d_location_disk = 1;
//		memcpy(np->d_pos, dp, req_hp->data_len);
		chain_p->c_pos += req_hp->data_len;
		np->d_seq = req_hp->my_seq;
		np->d_len = req_hp->data_len;
		np->d_offset = req_hp->offset;
		np->d_flushing = 0;
		np->d_flushed = 0;
		np->d_flushed_back = 0;
		np->d_flush_done = 0;
		np->d_released = 0;
		np->d_where = DDN_WHERE_NO;
		np->d_flush_overwritten = 0;
		np->d_callback_arg = NULL;
		list_add_tail(&np->d_slist, &chain_p->c_search_head);
		nid_log_debug("%s: add data node %p to chain %p search list %p", log_header, np,
				chain_p, &chain_p->c_search_head);
		pnp->pp_occupied += np->d_len;
//		dp += np->d_len;
		doff += np->d_len;

		// move to next data
		req_hp = (struct bwc_req_header *)((char *)req_hp + BWC_REQ_HEADERSZ);	// pointer to next req header
	}
	return 0;
}

/*
 * Input:
 * 	flushed_seq < final_seq
 */
int
__bwc_bufdevice_restore(struct bwc_interface *bwc_p, off_t start_off, uint64_t start_seq, uint64_t flushed_seq,
	uint64_t final_seq, struct bwc_dev_trailer *dtp, struct list_head *recover_buf_head)
{
	char *log_header = "__bwc_bufdevice_restore";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	int fd = priv_p->p_fhandle;
	struct bse_interface *bse_p;
	struct bwc_chain *chain_p;
	struct pp_page_node *pnp;
	struct bwc_seg_header *hp;
	uint64_t pre_seq;
	struct bwc_req_header *rhp;
	uint32_t pagesz = priv_p->p_pagesz;
	uint16_t noccupied = 0;
	off_t doff = start_off;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp, *nnp2;
	char *buf;
	int rc;

	nid_log_warning("%s: start (start_off:%ld start_seq:%lu, flushed_seq:%lu, final_seq:%lu) ...",
		log_header, start_off, start_seq, flushed_seq, final_seq);
	assert(flushed_seq < final_seq);

	/* skip all requests which had been flushed */
	pre_seq = 0;
	list_for_each_entry_safe(nnp, nnp2, struct bwc_rb_node, recover_buf_head, nn_list) {
		//nid_log_warning("%s: move to next node", log_header);
		buf = nnp->buf;
		hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);
		doff += BWC_SEG_HEADERSZ;
		nid_log_debug("%s: skip seg myseq:%lu, flushseq:%lu", log_header, hp->my_seq, flushed_seq);
		assert(hp->my_seq >= start_seq && hp->my_seq <= final_seq);
		assert(pre_seq < hp->my_seq);
		if (hp->my_seq > flushed_seq)
			break;

		/*
		 * since hp->my_seq is the largest seq of all requests in this segment
		 * So all requests in this segment should be skipped
		 */
		if (dtp && (dtp->dt_my_seq == hp->my_seq)) {
			/* I'm the last seg on disk, need rewind */
			doff = priv_p->p_first_seg_off;
		} else {
			/* to next seg */
			doff += hp->header_len + hp->data_len + BWC_SEG_TRAILERSZ;
		}
		pre_seq = hp->my_seq;
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	}

	priv_p->p_cur_offset = doff;
	priv_p->p_cur_flush_block = (priv_p->p_cur_offset - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
	priv_p->p_cur_block = priv_p->p_cur_flush_block;
	nid_log_warning("%s: flush_block:%u, flush_seq:%lu", log_header, priv_p->p_cur_flush_block, flushed_seq);
//	nid_log_warning("temp_test2:%u", priv_p->p_cur_flush_block);

	for (;;)  {
		nid_log_debug("%s: scanning seq:%lu, len:%u, cur_offset:%ld, cur_block:%u, noccupied:%u",
			log_header, hp->my_seq, hp->data_len, priv_p->p_cur_offset, priv_p->p_cur_block, noccupied);

		rhp = (struct bwc_req_header *)(buf + BWC_SEG_TRAILERSZ + BWC_SEG_HEADERSZ);
		doff += hp->header_len;
		rc = __bwc_seg_restore(priv_p, hp, rhp, doff, pre_seq);
		if (rc) {
			nid_log_error("%s: segment error", log_header);
			__print_seg_header(fd, hp, log_header);
		}

		doff += hp->data_len + BWC_SEG_TRAILERSZ;
		priv_p->p_cur_offset = doff;
		nid_log_debug("%s: after __bwc_seg_restore, cur_offset:%ld",
			log_header, priv_p->p_cur_offset);
		pthread_mutex_lock(&priv_p->p_occ_lck);
		while (priv_p->p_cur_offset > priv_p->p_flushing_st[priv_p->p_cur_block].f_end_offset) {
			priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = hp->my_seq;
			priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
			priv_p->p_noccupied = ++noccupied;
			assert(noccupied <= priv_p->p_flush_nblocks);
			priv_p->p_cur_block++;
		}
		pthread_mutex_unlock(&priv_p->p_occ_lck);

		/* stop if this is the last request */
		if (hp->my_seq == final_seq)
			break;
		assert(hp->my_seq < final_seq);

		/* if need rewind */
		if (dtp && (dtp->dt_my_seq == hp->my_seq)) {
			nid_log_debug("%s: need rewind: scanning seq:%lu, len:%u, cur_offset:%ld, cur_block:%u, noccupied:%u",
					log_header, hp->my_seq, hp->data_len, priv_p->p_cur_offset, priv_p->p_cur_block, noccupied);
			pthread_mutex_lock(&priv_p->p_occ_lck);
			while (priv_p->p_cur_block < priv_p->p_flush_nblocks) {
				priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = priv_p->p_seq_assigned;
				priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
				priv_p->p_noccupied = ++noccupied;
				priv_p->p_cur_block++;
			}
			pthread_mutex_unlock(&priv_p->p_occ_lck);
			assert(priv_p->p_noccupied <= priv_p->p_flush_nblocks);

			doff = priv_p->p_first_seg_off;
			priv_p->p_cur_offset = priv_p->p_first_seg_off;
			/* cannot assume start from block 0, in case BWC_DEV_HEADERSZ is too large */
			priv_p->p_cur_block = (priv_p->p_first_seg_off - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
		}

		/* next segment */
		pre_seq = hp->my_seq;
		assert(nnp->nn_list.next != recover_buf_head);
		nnp2 = list_entry(nnp->nn_list.next, struct bwc_rb_node, nn_list);
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
		nnp = nnp2;
		buf = nnp->buf;
		hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);
		assert(pre_seq <= hp->my_seq);
		doff += BWC_SEG_HEADERSZ;
		__print_seg_header(fd, hp, log_header);
	}
	/* seek the fd to where to start new IO */
	lseek(fd, priv_p->p_cur_offset, SEEK_SET);

	priv_p->p_lck_if.l_op->l_rlock(&priv_p->p_lck_if, &priv_p->p_lnode);
	list_for_each_entry(chain_p, struct bwc_chain, &priv_p->p_inactive_head, c_list) {
		if (chain_p->c_page) {
			uint32_t wasted_space;

			pnp = chain_p->c_page;
			assert(!pnp->pp_flushed);
			wasted_space = pagesz - pnp->pp_occupied;
			pnp->pp_flushed += wasted_space;
			pnp->pp_occupied = pagesz;
			__bwc_start_page(chain_p, pnp, 1);
		}

		if (!list_empty(&chain_p->c_search_head)) {
			nid_log_debug("%s: channel %d has c_search_head", log_header, chain_p->c_id);
			pnp = chain_p->c_page;
			assert(pnp);
			bse_p = &chain_p->c_bse;
			bse_p->bs_op->bs_add_list(bse_p, &chain_p->c_search_head);
		} else {
			nid_log_debug("%s: channel %d does not have c_search_head", log_header, chain_p->c_id);
		}

	}
	priv_p->p_lck_if.l_op->l_runlock(&priv_p->p_lck_if, &priv_p->p_lnode);

	/*
	 * set p_noccupied at last step to prevent the release_thread from doing release early
	 */
	pthread_mutex_lock(&priv_p->p_occ_lck);
	priv_p->p_noccupied = noccupied;
	pthread_mutex_unlock(&priv_p->p_occ_lck);

	return 0;
}

