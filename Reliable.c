#include "Reliable.h"

Payload *payloadCreate(uint16_t size, bool fin)
{
    Payload *payload = (Payload *)malloc(sizeof(Payload));
    payload->buf = (char *)malloc(sizeof(char) * size);
    payload->fin = fin;
    return payload;
}

void payloadClose(Payload *payload)
{
    if (payload == NULL)
        return;

    Free(payload->buf);
    Free(payload);
}

void *reliGetPayload(Reliable *reli)
{
    return queueGetUnblock(&reli->buffer);
}

int reliSend(Reliable *reli, Payload *payload)
{
    return queuePut(&reli->buffer, payload, 0);
}

ssize_t reliRecvfrom(Reliable *reli, char *buf, size_t size)
{
    return recvfrom(reli->skt, buf, size, 0, (struct sockaddr *)&(reli->srvaddr), &(reli->srvlen));
}

ssize_t reliSendto(Reliable *reli, const char *buf, const size_t len)
{
    return sendto(reli->skt, buf, len, 0, (struct sockaddr *)&reli->srvaddr, reli->srvlen);
}

uint32_t reliUpdateRWND(Reliable *reli, uint32_t _rwnd)
{
    reli->rwnd = _rwnd;
    return reli->rwnd;
}

Timer *reliSetTimer(Reliable *reli, double timesec, void *(*callback)(void *), void *args)
{
    Timer *timer = (Timer *)malloc(sizeof(Timer));
    timerInit(timer, timesec, callback, args);
    heapPush(&reli->timerHeap, timer);
    return timer;
}

