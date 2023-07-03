#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

typedef struct SafeQueue
{
    size_t head, tail, count, size;
    void **queue;
    pthread_mutex_t mtx;
    pthread_cond_t empty, full;
} SafeQueue;

void queueInit(SafeQueue *sq, size_t size);
void queueClear(SafeQueue *sq);
void queueLock(SafeQueue *sq);
void queueUnlock(SafeQueue *sq);
void *queueFront(const SafeQueue *sq);  //thread-unsafe
int queuePush(SafeQueue *sq, void *el); //thread-unsafe
int queuePop(SafeQueue *sq);            //thread-unsafe
int queuePut(SafeQueue *sq, void *el, int timeout);  //thread-safe
int queuePutUnblock(SafeQueue *sq, void *el);  //thread-safe
void *queueGet(SafeQueue *sq, int timeout); //thread-safe
void *queueGetUnblock(SafeQueue *sq); //thread-safe

// Adapted from https://github.com/willemt/heap
// Copyright (c) 2011, Willem-Hendrik Thiart
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * The names of its contributors may not be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#define DEFAULT_CAPACITY 13

typedef struct Heap
{
    size_t size, count;
    void **array;
    int (*cmp)(const void *, const void *);
} Heap;

void heapInit(Heap *h, int (*cmp)(const void *, const void *));
void heapClear(Heap *h);
size_t heapPushup(Heap *h, size_t idx);
size_t heapPushdown(Heap *h, size_t idx);
int heapPush(Heap *h, void *el);
void *heapTop(Heap *h);
int heapPop(Heap *h);
