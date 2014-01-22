/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************

  DESCRIPTION:

  This module contains template operating system primitives.

******************************************************************************/

#include <sys/time.h>
#include <sys/resource.h>

#include <ncsgl_defs.h>		/* Global defintions */

#include "ncs_osprm.h"		/* Req'd for primitive interfaces */

#include <signal.h>
#include <sched.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>

#include <sysf_exc_scr.h>
#include <ncssysf_tsk.h>
#include "ncs_tasks.h"
#include "ncssysf_lck.h"
#include "ncssysf_def.h"
#include "osaf_utility.h"

NCS_OS_LOCK gl_ncs_atomic_mtx;
#ifndef NDEBUG
bool gl_ncs_atomic_mtx_initialise = false;
#endif

/* mutex used for mimicking the SOCK_CLOEXEC flag, which was not supported by
 * Linux kernels prior to version 2.6.27. See socketpair(2) and socket(2) for a
 * description of SOCK_CLOEXEC. */
static pthread_mutex_t s_cloexec_mutex = PTHREAD_MUTEX_INITIALIZER;

/***************************************************************************
 *
 * uns64
 * ncs_os_time_ns(void)
 *
 * Description:
 *   This routine returns the number of ns since boot up
 *
 * Synopsis:
 *
 * Call Arguments:
 *    none
 *
 * Returns:
 *   Returns nano-seconds since boot up, otherwise 0
 *
 *
 ****************************************************************************/
uint64_t ncs_os_time_ns(void)
{
	uint64_t retval = 0;
	struct timespec ts;

	if (0 == clock_gettime(CLOCK_REALTIME, &ts)) {
		retval = ts.tv_sec;
		retval = ((retval * 1000 * 1000 * 1000) + ts.tv_nsec);
	}

	return retval;
}

/***************************************************************************
 *
 * int64
 * ncs_os_time_ms(void)
 *
 * Description:
 *   This routine returns the number of ms since boot up
 *
 * Synopsis:
 *
 * Call Arguments:
 *    none
 *
 * Returns:
 *   Returns milli-seconds since boot up, otherwise 0
 *
 * Notes:
 *   the timezone argument is ununsed by Linux(tm)
 *
 ****************************************************************************/
int64_t ncs_os_time_ms(void)
{
	int64_t retval = 0;	/* let 0 be an error */
	struct timeval tv;

	if (0 == gettimeofday(&tv, 0)) {
		retval = tv.tv_sec;
		retval = (retval * 1000) + (tv.tv_usec / 1000);
	} else {

	}
	return retval;
}

/***************************************************************************
 *
 * unsigned int
 * ncs_os_task( NCS_OS_task *, NCS_OS_TASK_REQUEST )
 *
 * Description:
 *   This routine handles all operating system Task requests.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    task    ............... pointer to a NCS_OS_TASK
 *    request ............... action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 **************************************************************************/

static
unsigned int ncs_linux_sleep(unsigned int ms_delay)
{
	struct timeval tv;
	tv.tv_sec = ms_delay / 1000;

	tv.tv_usec = ((ms_delay % 1000) * 1000);

	while (select(0, 0, 0, 0, &tv) != 0)
		if (errno == EINTR)
			continue;
		else
			return (NCSCC_RC_FAILURE);

	return (NCSCC_RC_SUCCESS);

}

