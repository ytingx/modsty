#ifndef __BUFFER_H__
#define __BUFFER_H__
#include "modstyhead.h"

//#define min(x, y) ((x) < (y) ? (x) : (y))
//8266
struct cycle_buffer
{
    unsigned char *buf;
    unsigned int size;
    unsigned int in;
    unsigned int out;
    pthread_mutex_t lock;
};

//缓冲区初始化
int init_cycle_buffer(void);

//取出数据
unsigned int fifo_get(unsigned char *buf, unsigned int len);

//存入数据
unsigned int fifo_put(unsigned char *buf, unsigned int len);

//单片机
struct chip_buffer
{
    unsigned char *buf;
    unsigned int size;
    unsigned int in;
    unsigned int out;
    pthread_mutex_t lock;
};

//缓冲区初始化
int init_chip_buffer(void);

//取出数据
unsigned int chip_get(unsigned char *buf, unsigned int len);

//存入数据
unsigned int chip_put(unsigned char *buf, unsigned int len);

#endif //__BUFFER_H__
