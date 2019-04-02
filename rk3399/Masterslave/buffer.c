#include "buffer.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

extern struct cycle_buffer *fifo;
extern struct chip_buffer *chip;

int init_cycle_buffer(void)
{
    int size = 1024*1024, ret;

    ret = size & (size - 1);
    if (ret)
        return ret;
    fifo = (struct cycle_buffer *) malloc(sizeof(struct cycle_buffer));
    if (!fifo)
        return -1;

    memset(fifo, 0, sizeof(struct cycle_buffer));
    fifo->size = size;
    fifo->in = fifo->out = 0;
    pthread_mutex_init(&fifo->lock, NULL);
    fifo->buf = (unsigned char *) malloc(size);
    if (!fifo->buf)
        free(fifo);
    else
        memset(fifo->buf, 0, size);
    return 0;
}

unsigned int fifo_get(unsigned char *buf, unsigned int len)
{
    unsigned int l;
    len = min(len, fifo->in - fifo->out);
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buf, fifo->buf + (fifo->out & (fifo->size - 1)), l);
    memcpy(buf + l, fifo->buf, len - l);
    fifo->out += len;
    return len;
}

unsigned int fifo_put(unsigned char *buf, unsigned int len)
{
    unsigned int l;
    len = min(len, fifo->size - fifo->in + fifo->out);
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buf + (fifo->in & (fifo->size - 1)), buf, l);
    memcpy(fifo->buf, buf + l, len - l);
    fifo->in += len;
    return len;
}

//单片机缓冲
int init_chip_buffer(void)
{
    int size = 1024*1024, ret;

    ret = size & (size - 1);
    if (ret)
        return ret;
    chip = (struct chip_buffer *) malloc(sizeof(struct chip_buffer));
    if (!chip)
        return -1;

    memset(chip, 0, sizeof(struct chip_buffer));
    chip->size = size;
    chip->in = chip->out = 0;
    pthread_mutex_init(&chip->lock, NULL);
    chip->buf = (unsigned char *) malloc(size);
    if (!chip->buf)
        free(chip);
    else
        memset(chip->buf, 0, size);
    return 0;
}

unsigned int chip_get(unsigned char *buf, unsigned int len)
{
    unsigned int l;
    len = min(len, chip->in - chip->out);
    l = min(len, chip->size - (chip->out & (chip->size - 1)));
    memcpy(buf, chip->buf + (chip->out & (chip->size - 1)), l);
    memcpy(buf + l, chip->buf, len - l);
    chip->out += len;
    return len;
}

unsigned int chip_put(unsigned char *buf, unsigned int len)
{
    unsigned int l;
    len = min(len, chip->size - chip->in + chip->out);
    l = min(len, chip->size - (chip->in & (chip->size - 1)));
    memcpy(chip->buf + (chip->in & (chip->size - 1)), buf, l);
    memcpy(chip->buf, buf + l, len - l);
    chip->in += len;
    return len;
}
