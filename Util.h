#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "Queue.h"

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 2
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif __APPLE__
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif __unix__
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif __sun
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#define UDP_DATAGRAM_SIZE 1024
#define SEGMENT_SIZE (UDP_DATAGRAM_SIZE - 8) // UDP header: 8 bytes
#define PAYLOAD_SIZE (SEGMENT_SIZE - 16)
#define MAX_BAND (50 * 1024 * 1024 / 8)
#define MAX_DELAY 0.5
#define MAX_BDP ((int)(MAX_BAND * MAX_DELAY))
#define MAX_RTO 60.0
#define BUFFER_SIZE (10 * MAX_BDP / PAYLOAD_SIZE)

#ifdef NOCC
#define INIT_CWDN MAX_BDP
#else
#define INIT_CWDN (3 * PAYLOAD_SIZE)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void ErrorHandler(const char *err);

double get_current_time();

void Free(void *p);

uint32_t rand32();

typedef struct SegmentHdr
{
    uint32_t seqNum, ackNum, rwnd;
    uint8_t reserved_byte;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t fin : 1;
    uint8_t syn : 1;
    uint8_t ack : 1;
    uint8_t reserved_bits : 5;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t reserved_bits : 5;
    uint8_t ack : 1;
    uint8_t syn : 1;
    uint8_t fin : 1;
#endif
    uint16_t checksum;
} SegmentHdr;

void seghdrPrint(const SegmentHdr *seghdr);

typedef struct Timer
{
    double timestamp;
    bool enable;
    void *(*callback)(void *);
    void *args;
} Timer;

void timerInit(Timer *timer, double timesec, void *(*_callback)(void *), void *_args);
void timerCancel(Timer *timer);
void *timerRun(const Timer *timer);
int timerCmp(const void *lhs, const void *rhs);
