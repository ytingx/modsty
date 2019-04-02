#ifndef __PSOCKSERVER_H__
#define __PSOCKSERVER_H__
#include "modstyhead.h"
#include "appfunction.h"

void print_time(char *ch, time_t *now);

int MAX(int a, int b);

//send单一回复
int sendsingle(int fd, byte *buffer, int len);

//串口上传信息至app
int UartToApp_Send(byte *buffer, int len);

//sockserver线程
void *sock_run(void *a);

//sockserver功能
int sockserver_run(int recvlen, byte *sockbuf);

#endif //__PSOCKSERVER_H__
