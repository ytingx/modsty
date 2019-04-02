#ifndef __PSOCKCLIENT_H__
#define __PSOCKCLIENT_H__
#include "modstyhead.h"

#define BUFLEN 128

//程序升级
void upgrade(int sockfd);

//版本验证
void versionvalidation(byte *buf);

int pwverification(int sockfd, byte *account);

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);

int print_uptime(int sockfd);

void *sockclient_run(void *a);

#endif //__PSOCKCLIENT_H__
