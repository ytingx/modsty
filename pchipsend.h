#ifndef __PCHIPSEND_H__
#define __PCHIPSEND_H__
#include "modstyhead.h"
#include "buffer.h"
#include "uart.h"

int slavechipuartsend(byte *sendbuf, int datalen);

int slavechipSend(int fd, byte *send_buf,int data_len);

//uart环形缓冲区线程
void *chip_uart_run(void *arg);

//取出缓冲区数据
void Chip_Split();

//uart发送
int Chip_Send(int fd, byte *send_buf,int data_len);

#endif //__PCHIPSEND_H__
