#ifndef __STRUCT_H__
#define __STRUCT_H__

//新节点地址
byte newnodemac[300];

//主机信息
struct hostinformation
{
    //绑定状态
    int bindhost;
    //IP获取方式
    int dhcp;
    //工作模式/主从机标识
    int mode;
    //网络工作方式-sta/ap
    int apsta;
    //自动场景重置标识
    int autoscereset;
    //主机数据版本号
    byte dataversion;
    //wifi重启状态
    int wifirestart;
    //wifi开关
    int wifisw;
    //组网状态
    int mesh;
    //mesh网关版本
    int meshversion;
    //使能
    int enable;
    //报警状态
    int alarm;
    //开启添加从机标识
    int receslave;
    //从机组网标识-1-2-3-4
    int slavemesh;
    //主机与MESH网关通信状态
    int meshuart;
    //主机运行时间
    time_t runtime;
    //主机空闲时间
    time_t freetime;
    //主机温湿度
    int temperature;
    int moderate;
    //wifi mac
    byte wifimac[10];
    //wan mac
    byte wanmac[10];
    //host sn
    byte hostsn[10];
    //slave sn
    byte slavesn[10];
    //登录服务器标识
    byte license[10];
};
struct hostinformation hostinfo;

//记录从机标识
struct slaveidentification
{
    int sockfd;
    int assignid;
    //温湿度
    int temperature;
    int moderate;
    //mesh网关版本
    int meshversion;
    //使能
    int enable;
    //软硬件版本
    byte version[10];
    //从机sn
    byte slavesn[10];
    //主机sn
    byte mastersn[10];
    //从机心跳时间
    time_t heartbeattime;
};
struct slaveidentification slaveident[10];

//应答场景、区域
struct scenearea
{
    //开始记录标识
    int starlog;
    //mac个数
    int maclen;
    //指令头
    byte headbuf[620];
    //最后一条配置
    byte endbuf[100];
    //未成功配置mac
    byte badmac[600];
};
//区域应答
struct scenearea sceneack;

//存储临时信息
struct regionalsave
{
    //指令总长度
    int len;
    //发送条数
    int count;
    //触发条件长度
    int tricondlen;
    //触发条件
    byte tricond[200];
    //执行任务长度
    byte tasklen[5];
    //配置-执行任务
    byte regassoc[1250];
    //主从机红外执行任务长度
    int irlen;
    //主从机红外执行任务
    byte irtask[256];
    //自动场景执行手动场景长度
    int manlen;
    //手动场景
    byte manualsce[128];
    //手动场景-执行安防长度
    int seclen;
    //手动场景-执行安防
    byte security[24];
    //自动场景-跨主机场景长度
    int crosshostlen;
    //自动场景-跨主机场景
    byte crosshostsce[64];
    //头
    byte reghead[1300];
    //名称
    byte regname[20];
};
struct regionalsave regionaltemp;

struct nodedata
{
    byte nodeinfo[15];
};
struct nodedata nodedev[260];

//节点原始数据
struct orignode
{
    byte originfo[10];
};
struct orignode orinode[260];

//区域关联信息
struct regionalcontrol
{
    //id
    int id;
    //绑定按键mac
    byte bingkeymac[5];
    //绑定按键id
    int bingkeyid;
    //执行任务长度
    int tasklen;
    //执行任务
    byte regtask[1250];
    //区域名称长度
    int namelen;
    //区域名称
    byte regname[20];
    //区域图标
    int iconid;
};
struct regionalcontrol regcontrol[25];

//本地关联信息
struct localass
{
    //关联信息
    byte bingkey[10];
};
struct localass localasoction[60];

//手动场景信息
struct manscecontrol
{
    //id
    int id;
    //绑定按键mac
    byte bingkeymac[5];
    //绑定按键id
    int bingkeyid;
    //执行任务长度
    int tasklen;
    //执行任务
    byte regtask[1250];
    //区域名称长度
    int namelen;
    //区域名称
    byte regname[20];
    //区域图标
    int iconid;
};
struct manscecontrol mscecontrol[25];

//自动场景信息
struct autoscecontrol
{
    //id
    int id;
    //使能
    int enable;
    //当前执行状态
    int status;
    //冷却时间
    int interval;
    //触发模式
    int trimode;
    //限制条件
    byte limitations[8];
    //触发条件长度
    int tricondlen;
    //触发条件
    byte tricond[200];
    //执行任务长度
    int tasklen;
    //执行任务
    byte regtask[1250];
    //区域名称长度
    int namelen;
    //区域名称
    byte regname[20];
    //区域图标
    int iconid;
};
struct autoscecontrol ascecontrol[15];

//自动场景触发条件
struct autoaction
{
    //活动侦测
    byte activity[10];
    //负载
    byte load[10];
    //门磁
    byte magnetic[10];
};

//自动场景触发条件-对应场景id
struct autoscetri
{
    struct autoaction autotion[15];
    int id;
    int ldentification;
    time_t lasttime;
};
struct autoscetri autotri[15];

