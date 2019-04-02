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

//#define NDEBUG
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
#define SHAKE "/mnt/mtd/shake"
//门磁
#define DOORMAGNET "/mnt/mtd/doormagnet"
//水浸
#define FLOODING "/mnt/mtd/flooding"
//红外活动侦测
#define IRACTIVITY "/mnt/mtd/iractivity"
//雨雪
#define RAINSNOW "/mnt/mtd/rainsnow"
//燃气
#define GASINFO "/mnt/mtd/gasinfo"
//风速
#define WINDSPEED "/mnt/mtd/windspeed"
//一氧化碳
#define CARMOXDE "/mnt/mtd/carmoxde"
//烟雾
#define SMOKERELATED "/mnt/mtd/smokerelated"
//本地区域关联
#define RELEVANCE "/mnt/mtd/relevance"
//智能锁
#define SMARTLOCK "/mnt/mtd/smartlock"
//智能锁关联
#define LOCKRELATED "/mnt/mtd/lockrelated"
//本地区域关联交换
#define RELESWAP "/mnt/mtd/releswap"
//节点注册
#define NODEID "/mnt/mtd/nodeid"
//节点原始信息
#define ORIGNODE "/mnt/mtd/orinode"
//手势识别
#define GESTURE "/mnt/mtd/gesture"
//综合数据
#define SYNTHESIZE "/mnt/mtd/synthesize"
//温度
#define TEMPERATURE "/mnt/mtd/temperature"
//手动场景
#define MANUALSCE "/mnt/mtd/manualsce"
//自动场景
#define AUTOSCE "/mnt/mtd/autosce"
//自动场景触发条件监测
#define MONITOR "/mnt/mtd/monitor"
//自动场景使能
#define AUTOENABLED "/mnt/mtd/autoenab"
//安防监控
#define SECMONITOR "/mnt/mtd/secmonitor"
//智能控制面板
#define SMARTCRLPANEL "/mnt/mtd/smartcrlpanel"
//传感器使能
#define SENSONENAB "/mnt/mtd/sensonenab"
//本地按键关联(APP)
#define LOCALASS "/mnt/mtd/localass"
//区域按键关联(APP)
#define REGASSOC "/mnt/mtd/regassoc"
//传感器阀值
#define THRESHOLD "/mnt/mtd/threshold"
//网关出厂帐号密码
#define INITIALPW "/mnt/mtd/initialpw"
//帐号密码
#define ACCOUNTPW "/mnt/mtd/accountpw"
//Mesh name, Mesh password
#define MESHPW "/mnt/mtd/meshpw"
//Wifi PW
#define WIFIPW "/mnt/mtd/wifipw"
//开关命名
#define DEVNAME "/mnt/mtd/devname"
//ip,net,gw,dns
#define IPADDR  "/mnt/mtd/ipaddr.sh"
//记录主机状态
#define HOSTSTATUS "/mnt/mtd/hoststatus"
//主从机信息
#define MASSLAINFO "/mnt/mtd/masslainfo"
//区域关联手动场景
#define APPSCENE "/mnt/mtd/appscene"
//APP自动场景的状态作为另外一个自动场景触发
#define AUTOTRIGGER "/mnt/mtd/autotrigger"
//红外设备
#define IRDEV "/mnt/mtd/irdev"
//红外快捷方式
#define IRFAST "/mnt/mtd/irfast"
//保存自动场景状态
#define AUTOSTA "/mnt/mtd/autosta"

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
