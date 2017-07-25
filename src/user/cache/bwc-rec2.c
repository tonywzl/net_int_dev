#include "bwc.h"

/*
 * Algorithm:
 * 	Scan mdz requests to make pseudo hdz segments and add segment meta
 * 	into recovery list.
 * 	no hdz rewind will happen in a generated pseudo segment
 */
static void
__bwc_mdz_req_scan(struct bwc_private *priv_p, struct bwc_req_header *mrhp,
	uint32_t req_to_scan, off_t *start_off, uint64_t *last_mdz_seq,
	struct list_head *recover_buf_head, int *hdz_rewind,
	uint64_t *start_seq)
{
	char *log_header = "__bwc_mdz_req_scan";
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	uint32_t rh_len_real, data_len_real, i, nreq;
	char *scan_buf;
	struct bwc_seg_header *seg_header;
	off_t first_wc_offset, pre_offset;
	uint32_t scan_left = req_to_scan;
	static int non_512 = 0, first_part = 1;

	nid_log_warning("%s: start", log_header);
	if (first_part) {
		assert(mhp->md_empty || mrhp->my_seq == mhp->md_start_seq);
		first_part = 0;
	}

	while (scan_left) {
		if (mrhp->my_seq > mhp->md_last_flushed_seq)
			break;
		scan_left--;
		mrhp++;
	}
	nid_log_warning("%s: %d flushed requests got skipped",
		log_header, req_to_scan - scan_left);

	if (scan_left == 0) {
		nid_log_warning("%s: the whole mdz part was flushed",
			log_header);
		return;
	}

	if (*start_seq == 0) {
		*start_seq = mrhp->my_seq;
		nid_log_warning("%s: mdz scan from seq: %lu",
			log_header, *start_seq);
	}

	first_wc_offset = mrhp->wc_offset;
	nid_log_warning("%s: first_wc_offset: %ld, first_seq:%lu",
		log_header, first_wc_offset, mrhp->my_seq);
	if (*start_off == 0) {
		*start_off = first_wc_offset;
	}
	pre_offset = first_wc_offset;

	while (scan_left) {
		nnp = rbn_p->nn_op->nn_get_node(rbn_p);
		scan_buf = nnp->buf + BWC_SEG_TRAILERSZ + BWC_SEG_HEADERSZ;
		seg_header = (struct bwc_seg_header *)(nnp->buf + BWC_SEG_TRAILERSZ);
		nreq = (scan_left > BWC_MDZ_SCAN_N_SEG) ? BWC_MDZ_SCAN_N_SEG : scan_left;
		data_len_real = 0;
		nid_log_debug("mdz_scan_seg: seq:%lu, pos:%lu, d_len:%u",
			mrhp->my_seq, mrhp->wc_offset, mrhp->data_len);

		for (i = 0; i < nreq; ++mrhp, ++i) {
			/*
			 * (mrhp)->wc_offset < first_wc_offset means
			 * the according hdz seg is rewinded
			 */

			if ((!(*hdz_rewind)) && (mrhp->wc_offset < first_wc_offset)) {
				*hdz_rewind += 1;
				pre_offset = 0;
				nid_log_warning("%s: hdz_need_rewind %d, "
					"mrhp seq:%lu, wc_offset :%lu, "
					"first_wc_offset: %ld",
					log_header, *hdz_rewind, mrhp->my_seq,
					mrhp->wc_offset, first_wc_offset);

				/*
				 * stop now so no hdz rewind wil exist in a
				 * pseudo segment
				 */
				break;
			}
			assert(mrhp->wc_offset >= pre_offset);
			data_len_real += mrhp->data_len;
			pre_offset = mrhp->wc_offset;
			nid_log_debug("%s: seq:%lu, pos:%lu, d_len:%u",
				log_header, mrhp->my_seq, mrhp->wc_offset,
				mrhp->data_len);
		}
		if (i == 0) {
			rbn_p->nn_op->nn_put_node(rbn_p, nnp);
			continue;
		}

		mrhp--;	// to the last request header
		seg_header->my_seq = mrhp->my_seq;
		seg_header->req_nr = i;
		if (i != BWC_MDZ_SCAN_N_SEG)
			non_512++;
		assert(non_512 <= 4);
		rh_len_real = seg_header->req_nr * BWC_REQ_HEADERSZ;	// length of all request headers in the segment
		mrhp -= seg_header->req_nr - 1;				// to the first request header
		memcpy((void *)scan_buf, (void *)mrhp, rh_len_real);	// copy all request headers in this segment
		mrhp += seg_header->req_nr;				// start point for next segment
		seg_header->header_len = __round_up(rh_len_real, priv_p->p_dio_alignment);
		seg_header->data_len =  __round_up(data_len_real, priv_p->p_dio_alignment);
		list_add_tail(&nnp->nn_list, recover_buf_head);
		scan_left -= seg_header->req_nr;
	}

	if (last_mdz_seq)
		*last_mdz_seq = seg_header->my_seq;
}

/*
 * Input:
 * 	seg_hp: the segment to process, must be non-empty
 */
static int
__bwc_seg_restore2(struct bwc_private *priv_p, struct bwc_seg_header *seg_hp,
	struct bwc_req_header *req_hp, uint64_t last_mdz_seq)
{
	char *log_header = "__bwc_seg_restore2";
	struct pp_interface *pp_p = priv_p->p_pp;
	struct bwc_chain *chain_p = NULL;
	struct ddn_interface *ddn_p;
	struct data_description_node *np;
	struct pp_page_node *pnp;
	struct bwc_req_header mrh, *end_req, *first_req;
	uint32_t pagesz = priv_p->p_pagesz;
	uint16_t cur_id = 0;

