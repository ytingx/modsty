#ifndef __SECURITYMONITOR_H__
#define __SECURITYMONITOR_H__
#include "modstyhead.h"

//配置
void securityconfig(byte *buf);

//读取安防执行条件
void readsecconditions(int id);

//配置信息读入内存
void readsectriggercond(byte *secconfig);

//读取安防配置
void readsecurityconfig(int id, byte *secconfig);

#endif //__SECURITYMONITOR_H__
