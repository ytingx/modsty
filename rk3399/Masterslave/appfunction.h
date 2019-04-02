#ifndef __APPFUNCTION_H__
#define __APPFUNCTION_H__
#include "modstyhead.h"

/* 
    功能实现 
               */

//帐号检索
int accountcheck(int *authority, byte *account);

//修改密码 0x11
void changepassword(byte *buf);

//绑定主机0x12
void bindhost(byte *buf);

//绑定主机强制0x13

//解绑主机0x14
void unbindhost(byte *buf);

//添加子帐号 0x15
void addaubaccount(byte *buf);

//删除子帐号 0x16
void deletesubaccount(byte *buf);

//获取子帐号信息 0x17
void getsubaccount(byte *buf);

//新增场景配置
void newmanualsce(int id, byte *buf);

//修改场景配置
void modifymanualsce(int sub, byte *buf);

//手动场景配置 0x50
void manualscenario(byte *buf);

//手动场景操作 0x62
void manualsceope(byte *buf);

//删除手动场景 0x52
 void delmanualscenario(byte *buf);

//修改自动场景配置
void automodifymanualsce(int sub, byte *buf);

//新增自动场景配置
void newautomanualsce(int id, byte *buf);

//自动场景配置 0x53
void autoscene(byte *buf);

//自动场景使能0x61
void autoscene_enable(byte *buf);

//删除自动场景 0x54
 void delautoscene(byte *buf);

//App开关本地 操作 单指令控制 0x60  0x63
void appsw(byte *buf);

//传感器使能 低6位(0~7)依次为：过零检测，人体感应，光检测 ,小夜灯，地震检测，温度,手势，蜂鸣器
void sensorenabled(byte *buf);

//本地按键关联，一个按键最多关联1项
void localassociation(byte *buf);

//同步节点 0xa1
void synchronizenode();

//同步红外设备信息
void synchronizeirdev();

//同步区域0xa1
void synchronizereg();

//同步手动场景
void synchronizemansce();

//同步自动场景
void synchronizeautosce();

//同步红外快捷方式
void synchronizeirfast();

//信息同步 0x31
void node_up(byte *buf);

//修改区域配置
void modifyregation(int sub, byte *buf);

//新增区域配置
void newregassociation(int id, byte *buf);

//区域功能配置(最多64个) 0x55  0x64
void regassociation(byte *buf);

//app区域操作 0x63 0x65
 void regoperation(byte *buf);

//app删除区域 0x57  0x66
 void appdelarea(byte *buf);

//节点状态查询 0x18
 void nodequery(byte *buf, char *FileName);

//设置开关光感阀门值 0x22
 void lightvalve(byte *buf);

//获取软件/硬件版本 0x4a
 void obtainshvers(byte *buf);

//获取主从机网络信息0x27
void getnetworkinfo(byte *buf);

//删除手势场景 0x28
void gesturescenedel(byte *buf);

//app灯具控制 0x29
 void lightingcontrol(byte *buf);

//app节点注册信息上传请求 0x2f 0x30
 void appnodeupask(byte *buf);

//设置时间
 int set_system_time(struct tm ptime);

//app同步网关时间 0x31,0x32
 void appsettime(byte *buf);

//主（从）机重启
 void appreboot(byte *buf);

//手动场景重命名0x53,0x54,0x55
void remanualscenario(byte *buf);

//设备命名0x52
void devicenaming(byte *buf);

//APP添加设置网下开关区域设置(下发命令) 0x58
void netassociation(byte *buf);

//APP删除网下开关区域设置(下发命令) 0x70
void delnetassociation(byte *buf);

//APP上组网/退出组网按键 0x61
void networking(byte *buf);

//APP上“设备全开”和“设备全关”功能(主机下发命令) 0x63
void groupcontrol(byte *buf);

//APP上设置开关温度误差 0x68
void tempererror(byte *buf);

//背光灯调光 0x11
void backlight(byte *buf);

//APP上设置网下区域关联手动场景(APP下发命令) 0x64
void appscene(byte *buf);

//App删除网下区域关联手动场景 0x72
void delappscene(byte *buf);

//APP自动场景的状态作为另外一个自动场景触发条件设置0x66
void autotrigger(byte *buf);

//删除 APP自动场景的状态作为另外一个自动场景触发条件设置 0x73
void delautotri(byte *buf);

//app删除开关 0x6a
void deletenode(byte *buf);

//检索删除红外设备
void delnodeirdev(byte *nodemac);

//检索删除红外快捷方式
void delnodeirfast(byte *nodemac);

//删除节点
void delnodedev(byte *buf);

//完整红外码发送 0x58
void completeirsend(byte *buf);

//配置红外设备 0x59
void configirdev(byte *buf);

//删除红外设备 0x5a
void delirdev(byte *buf);

//红外-一键匹配 0x5b
void irmatch(byte *buf);

//红外-智能学习 0x5c
void irlntelearn(byte *buf);

//添加红外快捷操作 0x5d
void irquickopera(byte *buf);

//删除红外快捷操作 0x5e
void delirquick(byte *buf);

//执行红外快捷操作 0x65
void runirquick(byte *buf);

#endif /* __APPFUNCTION_H__ */
