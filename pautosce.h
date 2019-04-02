#ifndef __PAUTOSCE_H__
#define __PAUTOSCE_H__
#include "modstyhead.h"

//传感器信息存储
/*
 * type:1,综合数据
 *      2,温度
 *      3,震动
 *      4,光感
 */
//返回值:0成功，-1失败
//int seninfostorage(int type, byte *sensor);

//传感器信息查询
/*
 * type:1,综合数据
 *      2,温度
 *      3,震动
 *      4,光感
 */
//返回值:0成功，-1失败
//int seninfoinquire(int type, byte *sensor, byte *psensor);

//读取文件内容
int readtriggercond();

//检测网络连接
//static int networkcheck();

//定时器
void *timer_run(void *a);

#endif //__AUTOSCE_H__