//安防监控信息
struct securitymonitor
{
    //id
    int id;
    //使能
    int enable;
    //当前布防状态
    int secmonitor;
    //当前执行状态
    int status;
    //启动布防时间段
    byte armingtime[12];
    //触发模式
    int trimode;
    //智能布防启动条件长度
    int secstartlen;
    //智能布防启动条件
    byte secstartcond[1024];
    //智能布防条件长度
    int tricondlen;
    //智能布防条件
    byte tricond[1024];
    //智能撤防条件长度
    int disarmlen;
    //智能撤防条件
    byte disarmtask[1024];
    //报警/执行条件长度
    int secenfolen;
    //报警/执行条件
    byte secenforce[1024];
    //执行任务长度
    int tasklen;
    //执行任务
    byte regtask[128];
};
struct securitymonitor secmonitor[10];

//智能布防启动条件
struct secstartconditions
{
    //活动侦测
    byte activity[10];
    //负载
    byte load[10];
    //门磁
    byte magnetic[10];
    //门锁
    byte doorlock[10];
    //时间
    byte timer[10];
};

//智能布防触发条件
struct secmonaction
{
    //活动侦测
    byte activity[10];
    //负载
    byte load[10];
    //门磁
    byte magnetic[10];
    //门锁
    byte doorlock[10];
    //时间
    byte timer[10];
};

//智能撤防触发条件
struct secdisarmtion
{
    //活动侦测
    byte activity[10];
    //负载
    byte load[10];
    //门磁
    byte magnetic[10];
    //门锁
    byte doorlock[10];
    //时间
    byte timer[10];
};

//智能安防执行/报警条件
struct secenforcement
{
    //活动侦测
    byte activity[10];
    //负载
    byte load[10];
    //门磁
    byte magnetic[10];
    //门锁
    byte doorlock[10];
    //温度
    byte temperature[10];
    //水浸
    byte flooding[10];
    //烟雾
    byte smoke[10];
    //燃气
    byte gas[10];
    //一氧化碳
    byte carbonmxde[10];
};

//智能布防触发条件-对应id
struct securitymon
{
    struct secstartconditions secstartcond[10];
    struct secmonaction secmontion[250];
    struct secdisarmtion secdisarm[250];
    struct secenforcement secenfo[250];
    int id;
    int seccountdown;
    //int ldentification;
    //time_t lasttime;
};
struct securitymon secmontri[10];

//红外缓存
struct infraredcache
{
    //索引
    byte id[5];
    //mac
    byte mac[5];
    //len
    byte len[5];
    //来源
    int source;
    //压缩
    int compression;
    //红外码
    byte ircode[500];
    //name len
    int namelen;
    //name
    byte name[40];
};
struct infraredcache ircache;

//帐号信息
struct accountinformation
{
    //帐号长度
    int accountlen;
    //帐号
    byte accountinfo[24];
    //密码长度
    int pwlen;
    //密码
    byte password[24];
    //权限
    int authority;
};
struct accountinformation accinfo[25];

//定时器
/*
 * 0:强制绑定主机
 * 1:震动报警
 */
struct timercount
{
    //开始计数标识
    int startlogo;
    //触发标识
    int triggerlogo;
    //计数
    int counting;
    //触发节点node
    byte nodemac[24];
};
struct timercount timers[10];

//智能锁用户信息
struct smartlockinformation
{
    //mac-2
    byte mac[6];
    //开锁方式
    byte method;
    //用户权限
    byte authority;
    //透传-1
    byte penetrate;
    //用户id
    byte lockuserid;
    //挟持标志
    byte reserved;
    //保留字节-2
    byte retention[6];
    //名称长度-1
    byte namelen;
    //名称-16
    byte name[24];
};
struct smartlockinformation smartlockinfo[260];

//智能控制面板关联
struct smartcontrolpanel
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //执行任务
    byte perftask[12];
};
struct smartcontrolpanel smartcrlpanel[128];

//手势关联
struct gesturebindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
};
struct gesturebindinginfo gesturebindinfo[260];

//智能锁关联
struct smartlockbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct smartlockbindinginfo smartlockbindinfo[260];

//震动关联
struct shockbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct shockbindinginfo shockbindinfo[260];

//烟雾关联
struct smokebindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct smokebindinginfo smokebindinfo[260];

//门磁关联
struct doormagnetbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct doormagnetbindinginfo doormagnetbindinfo[260];

//水浸关联
struct floodingbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct floodingbindinginfo floodingbindinfo[260];

//红外活动侦测关联
struct iractivitybindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct iractivitybindinginfo iractivitybindinfo[260];

//雨雪关联
struct rainsnowbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct rainsnowbindinginfo rainsnowbindinfo[260];

//燃气关联
struct gasbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct gasbindinginfo gasbindinfo[260];

//风速关联
struct windspeedbindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct windspeedbindinginfo windspeedbindinfo[260];

//一氧化碳关联
struct carbonmoxdebindinginfo
{
    //mac1
    byte trimac[6];
    //id
    byte triid;
    //type
    byte type;
    //mac2
    byte carmac[6];
    //id
    byte carid;
    //节点执行状态
    byte execstatus[6];
};
struct carbonmoxdebindinginfo carmoxdebindinfo[260];

#endif //__STRUCT_H__

