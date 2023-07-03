#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <signal.h>
#include "Util.h"

typedef struct Reliable Reliable;
#include "ReliableImpl.h"

#define SYNSENT 0
#define CONNECTED 1
#define FINWAIT 2
#define CLOSED 3

typedef struct Payload
{
    char *buf;
    uint16_t len;
    bool fin;
} Payload;

Payload *payloadCreate(uint16_t size, bool fin);
void payloadClose(Payload *payload);

struct Reliable
{
    int skt;
    struct sockaddr_in srvaddr;
    socklen_t srvlen;

    short status;
    uint32_t bytesInFlight, rwnd, cwnd;
    SafeQueue buffer;
    Heap timerHeap;

    char *seg_str;
    ReliableImpl *reliImpl;
    pthread_t thHandler;
};

Reliable *reliCreate(unsigned hport);
void reliClose(Reliable *reli);
int reliConnect(Reliable *reli, const char *ip, unsigned rport, bool nflag, uint32_t n);
void *reliGetPayload(Reliable *reli);           // return NULL if queue is empty
int reliSend(Reliable *reli, Payload *payload); // block if queue is full

ssize_t reliRecvfrom(Reliable *reli, char *seg_str, size_t size);

// Followings are APIs that you may need to use in ReliableImpl
// Sendto: Send a well-formed segment ('seg_str') to the destination.
// 'seg_str' is an array of char bytes. It should not contain UDP header.
// 'len' is the length of 'seg_str'.
ssize_t reliSendto(Reliable *reli, const char *seg_str, const size_t len);

// updateRWND: Update the receive window size.
//'rwnd' means the bytes of the receive window.
uint32_t reliUpdateRWND(Reliable *reli, uint32_t _rwnd);

// setTimer: Set a timer. We implement our own Timer in this lab (See Util.h).
// The function 'callback' will be called with 'args' as arguments
// after 'timesec' seconds.
Timer *reliSetTimer(Reliable *reli, double timesec, void *(*callback)(void *), void *args);