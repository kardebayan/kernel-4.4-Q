/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include "engine_request.h"


#define MyTag "[STALN]"
#define LOG_VRB(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)

/* #define STALN_DEBUG */
#ifdef STALN_DEBUG
#define LOG_DBG(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)    pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_WRN(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)

/*
* Single ring ctl init
*/
signed int init_ring_ctl(struct ring_ctrl *rctl)
{
	if (rctl == NULL)
		return -1;

	rctl->wcnt = 0;
	rctl->rcnt = 0;
	rctl->icnt = 0;
	rctl->gcnt = 0;
	rctl->size = 0;

	return 0;
}

signed int set_ring_size(struct ring_ctrl *rctl, unsigned int size)
{
	if (rctl == NULL)
		return -1;

	rctl->size = size;

	return 0;
}

signed int init_frame(struct frame *frame)
{
	if (frame == NULL)
		return -1;

	frame->state = FRAME_STATUS_EMPTY;

	return 0;
}

/*
* single request init
*/
signed int init_request(struct request *req)
{
	int f;

	if (req == NULL)
		return -1;

	req->state = REQUEST_STATE_EMPTY;
	init_ring_ctl(&req->fctl);
	set_ring_size(&req->fctl, MAX_FRAMES_PER_REQUEST);

	for (f = 0; f < MAX_FRAMES_PER_REQUEST; f++)
		init_frame(&req->frames[f]);

	req->pending_run = false;

	return 0;
}

/*
* per-frame data init
*/
signed int set_frame_data(struct frame *f, void *engine)
{
	if (engine == NULL || f == NULL) {
		LOG_ERR("NULL frame(%p)", (void *)f);
		return -1;
}
	f->data = engine;

	return 0;
}

/*
* TOP : per-sub-engine
* Size Limitaion: eng_reqs : [MAX_REQUEST_SIZE_PER_ENGINE]
*	     data : [MAX_REQUEST_SIZE_PER_ENGINE][MAX_FRAMES_PER_REQUEST]
*/
signed int register_requests(struct engine_requests *eng, size_t size)
{
	int f, r, d;
	char *_data;
	size_t len;

	if (eng == NULL)
		return -1;

	init_ring_ctl(&eng->req_ctl);
	set_ring_size(&eng->req_ctl, MAX_REQUEST_SIZE_PER_ENGINE);

	/* TODO: KE Risk : array out of bound */
	len = (size * MAX_FRAMES_PER_REQUEST) * MAX_REQUEST_SIZE_PER_ENGINE;
	_data = vmalloc(len);

	if (_data == NULL) {
		LOG_INF("[%s] vmalloc failed", __func__);
		return -1;
	}

	memset(_data, 0x0, len);
	LOG_INF("[%s]Engine struct total size is %zu", __func__, len);

	for (r = 0; r < MAX_REQUEST_SIZE_PER_ENGINE; r++) {
		init_request(&eng->reqs[r]);

		for (f = 0; f < MAX_FRAMES_PER_REQUEST; f++) {
			d = (r * MAX_FRAMES_PER_REQUEST + f) * size;
			set_frame_data(&eng->reqs[r].frames[f], &_data[d]);
		}
	}

	return 0;
}

signed int unregister_requests(struct engine_requests *eng)
{
	if (eng == NULL)
		return -1;

	vfree(eng->reqs[0].frames[0].data);

	return 0;
}


int set_engine_ops(struct engine_requests *eng, const struct engine_ops *ops)
{
	if (eng == NULL || ops == NULL)
		return -1;

	eng->ops = ops;

	return 0;
}

/*TODO: called in ENQUE_REQ */
signed int enque_request(struct engine_requests *eng, unsigned fcnt,
						void *req, pid_t pid)
{
	unsigned int r;
	unsigned int f;
	unsigned int enqnum = 0;

	if (eng == NULL)
		return -1;

	r = eng->req_ctl.wcnt;
	/* FIFO when wcnt starts from 0 */
	f = eng->reqs[r].fctl.wcnt;

	if (eng->reqs[r].state != REQUEST_STATE_EMPTY) {
		LOG_ERR("No empty requests available.");
		goto ERROR;
	}

	if (eng->ops->req_enque_cb == NULL || req == NULL) {
		LOG_ERR("NULL req_enque_cb or req");
		goto ERROR;
	}

	if (eng->ops->req_enque_cb(eng->reqs[r].frames, req)) {
		LOG_ERR("Failed to enque request, check cb");
		goto ERROR;
	}

	LOG_INF("request(%d) enqued with %d frames", r, fcnt);
	for (f = 0; f < fcnt; f++)
		eng->reqs[r].frames[f].state = FRAME_STATUS_ENQUE;
	eng->reqs[r].state = REQUEST_STATE_PENDING;

	eng->reqs[r].pid = pid;

	eng->reqs[r].fctl.wcnt = fcnt;
	eng->reqs[r].fctl.size = fcnt;

	eng->req_ctl.wcnt = (r + 1) % MAX_REQUEST_SIZE_PER_ENGINE;

#if REQUEST_REGULATION
	/*
	* pending request count for request base regulation
	*/
	for (r = 0; r < MAX_REQUEST_SIZE_PER_ENGINE; r++)
		if (eng->reqs[r].state == REQUEST_STATE_PENDING)
			enqnum++;
#else
	/*
	* running frame count for frame base regulation
	*/
	for (r = 0; r < MAX_REQUEST_SIZE_PER_ENGINE; r++)
		for (f = 0; f < MAX_FRAMES_PER_REQUEST; f++)
			if (eng->reqs[r].frames[f].state == FRAME_STATUS_RUNNING)
				enqnum++;
#endif
	return enqnum;
ERROR:
	return -1;
}

