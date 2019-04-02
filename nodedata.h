#ifndef __NODEDATA_H__
#define __NODEDATA_H__
#include "modstyhead.h"
#include "buffer.h"

//mac检索
int allocatemac();

//读取帐号信息
int readaccount();

//读取自动场景信息
int readautosce();

//读取手动场景信息
int readmansce();

//读取区域关联信息
int readareainfo();

//读取节点信息
int readnode();

//检索节点信息
void searchnode(int slaveident, byte *node);

//节点状态更新
void nodeupdate(int slaveident, byte *node);

//检查负载&传感器更新
void checkstatuschange(int type, byte *status, byte *change);

//节点状态反馈
void statefeedback(int slaveident, byte *node);

//功能键长按
int funkeypress(byte *key);

//功能键短按
int funkeyshort(byte *key);

//触摸功能
void touchfunction(int slaveident, byte *touch, byte nodetype);

//节点数据反馈
void datafeedback(int slaveident, byte *node);

//从机单片机指令上传
void slavesinglechipup(byte *buf);

//单片机指令上传
void singlechipup(int slaveident, byte *buf);

//智能控制面板应答
void smartctrpanelanswer(int slave, byte *buf);


#endif //__NODEDATA_H__
