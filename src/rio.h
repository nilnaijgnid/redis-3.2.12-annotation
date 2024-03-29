/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __REDIS_RIO_H
#define __REDIS_RIO_H

#include <stdio.h>
#include <stdint.h>
#include "sds.h"

struct _rio {
    /* Backend functions.
     * Since this functions do not tolerate short writes or reads the return
     * value is simplified to: zero on error, non zero on complete success. 
     * 由于函数不能容忍短写或者短读
     * 错误返回0，完成成功则返回非0*/
    
    size_t (*read)(struct _rio *, void *buf, size_t len);
    size_t (*write)(struct _rio *, const void *buf, size_t len);
    off_t (*tell)(struct _rio *);
    int (*flush)(struct _rio *);
    /* The update_cksum method if not NULL is used to compute the checksum of
     * all the data that was read or written so far. The method should be
     * designed so that can be called with the current checksum, and the buf
     * and len fields pointing to the new block of data to add to the checksum
     * computation. 
     * update_cksum方法（如果不是NULL）用于计算到目前为止读取或写入的所有数据的校验和。 
     * 应该设计该方法，以便可以使用当前校验和调用，并将指向新数据块的buf和len字段添加到校验和计算中。
     * */
    void (*update_cksum)(struct _rio *, const void *buf, size_t len);

    /* The current checksum */
    uint64_t cksum; // 当前的校验和

    /* number of bytes read or written */
    size_t processed_bytes; // 当前读取或者写入的字节数

    /* maximum single read or write chunk size */
    size_t max_processing_chunk;    // 单次读或写最大数据长度

    /* Backend-specific vars. */
    union {
        /* In-memory buffer target. 
           内存缓冲器 */
        struct {
            sds ptr;
            off_t pos;      /* 偏移量 */
        } buffer;
        /* Stdio file pointer target. 
           标准输入输出指针 */
        struct {
            FILE *fp;
            off_t buffered; /* Bytes written since last fsync. 自从上次fsync写入的字节数*/
            off_t autosync; /* fsync after 'autosync' bytes written. XXX */
        } file;
        /* Multiple FDs target (used to write to N sockets). 
           多个文件描述符（用于向多个socket写入数据）*/
        struct {
            int *fds;       /* File descriptors. 文件描述符 */
            int *state;     /* Error state of each fd. 0 (if ok) or errno. 每个文件描述符的状态，0位正常，错误则为错误码 */
            int numfds;     /* 文件描述符的数量 */
            off_t pos;      /* 偏移量 */
            sds buf;
        } fdset;
    } io;
};

typedef struct _rio rio;

/* The following functions are our interface with the stream. They'll call the
 * actual implementation of read / write / tell, and will update the checksum
 * if needed. 
 * 以下函数是操作数据流的接口，这些函数将调用read/write/tell的具体实现，并在需要时更新校验和。*/

// 写入方法
static inline size_t rioWrite(rio *r, const void *buf, size_t len) {
    while (len) {   // 如果还有数据要写入的话
        // 判断要写入的数据长度对否大于单次写入的最大限制
        // 如果超过了单次最大写入长度，则本次值写入max_processing_chunk，否则就讲剩下的数据全写入
        size_t bytes_to_write = (r->max_processing_chunk && r->max_processing_chunk < len) ? r->max_processing_chunk : len;
        if (r->update_cksum) r->update_cksum(r,buf,bytes_to_write); // 更新校验和
        if (r->write(r,buf,bytes_to_write) == 0)    // 写入数据
            return 0;
        buf = (char*)buf + bytes_to_write;
        len -= bytes_to_write;                      // 更新剩余要写入的数据长度
        r->processed_bytes += bytes_to_write;       // 更新已写入的数据长度信息
    }
    return 1;
}

// 读取方法
static inline size_t rioRead(rio *r, void *buf, size_t len) {
    while (len) {
        size_t bytes_to_read = (r->max_processing_chunk && r->max_processing_chunk < len) ? r->max_processing_chunk : len;
        if (r->read(r,buf,bytes_to_read) == 0)
            return 0;
        if (r->update_cksum) r->update_cksum(r,buf,bytes_to_read);
        buf = (char*)buf + bytes_to_read;
        len -= bytes_to_read;
        r->processed_bytes += bytes_to_read;
    }
    return 1;
}

static inline off_t rioTell(rio *r) {
    return r->tell(r);
}

static inline int rioFlush(rio *r) {
    return r->flush(r);
}

void rioInitWithFile(rio *r, FILE *fp);
void rioInitWithBuffer(rio *r, sds s);
void rioInitWithFdset(rio *r, int *fds, int numfds);

void rioFreeFdset(rio *r);

size_t rioWriteBulkCount(rio *r, char prefix, int count);
size_t rioWriteBulkString(rio *r, const char *buf, size_t len);
size_t rioWriteBulkLongLong(rio *r, long long l);
size_t rioWriteBulkDouble(rio *r, double d);

void rioGenericUpdateChecksum(rio *r, const void *buf, size_t len);
void rioSetAutoSync(rio *r, off_t bytes);

#endif
