#include "ReliableImpl.h"
#include "Congestion.h"

// You can add necessary functions here

// reliImplCreate: Constructor. You can add variables to maintain your
// sliding window states.
// 'reli' provides an interface to call functions of struct Reliable.
// 'seqNum' indicates the initail sequence number in the SYN segment.
// 'srvSeqNum' indicates the initial sequence number in the SYNACK segment.
// Update: We already provided the code of reliImplCreate.
// You do not need to implement it, but you can modify it if necessary.
ReliableImpl *reliImplCreate(Reliable *_reli, uint32_t _seqNum, uint32_t _srvSeqNum)
{
    ReliableImpl *reliImpl = (ReliableImpl *)malloc(sizeof(ReliableImpl));
    reliImpl->reli = _reli;
    reliImpl->lastByteSent = _seqNum;
    reliImpl->nextByteExpected = _srvSeqNum + 1; // nextByteExpected remains unchanged in this lab

    queueInit(&reliImpl->swnd, MAX_BDP / PAYLOAD_SIZE); // a queue to store sent segments
    reliImpl->lastByteAcked = _seqNum;
    reliImpl->lastAckNum = reliImpl->lastByteAcked + 1;

    reliImpl->status = SS;
    reliImpl->ssthresh = 20000;
    reliImpl->rto = MIN_RTO;
    reliImpl->srtt = -1;
    reliImpl->rttvar = -1;
    reliImpl->FRCount = 0; // Count for fast retransmission

    return reliImpl;
}

// reliImplClose: Destructor, free the memory for maintaining states.
void reliImplClose(ReliableImpl *reliImpl)
{
    // TODO: Your code here

    Free(reliImpl);
}

// reliImplChecksum: 16-bit Internet checksum (refer to RFC 1071 for calculation)
// This function should return the value of checksum (an unsigned 16-bit integer).
// You should calculate the checksum over 'buf', which is an array of char bytes
// 'len' is the length of buf.
// Update: We already provided the code of checksum and you do not need to implement it.
uint16_t reliImplChecksum(const char *buf, ssize_t len)
{
    uint32_t sum = 0;
    for (ssize_t i = 0; i < len - 1; i += 2)
        sum += *(uint16_t *)(buf + i);
    if (len & 1)
    {
        uint16_t tmp = 0;
        *((char *)(&tmp)) = buf[len - 1];
        sum += tmp;
    }
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

// reliImplRecvAck: When an ACK or FINACK segment is received, the framework will
// call this function to handle the segment.
// The checksum will be verified before calling this function, so you
// do not need to verify checksum again in this function.
// Remember to call reliUpdateRWND to update the receive window size.
// Note that this function should return the reduction of bytes in flight
// (a non-negative integer) so that Reliable.h/c can update the bytes in flight.
// 'buf' is an array of bytes of the received segment, including the segment header.
// 'len' is the length of 'buf'.
uint32_t reliImplRecvAck(ReliableImpl *reliImpl, const char *buf, uint16_t len)
{
    // TODO: Your code here

    return 0;
}

// reliImplSendData: This function is called when a piece of payload should be sent out.
// You should encapsulate a segment (you can refer to the code in reliConnect function in reliable.c) and
// call reliSetTimer (see Reliable.h) to set a Timer for retransmission.
// Use reliSendto (see Reliable.h) to send a segment to the receiver.
// Note that this function should return the increment of bytes in flight
// (a non-negative integer) so that Reliable.h/c can update the bytes in flight.
// 'payload' is an array of char bytes.
// 'payloadlen' is the length of payload.
// 'isFin'=True means a FIN segment should be sent out.
uint32_t reliImplSendData(ReliableImpl *reliImpl, char *payload, uint16_t payloadlen, bool isFin)
{
    // TODO: Your code here
    return 0;
}

// reliImplRetransmission: A callback function for retransmission
// when you call reliSetTimer.
// You should call updateCWND to update the congestion window size.
void *reliImplRetransmission(void *args)
{
    // TODO: Your code here
    return NULL;
}

// fastRetransmission: The reliImplRecvAck uses this function instead of
// reliImplRetransmission to do fast retransmission when reliImplRecvAck
// considers some segments should be fast retransmitted.
// You should call updateCWND to update the congestion window size.
void *reliImplFastRetransmission(void *args)
{
    // TODO: Your code here
    return NULL;
}