unsigned int ncs_os_task(NCS_OS_TASK *task, NCS_OS_TASK_REQUEST request)
{
	switch (request) {
	case NCS_OS_TASK_CREATE:
		{
			int rc;
			int policy;
			char *thread_prio;
			char *thread_policy;
			char buf1[256] = { 0 };
			char buf2[256] = { 0 };
			int max_prio;
			int min_prio;
	
			pthread_attr_t attr;
			struct sched_param sp;

			memset(&sp, 0, sizeof(struct sched_param));

			pthread_attr_init(&attr);

			policy = task->info.create.i_policy;
			sp.sched_priority = task->info.create.i_priority;

			sprintf(buf1, "%s%s", task->info.create.i_name, "_SCHED_PRIORITY");
			sprintf(buf2, "%s%s", task->info.create.i_name, "_SCHED_POLICY");			
			
			if ((thread_prio = getenv(buf1)) != NULL)
				sp.sched_priority = strtol(thread_prio, NULL, 0);
			
			if ((thread_policy = getenv(buf2)) != NULL)
				policy = strtol(thread_policy, NULL, 0);
		
			min_prio = sched_get_priority_min(policy);
			max_prio = sched_get_priority_max(policy);
			
			if ((sp.sched_priority < min_prio) || (sp.sched_priority > max_prio) ||
			    pthread_attr_setschedpolicy(&attr, policy) != 0) {
               			/* Set to defaults */
				syslog(LOG_NOTICE, "scheduling priority %d or policy %d for the "
				       "task %s is invalid, setting to default values",
				       sp.sched_priority, policy, task->info.create.i_name);
				policy = task->info.create.i_policy;
				sp.sched_priority = task->info.create.i_priority;
				syslog(LOG_INFO, "%s task default policy is %d, priority is %d",
				       task->info.create.i_name, policy, sp.sched_priority);
			}
				
#ifdef RLIMIT_RTPRIO
			/* Policy will be set to default SCHED_RR for a non-root user also who/when 
			 * has configured limits.conf with rtprio
			 */
			if (getuid() != 0) {
				struct rlimit mylimit;
				if (getrlimit(RLIMIT_RTPRIO, &mylimit) == 0 ) {
					if (mylimit.rlim_cur == 0) {
						policy = SCHED_OTHER;
						sp.sched_priority = sched_get_priority_min(policy);
					}
				}
			}
#else
			if (ncs_is_root() == false) {
				policy = SCHED_OTHER;	/* This policy is for normal user */
				sp.sched_priority = sched_get_priority_min(policy);
			}
#endif
			rc = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
			assert(0 == rc);

			rc = pthread_attr_setschedpolicy(&attr, policy);
			assert(0 == rc);

			if (task->info.create.i_stack_nbytes < PTHREAD_STACK_MIN)
				task->info.create.i_stack_nbytes = PTHREAD_STACK_MIN;
			rc = pthread_attr_setstacksize(&attr, task->info.create.i_stack_nbytes);
			if (rc != 0)
				return NCSCC_RC_INVALID_INPUT;

			rc = pthread_attr_setschedparam(&attr, &sp);
			assert(0 == rc);

			/* This will make the threads created by this process really concurrent */
			/* Giving scope as kernel threads, with PTHREAD_SCOPE_SYSTEM. */
			rc = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
			assert(0 == rc);

			/* create the new thread object */
			task->info.create.o_handle = malloc(sizeof(pthread_t));
			assert(NULL != task->info.create.o_handle);

			rc = pthread_create(task->info.create.o_handle, &attr,
					    (void *(*)(void *))task->info.create.i_entry_point,
					    task->info.create.i_ep_arg);
			if (rc != 0) {
				if (policy == SCHED_RR || policy == SCHED_FIFO)
					syslog(LOG_ERR, "Creation of real-time thread '%s' FAILED - '%s'",
							task->info.create.i_name, strerror(rc));
				else
					syslog(LOG_ERR, "Creation of thread '%s' FAILED - '%s'",
							task->info.create.i_name, strerror(rc));
				free(task->info.create.o_handle);
				return NCSCC_RC_FAILURE;
			}
			
			rc = pthread_attr_destroy(&attr);
			if (rc != 0) {
				syslog(LOG_ERR, "pthread_attr_destroy for %s FAILED - '%s'",
						task->info.create.i_name, strerror(rc));
				free(task->info.create.o_handle);
				return NCSCC_RC_INVALID_INPUT;
			}
		}
		break;

	case NCS_OS_TASK_RELEASE:
		{
			void *status = NULL;
			if (pthread_cancel(*(pthread_t *)task->info.release.i_handle) != 0) {
				/* Shutdown testing: Even if pthread_cancel() fails
				   we need to do a thread join. Otherwise, information related
				   to the destroyed thread (basically the exit-code) is
				   not completely flushed. Hence, we would soon run out of
				   the allowed quota of threads per process.
				 */
				/* DO NOTHING! Fall through and do a thread-join */
			}

			if (pthread_join(*(pthread_t *)task->info.release.i_handle, &status) != 0) {
				free(task->info.release.i_handle);
				return (NCSCC_RC_FAILURE);
			}

			free(task->info.release.i_handle);

			if ((status != PTHREAD_CANCELED) && (status != (void *)NCSCC_RC_SUCCESS))
				return (NCSCC_RC_FAILURE);
		}
		break;

	case NCS_OS_TASK_JOIN:
		{
			void *status = NULL;

			if (pthread_join(*(pthread_t *)task->info.release.i_handle, &status) != 0) {
				return (NCSCC_RC_FAILURE);
			}

		}
		break;
	case NCS_OS_TASK_DETACH:
		if (pthread_detach(*(pthread_t *)task->info.release.i_handle) != 0) {
			free(task->info.release.i_handle);
			return (NCSCC_RC_FAILURE);
		}

		free(task->info.release.i_handle);
		break;

	case NCS_OS_TASK_START:
		break;

	case NCS_OS_TASK_STOP:
		break;

	case NCS_OS_TASK_SLEEP:
		if (NCSCC_RC_SUCCESS != ncs_linux_sleep(task->info.sleep.i_delay_in_ms)) {
			return NCSCC_RC_FAILURE;
		}
		break;

	case NCS_OS_TASK_CURRENT_HANDLE:
		/* requires a malloc to hold pthread_t;
		   needs a TASK_HANDLE type abstraction */
		task->info.current_handle.o_handle = NULL;

		break;

	default:
		return NCSCC_RC_FAILURE;

	}

	return NCSCC_RC_SUCCESS;
}

void ncs_os_atomic_init(void)
{
#ifndef NDEBUG
	gl_ncs_atomic_mtx_initialise = true;
#endif
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_CREATE, 0);
}

void ncs_os_atomic_inc(uint32_t *p_uns32)
{
	assert(gl_ncs_atomic_mtx_initialise);
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_LOCK, 0);
	(*p_uns32)++;
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_UNLOCK, 0);
}

void ncs_os_atomic_dec(uint32_t *p_uns32)
{
	assert(gl_ncs_atomic_mtx_initialise);
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_LOCK, 0);
	(*p_uns32)--;
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_UNLOCK, 0);

}

void ncs_os_atomic_destroy(void)
{
#ifndef NDEBUG
	gl_ncs_atomic_mtx_initialise = false;
#endif
	m_NCS_OS_LOCK(&gl_ncs_atomic_mtx, NCS_OS_LOCK_RELEASE, 0);
}

/***************************************************************************
 *
 * unsigned int
 * ncs_os_lock( NCS_OS_LOCK *, NCS_OS_LOCK_REQUEST )
 *
 * Description:
 *   This routine handles all operating system lock primitives.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    lock    ............... pointer to a NCS_OS_LOCK
 *    request ............... action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 ****************************************************************************/
unsigned int ncs_os_lock(NCS_OS_LOCK * lock, NCS_OS_LOCK_REQUEST request, unsigned int type)
{
	switch (request) {
	case NCS_OS_LOCK_CREATE: {
		pthread_mutexattr_t mutex_attr;

		if (pthread_mutexattr_init(&mutex_attr) != 0)
			return (NCSCC_RC_FAILURE);

		if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) != 0)
			return (NCSCC_RC_FAILURE);

		if (pthread_mutex_init(&lock->lock, &mutex_attr) != 0) {
			pthread_mutexattr_destroy(&mutex_attr);
			return (NCSCC_RC_FAILURE);
		}

		if (pthread_mutexattr_destroy(&mutex_attr) != 0)
			return (NCSCC_RC_FAILURE);

		break;
	}
	case NCS_OS_LOCK_RELEASE:
		if (pthread_mutex_destroy(&lock->lock) != 0)
			return (NCSCC_RC_FAILURE);
		break;

	case NCS_OS_LOCK_LOCK:
		osaf_mutex_lock_ordie(&lock->lock);
		break;

	case NCS_OS_LOCK_UNLOCK:
		osaf_mutex_unlock_ordie(&lock->lock);
		break;

	default:
		return (NCSCC_RC_FAILURE);
	}

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
 *
 * uint32_t ncs_os_mq(NCS_OS_MQ_REQ_INFO *req)
 * 
 *
 * Description:
 *   This routine handles operating system primitives for message-queues.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    req ............... ....action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 ****************************************************************************/
