#ifndef __MODSTYHEAD_H__
#define __MODSTYHEAD_H__

//公共头文件
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stddef.h>
#include <sys/un.h>
#include <math.h>
#include <termios.h>
#include <pthread.h>

//设置参数
typedef unsigned char byte;

#define NDEBUG
#ifndef NDEBUG
    #define pr_debug(format, ...) fprintf(stderr, "File: "__FILE__", Line: %05d: "format"\n", __LINE__, ##__VA_ARGS__)
#else
    #define pr_debug(format, ...)
#endif

//宏定义
#define FALSE -1
#define TRUE   0
#define NODE_MAX 300
//#define BACKLOG 1
#define BUF_SIZE 254
#define BUFFER_SIZE 512
#define SERVER_PORT 6001
#define BACKLOG 20
#define MAX_CON_NO 10
#define MAX_DATA_SIZE 4096
//线程相关
//#define BUFFSIZE 1024 * 1024
//#define BUFFSIZE 1024 * 100
//#define min(x, y) ((x) < (y) ? (x) : (y))

//按键宏
#define GPIO_SET    0xA1
#define GPIO_CLEAR  0xA2
#define GPIO_DEFPIN 0xA3

#define GPIO_KEY3   (32 + 22)  //GPIO_1[22]
#define GPIO_KEY2   (32 + 19)  //GPIO_1[19]
#define GPIO_KEY1   (32 + 20)  //GPIO_1[20]
//#define GPIO_KEY4   (32 + 22)  //GPIO_1[22]

//LED
#define GPIO_LEDPIN 0xA3
#define GPIO_LED1   17  //GPIO_0[17]
#define GPIO_LED2   16  //GPIO_0[16]
#define GPIO_LED3   15  //GPIO_0[15]
#define GPIO_LED4   13  //GPIO_0[13]
#define GPIO_LED5   10  //GPIO_0[10]
#define GPIO_LED6   11  //GPIO_0[11]
#define GPIO_LED7   14  //GPIO_0[14]

//文件名
//震动
#define SHAKE "/data/modsty/shake"
//门磁
#define DOORMAGNET "/data/modsty/doormagnet"
//水浸
#define FLOODING "/data/modsty/flooding"
//红外活动侦测
#define IRACTIVITY "/data/modsty/iractivity"
//雨雪
#define RAINSNOW "/data/modsty/rainsnow"
//燃气
#define GASINFO "/data/modsty/gasinfo"
//风速
#define WINDSPEED "/data/modsty/windspeed"
//一氧化碳
#define CARMOXDE "/data/modsty/carmoxde"
//烟雾
#define SMOKERELATED "/data/modsty/smokerelated"
//本地区域关联
#define RELEVANCE "/data/modsty/relevance"
//智能锁
#define SMARTLOCK "/data/modsty/smartlock"
//智能锁关联
#define LOCKRELATED "/data/modsty/lockrelated"
//本地区域关联交换
#define RELESWAP "/data/modsty/releswap"
//节点注册
#define NODEID "/data/modsty/nodeid"
//节点原始信息
#define ORIGNODE "/data/modsty/orinode"
//手势识别
#define GESTURE "/data/modsty/gesture"
//综合数据
#define SYNTHESIZE "/data/modsty/synthesize"
//温度
#define TEMPERATURE "/data/modsty/temperature"
//手动场景
#define MANUALSCE "/data/modsty/manualsce"
//自动场景
#define AUTOSCE "/data/modsty/autosce"
//自动场景触发条件监测
#define MONITOR "/data/modsty/monitor"
//自动场景使能
#define AUTOENABLED "/data/modsty/autoenab"
//安防监控
#define SECMONITOR "/data/modsty/secmonitor"
//智能控制面板
#define SMARTCRLPANEL "/data/modsty/smartcrlpanel"
//传感器使能
#define SENSONENAB "/data/modsty/sensonenab"
//本地按键关联(APP)
#define LOCALASS "/data/modsty/localass"
//区域按键关联(APP)
#define REGASSOC "/data/modsty/regassoc"
//传感器阀值
#define THRESHOLD "/data/modsty/threshold"
//网关出厂帐号密码
#define INITIALPW "/data/modsty/initialpw"
//帐号密码
#define ACCOUNTPW "/data/modsty/accountpw"
//Mesh name, Mesh password
#define MESHPW "/data/modsty/meshpw"
//Wifi PW
#define WIFIPW "/data/modsty/wifipw"
//开关命名
#define DEVNAME "/data/modsty/devname"
//ip,net,gw,dns
#define IPADDR  "/data/modsty/ipaddr.sh"
//记录主机状态
#define HOSTSTATUS "/data/modsty/hoststatus"
//主从机信息
#define MASSLAINFO "/data/modsty/masslainfo"
//区域关联手动场景
#define APPSCENE "/data/modsty/appscene"
//APP自动场景的状态作为另外一个自动场景触发
#define AUTOTRIGGER "/data/modsty/autotrigger"
//红外设备
#define IRDEV "/data/modsty/irdev"
//红外快捷方式
#define IRFAST "/data/modsty/irfast"
//保存自动场景状态
#define AUTOSTA "/data/modsty/autosta"

#define stringSize(string) (sizeof(string) * sizeof(char))
//服务器侦听控制连接请求的端口
#define CMD_PORT 6001
//客户端侦听数据连接请求的端口
#define DATA_PORT 5020
#define ACK "ack?"

//各指令长度
//继电器，小夜灯，背光灯
#define SWITCH 5
//灯具:RGB
#define RGB    7

//led结构标识
struct countid
{
    int id;
    int count;
    int bout;
};
struct countid ctid[10];

//传感器数据
struct sensordata
{
    //综合数据
    byte synthesize[5];
    //温度
    byte temperature[5];
    //震动
    byte shake[5];
    //光感
    byte light[5];
};
struct sensordata sensdata[256];
#endif //__MODSTYHEAD_H__