	first_req = (struct bwc_req_header *)((char *)seg_hp + BWC_SEG_HEADERSZ);
	end_req = first_req + (seg_hp->req_nr - 1);
	assert(req_hp->my_seq >= first_req->my_seq && req_hp->my_seq <= end_req->my_seq);
	while (req_hp <= end_req) {
		assert(req_hp->my_seq <= seg_hp->my_seq);
		/*
		 * If this is a request which is not in mdz, must insert it to mdz
		 */
		if (req_hp->my_seq > last_mdz_seq) {
			assert(!req_hp->is_mdz);
			memset(&mrh, 0, sizeof(mrh));
			mrh.resource_id = req_hp->resource_id;
			mrh.data_len = req_hp->data_len;
			mrh.wc_offset = req_hp->wc_offset;
			mrh.my_seq = req_hp->my_seq;
			mrh.offset = req_hp->offset;
			mrh.is_mdz = 1;
			__bwc_mdz_add_request(priv_p, &mrh);
		}

		if (!req_hp->is_mdz && req_hp->my_seq <= last_mdz_seq) {
//			nid_log_warning("%s: overlap seq:%lu, last_mdz_seq:%lu",
//				log_header, req_hp->my_seq, last_mdz_seq);
			req_hp++;
			continue;
		}

		if (!req_hp->data_len) {
			req_hp++;	// bypass empty request
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
		//	nid_log_warning("temp_test_pp: start a new page: %p, start seq: %lu", pnp, req_hp->my_seq);
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
		//	nid_log_warning("temp_test_pp: end an old page: %p, end seq: %lu", pnp, req_hp->my_seq - 1);

			/* start a new page */
			if (priv_p->p_ssd_mode) {
				/* Risk here, if data flush not no time, the data on SSD may overwrite.*/
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
		//	nid_log_warning("temp_test_pp: start a new page: %p, start seq: %lu", pnp, req_hp->my_seq);
		}

		ddn_p = &chain_p->c_ddn;
		np = ddn_p->d_op->d_get_node(ddn_p);
		np->d_page_node = pnp;
		assert(req_hp->wc_offset);
		np->d_pos = req_hp->wc_offset;
		np->d_location_disk = 1;
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

		// move to next data
		req_hp = (struct bwc_req_header *)((char *)req_hp + BWC_REQ_HEADERSZ);	// pointer to next req header
	}
	return 0;
}

/*
 * Algorithm:
 * 	Add a new request header in memory, but not flush
 */
void
__bwc_mdz_add_request(struct bwc_private *priv_p, struct bwc_req_header *new_mrhp)
{
	uint32_t cur_req;
	struct bwc_req_header *mrhp;
	char *p;

	if (!priv_p->p_mdz_recover)
		return;

	cur_req = priv_p->p_mdz_next_req++;
	p = (char *)priv_p->p_mdz_rhp;
	mrhp = (struct bwc_req_header *)(p + cur_req * priv_p->p_mdz_reqheadersz);
	memcpy(mrhp, new_mrhp, priv_p->p_mdz_reqheadersz);
	assert((new_mrhp->data_len && new_mrhp->resource_id) || !new_mrhp->resource_id);
	assert(new_mrhp->my_seq == priv_p->p_mdz_last_seq + 1 || !priv_p->p_mdz_last_seq);
	priv_p->p_mdz_last_seq = new_mrhp->my_seq;

	//nid_log_warning("__bwc_mdz_add_request: add req:%lu", mrhp->my_seq);
//	nid_log_warning("temp_test: p_mdz_next_req:%u, p_mdz_last_seq:%lu",
//			priv_p->p_mdz_next_req, priv_p->p_mdz_last_seq);
}

void
__bwc_mdz_flush_header(struct bwc_private *priv_p)
{
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	int fd = priv_p->p_fhandle;
	ssize_t nwritten;

	if (!priv_p->p_mdz_recover)
		return;

	MD5((void *)mhp->md_magic_ver, sizeof(*mhp) - sizeof(mhp->md_checksum), mhp->md_checksum);
	nwritten = __pwrite_n(fd, mhp, priv_p->p_mdz_headersz, 0);					// the header
	assert(nwritten == priv_p->p_mdz_headersz);
	nwritten = __pwrite_n(fd, mhp, priv_p->p_mdz_headersz, priv_p->p_mdz_size - priv_p->p_mdz_headersz);	// the trailer
	assert(nwritten == priv_p->p_mdz_headersz);
	nid_log_debug ("mdz_flush_header: start_seg:%u, end_seg:%u, flushed_seq:%lu, md_last_hseg_off:%lu",
		mhp->md_start_seg, mhp->md_end_seg, mhp->md_flushed_seq, mhp->md_last_hseg_off);
}

/*
 * Algorithm:
 * 	Update data part offset on write cache for each hdz request header of one segment.
 */
void
__bwc_hdz_update_offset(struct bwc_private *priv_p, uint32_t req_nr,
	uint32_t rh_len, off_t seg_off)
{
	struct bwc_req_header *rhp = (struct bwc_req_header *)priv_p->p_rh_vec;
	off_t data_off;

	if (!priv_p->p_mdz_recover)
		return;
	data_off = seg_off + BWC_SEG_HEADERSZ + rh_len;
	while (req_nr) {
		rhp->wc_offset = data_off;
//		nid_log_warning("temp_test_rqh: my_seq: %lu, wc_offset: %lu",
//			rhp->my_seq, rhp->wc_offset);
		data_off += rhp->data_len;
		rhp++;
		req_nr--;
	}
}

/*
 * Algorithm:
 * 	setup data offset in write cache since when doing update, we may
 * 	not know data offset in write cache
 */
void
__bwc_mdz_update_offset(struct bwc_private *priv_p, uint32_t start_req,
	off_t start_off)
{
	struct bwc_req_header *mrhp;
	char *p;

	if (!priv_p->p_mdz_recover)
		return;

//	nid_log_warning("new seg start: first data off %lu", start_off);
	p = (char *)priv_p->p_mdz_rhp;
	mrhp = (struct bwc_req_header *)(p + start_req * priv_p->p_mdz_reqheadersz);
	while (start_req < priv_p->p_mdz_next_req) {
		mrhp->wc_offset = start_off;
//		nid_log_warning("in seg: seq: %lu, data off %lu d_len:%u",
//			mrhp->my_seq, mrhp->wc_offset, mrhp->data_len);
		assert (mrhp->wc_offset);
		start_req++;
		start_off += mrhp->data_len;
		mrhp++;
	}
}

/*
 * Input:
 * 	shp: hdz segment header
 */
void
__bwc_mdz_try_invalid(struct bwc_private *priv_p, struct bwc_seg_header *shp)
{
	char *log_header = "__bwc_mdz_try_invalid";
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	struct bwc_dev_trailer *dtp = priv_p->p_dtp;
	off_t seg_off = shp->wc_offset;
	off_t end_seg_off;

	if (!priv_p->p_mdz_recover)
		return;

	end_seg_off = seg_off + BWC_SEG_HEADERSZ + shp->header_len + shp->data_len + BWC_SEG_TRAILERSZ;
	if (!priv_p->p_mdz_last_is_trail && mhp->md_last_hseg_seq == dtp->dt_my_seq)
		priv_p->p_mdz_last_is_trail = 1;

	if ((seg_off <= mhp->md_last_hseg_off && end_seg_off >= mhp->md_last_hseg_off) ||
	    (priv_p->p_mdz_last_is_trail && (mhp->md_last_hseg_seq != dtp->dt_my_seq))) {
		mhp->md_empty = 1;
		mhp->md_start_seg = mhp->md_end_seg;
		__bwc_mdz_flush_header(priv_p);
		nid_log_warning("%s: invalid the mdz", log_header);
	}
//	nid_log_warning("temp_test1: last_offset:%lu, cur seg_start_off:%lu, cur seg_end_off:%lu, distance:%d",
//		       	mhp->md_last_hseg_off, seg_off, end_seg_off, (int)(mhp->md_last_hseg_off - end_seg_off));
}

/*
 * Input:
 * 	seg_off: the offset of the hdz segment whose requests are being added in mdz
 * 	shp: hdz segment header
 */
void
__bwc_mdz_try_flush(struct bwc_private *priv_p, struct bwc_seg_header *shp)
{
	char *log_header = "__bwc_mdz_try_flush";
	int fd = priv_p->p_fhandle;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	struct bwc_req_header *rhp;
	uint32_t new_flush_seg;
	int need_flush_header = 0;
	off_t seg_off = shp->wc_offset;
	uint32_t i,
		segsz_byte = priv_p->p_mdz_reqheadersz * priv_p->p_mdz_segsz;
	ssize_t nwritten;

	if (!priv_p->p_mdz_recover)
		return;

	/*
	 * Don't flush to device until there are at least p_mdz_segsz
	 * request headers added in memory.
	 */
//	nid_log_warning("temp_test: p_mdz_next_req:%u p_mdz_segsz:%u",
//			priv_p->p_mdz_next_req, priv_p->p_mdz_segsz);
	if (priv_p->p_mdz_next_req < priv_p->p_mdz_segsz) {
		/* not accumulated one mdz segment yet */
		goto update_start;
	}

	if (!mhp->md_first_seg) {
		nid_log_warning("%s: got first segment", log_header);
		mhp->md_first_seg = 1;
	}

	/*
	 * need to flush one mdz segment
	 */
	if (mhp->md_empty) {
		/*
		 * using current md_end_seg. Neither md_end_seg nor md_start_seg
		 * should be changed
		 */
		assert(mhp->md_start_seg == mhp->md_end_seg);
		mhp->md_empty = 0;
		rhp = priv_p->p_mdz_rhp;
		if (!mhp->md_start_seq) {
			assert(rhp->my_seq == 1);
		} else {
			assert(rhp->my_seq == mhp->md_start_seq + priv_p->p_mdz_segsz);

			if (++mhp->md_start_seg == priv_p->p_mdz_seg_num)
				mhp->md_start_seg = 0;
			if (++mhp->md_end_seg == priv_p->p_mdz_seg_num)
				mhp->md_end_seg = 0;
		}
		mhp->md_start_seq = rhp->my_seq;
	} else {
		if (++mhp->md_end_seg == priv_p->p_mdz_seg_num)
			mhp->md_end_seg = 0;
		assert(mhp->md_start_seg != mhp->md_end_seg);
	}

	/* flush down one mdz segment */
	nwritten = __pwrite_n(fd, (void *)priv_p->p_mdz_rhp, segsz_byte,
		priv_p->p_mdz_headersz + segsz_byte * mhp->md_end_seg);
	assert(nwritten == segsz_byte);

	/*
	 * Move unflushed mdz request headers in memory to the start
	 * point of the buffer.
	 */
	if (priv_p->p_mdz_next_req > priv_p->p_mdz_segsz) {
		uint32_t headers_to_move = priv_p->p_mdz_next_req - priv_p->p_mdz_segsz;
		assert(priv_p->p_mdz_next_req < 2 * priv_p->p_mdz_segsz);
		memmove((void *)priv_p->p_mdz_rhp,
			(void *)(priv_p->p_mdz_rhp + priv_p->p_mdz_segsz),
	       		priv_p->p_mdz_reqheadersz * headers_to_move);
	}
	priv_p->p_mdz_next_req -= priv_p->p_mdz_segsz;

	/*
	 * update mdz header
	 */
	mhp->md_last_hseg_off = seg_off;
	mhp->md_last_hseg_seq = shp->my_seq;
	mhp->md_last_flushed_seq = shp->flushed_seq;
	mhp->md_last_seq = priv_p->p_mdz_last_seq & priv_p->p_mdz_segsz_mask;
	priv_p->p_mdz_last_is_trail = 0;
	need_flush_header = 1;

//	nid_log_warning("temp_test: md_last_seq:%lu, p_mdz_next_req:%u, p_seq_assigned:%lu",
//			mhp->md_last_seq, priv_p->p_mdz_next_req, priv_p->p_seq_assigned);
	assert(mhp->md_last_seq + priv_p->p_mdz_next_req == priv_p->p_seq_assigned);

update_start:
	if (priv_p->p_rec_seq_flushed >= mhp->md_flushed_seq + priv_p->p_mdz_segsz) {
		uint32_t new_seq_flushed = priv_p->p_rec_seq_flushed - mhp->md_flushed_seq;

		/*
		 * need move md_start_seg forward
		 */
		new_flush_seg = new_seq_flushed / priv_p->p_mdz_segsz;
//		nid_log_warning("%s: new_flush_seg:%u, md_flushed_seq:%lu, p_rec_seq_flushed:%lu, "
//			"md_start_seg:%u, md_end_seg:%u, md_empty:%d",
//			log_header, new_flush_seg, mhp->md_flushed_seq, priv_p->p_rec_seq_flushed,
//			mhp->md_start_seg, mhp->md_end_seg, mhp->md_empty);
		for (i = 0; i < new_flush_seg; i++) {
			if (!mhp->md_empty && mhp->md_start_seg == mhp->md_end_seg) {
//				nid_log_warning("%s: setting md_empty = 1, md_start_seg:%u, "
//					"rec_seq_flushed:%lu, md_flushed_seq:%lu",
//					log_header,mhp->md_start_seg, priv_p->p_rec_seq_flushed, mhp->md_flushed_seq);

				mhp->md_empty = 1;	// the last mdz segment got flushed
				i++;			// already processed this segment
				break;
			}
			mhp->md_start_seq += priv_p->p_mdz_segsz;
			if (++mhp->md_start_seg == priv_p->p_mdz_seg_num)
				/* need rewind */
				mhp->md_start_seg = 0;
		}
		assert(i == new_flush_seg);
		mhp->md_flushed_seq += priv_p->p_mdz_segsz * new_flush_seg;
		need_flush_header = 1;
	}

	if (need_flush_header)
		__bwc_mdz_flush_header(priv_p);
}

int
__bufdevice_rebuild_mdz_header(struct bwc_private *priv_p)
{
	char *log_header = "__bufdevice_rebuild_mdz_header";
	int fd = priv_p->p_fhandle;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	unsigned char checksum[16];
	ssize_t nread;
	char ver[BWC_VERSZ + 1];

	nid_log_warning("%s: start ...", log_header);
	if (!priv_p->p_mdz_recover)
		return 0;

	memset(checksum , 0, 16);
	lseek(fd, 0, SEEK_SET);
	nread = __read_n(fd, mhp, priv_p->p_mdz_headersz);
	assert(nread == priv_p->p_mdz_headersz);

	if (!mhp->md_start) {
		/* this is a new cache */
		nid_log_warning("%s: new bufdevice!", log_header);
		memset(mhp, 0, sizeof(*mhp));
		mhp->md_start = 1;
		mhp->md_size = priv_p->p_mdz_seg_num;
		mhp->md_empty = 1;
		memcpy(mhp->md_magic_ver, BWC_DEV_MDZ_VER, BWC_VERSZ);
		__bwc_mdz_flush_header(priv_p);
	} else {
		assert(mhp->md_size == priv_p->p_mdz_seg_num);
		MD5(( void *)mhp->md_magic_ver, sizeof(*mhp) - sizeof(mhp->md_checksum), checksum);
		if (memcmp(mhp->md_checksum, checksum, 16)){
			nid_log_error("%s: mdz_header_magic checking failed.", log_header);
			nid_log_error("%s: md_checksum: {%.16s} checksum: {%.16s}",log_header, mhp->md_checksum, checksum);
			return -1;
		} else {
			nid_log_error("%s: mdz_header_magic checking passed.", log_header);
		}

		ver[BWC_VERSZ] = '\0';
		memcpy(ver, mhp->md_magic_ver, BWC_VERSZ);
		if (memcmp(mhp->md_magic_ver, BWC_DEV_MDZ_VER, BWC_VERSZ)){
			nid_log_warning("%s: mdz_header_verion not matching, binary version:{%s}, mdz_header_verion:{%s}",
					log_header, BWC_DEV_MDZ_VER, ver);
			if (__bwc_ver_cmp_check(mhp->md_magic_ver, BWC_DEV_MDZ_VER)) {
				nid_log_error("%s: mdz_header_verion not compatible.",
						log_header);
				return -2;
			}
		} else {
			nid_log_warning("%s: mdz_header_version checking passed, version:{%s}",
				       	log_header, ver);
		}

	}
	return 0;
}

void
__bwc_mdz_scan(struct bwc_private *priv_p, uint64_t *start_seq,
	off_t *start_off, uint64_t *last_mdz_seq,
	struct list_head *recover_buf_head, int *hdz_rewind)
{
	char *log_header = "__bwc_mdz_scan";
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	int fd = priv_p->p_fhandle;
	struct bwc_req_header *mrhp;
	char *mdz_buf;
	uint32_t seg_to_scan, req_to_scan;
	ssize_t nread;
	off_t seg_off, start_point;
	int length;

