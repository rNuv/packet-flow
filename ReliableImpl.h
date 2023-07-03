#pragma once

#include "Util.h"

typedef struct ReliableImpl ReliableImpl;
#include "Reliable.h"

struct ReliableImpl
{
    Reliable *reli;
    uint32_t lastByteSent, nextByteExpected;

    // Variables for maintaining sliding window
    uint32_t lastByteAcked, lastAckNum; // lastAckNum = lastByteAcked + 1
    uint32_t FRCount;
    SafeQueue swnd;
    double rto;

    int status;
    uint32_t ssthresh;
    double srtt, rttvar;
};

ReliableImpl *reliImplCreate(Reliable *_reli, uint32_t _seqNum, uint32_t _srvSeqNum);
void reliImplClose(ReliableImpl *reliImpl);
uint16_t reliImplChecksum(const char *buf, ssize_t len);
uint32_t reliImplRecvAck(ReliableImpl *reliImpl, const char *buf, uint16_t len);
uint32_t reliImplSendData(ReliableImpl *reliImpl, char *payload, uint16_t payloadlen, bool isFin);
void *reliImplRetransmission(void *args);
void *reliImplFastRetransmission(void *args);

// You can add necessary struct or class here