#define NCS_OS_MQ_MSG_DATA_OFFSET ( (long)(((NCS_OS_MQ_MSG*)(0))->data) )

uint32_t ncs_os_mq(NCS_OS_MQ_REQ_INFO *info)
{
#define NCS_OS_MQ_PROTECTION_FLAGS (0644)	/* Note: Octal form of number !! */
	switch (info->req) {
	case NCS_OS_MQ_REQ_MSG_SEND:
	case NCS_OS_MQ_REQ_MSG_SEND_ASYNC:
		{
			struct msgbuf *buf = (struct msgbuf *)info->info.send.i_msg;
			uint32_t flags;

			buf->mtype = info->info.send.i_mtype;	/* Fits into the "LL_HDR" of NCS_OS_MQ_MSG struct */

			if (info->req == NCS_OS_MQ_REQ_MSG_SEND_ASYNC)
				flags = IPC_NOWAIT;
			else
				flags = 0;

			if (msgsnd(info->info.send.i_hdl,
				   buf, NCS_OS_MQ_MSG_DATA_OFFSET + info->info.send.i_len, flags) == -1) {
				/* perror("os_defs.c:send"); */
				return NCSCC_RC_FAILURE;
			}
		}
		break;
	case NCS_OS_MQ_REQ_MSG_RECV:
	case NCS_OS_MQ_REQ_MSG_RECV_ASYNC:
		{
			struct msgbuf *buf = (struct msgbuf *)info->info.send.i_msg;
			uint32_t flags;
			uint32_t max_recv = info->info.recv.i_max_recv + NCS_OS_MQ_MSG_DATA_OFFSET;
			int32_t i_mtype = info->info.recv.i_mtype;

			if (max_recv > sizeof(NCS_OS_MQ_MSG))
				return NCSCC_RC_FAILURE;

			if (info->req == NCS_OS_MQ_REQ_MSG_RECV)
				flags = 0;
			else
				flags = IPC_NOWAIT;

			while (msgrcv(info->info.recv.i_hdl, buf, max_recv, i_mtype, flags) == -1) {
				if (errno == EINTR)
					continue;
				return NCSCC_RC_FAILURE;
			}
		}
		break;
	case NCS_OS_MQ_REQ_CREATE:
		{
			/* Create logic to-be-made more robust: Phani */
			/* Create flag to-be-changed from 644 to something better: Phani */
			/* Check to-be-added to validate NCS_OS_MQ_MAX_PAYLOAD limit: Phani */
			NCS_OS_MQ_HDL tmp_hdl;

			info->info.create.o_hdl = msgget(*info->info.create.i_key,
							 NCS_OS_MQ_PROTECTION_FLAGS | IPC_CREAT | IPC_EXCL);

			if (info->info.create.o_hdl == -1) {
				/*  Queue already exists. We should start with
				 **  a fresh queue. So let us delete this queue 
				 */
				tmp_hdl = msgget(*info->info.create.i_key, NCS_OS_MQ_PROTECTION_FLAGS);

				if (msgctl(tmp_hdl, IPC_RMID, NULL) != 0) {
					/* Queue deletion unsuccessful */
					return NCSCC_RC_FAILURE;
				}
				info->info.create.o_hdl = msgget(*info->info.create.i_key,
								 NCS_OS_MQ_PROTECTION_FLAGS | IPC_CREAT | IPC_EXCL);

				if (info->info.create.o_hdl == -1) {
					return NCSCC_RC_FAILURE;
				}
			}
		}
		break;
	case NCS_OS_MQ_REQ_OPEN:
		{
			info->info.open.o_hdl = msgget(*info->info.open.i_key, NCS_OS_MQ_PROTECTION_FLAGS);

			if (info->info.open.o_hdl == -1)
				return NCSCC_RC_FAILURE;
		}
		break;
	case NCS_OS_MQ_REQ_DESTROY:
		{
			if (msgctl(info->info.destroy.i_hdl, IPC_RMID, NULL) == -1)
				return NCSCC_RC_FAILURE;
		}
		break;
	case NCS_OS_MQ_REQ_RESIZE:
		{
			struct msqid_ds buf;

			if (msgctl(info->info.resize.i_hdl, IPC_STAT, &buf) == -1)
				return NCSCC_RC_FAILURE;

			buf.msg_qbytes = info->info.resize.i_newqsize;

			if (msgctl(info->info.resize.i_hdl, IPC_SET, &buf) == -1)
				return NCSCC_RC_FAILURE;
		}
		break;
	default:
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
 *
 * uint32_t ncs_os_posix_mq(NCS_OS_POSIX_MQ_REQ_INFO *req)
 * 
 *
 * Description:
 *   This routine handles operating system primitives for POSIX message-queues.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    req ............... ....action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 ****************************************************************************/

uint32_t ncs_os_posix_mq(NCS_OS_POSIX_MQ_REQ_INFO *req)
{

	switch (req->req) {
	case NCS_OS_POSIX_MQ_REQ_MSG_SEND:
	case NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC:
		{

			NCS_OS_MQ_REQ_INFO os_req;

			if (req->req == NCS_OS_POSIX_MQ_REQ_MSG_SEND)
				os_req.req = NCS_OS_MQ_REQ_MSG_SEND;
			else
				os_req.req = NCS_OS_MQ_REQ_MSG_SEND_ASYNC;

			os_req.info.send.i_hdl = req->info.send.mqd;
			os_req.info.send.i_len = req->info.send.datalen;
			os_req.info.send.i_msg = req->info.send.i_msg;
			os_req.info.send.i_mtype = req->info.send.i_mtype;

			if (ncs_os_mq(&os_req) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;
		}
		break;

	case NCS_OS_POSIX_MQ_REQ_MSG_RECV:
	case NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC:
		{

			NCS_OS_MQ_REQ_INFO os_req;

			if (req->req == NCS_OS_POSIX_MQ_REQ_MSG_RECV)
				os_req.req = NCS_OS_MQ_REQ_MSG_RECV;
			else
				os_req.req = NCS_OS_MQ_REQ_MSG_RECV_ASYNC;

			os_req.info.recv.i_hdl = req->info.recv.mqd;
			os_req.info.recv.i_max_recv = req->info.recv.datalen;
			os_req.info.recv.i_msg = req->info.recv.i_msg;
			os_req.info.recv.i_mtype = req->info.recv.i_mtype;

			if (ncs_os_mq(&os_req) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

		}
		break;
	case NCS_OS_POSIX_MQ_REQ_GET_ATTR:
		{
			struct msqid_ds buf;

			if (msgctl(req->info.attr.i_mqd, IPC_STAT, &buf) == -1)
				return NCSCC_RC_FAILURE;

			req->info.attr.o_attr.mq_curmsgs = buf.msg_qnum;	/* No of messages in Q */
			req->info.attr.o_attr.mq_msgsize = buf.__msg_cbytes;	/* Current Q size */
			req->info.attr.o_attr.mq_stime = buf.msg_stime;
			req->info.attr.o_attr.mq_maxmsg = buf.msg_qbytes;	/* Maximum Q size */
		}
		break;
	case NCS_OS_POSIX_MQ_REQ_OPEN:
		{
			NCS_OS_MQ_REQ_INFO os_req;
			NCS_OS_MQ_KEY key;
			FILE *file;
			char filename[264];
			struct msqid_ds buf;

			memset(&buf, 0, sizeof(struct msqid_ds));

			sprintf(filename, "/tmp/%s%d", req->info.open.qname, req->info.open.node);

			if (req->info.open.iflags & O_CREAT) {
				os_req.req = NCS_OS_MQ_REQ_CREATE;

				file = fopen(filename, "w");
				if (file == NULL)
					return NCSCC_RC_FAILURE;

				key = ftok(filename, 1);
				os_req.info.create.i_key = &key;

				if (fclose(file) != 0)
					return NCSCC_RC_FAILURE;
			} else {

				os_req.req = NCS_OS_MQ_REQ_OPEN;

				key = ftok(filename, 1);
				os_req.info.open.i_key = &key;
			}

			if (ncs_os_mq(&os_req) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;

			if (req->info.open.iflags & O_CREAT) {
				req->info.open.o_mqd = os_req.info.create.o_hdl;
				if (msgctl(req->info.open.o_mqd, IPC_STAT, &buf) == -1) {
					return NCSCC_RC_FAILURE;
				}
				buf.msg_qbytes = req->info.open.attr.mq_msgsize;
				if (msgctl(req->info.open.o_mqd, IPC_SET, &buf) == -1) {
					return NCSCC_RC_FAILURE;
				}
			} else
				req->info.open.o_mqd = os_req.info.open.o_hdl;

		}
		break;
	case NCS_OS_POSIX_MQ_REQ_RESIZE:
		{
			NCS_OS_MQ_REQ_INFO os_req;

			os_req.req = NCS_OS_MQ_REQ_RESIZE;
			os_req.info.resize.i_hdl = req->info.resize.mqd;
			os_req.info.resize.i_newqsize = req->info.resize.i_newqsize;

			if (ncs_os_mq(&os_req) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;
		}
		break;
	case NCS_OS_POSIX_MQ_REQ_CLOSE:
		{
			NCS_OS_MQ_REQ_INFO os_req;

			os_req.req = NCS_OS_MQ_REQ_DESTROY;
			os_req.info.destroy.i_hdl = req->info.close.mqd;

			if (ncs_os_mq(&os_req) != NCSCC_RC_SUCCESS)
				return NCSCC_RC_FAILURE;
		}
		break;
	case NCS_OS_POSIX_MQ_REQ_UNLINK:
		{
			char filename[264];

			memset(filename, 0, sizeof(filename));
			sprintf(filename, "/tmp/%s%d", req->info.unlink.qname, req->info.unlink.node);

			if (unlink(filename) != 0)
				return NCSCC_RC_FAILURE;
		}
		break;

	default:
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;

}

/***************************************************************************
 *
 * uint32_t ncs_os_posix_shm(NCS_OS_POSIX_SHM_REQ_INFO *req)
 * 
 *
 * Description:
 *   This routine handles operating system primitives for POSIX shared-memory.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    req ............... ....action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 ****************************************************************************/
static int32_t ncs_shm_prot_flags(uint32_t flags)
{

	int prot_flag = PROT_NONE;

	if (flags & O_WRONLY) {
		prot_flag |= PROT_WRITE;
	}

	if (flags & O_RDWR) {
		prot_flag |= (PROT_READ | PROT_WRITE);
	}

	/* By default, prot_flag is PROT_READ */
	if (prot_flag == PROT_NONE)
		prot_flag |= PROT_READ;

	return prot_flag;
}

uint32_t ncs_os_posix_shm(NCS_OS_POSIX_SHM_REQ_INFO *req)
{
	uint32_t prot_flag;
	int32_t ret_flag;
	long shm_size;
	char shm_name[PATH_MAX];

	switch (req->type) {
	case NCS_OS_POSIX_SHM_REQ_OPEN:	/* opens and mmaps */

		if (req->info.open.i_name == NULL) {
			printf("input validation failed \n");
			return NCSCC_RC_FAILURE;
		}

		/* checks i_size is greater than LONG_MAX */
		if (req->info.open.i_size > LONG_MAX) {
			/*ftruncate accepts long int as size ,So we are allowed to pass max value of long int as size */
			printf("size value for ftruncate exceed max limit\n");
			return NCSCC_RC_FAILURE;
		}
		shm_size = (long)req->info.open.i_size;
		snprintf(shm_name, PATH_MAX, "/opensaf/%s", req->info.open.i_name);
		req->info.open.o_fd = shm_open(shm_name, req->info.open.i_flags, 0666);
		if (req->info.open.o_fd < 0) {
			return NCSCC_RC_FAILURE;
		} else {
			if (ftruncate(req->info.open.o_fd, (off_t) shm_size /* off_t == long */ ) < 0) {
				printf("ftruncate failed with errno value %d \n", errno);
				return NCSCC_RC_FAILURE;
			}

			prot_flag = ncs_shm_prot_flags(req->info.open.i_flags);
			req->info.open.o_addr =
			    mmap(req->info.open.o_addr, (size_t)shm_size /* size_t == unsigned long */ ,
				 prot_flag, req->info.open.i_map_flags, req->info.open.o_fd, req->info.open.i_offset);
			if (req->info.open.o_addr == MAP_FAILED) {
				printf("mmap failed and errno value %d\n", errno);
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_OS_POSIX_SHM_REQ_CLOSE:	/* close is munmap */

		/* checks i_size is greater than LONG_MAX */
		if (req->info.close.i_size > LONG_MAX) {
			printf("size value exceed max limit\n");
			return NCSCC_RC_FAILURE;
		}

		shm_size = (long)req->info.close.i_size;
		ret_flag = munmap(req->info.close.i_addr, (size_t)shm_size /* size_t == unsigned long */ );
		if (ret_flag < 0) {
			printf("munmap failed with errno value %d\n", errno);
			return NCSCC_RC_FAILURE;
		}
		if (req->info.close.i_fd > 0) {

			ret_flag = close(req->info.close.i_fd);
			if (ret_flag < 0) {
				printf("close failed with errno value %d\n", errno);
				return NCSCC_RC_FAILURE;
			}
		}
		break;

	case NCS_OS_POSIX_SHM_REQ_UNLINK:	/* unlink is shm_unlink */

		snprintf(shm_name, PATH_MAX, "/opensaf/%s", req->info.unlink.i_name);
		ret_flag = shm_unlink(shm_name);
		if (ret_flag < 0) {
			printf("shm_unlink failed with errno value %d\n", errno);
			return NCSCC_RC_FAILURE;
		}
		break;

	case NCS_OS_POSIX_SHM_REQ_READ:
		memcpy(req->info.read.i_to_buff, (void *)((char *)req->info.read.i_addr + req->info.read.i_offset),
		       req->info.read.i_read_size);
		break;

	case NCS_OS_POSIX_SHM_REQ_WRITE:
		memcpy((void *)((char *)req->info.write.i_addr + req->info.write.i_offset), req->info.write.i_from_buff,
		       req->info.write.i_write_size);
		break;

	default:
		printf("Option Not supported %d \n", req->type);
		return NCSCC_RC_FAILURE;
		break;
	}

	return NCSCC_RC_SUCCESS;

}

/***************************************************************************
 *
 * void*
 * ncs_os_udef_alloc
 *
 * Description
 *   This function conforms to the function prototype NCS_POOL_MALLOC, which
 * is typedef'ed in os_prims.h, and co-incidentally (by design) aligns with
 * the needs of the USRBUF pool manager in usrbuf.h.
 *
 * Synopsis
 *
 * Call Arguments:
 *   size     sizeof mem thing to allocate
 *   pool_id  Memory pool from which memory should be allocated
 *   pri      priority, being hi, medium or low
 *
 * Returns
 *   ptr to allocated mem block, or
 *   NULL, no memory available
 *
 * NOTE: Default implementation
 *
 ****************************************************************************/

void *ncs_os_udef_alloc(uint32_t size, uint8_t pool_id, uint8_t pri)
{
	(void) pool_id; (void) pri;
	return malloc(size);
}

/***************************************************************************
 *
 * void
 * ncs_os_udef_free
 *
 * Description
 *   This function conforms to function prototype NCS_POOL_MFREE, which
 * is typedef'ed in os_prims.h and co-incidentally (by design) aligns with
 * the needs of the USRBUF pool manager in usrbuf.h
 *
 * Synopsis
 *
 * Call Arguments
 *   ptr     to the memory hunk to be freed
 *   pool_id from which the memory came and now wants to return to
 *
 * Returns
 *   nothing
 *
 * NOTE: Default implementation
 *
 ****************************************************************************/

void ncs_os_udef_free(void *ptr, uint8_t pool_id)
{
	(void) pool_id;
	free(ptr);
}

/***************************************************************************
 *
 * ncs_os_process_execute_timed
 *
 * Description: To execute a module in a new process with time-out.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   req - Request parameters.
 *
 * Returns:
 *   Success or failure
 *
 * Notes:
 *
 **************************************************************************/
uint32_t ncs_os_process_execute_timed(NCS_OS_PROC_EXECUTE_TIMED_INFO *req)
{
	int count;
	int pid;
	NCS_OS_ENVIRON_SET_NODE *node = NULL;

	if ((req->i_script == NULL) || (req->i_cb == NULL))
		return NCSCC_RC_FAILURE;

	if (req->i_set_env_args == NULL)
		count = 0;
	else {
		count = req->i_set_env_args->num_args;
		node = req->i_set_env_args->env_arg;
	}

	m_NCS_LOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

	if (module_cb.init != true) {
		/* this will initializes the execute module control block */
		if (start_exec_mod_cb() != NCSCC_RC_SUCCESS) {
			m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
			syslog(LOG_ERR, "%s: start_exec_mod_cb failed", __FUNCTION__);
			return NCSCC_RC_FAILURE;
		}
	}

	osaf_mutex_lock_ordie(&s_cloexec_mutex);

	if ((pid = fork()) == 0) {
		/*
		 ** Make sure forked processes have default scheduling class
		 ** independent of the callers scheduling class.
		 */
		struct sched_param param = {.sched_priority = 0 };
		if (sched_setscheduler(0, SCHED_OTHER, &param) == -1)
			syslog(LOG_ERR, "%s: Could not setscheduler: %s", __FUNCTION__, strerror(errno));

		/* set the environment variables */
		for (; count > 0; count--) {
			setenv(node->name, node->value, node->overwrite);
			node++;
		}

		/* By default we close all inherited file descriptors in the child */
		if (getenv("OPENSAF_KEEP_FD_OPEN_AFTER_FORK") == NULL) {
			/* Close all inherited file descriptors */
			int i = sysconf(_SC_OPEN_MAX);
			if (i == -1) {
				syslog(LOG_ERR, "%s: sysconf failed - %s", __FUNCTION__, strerror(errno));
				exit(EXIT_FAILURE);
			}
			for (i--; i >= 0; --i)
				(void) close(i); /* close all descriptors */

			/* Redirect standard files to /dev/null */
			if (freopen("/dev/null", "r", stdin) == NULL)
				syslog(LOG_ERR, "%s: freopen stdin failed - %s", __FUNCTION__, strerror(errno));
			if (freopen("/dev/null", "w", stdout) == NULL)
				syslog(LOG_ERR, "%s: freopen stdout failed - %s", __FUNCTION__, strerror(errno));
			if (freopen("/dev/null", "w", stderr) == NULL)
				syslog(LOG_ERR, "%s: freopen stderr failed - %s", __FUNCTION__, strerror(errno));
		}

		/* RUNASROOT gives the OpenSAF user a possibility to maintain the < 4.2 behaviour.
		 * For example the UML environment needs this because of its simplified user management.
		 * OpenSAF processes will otherwise be started as the real host user and will
		 * have problems e.g. writing PID files to the root owned directory.
		 */
#ifndef RUNASROOT
		/* Check owner user ID of file and change group and user accordingly */
		{
			struct stat buf;
			if (stat(req->i_script, &buf) == 0) {
				if (setgid(buf.st_gid) == -1)
					syslog(LOG_ERR, "setgid %u failed - %s", buf.st_gid, strerror(errno));
				if (setuid(buf.st_uid) == -1)
					syslog(LOG_ERR, "setuid %u failed - %s", buf.st_uid, strerror(errno));
			} else {
				syslog(LOG_ERR, "Could not stat %s - %s", req->i_script, strerror(errno));
				exit(128);
			}
		}
#endif

		/* child part */
		if (execvp(req->i_script, req->i_argv) == -1) {
			syslog(LOG_ERR, "%s: execvp '%s' failed - %s", __FUNCTION__, req->i_script, strerror(errno));
			exit(128);
		}
	} else if (pid > 0) {
		/* 
		 * Parent - Add new pid in the tree,
		 * start a timer, Wait for a signal from child. 
		 */
		osaf_mutex_unlock_ordie(&s_cloexec_mutex);

		if (NCSCC_RC_SUCCESS != add_new_req_pid_in_list(req, pid)) {
			m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
			syslog(LOG_ERR, "%s: failed to add PID", __FUNCTION__);
			return NCSCC_RC_FAILURE;
		}
	} else {
		/* fork ERROR */
		syslog(LOG_ERR, "%s: fork failed - %s", __FUNCTION__, strerror(errno));
		osaf_mutex_unlock_ordie(&s_cloexec_mutex);
		m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
		return NCSCC_RC_FAILURE;
	}

	m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
 *
 * ncs_sel_obj_*  primitives
 * 
 *
 * Description:
 *   These primitives implement creation and manipulation routines for
 *   objects on which POSIX select() could be done.  The term 
 *   "selection-objects" will be used to denote these objects.
 *
 *   These primitives make use "fd"s generated by "socket()" invocation as
 *   selection-objects. On LINUX, AF_UNIX sockets shall be used.
 *
 * Synopsis:
 *
 *   The following macros comprise the "ncs_sel_obj_*" primitive set
 *      m_NCS_SEL_OBJ_CREATE   =>   ncs_sel_obj_create()
 *      m_NCS_SEL_OBJ_DESTROY  =>   ncs_sel_obj_destroy()
 *      m_NCS_SEL_OBJ_IND      =>   ncs_sel_obj_ind()
 *      m_NCS_SEL_OBJ_RMV_IND  =>   ncs_sel_obj_rmv_ind()
 *      m_NCS_SEL_OBJ_SELECT   =>   ncs_sel_obj_select()
 *      m_NCS_SEL_OBJ_POLL_SINGLE_OBJ =>   ncs_sel_obj_poll_single_obj()
 *
 *   The following macros are an important part of the "ncs_sel_obj_*"
 *   primitive set but Operating System provided macros have been used
 *   for this. Therefore, they do not have a corresponding function
 *   implemented in this file. 
 *      m_NCS_SEL_OBJ_ZERO     =>   based on FD_ZERO
 *      m_NCS_SEL_OBJ_SET      =>   based on FD_SET
 *      m_NCS_SEL_OBJ_CLR      =>   based on FD_CLR
 *      m_NCS_SEL_OBJ_ISSET    =>   based on FD_ISSET
 *
 *
 ****************************************************************************/
#define MAX_INDS_AT_A_TIME 10

#ifdef DEBUG_CODE

#define DIAG(x)         printf(x)
#define DIAG1(x, y)     printf(x, y)
#define DIAG2(x, y, z)  printf(x, y, z)
#else

#define DIAG(x)
#define DIAG1(x, y)
#define DIAG2(x, y, z)
#endif

uint32_t ncs_sel_obj_create(NCS_SEL_OBJ *o_sel_obj)
{
	int s_pair[2];
	int flags = 0;

	osaf_mutex_lock_ordie(&s_cloexec_mutex);
	if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, s_pair)) {
		syslog(LOG_ERR, "%s: socketpair failed - %s", __FUNCTION__, strerror(errno));
		osaf_mutex_unlock_ordie(&s_cloexec_mutex);
		return NCSCC_RC_FAILURE;
	}

	flags = fcntl(s_pair[0], F_GETFD, 0);
	fcntl(s_pair[0], F_SETFD, (flags | FD_CLOEXEC));

	flags = fcntl(s_pair[1], F_GETFD, 0);
	fcntl(s_pair[1], F_SETFD, (flags | FD_CLOEXEC));

	osaf_mutex_unlock_ordie(&s_cloexec_mutex);

	if (s_pair[0] > s_pair[1]) {
		/* Ensure s_pair[1] is equal or greater */
		int temp = s_pair[0];
		s_pair[0] = s_pair[1];
		s_pair[1] = temp;
	}
	o_sel_obj->raise_obj = s_pair[0];
	o_sel_obj->rmv_obj = s_pair[1];
	/* Raising indications should be a non-blocking operation. Otherwise, 
	   it can lead to deadlocks among reader and writer applications. 
	 */
	flags = fcntl(o_sel_obj->raise_obj, F_GETFL, 0);
	if (fcntl(o_sel_obj->raise_obj, F_SETFL, (flags | O_NONBLOCK)) == -1) {
		syslog(LOG_ERR, "%s: fcntl failed - %s", __FUNCTION__, strerror(errno));
		(void) ncs_sel_obj_destroy(*o_sel_obj);
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

uint32_t ncs_sel_obj_destroy(NCS_SEL_OBJ i_ind_obj)
{
	shutdown(i_ind_obj.raise_obj, SHUT_RDWR);
	close(i_ind_obj.raise_obj);
	shutdown(i_ind_obj.rmv_obj, SHUT_RDWR);
	close(i_ind_obj.rmv_obj);
	return NCSCC_RC_SUCCESS;
}

uint32_t ncs_sel_obj_rmv_operation_shut(NCS_SEL_OBJ *i_ind_obj)
{
	if (i_ind_obj == NULL)
		return NCSCC_RC_FAILURE;

	if (i_ind_obj->rmv_obj == -1)
		return NCSCC_RC_FAILURE;

	shutdown(i_ind_obj->rmv_obj, SHUT_RDWR);
	close(i_ind_obj->rmv_obj);

	i_ind_obj->rmv_obj = -1;

	return NCSCC_RC_SUCCESS;
}

/* This function will make select unblock */
uint32_t ncs_sel_obj_raise_operation_shut(NCS_SEL_OBJ *i_ind_obj)
{
	if (i_ind_obj == NULL)
		return NCSCC_RC_FAILURE;

	if (i_ind_obj->raise_obj == -1)
		return NCSCC_RC_FAILURE;

	shutdown(i_ind_obj->raise_obj, SHUT_RDWR);
	close(i_ind_obj->raise_obj);

	i_ind_obj->raise_obj = -1;

	return NCSCC_RC_SUCCESS;
}

uint32_t ncs_sel_obj_ind(NCS_SEL_OBJ i_ind_obj)
{
	int rc;

retry:
	/* The following call can block, in such a case a failure is returned */
	if ((rc = write(i_ind_obj.raise_obj, "A", 1)) != 1) {
		if (rc == -1) {
			if (errno == EINTR)
				goto retry;

			syslog(LOG_ERR, "%s: write failed - %s", __FUNCTION__, strerror(errno));
		}

		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;
}

int ncs_sel_obj_rmv_ind(NCS_SEL_OBJ i_ind_obj, bool nonblock, bool one_at_a_time)
{
	char tmp[MAX_INDS_AT_A_TIME];
	int ind_count, tot_inds_rmvd;
	int num_at_a_time;
	int raise_non_block_flag_set = 0, rmv_non_block_flag_set = 0, flags = 0;

	tot_inds_rmvd = 0;
	num_at_a_time = (one_at_a_time ? 1 : MAX_INDS_AT_A_TIME);

	/* If one_at_a_time == false, remove MAX_INDS_AT_A_TIME in a 
	 * non-blocking way and count the number of indications 
	 * so removed using "tot_inds_rmvd"
	 *
	 * If one_at_a_time == true,  then quit the infinite loop 
	 * after removing at most 1 indication.
	 */
	flags = fcntl(i_ind_obj.raise_obj, F_GETFL, 0);
	if ((flags & O_NONBLOCK) != O_NONBLOCK) {
		fcntl(i_ind_obj.raise_obj, F_SETFL, (flags | O_NONBLOCK));
		raise_non_block_flag_set = true;
	}
	flags = fcntl(i_ind_obj.rmv_obj, F_GETFL, 0);
	if ((flags & O_NONBLOCK) != O_NONBLOCK) {
		fcntl(i_ind_obj.rmv_obj, F_SETFL, (flags | O_NONBLOCK));
		rmv_non_block_flag_set = true;
	}
	for (;;) {
		ind_count = recv(i_ind_obj.rmv_obj, &tmp, num_at_a_time, 0);

		DIAG1("RMV_IND:ind_count in nonblock-recv = %d\n", ind_count);

		if (ind_count > 0) {
			/* Only one indication should be removed at a time, return immediately */
			if (one_at_a_time) {
				assert(ind_count == 1);
				return 1;
			}

			/* Some indications were removed */
			tot_inds_rmvd += ind_count;
		} else if (ind_count <= 0) {
			if (errno == EAGAIN)
				/* All queued indications have been removed */
				break;
			else if (errno == EINTR)
				/* recv() call was interrupted. Needs to be invoked again */
				continue;
			else {
				/* Unknown error. */
				syslog(LOG_ERR, "%s: recv failed - %s", __FUNCTION__, strerror(errno));
				return -1;
			}
		}
	}			/* End of infinite loop */

	if (raise_non_block_flag_set == true) {
		flags = fcntl(i_ind_obj.raise_obj, F_GETFL, 0);
		flags = flags & ~(O_NONBLOCK);
		fcntl(i_ind_obj.raise_obj, F_SETFL, flags);	
	}
	if (rmv_non_block_flag_set == true) {
		flags = fcntl(i_ind_obj.rmv_obj, F_GETFL, 0);
		flags = flags & ~(O_NONBLOCK);
		fcntl(i_ind_obj.rmv_obj, F_SETFL, flags);	
	}

	/* Reaching here implies that all pending indications have been removed 
	 * and "tot_inds_rmvd" contains a count of indications removed. 
	 *
	 * Now, action to be taken could be one of the following
	 * a) if  (tot_inds_rmvd !=0) : All indications removed, need some 
	 *          processing, so will return "tot_inds_rmvd"
	 *
	 * b) if  (tot_inds_rmvd == 0) and (nonblock) : Caller was just checking
	 *          if any indications were pending, he didn't know that there were
	 *          no indications pending. Simply return 0;
	 *
	 * c) if  (tot_inds_rmvd == 0) and (!nonblock) : There are no indications
	 *          pending but we should not return unless there is an indication
	 *          arrives.  
	 */
	if ((tot_inds_rmvd != 0) || (nonblock)) {
		/* Case (a) or case (b) above */
		return tot_inds_rmvd;
	}

	/* Case (c) described above */
	for (;;) {
		/* We now block on receive.  */
		ind_count = recv(i_ind_obj.rmv_obj, &tmp, num_at_a_time, 0);
		if (ind_count > 0) {
			/* Some indication has arrived. */

			/* NOTE: There may be more than "num_at_a_time" indications
			 * queued up. We could do another "tot_inds_rmvd" calculation,
			 * but that's not done here, as it is involves some effort and can
			 * conveniently be postponed till the next invocation of this 
			 * function.
			 */
			return ind_count;
		} else if ((ind_count < 0) && (errno != EINTR)) {
			/* Unknown mishap. Should reach here only if
			 * the i_rmv_ind_obj has now become invalid.
			 * Close down and return error.
			 * FIXME: TODO
			 */
			shutdown(i_ind_obj.rmv_obj, SHUT_RDWR);
			close(i_ind_obj.rmv_obj);
			DIAG("RMV_IND2. Returning -1\n");
			return -1;
		}
	}			/* End of infinite loop */
}

int ncs_sel_obj_select(NCS_SEL_OBJ highest_sel_obj,
		       NCS_SEL_OBJ_SET *rfds, NCS_SEL_OBJ_SET *wfds, NCS_SEL_OBJ_SET *efds, uint32_t *timeout_in_10ms)
{
	struct timeval tmout_in_tv = { 0, 0 };
	struct timeval *p_tmout_in_tv;
	int rc;
	uint32_t rem10ms = 0, old_rem10ms = 0;
	NCS_SEL_OBJ_SET save_rfds, save_wfds, save_efds;

	FD_ZERO(&save_rfds);
	FD_ZERO(&save_wfds);
	FD_ZERO(&save_efds);

	if (timeout_in_10ms != NULL) {
		old_rem10ms = *timeout_in_10ms;
	}
	if (rfds)
		save_rfds = *rfds;
	if (wfds)
		save_wfds = *wfds;
	if (efds)
		save_efds = *efds;

	do {
		if (rfds)
			*rfds = save_rfds;
		if (wfds)
			*wfds = save_wfds;
		if (efds)
			*efds = save_efds;

		/* STEP: Calculate time to make to select() */
		if (timeout_in_10ms != NULL) {
			tmout_in_tv.tv_sec = old_rem10ms / 100;
			tmout_in_tv.tv_usec = ((old_rem10ms) % 100) * 10000;
			p_tmout_in_tv = &tmout_in_tv;
		} else
			p_tmout_in_tv = NULL;

		rc = select(highest_sel_obj.rmv_obj + 1, rfds, wfds, efds, p_tmout_in_tv);

		if (timeout_in_10ms != NULL) {
			rem10ms = (tmout_in_tv.tv_sec * 100);
			rem10ms += ((tmout_in_tv.tv_usec + 5000) / 10000);

			if (old_rem10ms < rem10ms) {
				/* STEP: Gadbad ho gaya */
				rem10ms = old_rem10ms / 10;
				if (rem10ms == 0) {
					rc = 0;	/* TIMEOUT */
					break;
				}
			}
			old_rem10ms = rem10ms;
		}

	} while ((rc == -1) && (errno == EINTR));

	if (timeout_in_10ms != NULL) {
		/* Convert <sec,usec> to 10ms units (call it centiseconds=cs, say)
		 * tv_usec is rouded off to closest 10ms(=1cs) multiple. 
		 * (Eg.:94999us ~ 9cs and 95000us ~ 10cs)
		 */
		*timeout_in_10ms = rem10ms;
		*timeout_in_10ms = rem10ms;
	}
	return rc;
}

int32_t ncs_sel_obj_poll_single_obj(NCS_SEL_OBJ sel_obj, uint32_t *timeout_in_10ms)
{
	struct pollfd pfd;	/* Poll struct for user's fd      */
	int poll_wait_time_in_ms;	/* Total wait time req. by caller */
	struct timespec poll_start_time;	/* Time when poll() is first called */

	/* Working variables */
	struct timespec curr_time;	/* current time */
	long long diff_time_in_ms;	/* time diff in ms */
	int pollres;		/* poll() return code */
	int poll_wait_left_in_ms;	/* Wait time left in "ms" */
	int save_errno;		/* errno of poll() call */

	/* STEP: Setup poll struct */
	pfd.fd = sel_obj.rmv_obj;
	pfd.events = POLLIN;
	pfd.revents = 0;

	/* STEP: Record start time and determine total wait time  */
	clock_gettime(CLOCK_REALTIME, &poll_start_time);
	if ((timeout_in_10ms == NULL) || ((*timeout_in_10ms) < 0)) {
		/* User wants infinite wait */
		poll_wait_time_in_ms = -1;	/* -ve value = infinite wait for POLL */
	} else {
		poll_wait_time_in_ms = 10 * (*timeout_in_10ms);
	}
	poll_wait_left_in_ms = poll_wait_time_in_ms;

	/* STEP: Start wait loop */
	while (1) {
		pollres = poll(&pfd, 1, poll_wait_left_in_ms);

		/* Save errno immediately to avoid side-effects */
		save_errno = errno;

		/* Calculate wait time remaining */
		if (poll_wait_time_in_ms > 0) {
			clock_gettime(CLOCK_REALTIME, &curr_time);
			diff_time_in_ms =
			    ((curr_time.tv_sec) * (1000L) + curr_time.tv_nsec / (1000 * 1000)) -
			    ((poll_start_time.tv_sec) * (1000L) + poll_start_time.tv_nsec / (1000 * 1000));
			poll_wait_left_in_ms = poll_wait_time_in_ms - diff_time_in_ms;
			if (poll_wait_left_in_ms < 0)
				poll_wait_left_in_ms = 0;

			/* Update in/out variable */
			*timeout_in_10ms = poll_wait_left_in_ms / 10;
		}

		if ((pollres >= 0) || (save_errno != EINTR)) {

			if ((pollres == 1) && ((pfd.revents == (POLLIN | POLLHUP)))) {
				/* we reach here when one fd is closed */
				return -1;
			}

			assert((pollres == 0) ||
			       ((pollres == 1) && (pfd.revents == POLLIN)) ||
			       ((pollres == -1) && (save_errno != EINTR)));

			return pollres;
		}

		if (poll_wait_left_in_ms == 0) {
			/* We reach here only as a rare coincidence */
			assert((pollres == -1) && (save_errno == EINTR));
			return 0;	/* TIMEOUT : Rare coincidence  EINTR + TIMEOUT */
		}
	}			/* while(1) */
}

FILE *ncs_os_fopen(const char *fpath, const char *fmode)
{
	FILE *fp = NULL;
	int flags = 0;
	osaf_mutex_lock_ordie(&s_cloexec_mutex);
	fp = fopen(fpath, fmode);
	if (fp == NULL) {
		osaf_mutex_unlock_ordie(&s_cloexec_mutex);
		return NULL;
	}

	flags = fcntl(fileno(fp), F_GETFD, 0);
	fcntl(fileno(fp), F_SETFD, (flags | FD_CLOEXEC));

	osaf_mutex_unlock_ordie(&s_cloexec_mutex);

	return fp;
}
