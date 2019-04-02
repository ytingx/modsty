#ifndef __PLEDCHECK_H__
#define __PLEDCHECK_H__
#include "modstyhead.h"
#include "buffer.h"

//led指令发送
int Uart_Crc(int fd, byte *send_buf,int data_len);

//获取指令节点状态
//int getnodestate(byte *nodemac);

//led命令执行部分
void *ledcheck(void *a);

#endif //__PLEDCHECK_H__