/* ConfigWMFERequest / ConfigOCCRequest abstraction
* TODO: locking should be here NOT camera_owe.c
*/
signed int request_handler(struct engine_requests *eng, spinlock_t *lock)
{
	unsigned int f, fn;
	unsigned int r;
	enum REQUEST_STATE_ENUM rstate;
	enum FRAME_STATUS_ENUM fstate;
	unsigned long flags;
	signed int ret = -1;

	if (eng == NULL)
		return -1;

	spin_lock_irqsave(lock, flags);

	/*
	* ioctl calls  enque_request() request wcnt inc
	* if request_handler is NOT called after enque_request() in serial,
	* wcnt may inc many times maore than 1
	*/
	r = eng->req_ctl.gcnt;
	LOG_DBG("[%s] processing request(%d)\n", __func__, r);

	rstate = eng->reqs[r].state;
#if REQUEST_REGULATION
	(void) fn;
	if (rstate != REQUEST_STATE_PENDING) {
		LOG_WRN("[%s]No pending request(%d), state:%d\n", __func__,
								r, rstate);
		spin_unlock_irqrestore(lock, flags);

		return 0;
	}
	/* running request contains all running frames */
	eng->reqs[r].state = REQUEST_STATE_RUNNING;

	for (f = 0; f < MAX_FRAMES_PER_REQUEST; f++) {
		fstate = eng->reqs[r].frames[f].state;
		if (fstate == FRAME_STATUS_ENQUE) {
			LOG_INF("[%s]Processing request(%d) of frame(%d)\n",
							__func__,  r, f);

			spin_unlock_irqrestore(lock, flags);
			ret = eng->ops->frame_handler(&eng->reqs[r].frames[f]);
			spin_lock_irqsave(lock, flags);
			if (ret)
				LOG_WRN("[%s]failed:frame %d of request %d",
							__func__, f, r);

			eng->reqs[r].frames[f].state = FRAME_STATUS_RUNNING;
		}
	}

	eng->req_ctl.gcnt = (r + 1) % MAX_REQUEST_SIZE_PER_ENGINE;
#else
	/*
	* TODO: to prevent IRQ timing issue due to multi-frame requests,
	* frame-based reguest handling should be used instead.
	*/
	if (eng->reqs[r].pending_run == false) {
		if (rstate != REQUEST_STATE_PENDING) {
			LOG_WRN("[%s]No pending request(%d), state:%d\n", __func__,
								r, rstate);
			spin_unlock_irqrestore(lock, flags);

			return 0;
		}
		eng->reqs[r].pending_run = true;
		eng->reqs[r].state = REQUEST_STATE_RUNNING;
	} else
		if (rstate != REQUEST_STATE_RUNNING) {
			LOG_WRN("[%s]No pending_run request(%d), state:%d\n", __func__,
								r, rstate);
			spin_unlock_irqrestore(lock, flags);

			return 0;
		}


	/* pending requets can have running frames */
	f = eng->reqs[r].fctl.gcnt;
	LOG_DBG("[%s]Iterating request(%d) of frame(%d)\n",
						__func__,  r, f);

	fstate = eng->reqs[r].frames[f].state;
	if (fstate == FRAME_STATUS_ENQUE) {
		LOG_INF("[%s]Processing request(%d) of frame(%d)\n",
						__func__,  r, f);

		eng->reqs[r].frames[f].state = FRAME_STATUS_RUNNING;
		spin_unlock_irqrestore(lock, flags);
		ret = eng->ops->frame_handler(&eng->reqs[r].frames[f]);
			spin_lock_irqsave(lock, flags);
		if (ret)
			LOG_WRN("[%s]failed:frame %d of request %d",
						__func__, f, r);

	} else {
		spin_unlock_irqrestore(lock, flags);
		LOG_INF("[%s]already running frame %d of request %d",
						__func__, f, r);
		return 1;
		}

	fn = (f + 1) % MAX_FRAMES_PER_REQUEST;
	fstate = eng->reqs[r].frames[fn].state;
	if (fstate == FRAME_STATUS_EMPTY || fn == 0) {
		eng->reqs[r].pending_run = false;
	eng->req_ctl.gcnt = (r + 1) % MAX_REQUEST_SIZE_PER_ENGINE;
	} else
		eng->reqs[r].fctl.gcnt = fn;

#endif

	spin_unlock_irqrestore(lock, flags);

	return 1;

}


