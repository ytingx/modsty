#ifndef __PUARTSEND_H__
#define __PUARTSEND_H__
#include "modstyhead.h"
#include "buffer.h"
#include "uart.h"

//自动场景存储信息
void autoscestoreinfo();

//手动场景存储信息
void manscestoreinfo();

//存储信息
void storeinfo();

//uart环形缓冲区线程
void *uart_run(void *arg);

//取出缓冲区数据
void UartSplit();

//uart发送
int UartSend(int fd, byte *send_buf,int data_len);

#endif //__PUARTSEND_H__