static void setSktTimeout(int skt, int timesec)
{
    struct timeval tv;
    tv.tv_sec = timesec;
    tv.tv_usec = 0;
    setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

static void *reliHandler(void *args);

Reliable *reliCreate(unsigned hport)
{
    Reliable *reli = (Reliable *)malloc(sizeof(Reliable));
    reli->status = CLOSED;
    reli->bytesInFlight = 0;
    reli->rwnd = MAX_BDP;
    reli->cwnd = INIT_CWDN;
    reli->seg_str = NULL;
    reli->reliImpl = NULL;
    queueInit(&reli->buffer, BUFFER_SIZE);
    heapInit(&reli->timerHeap, timerCmp);

    reli->skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (reli->skt == -1)
    {
        reliClose(reli);
        return NULL;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(hport);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(reli->skt, (struct sockaddr *)&addr, sizeof(addr));

    srand((unsigned)time(NULL));
    return reli;
}

void reliClose(Reliable *reli)
{
    if (reli == NULL)
        return;

    Payload *fin = payloadCreate(0, true);
    reliSend(reli, fin);                 // payload.fin=true;
    pthread_join(reli->thHandler, NULL); // return error number if thHanlder is not initialized

    while (reli->buffer.count > 0)
    {
        Payload *payload = queueFront(&reli->buffer);
        queuePop(&reli->buffer);
        Free(payload->buf);
    }
    queueClear(&reli->buffer);

    while (reli->timerHeap.count > 0)
    {
        Timer *timer = heapTop(&reli->timerHeap);
        heapPop(&reli->timerHeap);
        Free(timer);
    }
    heapClear(&reli->timerHeap);

    reliImplClose(reli->reliImpl);
    Free(reli->seg_str);
    Free(reli);
}

int reliConnect(Reliable *reli, const char *ip, unsigned rport, bool nflag, uint32_t n)
{
    if (reli == NULL)
        return -1;

    reli->srvaddr.sin_family = AF_INET;
    reli->srvaddr.sin_port = htons(rport);
    reli->srvaddr.sin_addr.s_addr = inet_addr(ip);
    reli->srvlen = sizeof(reli->srvaddr);

    setSktTimeout(reli->skt, 1);
    reli->seg_str = (char *)malloc(sizeof(char) * SEGMENT_SIZE);
    uint32_t seqNum = nflag ? n : rand32();
    uint32_t srvSeqNum = 0;

    uint16_t buflen = sizeof(SegmentHdr) + 0;
    char *buf = (char *)malloc(buflen * sizeof(char));
    SegmentHdr *seg = (SegmentHdr *)buf;
    seg->seqNum = htonl(seqNum);
    seg->reserved_bits = seg->reserved_byte = 0;
    seg->ackNum = seg->rwnd = seg->ack = seg->fin = 0;
    seg->syn = 1;
    seg->checksum = 0;
    // memcpy(buf + sizeof(SegmentHdr), NULL, 0);
    seg->checksum = reliImplChecksum(buf, buflen);

    int synretry = 0;
    reli->status = SYNSENT;
    while (reli->status != CONNECTED)
    {
        reliSendto(reli, buf, buflen);
        ssize_t len = reliRecvfrom(reli, reli->seg_str, SEGMENT_SIZE);
        if (len < 0 || reliImplChecksum(reli->seg_str, len) != 0)
        {
            if (synretry > 60)
            {
                reli->status = CLOSED;
                return -1;
            }
            synretry += 1;
            continue;
        }

        seg = (SegmentHdr *)reli->seg_str;
        if (seg->syn && seg->ack && ntohl(seg->ackNum) == (seqNum + 1))
        {
            reli->status = CONNECTED;
            srvSeqNum = ntohl(seg->seqNum);
        }
    }

    setSktTimeout(reli->skt, 0);
    reli->reliImpl = reliImplCreate(reli, seqNum, srvSeqNum);
    pthread_create(&(reli->thHandler), NULL, reliHandler, reli);
    return 0;
}

static void *reliHandler(void *args)
{
    Reliable *reli = (Reliable *)args;
    fd_set inputs, outputs;
    while (reli->status != CLOSED)
    {
        FD_ZERO(&inputs);
        FD_SET(reli->skt, &inputs);
        FD_ZERO(&outputs);
        FD_SET(reli->skt, &outputs);
        select(reli->skt + 1, &inputs, &outputs, NULL, NULL);

        if (FD_ISSET(reli->skt, &inputs))
        {
            ssize_t len = reliRecvfrom(reli, reli->seg_str, SEGMENT_SIZE);
            if (len <= 0 || reliImplChecksum(reli->seg_str, len) != 0)
                goto outputLabel;
            SegmentHdr *seg = (SegmentHdr *)reli->seg_str;
            if (reli->status == CONNECTED)
            {
                if (seg->ack && !seg->syn && !seg->fin)
                    reli->bytesInFlight -= reliImplRecvAck(reli->reliImpl, reli->seg_str, len);
            }
            else if (reli->status == FINWAIT)
            {
                if (seg->ack && !seg->syn && !seg->fin)
                    reli->bytesInFlight -= reliImplRecvAck(reli->reliImpl, reli->seg_str, len);
                else if (seg->ack && seg->fin)
                {
                    reli->bytesInFlight -= reliImplRecvAck(reli->reliImpl, reli->seg_str, len);
                    reli->status = CLOSED;
                }
            }
        }

    outputLabel:;
        if (FD_ISSET(reli->skt, &outputs))
        {
            if (reli->status != CONNECTED || reli->bytesInFlight >= MIN(reli->rwnd, reli->cwnd))
                goto timerLabel;
            Payload *payload = reliGetPayload(reli);
            if (payload == NULL)
                goto timerLabel;
            if (payload->fin)
            {
                reli->bytesInFlight += reliImplSendData(reli->reliImpl, NULL, 0, true);
                reli->status = FINWAIT;
            }
            else
                reli->bytesInFlight += reliImplSendData(reli->reliImpl, payload->buf, payload->len, false);
            usleep(1); // Avoid sending too fast and overflowing UDP buffer at the receiver
            payloadClose(payload);
        }

    timerLabel:;
        double now = get_current_time();
        while (reli->timerHeap.count > 0)
        {
            Timer *timer = heapTop(&reli->timerHeap);
            if (now < timer->timestamp)
                break;
            if (timer->enable)
                timerRun(timer);

            heapPop(&reli->timerHeap);
            Free(timer);
        }
    }
    return NULL;
}
