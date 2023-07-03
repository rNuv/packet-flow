#include "Queue.h"

void queueInit(SafeQueue *sq, size_t size)
{
    sq->size = size;
    sq->queue = (void **)malloc((sizeof(void *) * size));
    sq->count = 0;
    pthread_mutex_init(&(sq->mtx), NULL);
    pthread_cond_init(&(sq->empty), NULL);
    pthread_cond_init(&(sq->full), NULL);
};

void queueClear(SafeQueue *sq)
{
    pthread_mutex_lock(&(sq->mtx));
    if (sq->queue)
    {
        free(sq->queue);
        sq->queue = NULL;
    }
    pthread_mutex_unlock(&(sq->mtx));
}

void queueLock(SafeQueue *sq)
{
    pthread_mutex_lock(&sq->mtx);
}

void queueUnlock(SafeQueue *sq)
{
    pthread_mutex_unlock(&sq->mtx);
}

void *queueFront(const SafeQueue *sq) //unsafe
{
    return sq->queue[sq->head];
}

int queuePush(SafeQueue *sq, void *el)
{
    if (sq->count == sq->size)
        return -1;
    sq->queue[sq->tail++] = el;
    if (sq->tail >= sq->size)
        sq->tail -= sq->size;
    sq->count++;
    return 0;
}

int queuePop(SafeQueue *sq) //unsafe
{
    if (sq->count == 0)
        return -1;
    sq->head++;
    if (sq->head >= sq->size)
        sq->head -= sq->size;
    sq->count--;
    return 0;
}

static struct timespec getOutTime(int timesec)
{
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + timesec;
    outtime.tv_nsec = now.tv_usec * 1000;
    return outtime;
}

int queuePut(SafeQueue *sq, void *el, int timeout)
{
    pthread_mutex_lock(&sq->mtx);
    if (timeout > 0)
    {
        struct timespec outtime = getOutTime(timeout);
        int err = 0;
        while (sq->count == sq->size)
        {
            err = pthread_cond_timedwait(&sq->full, &sq->mtx, &outtime);
            if (err == ETIMEDOUT)
            {
                pthread_mutex_unlock(&sq->mtx);
                return -1;
            }
        }
    }
    else
    {
        while (sq->count == sq->size)
            pthread_cond_wait(&sq->full, &sq->mtx);
    }
    int res = queuePush(sq, el);
    pthread_cond_signal(&sq->empty);
    pthread_mutex_unlock(&sq->mtx);
    return res;
}

int queuePutUnblock(SafeQueue *sq, void *el)
{
    pthread_mutex_lock(&sq->mtx);
    if (sq->count == sq->size)
    {
        pthread_mutex_unlock(&sq->mtx);
        return -1;
    }
    int res = queuePush(sq, el);
    pthread_cond_signal(&sq->empty);
    pthread_mutex_unlock(&sq->mtx);
    return res;
}

void *queueGet(SafeQueue *sq, int timeout)
{
    void *res = NULL;
    pthread_mutex_lock(&sq->mtx);
    if (timeout > 0)
    {
        struct timespec outtime = getOutTime(timeout);
        int err = 0;
        while (sq->count == 0)
        {
            err = pthread_cond_timedwait(&sq->empty, &sq->mtx, &outtime);
            if (err == ETIMEDOUT)
            {
                pthread_mutex_unlock(&sq->mtx);
                return res;
            }
        }
    }
    else
    {
        while (sq->count == 0)
            pthread_cond_wait(&sq->empty, &sq->mtx);
    }
    res = queueFront(sq);
    queuePop(sq);
    pthread_cond_signal(&sq->full);
    pthread_mutex_unlock(&sq->mtx);
    return res;
}

void *queueGetUnblock(SafeQueue *sq)
{
    pthread_mutex_lock(&sq->mtx);
    if (sq->count == 0)
    {
        pthread_mutex_unlock(&sq->mtx);
        return NULL;
    }
    void *res = queueFront(sq);
    queuePop(sq);
    pthread_cond_signal(&sq->full);
    pthread_mutex_unlock(&sq->mtx);
    return res;
}

void heapInit(Heap *h, int (*cmp)(const void *, const void *))
{
    h->count = 0;
    h->size = DEFAULT_CAPACITY;
    h->array = (void **)malloc(sizeof(void *) * h->size);
    h->cmp = cmp;
}

void heapClear(Heap *h)
{
    if (h->array)
    {
        free(h->array);
        h->array = NULL;
    }
}

static int heapCheckSize(Heap *h)
{
    if (h->count < h->size)
        return 0;

    h->size *= 2;
    h->array = (void **)realloc(h->array, sizeof(void *) * h->size);
    return 1;
}

static void swap(void **a, void **b)
{
    void *tmp = *a;
    *a = *b;
    *b = tmp;
}

size_t heapPushup(Heap *h, size_t idx)
{
    while (idx != 0)
    {
        size_t parent = ((idx - 1) >> 1);

        if (h->cmp(h->array[idx], h->array[parent]) <= 0)
            break;
        swap(&(h->array[idx]), &(h->array[parent]));
        idx = parent;
    }
    return idx;
}

size_t heapPushdown(Heap *h, size_t idx)
{
    while (idx < h->count)
    {
        size_t childl, childr, child;
        childl = (idx << 1) + 1;
        childr = (idx << 1) + 2;

        if (childl >= h->count)
            break;
        if (childr >= h->count)
            child = childl;
        else
            child = (h->cmp(h->array[childl], h->array[childr]) > 0) ? childl : childr;

        if (h->cmp(h->array[idx], h->array[child]) >= 0)
            break;
        swap(&(h->array[idx]), &(h->array[child]));
        idx = child;
    }
    return idx;
}

int heapPush(Heap *h, void *el)
{
    heapCheckSize(h);
    h->array[h->count] = el;
    heapPushup(h, h->count++);
    return 0;
}

void *heapTop(Heap *h)
{
    if (h->count == 0)
        return NULL;
    return h->array[0];
}

int heapPop(Heap *h)
{
    if (h->count == 0)
        return -1;

    void *res = h->array[0];
    h->array[0] = h->array[--(h->count)];
    heapPushdown(h, 0);

    return 0;
}
