#ifndef __PSLAVESOCKCLIENT_H__
#define __PSLAVESOCKCLIENT_H__
#include "modstyhead.h"

#define BUFLEN 128

int slabroadcast(struct sockaddr_in* seraddr);

//程序升级
void slaveupgrade(int sockfd);

//版本验证
void slaveversionvalidation(byte *buf);

int slavepwverification(int sockfd, byte *account);

int slaveconnect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);

int slaveprint_uptime(int sockfd);

void *slavesockclient_run(void *a);

#endif //__PSLAVESOCKCLIENT_H__
