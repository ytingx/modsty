#ifndef __PBROADCAST_H__
#define __PBROADCAST_H__
#include "modstyhead.h"

#define IP_FOUND     0x41
#define IP_FOUND_ACK 0x42
#define PORT 9999

//修改网络配置
void modifynetwork(void);

//广播线程
void *bcastserver_run(void *a);

#endif //__PBROADCAST_H__
