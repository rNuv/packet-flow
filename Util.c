#include "Util.h"

void ErrorHandler(const char *err)
{
    fprintf(stderr, "%s\n", err);
    exit(0);
}

double get_current_time()
{
    struct timeval current_t;
    gettimeofday(&current_t, NULL);
    return current_t.tv_sec + 1.0 * current_t.tv_usec / 1000000;
}

void Free(void *p)
{
    if (p != NULL)
    {
        free(p);
        p = NULL;
    }
}

uint32_t rand32()
{
    uint32_t x;
    x = rand() & 0xff;
    x |= (rand() & 0xff) << 8;
    x |= (rand() & 0xff) << 16;
    x |= (rand() & 0xff) << 24;
    return x;
}

void seghdrPrint(const SegmentHdr *seghdr)
{
    printf("%u,%u,%u\n", seghdr->seqNum, seghdr->ackNum, seghdr->rwnd);
    printf("%u,%u,%u\n", seghdr->ack, seghdr->syn, seghdr->fin);
    printf("%u %u\n", *((uint8_t *)seghdr + 12), *((uint8_t *)seghdr + 13));
    printf("\n");
}

void timerInit(Timer *timer, double timesec, void *(*_callback)(void *), void *_args)
{
    timer->timestamp = get_current_time() + timesec;
    timer->callback = _callback;
    timer->args = _args;
    timer->enable = true;
}

void timerCancel(Timer *timer)
{
    timer->enable = false;
}

void *timerRun(const Timer *timer)
{
    return timer->callback(timer->args);
}

int timerCmp(const void *lhs, const void *rhs)
{
    double a = ((Timer *)lhs)->timestamp, b = ((Timer *)rhs)->timestamp;
    if (a == b)
        return 0;
    return (a < b) ? 1 : -1;
}