int update_request(struct engine_requests *eng, pid_t *pid)
{
	unsigned int i, f, n;
	int req_jobs = -1;

	if (eng == NULL)
		return -1;

	/* TODO: request ring */
	for (i = eng->req_ctl.icnt; i < MAX_REQUEST_SIZE_PER_ENGINE; i++) {
		/* find 1st running request */
		if (eng->reqs[i].state != REQUEST_STATE_RUNNING)
			continue;

		/* find 1st running frame, f. */
		for (f = 0; f < MAX_FRAMES_PER_REQUEST; f++) {
			if (eng->reqs[i].frames[f].state ==
							FRAME_STATUS_RUNNING)
				break;
		}
		/* TODO: no running frame in a running request */
		if (f == MAX_FRAMES_PER_REQUEST) {
			LOG_ERR("[%s]No running frames in a running request(%d).", __func__, i);
			break;
		}

		eng->reqs[i].frames[f].state = FRAME_STATUS_FINISHED;
		LOG_INF("[%s]request %d of frame %d finished.\n", __func__, i, f);
		/*TODO: to obtain statistics */
		if (eng->ops->req_feedback_cb == NULL) {
			LOG_DBG("NULL req_feedback_cb");
			goto NO_FEEDBACK;
		}

		if (eng->ops->req_feedback_cb(&eng->reqs[i].frames[f])) {
			LOG_ERR("Failed to feedback statistics, check cb");
			goto NO_FEEDBACK;
		}
NO_FEEDBACK:
		n = f + 1;
		if ((n == MAX_FRAMES_PER_REQUEST) ||
			((n < MAX_FRAMES_PER_REQUEST) &&
			(eng->reqs[i].frames[n].state == FRAME_STATUS_EMPTY))) {

			req_jobs = 0;
			(*pid) = eng->reqs[i].pid;

			eng->reqs[i].state = REQUEST_STATE_FINISHED;
			eng->req_ctl.icnt = (eng->req_ctl.icnt + 1) % MAX_REQUEST_SIZE_PER_ENGINE;
		} else {
			LOG_INF("[%s]more frames left of request(%d/%d).", __func__, i, eng->req_ctl.icnt);
		}
		break;
	}

	return req_jobs;
}

/*TODO: called in DEQUE_REQ */
signed int deque_request(struct engine_requests *eng, unsigned *fcnt,
								void *req) {
	unsigned int r;
	unsigned int f;

	if (eng == NULL)
		return -1;

	r = eng->req_ctl.rcnt;
	/* FIFO when rcnt starts from 0 */
	f = eng->reqs[r].fctl.rcnt;

	if (eng->reqs[r].state != REQUEST_STATE_FINISHED) {
		LOG_ERR("[%s]Request(%d) NOT finished", __func__, r);
		goto ERROR;
	}
#if 0
	for (f = 0; f < fcnt; f++)
		if (eng->reqs[r].frames[f].state != FRAME_STATUS_FINISHED) {
			LOG_ERR("Frame(%d) NOT finised", f);
			goto ERROR;
		}
#else
	*fcnt = eng->reqs[r].fctl.size;
	LOG_INF("[%s]deque request(%d) has %d frames", __func__, r, *fcnt);
#endif
	if (eng->ops->req_deque_cb == NULL || req == NULL) {
		LOG_ERR("[%s]NULL req_deque_cb/req", __func__);
		goto ERROR;
	}

	if (eng->ops->req_deque_cb(eng->reqs[r].frames, req)) {
		LOG_ERR("[%s]Failed to deque, check req_deque_cb", __func__);
		goto ERROR;
	}

	for (f = 0; f < *fcnt; f++)
		eng->reqs[r].frames[f].state = FRAME_STATUS_EMPTY;
	eng->reqs[r].state = REQUEST_STATE_EMPTY;
	eng->reqs[r].pid = 0;

	eng->reqs[r].fctl.wcnt = 0;
	eng->reqs[r].fctl.rcnt = 0;
	eng->reqs[r].fctl.icnt = 0;
	eng->reqs[r].fctl.gcnt = 0;
	eng->reqs[r].fctl.size = 0;

	eng->req_ctl.rcnt = (r + 1) % MAX_REQUEST_SIZE_PER_ENGINE;

	return 0;
ERROR:
	return -1;
}