	nid_log_warning("%s: start with mhp info start_seg:%u, end_seg:%u",
		log_header, mhp->md_start_seg, mhp->md_end_seg);

	/*
	 * If the mdz never been updated, there is nothing to scan
	 */
	if (!mhp->md_first_seg)
		return;

	/* allocate large enough space to hold the whole mdz */
	posix_memalign((void **)&mdz_buf, getpagesize(), priv_p->p_mdz_size);

	/*
	 * go through from start_seg to end_seg
	 */
	if (mhp->md_end_seg < mhp->md_start_seg) {
		/*
		 * From start segment to the end of mdz
		 */
		seg_to_scan = priv_p->p_mdz_seg_num - mhp->md_start_seg;
		req_to_scan = seg_to_scan * priv_p->p_mdz_segsz;
		start_point = mhp->md_start_seg * priv_p->p_mdz_segsz *
			priv_p->p_mdz_reqheadersz;
		seg_off = priv_p->p_mdz_headersz + start_point;
		length = req_to_scan * priv_p->p_mdz_reqheadersz;
		nread = __pread_n(fd, mdz_buf, length, seg_off);
		assert(nread == length);
		mrhp = (struct bwc_req_header *)mdz_buf;
		nid_log_warning("%s: rewind, req_to_scan:%u, start_seg:%u, "
			"end_seg:%u, total_seg_num:%u",
			log_header, req_to_scan, mhp->md_start_seg,
			mhp->md_end_seg, priv_p->p_mdz_seg_num);
		__bwc_mdz_req_scan(priv_p, mrhp, req_to_scan, start_off,
			NULL, recover_buf_head, hdz_rewind, start_seq);

		/*
		 * From the first segment of the mdz to the end segment
		 */
		seg_to_scan = mhp->md_end_seg + 1;
		req_to_scan = seg_to_scan * priv_p->p_mdz_segsz;
		seg_off = priv_p->p_mdz_headersz;
		length = req_to_scan * priv_p->p_mdz_reqheadersz;
		nread = __pread_n(fd, mdz_buf, length, seg_off);
		assert(nread == length);
		mrhp = (struct bwc_req_header *)mdz_buf;
		nid_log_warning("%s: rewind, req_to_scan:%u, start_seg:%u, "
			"end_seg:%u, total_seg_num:%u",
			log_header, req_to_scan, mhp->md_start_seg,
			mhp->md_end_seg, priv_p->p_mdz_seg_num);
		__bwc_mdz_req_scan(priv_p, mrhp, req_to_scan, start_off,
			last_mdz_seq, recover_buf_head, hdz_rewind, start_seq);

	} else {
		seg_to_scan = mhp->md_end_seg - mhp->md_start_seg + 1;
		req_to_scan = seg_to_scan * priv_p->p_mdz_segsz;
		start_point = mhp->md_start_seg * priv_p->p_mdz_segsz *
			priv_p->p_mdz_reqheadersz;
		seg_off = priv_p->p_mdz_headersz + start_point;
		length = req_to_scan * priv_p->p_mdz_reqheadersz;
		nread = __pread_n(fd, mdz_buf, length, seg_off);
		assert(nread == length);
		mrhp = (struct bwc_req_header *)mdz_buf;
		nid_log_warning("%s: do not need rewind, req_to_scan:%u, "
			"start_seg:%u, end_seg:%u, total_seg_num:%u",
			log_header, req_to_scan, mhp->md_start_seg,
			mhp->md_end_seg, priv_p->p_mdz_seg_num);
		__bwc_mdz_req_scan(priv_p, mrhp, req_to_scan, start_off,
			last_mdz_seq, recover_buf_head, hdz_rewind, start_seq);
	}

