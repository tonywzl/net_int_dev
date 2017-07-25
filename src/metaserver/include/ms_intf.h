#ifndef MS_INTF_H
#define MS_INTF_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/cdefs.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/uio.h>

#include "ioflag.h"
#include "msret.h"
typedef void (*Callback)(MsRet, void *);

MsRet MS_Write_Async(const char *voluuid,
                     const char *snapuuid,
                     const void *buf,
                     off_t voff,
                     size_t len,
                     Callback func,
                     void *arg,
                     void *fpBuf,
                     IoFlag flag,
                     size_t *more);

MsRet MS_Writev_Async(const char *voluuid,
                      const char *snapuuid,
                      struct iovec *iovec,
                      off_t voff,
                      size_t count,
                      Callback func,
                      void *arg,
                      void *fpBuf,
                      IoFlag flag,
                      size_t *more);

MsRet MS_Read_Async(const char *voluuid,
                    const char *snapuuid,
                    void *buf,
                    off_t voff,
                    size_t len,
                    Callback func,
                    void *arg,
                    IoFlag flag);

MsRet MS_Read_Fp_Async(const char *voluuid,
                       const char *snapuuid,
                       off_t voff,
                       size_t len,
                       Callback func,
                       void *arg,
                       void *fp,		// non-existing fp will cleared to 0
                       bool *fpFound, 	// output parm indicating fp found or not
                       IoFlag flag);

MsRet MS_Read_Data_Async(struct iovec* vec,
                         size_t count,
                         void *fp,
                         // bool *fpToRead, enable until required by nid
                         Callback func, 
                         void *arg,
                         IoFlag flag);

/* new interface for replication
 * 3 new interfaces added to provide intf for nid to implement repl workflow.
 * One thing in common of these 3 interfaces is they all require a consecutive 
 * namespace range to operate on. How big the range is up on caller (nid) 's decision
 * but if it is too small, the efficiency gets low. 4MB is suggested as initial value.
 * But it can be varied along the progress of the workflow by the caller.
 * Below is one example:
 *		1. nid call read intf the 1st time using 4mb as ns range but the result turns
 *			out to be a very small group of fingerprints which is not worthy of the
 *			overhead of one time network transfer
 *		2. nid either increase the size of ns range or keep using the same 4mb, until 
 *			there are enough output to send to the target side
 *		3. after the network transfer, the 1st write intf is called on target side, but
 *			called by using a much larger ns range compared with original 4mb
 *		4. in each intf call, the boolean array is the indicator which ns offset has fp
 *			to operate on
 *		about missing snapuuid in all target write intf:
 *		   	1. when replicating a volume,  the workflow is always
 *				1.1 source side take a snapshot
 *				1.2 source side read from the snapshot just taken, so io not blocked on src side
 *				1.3 target side write to current target volume
 *				1.4 target side take a snapshot after all write done
 *	   			1.5 now <src vol : src snap> has all its data repl-ed to <tgt vol: tgt snap>
 *			2. based on workflow in 1.,
 * 				2.1 if b/w a volume on src side and a volume on tgt side, there was no any
 *					repl done before, we can repl any snapshot on src side
 *				2.2 if b/w a volume on src side and a volume on tgt side, there was already
 *         			repl done before, we can ONLY repl snapshots taken after the latest repl
 */

/*
 * function: MS_Write_Fp_Async
 * -----------------------------------------
 *		this is the 1st phase write function in replication target side
 * 		inputs together provides a list of fingerprints scattered in a consecutive namespace
 *		range, expecting the function to only those namespace offsets whose corresponding
 *		fingerprints already exist, and mark those offsets whose fingerprints don't exist
 *		on return, so that caller have a list of fingerprints needed to fetch the mapping data
 *  
 * voluuid: 		IN 	volume uuid
 * voff:			IN 	ns offset
 * len:				IN 	ns length
 * func / arg:		IN 	callback function and parm ptr
 * fp:				IN 	fingerprint buffer
 * fpToWrite:		IN 	bool array indicating which ns offset has fp to write
 * fpMiss:          OUT bool array indicating which ns offset's associating fp doesn't exist
 *
 * return: status
 *
 * note:
 *		# of pages in ns range = len / pagesize(e.g 4kb) = length of fpToWrite
 * 		= length of fpMiss
 *		however,
 *		# of fingerprints in fp = number of true bytes only in fpToWrite
 */
MsRet MS_Write_Fp_Async(const char *voluuid,
                        off_t voff,
                        size_t len,
                        Callback func,
                        void *arg,
                        void *fp,
                        bool *fpToWrite,
                        bool *fpMiss);

/*
 * function: MS_Write_Data_Async
 * -----------------------------------------
 *		this is the 2nd phase write function in replication target side
 * 		inputs together provides
 *			1. a list of page memory &
 *			2. a list of corresponding fingerprints for each page &
 *			3. starting namespace offset and length of a consecutive range
 *		and expect the funtion save these data and update corresponding
 *		fingerprint and namespace mappings on return
 *  
 * voluuid: 		IN 	volume uuid
 * iovec / count:	IN 	memory of data pages
 * voff / len		IN 	ns offset and length
 * func / arg:		IN 	callback function and parm ptr
 * fp:				IN 	fingerprint buffer
 * fpToWrite:		IN 	bool array indicating which ns offset has fp to write
 *
 * return: status
 *
 * note:
 *		# of pages in ns range = len / pagesize(e.g 4kb) = length of fpToWrite
 *		however,
 *		# of fingerprints in fp = number of true bytes only in fpToWrite
 *
 */
MsRet MS_Write_Data_Async(const char *voluuid,
                          struct iovec *iovec,
                          size_t count,
                          off_t voff,
                          size_t len,
                          Callback func,
                          void *arg,
                          void *fp,
                          bool *fpToWrite);

/*
 * function: MS_Read_Fp_Snapdiff_Async
 * -----------------------------------------
 *		this is called on repl source side at the beginning of each delta in the
 *		incremental repl design, to return a list of overwritten namespace offsets
 *		and their corresponding fingerprints, in a consecutive namespace range
 *  
 * voluuid: 		IN 	volume id
 * snapuuidPre:		IN  uuid of last snapshot which had been completely repl-ed to target
 * snapuuidCur:		IN 	uuid of the snapshot just taken right before this delta repl starts 
 * voff / len		IN 	ns offset and length
 * func / arg:		IN 	callback function and parm ptr
 * fp:				OUT	fingerprint buffer, entries don't have value set will be cleared to 0
 * fpDiff	:		OUT	bool array indicating which ns offset has been overwritten
 *						and thus has fingerprint value in fp buffer
 *
 * return: status
 *
 * note:
 *		# of pages in ns range = len / pagesize(e.g 4kb) = max # of fp in fp buffer
 *		= length of fpDiff
 *		however in output parms on return:
 *		# of fingerprints set in fp buffer = number of true bytes only in fpDiff
 */
MsRet MS_Read_Fp_Snapdiff_Async(const char *voluuid,
                                const char *snapuuidPre,
                                const char *snapuuidCur,
                                off_t voff,
                                size_t len,
                                Callback func,
                                void *arg,
                                void *fp,
                                bool *fpDiff);


MsRet MS_Flush(Callback, void *);
void Ms_Start();
void Ms_Stop();

#endif
