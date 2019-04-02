#ifndef __MESH_H__
#define __MESH_H__
#include "modstyhead.h"

//分配节点MAC
struct noderawinfo
{
    byte macinfo[10];
};
struct noderawinfo nodemac[300];

//存储节点原始信息
void saveorignode(byte *rawmac);

//读取节点原始信息
int readorignode();

//检索节点原始信息
int checkorignode(byte *rawmac, byte *ackmac);

//节点原始信息
void savenoderawinfo(byte *rawmac);

//分配节点MAC
void distributionmac();

//开启mesh组网
void meshnetworking();

//关闭mesh组网
void closemeshnet();

//解散mesh网络
void disbandedmesh();

//组网结束
void meshnetover(byte *buf);

#endif //__MESH_H__