	nid_log_warning("%s: last_mdz_seq:%lu", log_header, *last_mdz_seq);
//	nid_log_warning("temp_test: mdz_buf:%p, mdz_buf2:%p", mdz_buf, mdz_buf2);
	assert(*last_mdz_seq == mhp->md_last_seq);
	free(mdz_buf);
}

/*
 * Algorithm:
 * 	hdz(hybrid data zone) scan after mdz(meta data zone) scanned
 *
 * Input:
 * 	mdz_last_hseg_off: from mdz header
 * 	last_mdz_seq: from __bwc_mdz_req_scan()
 */
int
__bwc_hdz_scan(struct bwc_private *priv_p, uint64_t last_mdz_seq,
	uint64_t *flushed_seq, uint64_t *final_seq,
	struct list_head *recover_buf_head, int hdz_rewind)
{
	char *log_header = "__bwc_hdz_scan";
	int fd = priv_p->p_fhandle;
	struct bwc_seg_header *scan_header = priv_p->p_shp;
	struct bwc_seg_trailer *scan_trailer = priv_p->p_stp;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	struct bwc_rb_node *nnp = NULL;
	char *buf;
	uint64_t cur_seq = last_mdz_seq, cur_flushed_seq = 0;
	int first_corrupted, seg_corrupted, dtp_corrupted;
	int rc = 0;
	off_t seg_off;
	uint32_t forward_scan_counter = 0;
	uint64_t first_seq;
	ssize_t nread;

	nid_log_warning("%s: start ...", log_header);

	/*
	 * lseek to the start point of the trailer, then
	 * fill in the trailer buffer for recovery.
	 */
	lseek(fd, priv_p->p_bufdevicesz - BWC_DEV_TRAILERSZ, SEEK_SET);
	nread = __read_n(fd, priv_p->p_dtp, BWC_DEV_TRAILERSZ);
	assert(nread == BWC_DEV_TRAILERSZ);

	/*
	 * checking the hdz segment in the last part of mdz
	 */
	nnp = rbn_p->nn_op->nn_get_node(rbn_p);
	buf = nnp->buf;
	if (mhp->md_last_hseg_off)
		seg_off = mhp->md_last_hseg_off;
	else
		seg_off = priv_p->p_first_seg_off;
	lseek(fd, seg_off, SEEK_SET);
	nread = __read_n(fd, (void *)(buf + BWC_SEG_TRAILERSZ),
		BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
	assert(nread == BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);

	first_corrupted = __bwc_seg_checking(priv_p, scan_header, scan_trailer,
		&nnp, &seg_off, recover_buf_head, cur_seq);
	assert(!last_mdz_seq || !first_corrupted);
	if (first_corrupted) {
		nid_log_warning("%s: both mdz and hdz are empty", log_header);
		goto out;
	}

	first_seq = __my_first_seq(scan_header);
	nid_log_warning("%s: first time my_seq:%lu, my_first_seq:%lu, my_flushed_seq:%lu",
		log_header, scan_header->my_seq, first_seq, scan_header->flushed_seq);

	/*
	 * forward (clockwise) scanning to find out the final segment
	 */
	if (scan_header->my_seq <= last_mdz_seq) {
		assert(scan_header->my_seq == last_mdz_seq);
		nid_log_warning("%s: the whole hdz seg overlapped by the "
			"last mdz seg, last_mdz_seq:%lu, my_seq:%lu",
			log_header, last_mdz_seq, scan_header->my_seq);
	} else {
		cur_seq = scan_header->my_seq;
	}
	assert(cur_seq == scan_header->my_seq);
	cur_flushed_seq = scan_header->flushed_seq;

forward_next:
	forward_scan_counter++;
	seg_corrupted = __bwc_seg_checking(priv_p, scan_header, scan_trailer,
		&nnp, &seg_off, recover_buf_head, cur_seq);
	if (!seg_corrupted) {
		first_seq = __my_first_seq(scan_header);
		nid_log_warning("%s: my_seq:%lu, my_first_seq:%lu, my_flushed_seq:%lu",
			log_header, scan_header->my_seq, first_seq, scan_header->flushed_seq);

		if (scan_header->my_seq > cur_seq) {
			cur_seq = scan_header->my_seq;
			cur_flushed_seq = scan_header->flushed_seq;
			nid_log_debug("%s: cur_seq:%lu, cur_flushed_seq:%lu",
				log_header, cur_seq, cur_flushed_seq);
			goto forward_next;
		} else {
			goto out;
		}

	} else {
		dtp_corrupted = __bwc_dev_trailer_checking(priv_p, priv_p->p_dtp);
		if (dtp_corrupted || cur_seq > priv_p->p_dtp->dt_my_seq) {
			nid_log_warning("%s: found the last hdz seq %lu",
				log_header, cur_seq);
			goto out;
		}
		nid_log_warning("%s: need rewind, my_seq:%lu, "
			"cur_seq:%lu, dtp_my_seq:%lu",
			log_header, scan_header->my_seq, cur_seq,
			priv_p->p_dtp->dt_my_seq);
		if (!nnp)
			nnp = rbn_p->nn_op->nn_get_node(rbn_p);
		buf = nnp->buf;

		seg_off = priv_p->p_first_seg_off;
		lseek(fd, seg_off, SEEK_SET);
		nread = __read_n(fd, (void *)(buf + BWC_SEG_TRAILERSZ),
			BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
		assert(nread == BWC_RECOVER_MAXREAD - BWC_SEG_TRAILERSZ);
		seg_corrupted = __bwc_seg_checking(priv_p, scan_header,
			scan_trailer, &nnp, &seg_off, recover_buf_head, cur_seq);
		if (!seg_corrupted && scan_header->my_seq > cur_seq) {
			hdz_rewind++;
			assert(hdz_rewind <= 2);
//			first_seq = __my_first_seq(scan_header);
//			nid_log_warning("temp_test: rewind, my_seg:%lu, "
//				my_first_seq:%lu",
//				scan_header->my_seq, first_seq);
			cur_seq = scan_header->my_seq;
			cur_flushed_seq = scan_header->flushed_seq;
			nid_log_warning("%s: hdz rewind, cur_seq:%lu, "
				"cur_flushed_seq:%lu",
				log_header, cur_seq, cur_flushed_seq);
			goto forward_next;
		} else {
			goto out;
		}
	}

out:
	nid_log_warning("%s: my_seq:%lu, cur_seq:%lu, "
		"forward_scan_counter:%u",
		log_header, scan_header->my_seq, cur_seq, forward_scan_counter);

	if (nnp)
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	*final_seq = cur_seq;
	*flushed_seq = cur_flushed_seq;
	return rc;
}

int
__bwc_bufdevice_restore2(struct bwc_interface *bwc_p, uint64_t start_seq,
	uint64_t flushed_seq, uint64_t final_seq, struct bwc_dev_trailer *dtp,
	struct list_head *recover_buf_head, uint64_t last_mdz_seq)
{
	char *log_header = "__bwc_bufdevice_restore2";
	struct bwc_private *priv_p = (struct bwc_private *)bwc_p->bw_private;
	int fd = priv_p->p_fhandle;
	struct bse_interface *bse_p;
	struct bwc_chain *chain_p;
	struct pp_page_node *pnp;
	struct bwc_seg_header *hp;
	struct bwc_req_header mrh;
	uint64_t pre_seq;
	struct bwc_req_header *rhp, *end_rhp, *rhp2;//, *last_flushed_rhp;
	struct bwc_mdz_header *mhp = priv_p->p_mdz_hp;
	uint32_t pagesz = priv_p->p_pagesz;
	uint16_t noccupied = 0;
	struct bwc_rbn_interface *rbn_p = &priv_p->p_rbn;
	struct bwc_rb_node *nnp, *nnp2;
	char *buf;
	int rc, got_not_flushed = 0, got_not_empty = 0;

	nid_log_warning("%s: start (start_seq:%lu, flushed_seq:%lu, final_seq:%lu, "
		"last_mdz_seq:%lu, p_cur_offset:%lu, p_cur_flush_block:%u, "
		"p_cur_block:%u) ...",
		log_header, start_seq, flushed_seq, final_seq,
		last_mdz_seq, priv_p->p_cur_offset,
		priv_p->p_cur_flush_block, priv_p->p_cur_block);
	assert(flushed_seq <= final_seq);
	assert(start_seq <= flushed_seq + 1 || (flushed_seq == 0 && start_seq == 1));

	/*
	 * dtp could be NULL since device trailer could be corrupted
	 */
	if (dtp)
		nid_log_warning("%s: dev trailer my_seq:%lu",
			log_header, dtp->dt_my_seq);
	priv_p->p_cur_offset = priv_p->p_first_seg_off;

	/* skip all requests which had been flushed */
	pre_seq = 0;
	list_for_each_entry_safe(nnp, nnp2, struct bwc_rb_node, recover_buf_head, nn_list) {
		buf = nnp->buf;
		hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);	// pointer to segment header
		assert(hp->my_seq >= start_seq && hp->my_seq <= final_seq);
		// the same if the first hdz seg is fully included in the last mdz seg
		assert((pre_seq < hp->my_seq) || (pre_seq == hp->my_seq && pre_seq == last_mdz_seq));
		if (hp->my_seq > flushed_seq) {	// hp is not fully flushed
			got_not_flushed = 1;
			if (hp->req_nr) {
				got_not_empty = 1;
				break;
			}
		}

		if (hp->req_nr) {
			uint32_t i;
			rhp = (struct bwc_req_header *)((char *)hp + BWC_SEG_HEADERSZ);	// the first req header of the segment
			for (i = 0; i < hp->req_nr; ++i, ++rhp) {
				if (rhp->my_seq <= last_mdz_seq)
					continue;
				memset(&mrh, 0, sizeof(mrh));
				mrh.resource_id = rhp->resource_id;
				mrh.data_len = rhp->data_len;
				mrh.wc_offset = rhp->wc_offset;
				mrh.my_seq = rhp->my_seq;
				mrh.offset = rhp->offset;
				mrh.is_mdz = 1;
				__bwc_mdz_add_request(priv_p, &mrh);
			}

			if (hp->my_seq >= last_mdz_seq) {
				/* this must be a hdz seg */
				priv_p->p_cur_offset = hp->wc_offset + hp->header_len +
					hp->data_len + BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
			}

		} else if (hp->my_seq > last_mdz_seq) {
			memset(&mrh, 0, sizeof(mrh));
			mrh.data_len = 0;
			mrh.wc_offset = hp->wc_offset + BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
			mrh.my_seq = hp->my_seq;
			mrh.is_mdz = 1;
			__bwc_mdz_add_request(priv_p, &mrh);
			priv_p->p_cur_offset = mrh.wc_offset;
		}


		/*
		 * since hp->my_seq is the largest seq of all requests in this segment
		 * So all requests in this segment should be skipped
		 */
		pre_seq = hp->my_seq;
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
	}

	/*
	 * If we got no non-flushed requests, it's not necessary to do recover
	 */
	if (!got_not_flushed || !got_not_empty) {
		if (!got_not_flushed)
			nid_log_warning("%s: all segments flushed, got nothing to recover", log_header);
		if (!got_not_empty)
			nid_log_warning("%s: all non_empty segments flushed, got nothing to recover", log_header);

		priv_p->p_cur_flush_block = (priv_p->p_cur_offset - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
		priv_p->p_cur_block = priv_p->p_cur_flush_block;

		goto out;
	}

	rhp = (struct bwc_req_header *)((char *)hp + BWC_SEG_HEADERSZ);	// the first req header of the segment
	rhp2 = rhp;
	while (rhp2->my_seq <= flushed_seq)
		rhp2++;
	nid_log_warning("temp_test_recover: my_seq: %lu, wc_offset: %lu", rhp2->my_seq, rhp2->wc_offset);
	priv_p->p_cur_offset = rhp2->wc_offset;
	priv_p->p_cur_flush_block = (rhp2->wc_offset - priv_p->p_mdz_size) / priv_p->p_flush_blocksz;
	priv_p->p_cur_block = priv_p->p_cur_flush_block;

	nid_log_warning("%s: cur_offset:%ld, flush_block:%u, flush_seq:%lu",
			log_header, priv_p->p_cur_offset, priv_p->p_cur_flush_block, flushed_seq);
	nid_log_warning("temp_test2:%u", priv_p->p_cur_flush_block);
	end_rhp = rhp + hp->req_nr - 1;	// the last request in the segment

	/*
	 * There can be duplicated requests between the last pseudo hdz segment and the
	 * first real hdz segment. And __bwc_seg_restore2() will handle the overlapped requests by pre_seq.
	 */
	struct bwc_seg_header *pre_hp = hp;
	for (;;) {
		nid_log_debug("%s: scanning seq:%lu, len:%u, cur_offset:%ld, cur_block:%u, noccupied:%u",
			log_header, hp->my_seq, hp->data_len, priv_p->p_cur_offset, priv_p->p_cur_block, noccupied);
		if (hp->req_nr) {
			rc = __bwc_seg_restore2(priv_p, hp, rhp, last_mdz_seq);
			if (rc) {
				nid_log_error("%s: segment error", log_header);
			}

			if (end_rhp->is_mdz) {
				/* try best to guess current offset */
				priv_p->p_cur_offset = end_rhp->wc_offset + end_rhp->data_len;
				nid_log_warning("temp_test: mdz seg %lu cur_offset: %lu", hp->my_seq, priv_p->p_cur_offset);
			} else {
				/* exactly tell the current offset */
				priv_p->p_cur_offset = end_rhp->wc_offset + end_rhp->data_len + BWC_SEG_TRAILERSZ;
				nid_log_warning("temp_test: hdz seg %lu cur_offset: %lu", hp->my_seq, priv_p->p_cur_offset);
			}
		} else {
			/*
			 * an empty segment must be a hdz segment. create an empty request for mdz
			 */
//			priv_p->p_cur_offset += BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
			nid_log_warning("temp_test: adding empty seg %lu cur_offset: %lu", hp->my_seq, priv_p->p_cur_offset);
			memset(&mrh, 0, sizeof(mrh));
			mrh.data_len = 0;
			mrh.wc_offset = hp->wc_offset + BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
			mrh.my_seq = hp->my_seq;
			mrh.is_mdz = 1;
			__bwc_mdz_add_request(priv_p, &mrh);
			/* exactly tell the current offset */
			priv_p->p_cur_offset = hp->wc_offset + BWC_SEG_HEADERSZ + BWC_SEG_TRAILERSZ;
		}
		nid_log_debug("%s: after __bwc_seg_restore2, cur_offset:%ld",
			log_header, priv_p->p_cur_offset);

		pthread_mutex_lock(&priv_p->p_occ_lck);
		while (priv_p->p_cur_offset > priv_p->p_flushing_st[priv_p->p_cur_block].f_end_offset) {
			priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = hp->my_seq;
			priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
			++noccupied;
			assert(noccupied <= priv_p->p_flush_nblocks);
			priv_p->p_cur_block++;
		}
		pthread_mutex_unlock(&priv_p->p_occ_lck);

		/* stop if this is the last request */
		if (hp->my_seq == final_seq){
			nid_log_warning("%s: scanning seq:%lu, len:%u, cur_offset:%ld, cur_block:%u, noccupied:%u",
				log_header, hp->my_seq, hp->data_len, priv_p->p_cur_offset, priv_p->p_cur_block, noccupied);
			break;
		}
		assert(hp->my_seq < final_seq);

		/* if need hdz rewind */
		if (dtp && dtp->dt_my_seq == hp->my_seq) {
			nid_log_warning("%s: need rewind: scanning seq:%lu, len:%u, cur_offset:%ld, cur_block:%u, noccupied:%u",
				log_header, hp->my_seq, hp->data_len, priv_p->p_cur_offset, priv_p->p_cur_block, noccupied);
			pthread_mutex_lock(&priv_p->p_occ_lck);
			while (priv_p->p_cur_block < priv_p->p_flush_nblocks) {
				priv_p->p_flushing_st[priv_p->p_cur_block].f_seq = hp->my_seq;	// empty block
				priv_p->p_flushing_st[priv_p->p_cur_block].f_occupied = 1;
				++noccupied;
				priv_p->p_cur_block++;
			}
			pthread_mutex_unlock(&priv_p->p_occ_lck);
			assert(noccupied <= priv_p->p_flush_nblocks);

			priv_p->p_cur_offset = priv_p->p_first_seg_off;
			/* cannot assume start from block 0, in case priv_p->p_first_seg_off is too large */
			priv_p->p_cur_block = (priv_p->p_first_seg_off - priv_p->p_mdz_size)/ priv_p->p_flush_blocksz;
		}

		/* next segment */
		pre_seq = hp->my_seq;
		assert(nnp->nn_list.next != recover_buf_head);
		nnp2 = list_entry(nnp->nn_list.next, struct bwc_rb_node, nn_list);
		rbn_p->nn_op->nn_put_node(rbn_p, nnp);
		nnp = nnp2;
		buf = nnp->buf;
		pre_hp = hp;
		assert(pre_hp);
		hp = (struct bwc_seg_header *)(buf + BWC_SEG_TRAILERSZ);
		rhp = (struct bwc_req_header *)((char *)hp + BWC_SEG_HEADERSZ);	// the first req header of the segment
		if (hp->req_nr){
			end_rhp = rhp + hp->req_nr - 1;	// the last request in the segment
		}
		assert(pre_seq <= hp->my_seq);
	}

out:
	/* seek the fd to where to start new IO */
	lseek(fd, priv_p->p_cur_offset, SEEK_SET);
//	nid_log_warning("temp_test: final cur_offset:%lu", priv_p->p_cur_offset);

	mhp->md_last_seq = last_mdz_seq;
	assert(last_mdz_seq + priv_p->p_mdz_next_req == final_seq);
	nid_log_warning("%s: md_first_seg:%d, md_last_seq:%lu, p_mdz_next_req:%u, final_seq:%lu",
		log_header, (int)mhp->md_first_seg, mhp->md_last_seq, priv_p->p_mdz_next_req, final_seq);

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

	nid_log_warning("%s: end (start_seq:%lu, flushed_seq:%lu, final_seq:%lu, last_mdz_seq:%lu, p_cur_offset:%lu, p_cur_flush_block:%u, p_cur_block:%u) ...",
		log_header, start_seq, flushed_seq, final_seq, last_mdz_seq, priv_p->p_cur_offset, priv_p->p_cur_flush_block, priv_p->p_cur_block);
	return 0;
}
