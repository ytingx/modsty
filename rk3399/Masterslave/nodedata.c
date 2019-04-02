#include "nodedata.h"
#include "struct.h"

//static int lock_i;
//static int lock_g;

extern int uart_fd;
extern int g_led_on;
extern int g_led;
extern int single_fd;
//extern int g_bindhost;
extern int master;
extern int new_fd;
extern int client_fd;
extern int answer;
extern struct cycle_buffer *fifo;
extern byte ackswer[15];
extern byte newnodemac[300];

int allocatemac()
{
    int i = 2;
    int g = 0;
    int mac = 1;
    for(i = 2; i < 260; i++)
    {
        /*
        for(g = 0; g < 8; g++)
        {
            pr_debug("nodedev[%d].nodeinfo[%d]:%0x\n", i, g, nodedev[i].nodeinfo[g]);
        }
        */
        if(nodedev[i].nodeinfo[2] == 0)
        {
            //保留mac
            if(nodedev[i].nodeinfo[1] > 0)
            {
                return nodedev[i].nodeinfo[1];
            }
            //空闲
            else if(nodedev[i].nodeinfo[1] == 0)
            {
                break;
            }
        }
        else
        {
            mac++;
        }
    }
    return mac + 1;
}

//生成主机登录服务器许可
void loginpermission()
{
    int i = 4;
    byte licestr[32] = {0};
    byte licehex[20] = {0x6c,0x69,0x63,0x65};

    srand((unsigned)time(NULL));
    for(i = 4; i < 12; i++)
    {
        licehex[i] = rand() % 51 + 50;
    }
    HexToStr(licestr, licehex, 12);
    memcpy(hostinfo.license, licehex + 4, 8);
    replace(HOSTSTATUS, licestr, 8);
}

//读取主从机信息
void readmasterslave()
{
    FILE *fd = NULL;
    byte mslavestr[64] = {0};
    byte mslavehex[32] = {0};
    
    if((fd = fopen(MASSLAINFO, "r")) == NULL)
    {
        pr_debug("%s: %s\n", MASSLAINFO, strerror(errno));
        return;
    }

    while(fgets(mslavestr, 64, fd))
    {
        if(strlen(mslavestr) > 10)
        {
            StrToHex(mslavehex, mslavestr, strlen(mslavestr) / 2);
            //从机编号
            slaveident[mslavehex[6]].assignid = mslavehex[6];
            pr_debug("slaveident[%d].assignid:%d\n", mslavehex[6], slaveident[mslavehex[6]].assignid);
            //从机sn
            memcpy(slaveident[mslavehex[6]].slavesn, mslavehex, 7);
            memcpy(hostinfo.slavesn, mslavehex, 7);
            
            //主机sn
            memcpy(slaveident[mslavehex[6]].mastersn, mslavehex + 7, 7);
            memcpy(hostinfo.hostsn, mslavehex + 7, 7);

            //使能信息
            slaveident[mslavehex[6]].enable = 0xff;
        }
    }
    fclose(fd);
    fd = NULL;
}

//读取主从机配置信息
int readconfiginfo()
{
    byte configstr[128] = {0};
    byte licensehex[64] = {0};
    
    //主机使能
    hostinfo.enable = 0xff;
    
    //ip获取方式
    if(0 == filefind("dhcp", configstr, HOSTSTATUS, 4))
    {
        hostinfo.dhcp = configstr[4] - '0';
        if(hostinfo.dhcp == 0)
        {
            modifynetwork();
        }
    }
    else
    {
        hostinfo.dhcp = 0;
    }

    //网络工作模式
    if(0 == filefind("apsta", configstr, HOSTSTATUS, 5))
    {
        hostinfo.apsta = configstr[5] - '0';
    }
    else
    {
        hostinfo.apsta = 1;
    }

    if(hostinfo.apsta == 1)
    {
        system("route add default gw 192.168.100.1");
    }
    else
    {
        hostinfo.wifisw = 1;
    }
    //主机登录服务器许可
    if(0 == filefind("6C696365", configstr, HOSTSTATUS, 8))
    {
        StrToHex(licensehex, configstr, strlen(configstr) / 2);
        memcpy(hostinfo.license, licensehex + 4, 8);
    }
    else
    {
        loginpermission();
    }

    //绑定状态
    if(0 == filefind("bindhost", configstr, HOSTSTATUS, 8))
    {
        hostinfo.bindhost = configstr[9] - '0';
    }
    else
    {
        hostinfo.bindhost = 0;
    }
    pr_debug("hostinfo.bindhost:%d\n", hostinfo.bindhost);
    
    //mesh状态
    if(0 == filefind("mesh", configstr, HOSTSTATUS, 4))
    {
        hostinfo.mesh = configstr[4] - '0';
    }
    else
    {
        hostinfo.mesh = 2;
    }
    pr_debug("hostinfo.mesh:%d\n", hostinfo.mesh);
}

//创建恢复出厂shell文件
void createrestore()
{
    FILE *fp = NULL;
    int i = 0;
    byte restorefactory[2048] = {"#!/bin/sh\n\nrm -f /data/modsty/carmoxde\nrm -f /data/modsty/doormagnet\nrm -f /data/modsty/flooding\nrm -f /data/modsty/iractivity\nrm -f /data/modsty/rainsnow\nrm -f /data/modsty/gasinfo\nrm -f /data/modsty/windspeed\nrm -f /data/modsty/smokerelated\nrm -f /data/modsty/hoststatus\nrm -f /data/modsty/accountpw\nrm -f /data/modsty/masslainfo\nrm -f /data/modsty/meshpw\nrm -f /data/modsty/wifipw\nrm -f /data/modsty/initialpw\nrm -f /data/modsty/ipaddr.sh\nrm -f /data/modsty/nodeid\nrm -f /data/modsty/orinode\nrm -f /data/modsty/manualsce\nrm -f /data/modsty/autosce\nrm -f /data/modsty/autoenab\nrm -f /data/modsty/sensonenab\nrm -f /data/modsty/localass\nrm -f /data/modsty/regassoc\nrm -f /data/modsty/devname\nrm -f /data/modsty/acklog\nrm -f /data/modsty/led*\nrm -f /data/modsty/uart*\nrm -f /data/modsty/ir*\nrm -f /data/modsty/err*\nrm -f /data/modsty/secmonitor\nrm -f /data/modsty/smartlock\nrm -f /data/modsty/shake\nrm -f /data/modsty/smokerelated\nrm -f /data/modsty/lockrelated\nrm -f /data/modsty/gesture\nrm -f /data/modsty/smartcrlpanel\n"};
    byte systemreset[2048] = {"#!/bin/sh\n\nrm -f /data/modsty/carmoxde\nrm -f /data/modsty/doormagnet\nrm -f /data/modsty/flooding\nrm -f /data/modsty/iractivity\nrm -f /data/modsty/rainsnow\nrm -f /data/modsty/gasinfo\nrm -f /data/modsty/windspeed\nrm -f /data/modsty/smokerelated\nrm -f /data/modsty/nodeid\nrm -f /data/modsty/orinode\nrm -f /data/modsty/manualsce\nrm -f /data/modsty/autosce\nrm -f /data/modsty/autoenab\nrm -f /data/modsty/sensonenab\nrm -f /data/modsty/localass\nrm -f /data/modsty/regassoc\nrm -f /data/modsty/devname\nrm -f /data/modsty/acklog\nrm -f /data/modsty/led*\nrm -f /data/modsty/uart*\nrm -f /data/modsty/ir*\nrm -f /data/modsty/secmonitor\nrm -f /data/modsty/smartlock\nrm -f /data/modsty/shake\nrm -f /data/modsty/smokerelated\nrm -f /data/modsty/lockrelated\nrm -f /data/modsty/gesture\nrm -f /data/modsty/smartcrlpanel\n"};

    if((fp = fopen("/data/modsty/restorefactory.sh", "w")) == NULL)
    {
        pr_debug("/data/modsty/restorefactory.sh: %s\n", strerror(errno));
        exit(-1);
    }
    i = fputs(restorefactory, fp);
    pr_debug("i:%d\n", i);
    fclose(fp);
    fp = NULL;

    if((fp = fopen("/data/modsty/system_reset.sh", "w")) == NULL)
    {
        pr_debug("/data/modsty/system_reset.sh: %s\n", strerror(errno));
        exit(-1);
    }
    i = fputs(systemreset, fp);
    pr_debug("i:%d\n", i);
    fclose(fp);
    fp = NULL;
}

//读取帐号信息
int readaccount()
{
    FILE *fd = NULL;
    int i = 1;
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte nodestr[128] = {0};
    byte nodehex[64] = {0};
    byte orireg[20] = {"01\n02\n03\n04"};
    byte oriapsta[20] = {"apsta0"};
    byte oridhcp[20] = {"dhcp1"};
    byte orimode[20] = {"mode1"};
    byte orimesh[10] = {"mesh2"};
    byte oridataverhex[10] = {0};
    byte oripw[64] = {"0561646D696E0631323334353601"};

    byte initialpwstr[64] = {0};

    byte wifiapssid[128] = {"uci set wireless.ap.ssid=Modsty"};
    byte wifiapssidstr[32] = {0};

    memset(accinfo, 0, sizeof(accinfo));

    if(-1 == access("/data/modsty/hoststatus", F_OK))
    {
        pr_debug("创建hoststatus\n");
        replace(HOSTSTATUS, oridhcp, 4);
        replace(HOSTSTATUS, orimode, 4);
        replace(HOSTSTATUS, orimesh, 4);
        
        //创建恢复出厂shell文件
        createrestore();
        
        //生成主机登录服务器许可
        loginpermission();

        IRUART0_Send(single_fd, chipsend, chipsend[2]);
        
        sleep(3);
        exit(0);
    }
    if(-1 == access("/data/modsty/initialpw", F_OK))
    {
        pr_debug("创建initialpw\n");
        replace(INITIALPW, oripw, 12);
    }
    if(-1 == access("/data/modsty/regassoc", F_OK))
    {
        pr_debug("创建regassoc\n");
        replace(REGASSOC, orireg, 2);
    }

    //初始密码
    if(0 == filefind("0561646D696E", initialpwstr, INITIALPW, 12))
    {
        pr_debug("读取初始密码\n");
        StrToHex(nodehex, initialpwstr, strlen(initialpwstr) / 2);
        //帐号长度
        accinfo[0].accountlen = nodehex[0];
        //帐号
        memcpy(accinfo[0].accountinfo, nodehex + 1, nodehex[0]);
        //密码长度
        accinfo[0].pwlen = nodehex[nodehex[0] + 1];
        pr_debug("初始密码长度:%d\n", accinfo[0].pwlen);
        //密码
        memcpy(accinfo[0].password, nodehex + (nodehex[0] + 2), accinfo[0].pwlen);
        //权限
        accinfo[0].authority = nodehex[(strlen(nodestr) / 2) - 1];
    }

    if((fd = fopen(ACCOUNTPW, "r")) == NULL)
    {
        pr_debug("%s: %s\n", ACCOUNTPW, strerror(errno));
        return -1;
    }
    while(fgets(nodestr, 128, fd))
    {
        StrToHex(nodehex, nodestr, strlen(nodestr) / 2);
        //帐号长度
        accinfo[i].accountlen = nodehex[0];
        pr_debug("帐号长度:%d\n", accinfo[i].accountlen);
        //帐号
        memcpy(accinfo[i].accountinfo, nodehex + 1, nodehex[0]);
        //密码长度
        accinfo[i].pwlen = nodehex[nodehex[0] + 1];
        //密码
        memcpy(accinfo[i].password, nodehex + (nodehex[0] + 2), accinfo[i].pwlen);
        //权限
        accinfo[i].authority = nodehex[(strlen(nodestr) / 2) - 1];
        i++;
        if(i == 20)
        {
            break;
        }
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取智能锁用户信息
int readsmartlockinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(SMARTLOCK, "r")) == NULL)
    {
        pr_debug("%s: %s\n", SMARTLOCK, strerror(errno));
        return -1;
    }
    memset(&smartlockinfo, 0, sizeof(smartlockinfo));
    
    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            smartlockinfo[i].mac[0] = lockhex[offset];
            smartlockinfo[i].mac[1] = lockhex[offset + 1];
            offset += 2;
            
            smartlockinfo[i].method = lockhex[offset];
            offset += 1;

            smartlockinfo[i].authority = lockhex[offset];
            offset += 1;
            
            smartlockinfo[i].penetrate = lockhex[offset];
            offset += 1;
            
            smartlockinfo[i].lockuserid = lockhex[offset];
            offset += 1;

            smartlockinfo[i].reserved = lockhex[offset];
            offset += 1;

            smartlockinfo[i].retention[0] = lockhex[offset];
            smartlockinfo[i].retention[1] = lockhex[offset + 1];
            offset += 2;

            smartlockinfo[i].namelen = lockhex[offset];
            offset += 1;

            memcpy(smartlockinfo[i].name, lockhex + offset, 16);
            offset += 16;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
        memset(lockhex, 0, sizeof(lockhex));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取智能控制面板关联
int readsmartctlpanel()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(SMARTCRLPANEL, "r")) == NULL)
    {
        pr_debug("%s: %s\n", SMARTCRLPANEL, strerror(errno));
        return -1;
    }
    memset(&smartcrlpanel, 0, sizeof(smartcrlpanel));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            smartcrlpanel[i].trimac[0] = lockhex[offset];
            smartcrlpanel[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            smartcrlpanel[i].triid = lockhex[offset];
            offset += 1;
            //type
            smartcrlpanel[i].type = lockhex[offset];
            offset += 1;
            //执行任务
            memcpy(smartcrlpanel[i].perftask, lockhex + offset, 6);
            offset += 6;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取手势关联
int readgestureinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(GESTURE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", GESTURE, strerror(errno));
        return -1;
    }
    memset(&gesturebindinfo, 0, sizeof(gesturebindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            gesturebindinfo[i].trimac[0] = lockhex[offset];
            gesturebindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            gesturebindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            gesturebindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            gesturebindinfo[i].carmac[0] = lockhex[offset];
            gesturebindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            gesturebindinfo[i].carid = lockhex[offset];
            offset += 1;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取门磁关联
int readdoormagnetinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(DOORMAGNET, "r")) == NULL)
    {
        pr_debug("%s: %s\n", DOORMAGNET, strerror(errno));
        return -1;
    }
    memset(&doormagnetbindinfo, 0, sizeof(doormagnetbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            doormagnetbindinfo[i].trimac[0] = lockhex[offset];
            doormagnetbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            doormagnetbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            doormagnetbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            doormagnetbindinfo[i].carmac[0] = lockhex[offset];
            doormagnetbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            doormagnetbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(doormagnetbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取水浸关联
int readfloodinginfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(FLOODING, "r")) == NULL)
    {
        pr_debug("%s: %s\n", FLOODING, strerror(errno));
        return -1;
    }
    memset(&floodingbindinfo, 0, sizeof(floodingbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            floodingbindinfo[i].trimac[0] = lockhex[offset];
            floodingbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            floodingbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            floodingbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            floodingbindinfo[i].carmac[0] = lockhex[offset];
            floodingbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            floodingbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(floodingbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取红外活动侦测关联
int readiractivityinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(IRACTIVITY, "r")) == NULL)
    {
        pr_debug("%s: %s\n", IRACTIVITY, strerror(errno));
        return -1;
    }
    memset(&iractivitybindinfo, 0, sizeof(iractivitybindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            iractivitybindinfo[i].trimac[0] = lockhex[offset];
            iractivitybindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            iractivitybindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            iractivitybindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            iractivitybindinfo[i].carmac[0] = lockhex[offset];
            iractivitybindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            iractivitybindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(iractivitybindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取雨雪关联
int readrainsnowinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(RAINSNOW, "r")) == NULL)
    {
        pr_debug("%s: %s\n", RAINSNOW, strerror(errno));
        return -1;
    }
    memset(&rainsnowbindinfo, 0, sizeof(rainsnowbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            rainsnowbindinfo[i].trimac[0] = lockhex[offset];
            rainsnowbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            rainsnowbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            rainsnowbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            rainsnowbindinfo[i].carmac[0] = lockhex[offset];
            rainsnowbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            rainsnowbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(rainsnowbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取燃气关联
int readgasinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(GASINFO, "r")) == NULL)
    {
        pr_debug("%s: %s\n", GASINFO, strerror(errno));
        return -1;
    }
    memset(&gasbindinfo, 0, sizeof(gasbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            gasbindinfo[i].trimac[0] = lockhex[offset];
            gasbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            gasbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            gasbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            gasbindinfo[i].carmac[0] = lockhex[offset];
            gasbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            gasbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(gasbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取风速关联
int readwindspeed()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(WINDSPEED, "r")) == NULL)
    {
        pr_debug("%s: %s\n", WINDSPEED, strerror(errno));
        return -1;
    }
    memset(&windspeedbindinfo, 0, sizeof(windspeedbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            windspeedbindinfo[i].trimac[0] = lockhex[offset];
            windspeedbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            windspeedbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            windspeedbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            windspeedbindinfo[i].carmac[0] = lockhex[offset];
            windspeedbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            windspeedbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(windspeedbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取一氧化碳关联
int readcarmoxde()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(CARMOXDE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", CARMOXDE, strerror(errno));
        return -1;
    }
    memset(&carmoxdebindinfo, 0, sizeof(carmoxdebindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            carmoxdebindinfo[i].trimac[0] = lockhex[offset];
            carmoxdebindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            carmoxdebindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            carmoxdebindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            carmoxdebindinfo[i].carmac[0] = lockhex[offset];
            carmoxdebindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            carmoxdebindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(carmoxdebindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}


//读取烟雾关联
int readsmokeinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(SMOKERELATED, "r")) == NULL)
    {
        pr_debug("%s: %s\n", SMOKERELATED, strerror(errno));
        return -1;
    }
    memset(&smokebindinfo, 0, sizeof(smokebindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            smokebindinfo[i].trimac[0] = lockhex[offset];
            smokebindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            smokebindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            smokebindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            smokebindinfo[i].carmac[0] = lockhex[offset];
            smokebindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            smokebindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(smokebindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取震动关联
int readshakeinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(SHAKE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", SHAKE, strerror(errno));
        return -1;
    }
    memset(&shockbindinfo, 0, sizeof(shockbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            shockbindinfo[i].trimac[0] = lockhex[offset];
            shockbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            shockbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            shockbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            shockbindinfo[i].carmac[0] = lockhex[offset];
            shockbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            shockbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(shockbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取智能锁关联
int readlockinfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};
    byte lockhex[64] = {0};

    if((fd = fopen(LOCKRELATED, "r")) == NULL)
    {
        pr_debug("%s: %s\n", LOCKRELATED, strerror(errno));
        return -1;
    }
    memset(&smartlockbindinfo, 0, sizeof(smartlockbindinfo));

    while(fgets(areastr, 128, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(lockhex, areastr, strlen(areastr) / 2);

            //mac1
            smartlockbindinfo[i].trimac[0] = lockhex[offset];
            smartlockbindinfo[i].trimac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            smartlockbindinfo[i].triid = lockhex[offset];
            offset += 1;
            //type
            smartlockbindinfo[i].type = lockhex[offset];
            offset += 1;
            //mac2/场景id/区域id
            smartlockbindinfo[i].carmac[0] = lockhex[offset];
            smartlockbindinfo[i].carmac[1] = lockhex[offset + 1];
            offset += 2;
            //id
            smartlockbindinfo[i].carid = lockhex[offset];
            offset += 1;
            //节点执行状态
            memcpy(smartlockbindinfo[i].execstatus, lockhex + offset, 3);
            offset += 3;
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取自动场景信息
int readautosce()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[3000] = {0};
    byte areahex[1500] = {0};

    memset(ascecontrol, 0, sizeof(ascecontrol));
    if((fd = fopen(AUTOSCE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", AUTOSCE, strerror(errno));
        return -1;
    }
    while(fgets(areastr, 3000, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(areahex, areastr, strlen(areastr) / 2);
            i = areahex[offset];
            //id
            ascecontrol[i].id = areahex[offset];
            offset += 1;
            //pr_debug("autosceid:%0x\n", ascecontrol[i].id);
            //使能
            ascecontrol[i].enable = areahex[offset];
            offset += 1;
            //当前状态
            ascecontrol[i].status = areahex[offset];
            offset += 1;
            //冷却时间
            ascecontrol[i].interval = (areahex[offset] << 8) + areahex[offset + 1];
            offset += 2;
            //预留6字节
            offset += 6;
            //触发模式
            ascecontrol[i].trimode = areahex[offset];
            offset += 1;
            //限制条件
            memcpy(ascecontrol[i].limitations, areahex + offset, 5);
            offset += 5;
            //触发条件长度
            ascecontrol[i].tricondlen = areahex[offset];
            offset += 1;
            //触发条件
            memcpy(ascecontrol[i].tricond, areahex + offset, ascecontrol[i].tricondlen);
            offset += ascecontrol[i].tricondlen;
            //执行任务长度
            ascecontrol[i].tasklen = (areahex[offset] << 8) + areahex[offset + 1];
            offset += 2;
            //pr_debug("ascecontrol[%d].tasklen:%d\n", i, ascecontrol[i].tasklen);
            //执行任务数据
            memcpy(ascecontrol[i].regtask, areahex + offset, ascecontrol[i].tasklen);
            offset += ascecontrol[i].tasklen;
            //场景名称长度
            ascecontrol[i].namelen = areahex[offset];
            offset += 1;
            //手动场景名称
            memcpy(ascecontrol[i].regname, areahex + offset, 16);
            offset += 16;
            //手动场景图标id
            ascecontrol[i].iconid = areahex[offset];
            //pr_debug("areahex[%d]:%0x\n", ((strlen(areastr) / 2) - 1), areahex[(strlen(areastr) / 2) - 1]);
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取手动场景信息
int readmansce()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[3000] = {0};
    byte areahex[1500] = {0};

    memset(mscecontrol, 0, sizeof(mscecontrol));
    if((fd = fopen(MANUALSCE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", MANUALSCE, strerror(errno));
        return -1;
    }
    while(fgets(areastr, 3000, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(areahex, areastr, strlen(areastr) / 2);
            i = areahex[offset];
            //id
            mscecontrol[i].id = areahex[offset];
            offset += 1;
            //绑定按键mac
            mscecontrol[i].bingkeymac[0] = areahex[offset];
            mscecontrol[i].bingkeymac[1] = areahex[offset + 1];
            offset += 2;
            //绑定按键id
            mscecontrol[i].bingkeyid = areahex[offset];
            offset += 1;
            //执行任务长度
            mscecontrol[i].tasklen = (areahex[offset] << 8) + areahex[offset + 1];
            offset += 2;
            //执行任务数据
            memcpy(mscecontrol[i].regtask, areahex + offset, mscecontrol[i].tasklen);
            offset += mscecontrol[i].tasklen;
            //手动场景名称长度
            mscecontrol[i].namelen = areahex[offset];
            offset += 1;
            //手动场景名称
            memcpy(mscecontrol[i].regname, areahex + offset, 16);
            offset += 16;
            //手动场景图标id
            mscecontrol[i].iconid = areahex[offset];
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取本地关联
int readlocalass()
{
    FILE *fd = NULL;
    int i = 0;
    byte areastr[20] = {0};

    memset(localasoction, 0, sizeof(localasoction));
    if((fd = fopen(LOCALASS, "r")) == NULL)
    {
        pr_debug("%s: %s\n", LOCALASS, strerror(errno));
        return -1;
    }
    while(fgets(areastr, 20, fd))
    {
        if(strlen(areastr) > 10)
        {
            StrToHex(localasoction[i].bingkey, areastr, strlen(areastr) / 2);
            i++;
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取区域关联信息
int readareainfo()
{
    FILE *fd = NULL;
    int i = 0;
    int offset = 0;
    byte areastr[3000] = {0};
    byte areahex[1500] = {0};

    memset(regcontrol, 0, sizeof(regcontrol));
    if((fd = fopen(REGASSOC, "r")) == NULL)
    {
        pr_debug("%s: %s\n", REGASSOC, strerror(errno));
        return -1;
    }
    while(fgets(areastr, 3000, fd))
    {
        if(strlen(areastr) > 10)
        {
            offset = 0;
            StrToHex(areahex, areastr, strlen(areastr) / 2);
            i = areahex[offset];
            //id
            regcontrol[i].id = areahex[offset];
            offset += 1;
            //绑定按键mac
            regcontrol[i].bingkeymac[0] = areahex[offset];
            regcontrol[i].bingkeymac[1] = areahex[offset + 1];
            offset += 2;
            //绑定按键id
            regcontrol[i].bingkeyid = areahex[offset];
            offset += 1;
            //执行任务长度
            regcontrol[i].tasklen = (areahex[offset] << 8) + areahex[offset + 1];
            offset += 2;
            //执行任务数据
            memcpy(regcontrol[i].regtask, areahex + offset, regcontrol[i].tasklen);
            offset += regcontrol[i].tasklen;
            //区域名称长度
            regcontrol[i].namelen = areahex[offset];
            offset += 1;
            //区域名称
            memcpy(regcontrol[i].regname, areahex + offset, 16);
            offset += 16;
            //区域图标id
            regcontrol[i].iconid = areahex[offset];
            pr_debug("readareainfo:%d\n", i);
        }
        memset(areastr, 0, sizeof(areastr));
    }
    fclose(fd);
    fd = NULL;
    return i;
}

//读取节点
int readnode()
{
    FILE *fd = NULL;
    byte nodehex[15] = {0};
    byte nodestr[30] = {0};

    int i = 0;
    memset(nodedev, 0, sizeof(nodedev));
    if((fd = fopen(NODEID, "r")) == NULL)
    {
        pr_debug("%s: %s\n", NODEID, strerror(errno));
        return -1;
    }
    while(fgets(nodestr, 30, fd))
    {
        StrToHex(nodehex, nodestr, strlen(nodestr) / 2);
        //pr_debug("readnode:%s\n", nodestr);
        memcpy(nodedev[nodehex[1]].nodeinfo, nodehex, strlen(nodestr) / 2);
        //初始状态为离线
        nodedev[nodehex[1]].nodeinfo[11] = 0;
        /*
        for(i = 0; i < 12; i++)
        {
            pr_debug("nodedev[%d].nodeinfo[%d]:%0x\n", nodehex[1], i, nodedev[nodehex[1]].nodeinfo[i]);
        }
        */
        memset(nodestr, 0, sizeof(nodestr));
        memset(nodehex, 0, sizeof(nodehex));
    }
    fclose(fd);
    fd = NULL;
    return 0;
}

/*
//获取指令节点状态
int getnodestate(byte *nodemac, byte *acknode)
{
    int i = 0;
    for(i = 0; i < 300; i++)
    {
        //mac
        if(nodedev[i].nodeinfo[1] == 0)
        {
            pr_debug("getnodestate-1:%d\n", i);
            return 1;
        }
        else if(0 == memcmp(nodedev[i].nodeinfo, nodemac + 1, 2))
        {
            if(nodedev[i].nodeinfo[2] == 0)
            {
                pr_debug("节点信息同步-保留mac\n");
                return 2;
            }
            else if(nodedev[i].nodeinfo[2] == 0xff)
            {
                pr_debug("节点信息同步-保留mac\n");
                return 3;
            }
            else if(nodedev[i].nodeinfo[3] != 0)
            {
                memcpy(acknode, nodedev[i].nodeinfo, 12);
                return 0;
            }
        }
    }
    return -1;
}
*/

//传感器故障/温度报警
void sensorproblem(byte *node)
{
    byte appsend[20] = {0xf0,0x00,0x08};
    //温度
    if(node[3] == 0x06)
    {
        //温度传感器故障
        if(node[4] == 0xff)
        {
            memcpy(appsend + 3, node, 3);
            appsend[6] = 0x10;
            appsend[7] = node[3];
            UartToApp_Send(appsend, appsend[2]);
        }
        else
        {
            if(secmonitor[4].enable == 1)
            {
                //温度报警
                checkalarmcond(4, 6, node);
            }
        }
    }
    //活动侦测故障
    else if(node[3] == 0x07)
    {
        memcpy(appsend + 3, node, 3);
        appsend[6] = 0x10;
        appsend[7] = node[3];
        UartToApp_Send(appsend, appsend[2]);
    }
    //光感
    else if(node[3] == 0x08)
    {
        memcpy(appsend + 3, node, 3);
        appsend[6] = 0x10;
        appsend[7] = node[3];
        UartToApp_Send(appsend, appsend[2]);
    }
}

//记录新节点地址
void reconewnodemac(byte mac)
{
    int i = 0;
    for(i = 0; i < 260; i++)
    {
        if(newnodemac[i] == 0)
        {
            newnodemac[i] = mac;
            break;
        }
        else if(newnodemac[i] == mac)
        {
            break;
        }
    }
    pr_debug("reconewnodemac[%d]:%0x\n", i, newnodemac[i]);
}

//节点注册检查
void searchnode(int slave, byte *node)
{
    byte sensorpolice[10] = {0};
    byte nodestr[30] = {0};
    byte nodedata[20] = {0xb0,0x00,0x0f};
    byte stateapp[20] = {0xf0,0x00,0x08};
    //主机标识
    nodedev[node[2]].nodeinfo[0] = slave;

    if(node[2] == 0x01)
    {
        //网关上报版本号
        if(slave == 1)
        {
            //主机
            hostinfo.meshversion = node[4];
        }
        else
        {
            //从机
            slaveident[slave].meshversion = node[4];
        }
        return;
    }
    else if(nodedev[node[2]].nodeinfo[1] == 0)
    {
        pr_debug("hostinfo.mesh:%d\n", hostinfo.mesh);
        if((hostinfo.mesh == 1) || (hostinfo.mesh == node[2]))
        {
            //hostinfo.mesh = 1;
            memcpy(nodedev[node[2]].nodeinfo + 1, node + 2, 10);
            HexToStr(nodestr, nodedev[node[2]].nodeinfo, 11);
            replace(NODEID, nodestr, 4);

            if(node[3] == 0)
            {
                pr_debug("MAC地址复用调用2\n");
                return;
            }
            else
            {
                nodedev[node[2]].nodeinfo[11] = 0x01;
                //上报app
                memcpy(nodedata + 3, nodedev[node[2]].nodeinfo, 12);
                UartToApp_Send(nodedata, 15);
            }
        }
        else
        {
            //未知节点上报
            pr_debug("未知节点上报\n");
            stateapp[3] = slave;
            stateapp[4] = node[2];
            stateapp[5] = 0xff;
            UartToApp_Send(stateapp, stateapp[2]);
            return;
        }
    }
    else if(node[2] == nodedev[node[2]].nodeinfo[1])
    {
        memcpy(nodedev[node[2]].nodeinfo + 1, node + 2, 10);
        HexToStr(nodestr, nodedev[node[2]].nodeinfo, 11);
        replace(NODEID, nodestr, 4);

        if((node[3] == 0) || (node[3] == 0xff))
        {
            pr_debug("MAC地址复用调用1\n");
            return;
        }
        else
        {
            //传感器故障/温度报警
            if((node[3] > 0x00) && (node[3] < 0x0f))
            {
                //检查光感
                if((node[7] & 0xf0) == 0xf0)
                {
                    sensorpolice[0] = slave;
                    sensorpolice[1] = node[2];

                    //node type
                    sensorpolice[2] = node[3];
                    //data type
                    sensorpolice[3] = 0x08;

                    sensorproblem(sensorpolice);
                }
                //检查温度
                //mac
                sensorpolice[0] = slave;
                sensorpolice[1] = node[2];
                //node type
                sensorpolice[2] = node[3];
                //data type
                sensorpolice[3] = 0x06;
                //value
                sensorpolice[4] = node[8];
                sensorproblem(sensorpolice);
            }

            nodedev[node[2]].nodeinfo[11] = 0x01;
            //上报app
            memcpy(nodedata + 3, nodedev[node[2]].nodeinfo, 12);
            UartToApp_Send(nodedata, 15);
        }
    }
    else
    {
        pr_debug("searchnode-else\n");
        int i = 0;
        for(i = 0; i < 12; i++)
        {
            pr_debug("nodedev[%d].nodeinfo[i]:%0x\n", node[2], nodedev[node[2]].nodeinfo[i]);
        }
        return;
    }
    if(hostinfo.mesh == 1)
    {
        if((node[3] > 0x00) && (node[3] < 0x0f))
        {
            reconewnodemac(node[2]);
        }
    }
}

//手势执行节点
void gestureexecnode(int num, byte *node)
{
    byte uartsend[24] = {0};
    byte nodestatus = 0;
    //执行节点类型
    if((nodedev[node[2]].nodeinfo[2] > 0) && (nodedev[node[2]].nodeinfo[2] < 0x0f))
    {
        uartsend[0] = 0x63;
        uartsend[1] = 0x0e;
        //mac
        uartsend[2] = gesturebindinfo[num].carmac[1];
        //data type
        uartsend[3] = 0x01;
        //id
        uartsend[4] = gesturebindinfo[num].carid;
        
        //开关
        if(node[5] == 0x0c)
        {
            pr_debug("gesture-left\n");
            
            //value
            if(uartsend[4] == 5)
            {
                uartsend[5] = 0xa8;
            }
            else if(uartsend[4] == 6)
            {
                uartsend[5] = 0xa2;
            }
            else if(uartsend[4] == 7)
            {
                uartsend[5] = 0x8a;
            }
            else if(uartsend[4] == 8)
            {
                uartsend[5] = 0x2a;
            }
        }
        else if(node[5] == 0x0d)
        {
            pr_debug("gesture-right\n");
            
            //value
            if(uartsend[4] == 5)
            {
                uartsend[5] = 0xa9;
            }
            else if(uartsend[4] == 6)
            {
                uartsend[5] = 0xa6;
            }
            else if(uartsend[4] == 7)
            {
                uartsend[5] = 0x9a;
            }
            else if(uartsend[4] == 8)
            {
                uartsend[5] = 0x6a;
            }
        }
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }
    else if(nodedev[node[2]].nodeinfo[2] == 0x31)
    {
        uartsend[0] = 0x86;
        uartsend[1] = 0x08;
        //mac
        uartsend[2] = gesturebindinfo[num].carmac[1];
        
        //灯具
        if(node[5] == 0x0a)
        {
            pr_debug("gesture-up\n");
            //data type
            uartsend[3] = 0x07;
            uartsend[4] = 0x01;
        }
        else if(node[5] == 0x0b)
        {
            pr_debug("gesture-down\n");
            //data type
            uartsend[3] = 0x07;
            uartsend[4] = 0x00;
        }
        else if(node[5] == 0x0c)
        {
            pr_debug("gesture-left\n");
            //data type
            uartsend[3] = 0x06;
            uartsend[4] = 0x00;
        }
        else if(node[5] == 0x0d)
        {
            pr_debug("gesture-right\n");
            //data type
            uartsend[3] = 0x06;
            uartsend[4] = 0x01;
        }
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }
    else if((nodedev[node[2]].nodeinfo[2] > 0x10) && (nodedev[node[2]].nodeinfo[2] < 0x1f))
    {
        uartsend[0] = 0x63;
        uartsend[1] = 0x0e;
        //mac
        uartsend[2] = gesturebindinfo[num].carmac[1];
        //data type
        uartsend[3] = 0x01;
        //id
        uartsend[4] = gesturebindinfo[num].carid;
        
        //窗帘
        if((node[5] == 0x0a) || (node[5] == 0x0b))
        {
            pr_debug("gesture-up\n");
            pr_debug("gesture-down\n");
            
            //value
            if(uartsend[4] == 1)
            {
                uartsend[5] = 0xa8;
            }
            else if(uartsend[4] == 2)
            {
                uartsend[5] = 0xa2;
            }
        }
        else if(node[5] == 0x0c)
        {
            pr_debug("gesture-left\n");
            //value
            if(uartsend[4] == 1)
            {
                uartsend[5] = 0xab;
            }
            else if(uartsend[4] == 2)
            {
                uartsend[5] = 0xae;
            }
        }
        else if(node[5] == 0x0d)
        {
            pr_debug("gesture-right\n");
            //value
            if(uartsend[4] == 1)
            {
                uartsend[5] = 0xa9;
            }
            else if(uartsend[4] == 2)
            {
                uartsend[5] = 0xa6;
            }
        }
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }
}

//手势执行区域
void gestureexecarea(int num, byte *node)
{
    byte uartsend[24] = {0x65,0x0c};
    
    //id
    uartsend[2] = gesturebindinfo[num].carmac[0];
    //mac
    if((uartsend[2] > 0) && (uartsend[2] < 5))
    {
        uartsend[3] = 0xff;
    }
    else
    {
        uartsend[3] = regcontrol[uartsend[2]].bingkeymac[1];
    }
    //key id
    uartsend[4] = regcontrol[uartsend[2]].bingkeyid;

    //灯具
    if(node[5] == 0x0a)
    {
        pr_debug("gesture-up\n");
        //data type
        uartsend[5] = 0x07;
        uartsend[6] = 0x01;
    }
    else if(node[5] == 0x0b)
    {
        pr_debug("gesture-down\n");
        //data type
        uartsend[5] = 0x07;
        uartsend[6] = 0x00;
    }
    else if(node[5] == 0x0c)
    {
        pr_debug("gesture-left\n");
        //data type
        uartsend[5] = 0x06;
        uartsend[6] = 0x00;
    }
    else if(node[5] == 0x0d)
    {
        pr_debug("gesture-right\n");
        //data type
        uartsend[5] = 0x06;
        uartsend[6] = 0x01;
    }
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//手势执行手动场景
void gestureexecmansce(int num, byte *node)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};

    for(i = 0; i < 260; i++)
    {
        if(gesturebindinfo[i].type == 0)
        {
            pr_debug("手势关联检查完毕:%d\n", i);
            return;
        }
        else if(node[2] == gesturebindinfo[i].trimac[1])
        {
            if(node[5] == gesturebindinfo[i].triid)
            {
                appdata[3] = gesturebindinfo[i].carmac[0];

                //执行手动场景
                manualsceope(appdata);
                return;
            }
        }
    }
}

//手势功能
void gesturefunction(int slave, byte *node)
{
    byte uartsend[12] = {0x86,0x08,0xff};
    int i = 0;

    for(i = 0; i < 260; i++)
    {
        if(gesturebindinfo[i].type == 0)
        {
            pr_debug("手势关联检查完毕:%d\n", i);
            return;
        }
        else if(node[2] == gesturebindinfo[i].trimac[1])
        {
            //执行类型
            if(gesturebindinfo[i].type == 1)
            {
                //节点
                pr_debug("gesture-node\n");
                gestureexecnode(i, node);
                break;
            }
            else if(gesturebindinfo[i].type == 3)
            {
                //区域
                gestureexecarea(i, node);
                break;
            }
            else if(gesturebindinfo[i].type == 4)
            {
                //手动场景
                gestureexecmansce(i, node);
                break;
            }
            else
            {
                pr_debug("Gesture Unrecognized\n");
                return;
            }
        }
    }
}

//节点状态更新
void nodeupdate(int slave, byte *node)
{
    byte sensorpolice[10] = {0};
    byte stateapp[20] = {0xf0,0x00,0x08};
    byte appbuf[10] = {0x42,0x00,0x05};
    byte nodeupdatestr[30] = {0};
    
    appbuf[3] = slave;
    stateapp[3] = slave;
    switch(node[3])
    {
        case 0x02:
            pr_debug("触摸\n");
            break;
        case 0x07:
            pr_debug("活动侦测\n");
            if(node[5] == 0xff)
            {
                //mac
                sensorpolice[0] = slave;
                sensorpolice[1] = node[2];

                //node type
                sensorpolice[2] = nodedev[node[2]].nodeinfo[2];
                //data type
                sensorpolice[3] = 0x07;
                sensorproblem(sensorpolice);
            }
            //mac
            stateapp[4] = node[2];
            stateapp[5] = nodedev[node[2]].nodeinfo[2];
            //data type
            stateapp[6] = 0x07;
            //value
            stateapp[7] = node[5];
            UartToApp_Send(stateapp, 8);
            break;
        case 0x0a:
            pr_debug("手势\n");
            gesturefunction(slave, node);
            break;
        case 0x0c:
            pr_debug("使能\n");
            nodedev[node[2]].nodeinfo[8] = node[5];
            //mac
            stateapp[4] = node[2];
            stateapp[5] = nodedev[node[2]].nodeinfo[2];
            //data type
            stateapp[6] = 0x0c;
            //value
            stateapp[7] = node[5];
            UartToApp_Send(stateapp, 8);
            break;
        case 0x0e:
            pr_debug("节点恢复出厂\n");
            
            //节点删除流程
            appbuf[3] = slave;
            appbuf[4] = node[2];
            delnodedev(appbuf);

            //mac
            stateapp[4] = node[2];
            stateapp[5] = nodedev[node[2]].nodeinfo[2];
            //data type
            stateapp[6] = 0x0e;
            //value
            //stateapp[7] = node[5];
            UartToApp_Send(stateapp, 8);
            break;
        case 0x0f:
            pr_debug("传感器参数\n");
            memcpy(nodedev[node[2]].nodeinfo + 9, node + 4, 2);
            //mac
            stateapp[4] = node[2];
            stateapp[5] = nodedev[node[2]].nodeinfo[2];
            //data type
            stateapp[6] = 0x0f;
            //value
            memcpy(stateapp + 7, node + 4, 2);
            stateapp[2] = 0x09;
            UartToApp_Send(stateapp, 9);
            break;
        case 0x10:
            pr_debug("节点激活\n");
            //mac
            stateapp[4] = node[2];
            stateapp[5] = nodedev[node[2]].nodeinfo[2];
            stateapp[6] = 0x10;
            //value
            memcpy(stateapp + 7, node + 4, 2);
            stateapp[2] = 0x09;
            UartToApp_Send(stateapp, 9);
            break;
        default:
            pr_debug("nodeupdate未识别类型\n");
            break;
    }
}

//刷新自动场景条件
void refreshconditions(int type, byte *node)
{
    int i = 0;
    int z = 1;
    //获取当前时间
    time_t auto_time = 0;
    struct tm *ptr;
    time_t lt;
    lt = time(NULL); 
    ptr = localtime(&lt);       
    pr_debug("minute:%d\n", ptr->tm_min);
    pr_debug("hour:%d\n", ptr->tm_hour);
    pr_debug("wday:%d\n", ptr->tm_wday);
    
    while(z < 11)
    {
        //触发条件长度为0
        if(ascecontrol[z].tricondlen == 0)
        {
            z++;
            continue;
        }
        //执行条件长度为0
        else if(ascecontrol[z].tasklen == 0)
        {
            z++;
            continue;
        }
        for(i = 0; i < 10; i++)
        {
            //负载
            if(0x01 == type)
            {
                if(0 == autotri[z].autotion[i].load[0])
                {
                    break;
                }
                //mac对比
                else if(0 == memcmp(node, autotri[z].autotion[i].load + 1, 2))
                {
                    //设定条件-开
                    if(1 == (autotri[z].autotion[i].load[3] >> 7))
                    {
                        //检查设定负载有无变化
                        if(0x01 == ((node[3] >> ((autotri[z].autotion[i].load[3] & 0x7f) - 1)) & 0x01))
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].load[6] + autotri[z].autotion[i].load[7] + autotri[z].autotion[i].load[8]))
                            {
                                pr_debug("开-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].load[2]);
                                autotri[z].autotion[i].load[6] = ptr->tm_hour;
                                autotri[z].autotion[i].load[7] = ptr->tm_min;
                                autotri[z].autotion[i].load[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向负载计时
                        else
                        {
                            pr_debug("开-刷新-清除反向条件auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].load[2]);
                            autotri[z].autotion[i].load[6] = 0;
                            autotri[z].autotion[i].load[7] = 0;
                            autotri[z].autotion[i].load[8] = 0;
                        }
                    }
                    //设定条件-关
                    else
                    {
                        //检查设定负载有无变化
                        if(0x00 == ((node[3] >> ((autotri[z].autotion[i].load[3] & 0x7f) - 1)) & 0x01))
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].load[6] + autotri[z].autotion[i].load[7] + autotri[z].autotion[i].load[8]))
                            {
                                pr_debug("关-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].load[2]);
                                autotri[z].autotion[i].load[6] = ptr->tm_hour;
                                autotri[z].autotion[i].load[7] = ptr->tm_min;
                                autotri[z].autotion[i].load[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向负载计时
                        else
                        {
                            pr_debug("关-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].load[2]);
                            autotri[z].autotion[i].load[6] = 0;
                            autotri[z].autotion[i].load[7] = 0;
                            autotri[z].autotion[i].load[8] = 0;
                        }
                    }
                }
            }
            //活动侦测
            else if(0x07 == type)
            {
                if(0 == autotri[z].autotion[i].activity[0])
                {
                    break;
                }
                //mac对比
                else if(0 == memcmp(node, autotri[z].autotion[i].activity + 1, 2))
                {
                    //设定条件-开
                    if(1 == autotri[z].autotion[i].activity[3])
                    {
                        //检查设定活动侦测有无变化
                        if(0x01 == (node[4] & 0x01))
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].activity[6] + autotri[z].autotion[i].activity[7] + autotri[z].autotion[i].activity[8]))
                            {
                                pr_debug("开-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[z].autotion[i].activity[2]);
                                autotri[z].autotion[i].activity[6] = ptr->tm_hour;
                                autotri[z].autotion[i].activity[7] = ptr->tm_min;
                                autotri[z].autotion[i].activity[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向计时
                        else
                        {
                            pr_debug("开-清除反向刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[z].autotion[i].activity[2]);
                            autotri[z].autotion[i].activity[6] = 0;
                            autotri[z].autotion[i].activity[7] = 0;
                            autotri[z].autotion[i].activity[8] = 0;
                        }
                    }
                    //设定条件-关
                    else
                    {
                        //检查设定负载有无变化
                        if(0x00 == (node[4] & 0x01))
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].activity[6] + autotri[z].autotion[i].activity[7] + autotri[z].autotion[i].activity[8]))
                            {
                                pr_debug("关-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[z].autotion[i].activity[2]);
                                autotri[z].autotion[i].activity[6] = ptr->tm_hour;
                                autotri[z].autotion[i].activity[7] = ptr->tm_min;
                                autotri[z].autotion[i].activity[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向负载计时
                        else
                        {
                            pr_debug("关-清除反向刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[z].autotion[i].activity[2]);
                            autotri[z].autotion[i].activity[6] = 0;
                            autotri[z].autotion[i].activity[7] = 0;
                            autotri[z].autotion[i].activity[8] = 0;
                        }
                    }
                }
            }
            //门磁
            if(0x51 == type)
            {
                if(0 == autotri[z].autotion[i].magnetic[0])
                {
                    break;
                }
                //mac对比
                else if(0 == memcmp(node, autotri[z].autotion[i].magnetic + 1, 2))
                {
                    //设定条件-开
                    if(1 == autotri[z].autotion[i].magnetic[3])
                    {
                        //检查设定负载有无变化
                        if(0x01 == node[3])
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].magnetic[6] + autotri[z].autotion[i].magnetic[7] + autotri[z].autotion[i].magnetic[8]))
                            {
                                pr_debug("开-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].magnetic[2]);
                                autotri[z].autotion[i].magnetic[6] = ptr->tm_hour;
                                autotri[z].autotion[i].magnetic[7] = ptr->tm_min;
                                autotri[z].autotion[i].magnetic[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向负载计时
                        else
                        {
                            pr_debug("开-刷新-清除反向条件auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].magnetic[2]);
                            autotri[z].autotion[i].magnetic[6] = 0;
                            autotri[z].autotion[i].magnetic[7] = 0;
                            autotri[z].autotion[i].magnetic[8] = 0;
                        }
                    }
                    //设定条件-关
                    else
                    {
                        //检查设定负载有无变化
                        if(0x00 == node[3])
                        {
                            //检查是否已经开始计时-未计时则开始计时
                            if(0 == (autotri[z].autotion[i].magnetic[6] + autotri[z].autotion[i].magnetic[7] + autotri[z].autotion[i].magnetic[8]))
                            {
                                pr_debug("关-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].magnetic[2]);
                                autotri[z].autotion[i].magnetic[6] = ptr->tm_hour;
                                autotri[z].autotion[i].magnetic[7] = ptr->tm_min;
                                autotri[z].autotion[i].magnetic[8] = ptr->tm_sec;
                            }
                        }
                        //清除反向负载计时
                        else
                        {
                            pr_debug("关-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[z].autotion[i].magnetic[2]);
                            autotri[z].autotion[i].magnetic[6] = 0;
                            autotri[z].autotion[i].magnetic[7] = 0;
                            autotri[z].autotion[i].magnetic[8] = 0;
                        }
                    }
                }
            }
        }
        z++;
    }
    ptr = NULL;
}

//震动报警检查
void vibrationcheck(byte *nodemac, int value)
{
    if(0 == secmonitor[3].enable)
    {
        pr_debug("震动报警未使能\n");
        return;
    }

    //byte appsend[24] = {0xf0,0x00,0x0a,0x01,0x01,0x0f,0x2b,0x02};

    //有震动信息/事件
    if(0x04 == (value & 0x04))
    {
        int i = 0;
        //开始计时
        timers[1].startlogo = 1;
        for(i = 0; i < 6; i += 2)
        {
            if(0 == memcmp(timers[1].nodemac + i, nodemac, 2))
            {
                memcpy(timers[1].nodemac + i, nodemac, 2);
                break;
            }
            else if(0 == timers[1].nodemac[i])
            {
                timers[1].triggerlogo++;
                memcpy(timers[1].nodemac + i, nodemac, 2);
                break;
            }
            pr_debug("timers[1].triggerlogo:%d\n", timers[1].triggerlogo);
        }
        if(3 == timers[1].triggerlogo)
        {
            //震动报警
            checksecurityalarm(3, nodemac);

            //UartToApp_Send(appsend, appsend[2]);
            memset(&timers[1], 0, sizeof(timers[1]));
        }
        pr_debug("triggerlogo:%d\n", timers[1].triggerlogo);
    }
}

//节点状态反馈
void statefeedback(int slave, byte *temnode)
{
    int nodeup = 1;
    int offset = 0;
    byte high = 0;
    byte low = 0;
    byte vibramac[10] = {0};
    byte sensorpolice[10] = {0};
    byte autorefresh[10] = {0};
    byte node[20] = {0};
    byte nodestr[30] = {0};
    byte stateapp[20] = {0xf0,0x00,0x08};
    stateapp[3] = slave;
    memcpy(node, temnode + 2, 10);

    while(node[offset] != 0)
    {
        if(nodedev[node[offset]].nodeinfo[2] == 0)
        {
            //未知节点上报
            //非离线信息
            if(node[offset + 1] == 1)
            {
                nodeup = 0;
                pr_debug("未知节点上报0x00\n");
                stateapp[3] = slave;
                stateapp[4] = node[offset];
                stateapp[5] = 0xff;
                UartToApp_Send(stateapp, stateapp[2]);
            }
        }
        else
        {
            nodeup = 1;
            //开关
            if((nodedev[node[offset]].nodeinfo[2] > 0) && (nodedev[node[offset]].nodeinfo[2] < 0x0f))
            {
                //离线
                if(node[offset + 1] == 0x00)
                {
                    //离线
                    nodedev[node[offset]].nodeinfo[11] = 0x00;

                    //上报app
                    //mac
                    stateapp[4] = node[offset];
                    stateapp[5] = 0x00;
                    UartToApp_Send(stateapp, 8);
                }
                else
                {
                    //在线
                    nodedev[node[offset]].nodeinfo[11] = 0x01;

                    //pr_debug("nodedev[%d].nodeinfo[5]:%0x\n", node[offset], nodedev[node[offset]].nodeinfo[5]);
                    //pr_debug("node[%d]:%0x\n", offset + 2, node[offset + 2]);

                    //更新自动场景条件
                    autorefresh[0] = slave;
                    memcpy(autorefresh + 1, node + offset, 5);

                    //负载状态更新
                    if((nodedev[node[offset]].nodeinfo[5] & 0xf0) != (node[offset + 2] & 0xf0))
                    {
                        //更新自动场景条件
                        refreshconditions(1, autorefresh);

                        //上报app
                        //mac
                        stateapp[4] = node[offset];
                        //node type
                        stateapp[5] = nodedev[node[offset]].nodeinfo[2];
                        //data type
                        stateapp[6] = 0x01;
                        //value
                        memcpy(stateapp + 7, node + offset + 2, 3);
                        //len
                        stateapp[2] = 0x0a;
                        UartToApp_Send(stateapp, 10);

                        pr_debug("触发led校验\n");
                        pthread_mutex_lock(&fifo->lock);
                        g_led = 1;
                        g_led_on = 1;
                        pthread_mutex_unlock(&fifo->lock);
                    }
                    //传感器更新
                    else if(nodedev[node[offset]].nodeinfo[6] != node[offset + 3])
                    {
                        pr_debug("传感器变化\n");
                        
                        //检查活动侦测是否改变
                        if((nodedev[node[offset]].nodeinfo[6] & 0x01) != (node[offset + 3] & 0x01))
                        {
                            pr_debug("活动侦测变化\n");
                            //安防检查
                            smartsecuritycheck(7, autorefresh);
                            //更新自动场景条件
                            refreshconditions(7, autorefresh);
                        }
                        //检查光感
                        if((node[offset + 3] & 0xf0) == 0xf0)
                        {
                            sensorpolice[0] = slave;
                            sensorpolice[1] = nodedev[node[offset]].nodeinfo[1];

                            //node type
                            sensorpolice[2] = nodedev[node[offset]].nodeinfo[2];
                            //data type
                            sensorpolice[3] = 0x08;
                            sensorproblem(sensorpolice);
                        }
                        //震动报警检查
                        vibramac[0] = slave;
                        vibramac[1] = node[offset];
                        vibrationcheck(vibramac, node[offset + 3]);

                        //上报app
                        //mac
                        stateapp[4] = node[offset];
                        //node type
                        stateapp[5] = nodedev[node[offset]].nodeinfo[2];
                        //data type
                        stateapp[6] = 0x13;
                        //value
                        memcpy(stateapp + 7, node + offset + 2, 3);
                        //len
                        stateapp[2] = 0x0a;
                        UartToApp_Send(stateapp, 10);
                    }
                    //传感器更新-温度
                    else if(nodedev[node[offset]].nodeinfo[7] != node[offset + 4])
                    {
                        pr_debug("温度变化\n");
                        //mac
                        sensorpolice[0] = slave;
                        sensorpolice[1] = nodedev[node[offset]].nodeinfo[1];
                        //node type
                        sensorpolice[2] = nodedev[node[offset]].nodeinfo[2];
                        //data type
                        sensorpolice[3] = 0x06;
                        //value
                        sensorpolice[4] = node[offset + 4];
                        sensorproblem(sensorpolice);
                        
                        //上报app
                        //mac
                        stateapp[4] = node[offset];
                        //node type
                        stateapp[5] = nodedev[node[offset]].nodeinfo[2];
                        //data type
                        stateapp[6] = 0x13;
                        //value
                        memcpy(stateapp + 7, node + offset + 2, 3);
                        //len
                        stateapp[2] = 0x0a;
                        UartToApp_Send(stateapp, 10);
                    }
                    else
                    {
                        pr_debug("刷新自动场景条件-负载&传感器\n");
                        refreshconditions(1, autorefresh);
                        refreshconditions(7, autorefresh);
                        
                        //上报app
                        //mac
                        stateapp[4] = node[offset];
                        //node type
                        stateapp[5] = nodedev[node[offset]].nodeinfo[2];
                        //data type
                        stateapp[6] = 0x13;
                        //value
                        memcpy(stateapp + 7, node + offset + 2, 3);
                        //len
                        stateapp[2] = 0x0a;
                        UartToApp_Send(stateapp, 10);
                    }
                }
            }
            //其他未知设备
            else
            {
                //离线
                if(node[offset + 1] == 0x00)
                {
                    //离线
                    nodedev[node[offset]].nodeinfo[11] = 0x00;

                    //上报app
                    //mac
                    stateapp[4] = node[offset];
                    stateapp[5] = 0x00;
                    UartToApp_Send(stateapp, 8);
                }
                else
                {
                    //在线
                    nodedev[node[offset]].nodeinfo[11] = 0x01;
                    memcpy(nodedev[node[offset]].nodeinfo + 5, node + offset + 2, 3);

                    pr_debug("nodedev[%d].nodeinfo[5]:%0x\n", node[offset], nodedev[node[offset]].nodeinfo[5]);
                    pr_debug("node[%d]:%0x\n", offset + 2, node[offset + 2]);

                    //上报app
                    //mac
                    stateapp[4] = node[offset];
                    //node type
                    stateapp[5] = nodedev[node[offset]].nodeinfo[2];
                    //data type
                    stateapp[6] = 0x01;
                    //value
                    memcpy(stateapp + 7, node + offset + 2, 3);
                    //len
                    stateapp[2] = 0x0a;
                    UartToApp_Send(stateapp, 10);
                        
                    pr_debug("触发led校验\n");
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    g_led_on = 1;
                    pthread_mutex_unlock(&fifo->lock);
                }
            }
            //memcpy(nodedev[node[offset]].nodeinfo + 5, node + offset + 2, 3);
            //break;
        }
        if(nodeup == 1)
        {
            memcpy(nodedev[node[offset]].nodeinfo + 5, node + offset + 2, 3);
        }
        offset += 5;
    }
}

//功能键长按
int funkeypress(byte *key)
{
    int i = 1;
    byte appbuf[12] = {0x62,0x00,0x04};
        
    for(i = 0; i < 21; i++)
    {
        if(key[2] == mscecontrol[i].bingkeyid)
        {
            if(0 == memcmp(key, mscecontrol[i].bingkeymac, 2))
            {
                //场景id
                appbuf[3] = mscecontrol[i].id;
                manualsceope(appbuf);
                break;
            }
        }
    }
    pr_debug("手动场景扫描完毕\n");
}

//功能键短按
int funkeyshort(byte *key)
{
    int i = 1;
    byte uartsend[10] = {0x65,0x0c};
    byte defuartsend[10] = {0x65,0x0c};

    byte defaultmac[10] = {0xff,0xff};

    pr_debug("功能键短按-执行区域\n");
    for(i = 1; i < 21; i++)
    {
        if(key[2] == regcontrol[i].bingkeyid)
        {
            if(0 == memcmp(key, regcontrol[i].bingkeymac, 2))
            {
                //区域id
                uartsend[2] = regcontrol[i].id;
                //绑定按键mac
                uartsend[3] = key[1];
                //绑定按键id
                uartsend[4] = regcontrol[i].bingkeyid;
                uartsend[5] = 0;
                //执行状态-关
                if(0x01 == key[3])
                {
                    uartsend[6] = 0x00;
                }
                else
                {
                    uartsend[6] = 0x01;
                }
                //优先执行自定义区域
                break;
            }
            else if(0 == memcmp(defaultmac, regcontrol[i].bingkeymac, 2))
            {
                //区域id
                defuartsend[2] = regcontrol[i].id;
                //绑定按键mac
                defuartsend[3] = 0xff;
                //绑定按键id
                defuartsend[4] = regcontrol[i].bingkeyid;
                uartsend[5] = 0;
                //执行状态-关
                if(0x01 == key[3])
                {
                    defuartsend[6] = 0x00;
                }
                else
                {
                    defuartsend[6] = 0x01;
                }
            }
        }
    }
    if(uartsend[2] != 0)
    {
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }
    else if(defuartsend[2] != 0)
    {
        UART0_Send(uart_fd, defuartsend, defuartsend[1]);
    }
    pr_debug("区域功能扫描完毕:%d\n", uartsend[2]);
}

//本地键短按
void localbuttonshort(byte *key)
{
    int i = 0;
    byte uartsend[10] = {0x63,0x0e,0x00,0x01,0x00,0xaa};
    pr_debug("localbuttonshort-\n");
    for(i = 0; i < 50; i++)
    {
        //指令任务长度为0
        if(localasoction[i].bingkey[1] == 0)
        {
            pr_debug("本地关联扫描完毕:%d\n", i);
            break;
        }
        else if(0 == memcmp(key, localasoction[i].bingkey, 3))
        {
            //mac
            uartsend[2] = localasoction[i].bingkey[4];
            //node id
            uartsend[4] = localasoction[i].bingkey[5];
            
            //执行状态-关
            if(0x01 == key[3])
            {
                if(uartsend[4] == 1)
                {
                    uartsend[5] = 0x00;
                }
                else if(uartsend[4] == 5)
                {
                    uartsend[5] = 0xa8;
                }
                else if(uartsend[4] == 6)
                {
                    uartsend[5] = 0xa2;
                }
                else if(uartsend[4] == 7)
                {
                    uartsend[5] = 0x8a;
                }
                else if(uartsend[4] == 8)
                {
                    uartsend[5] = 0x2a;
                }
            }
            else
            {
                if(uartsend[4] == 1)
                {
                    uartsend[5] = 0x01;
                }
                else if(uartsend[4] == 5)
                {
                    uartsend[5] = 0xa9;
                }
                else if(uartsend[4] == 6)
                {
                    uartsend[5] = 0xa6;
                }
                else if(uartsend[4] == 7)
                {
                    uartsend[5] = 0x9a;
                }
                else if(uartsend[4] == 8)
                {
                    uartsend[5] = 0x6a;
                }
            }
        }
        else if(0 == memcmp(key, localasoction[i].bingkey + 3, 3))
        {
            //mac
            uartsend[2] = localasoction[i].bingkey[1];
            //node id
            uartsend[4] = localasoction[i].bingkey[2];
            
            //执行状态-关
            if(0x01 == key[3])
            {
                if(uartsend[4] == 5)
                {
                    uartsend[5] = 0xa8;
                }
                else if(uartsend[4] == 6)
                {
                    uartsend[5] = 0xa2;
                }
                else if(uartsend[4] == 7)
                {
                    uartsend[5] = 0x8a;
                }
                else if(uartsend[4] == 8)
                {
                    uartsend[5] = 0x2a;
                }
            }
            else
            {
                if(uartsend[4] == 5)
                {
                    uartsend[5] = 0xa9;
                }
                else if(uartsend[4] == 6)
                {
                    uartsend[5] = 0xa6;
                }
                else if(uartsend[4] == 7)
                {
                    uartsend[5] = 0x9a;
                }
                else if(uartsend[4] == 8)
                {
                    uartsend[5] = 0x6a;
                }
            }
        }
        if(uartsend[4] > 0)
        {
            UART0_Send(uart_fd, uartsend, uartsend[1]);
            break;
        }
    }
}

//智能控制面板关联
void smartctlpaneltouch(byte *key)
{
    int i = 0;
    byte uartsend[24] = {0x63,0x0e};
    byte appdata[24] = {0x62,0x00,0x04};

    //type
    uartsend[3] = 0x01;

    for(i = 0; i < 128; i++)
    {
        if(smartcrlpanel[i].trimac[1] == 0)
        {
            pr_debug("smartctlpaneltouch check over:%d\n", i);
            return;
        }
        //匹配mac
        else if(0 == memcmp(smartcrlpanel[i].trimac, key, 2))
        {
            if(smartcrlpanel[i].triid == key[2])
            {
                if(smartcrlpanel[i].type == 0x01)
                {
                    uartsend[2] = smartcrlpanel[i].perftask[1];
                    uartsend[4] = smartcrlpanel[i].perftask[2];
                    
                    //开关/控制盒
                    if(smartcrlpanel[i].perftask[2] == 5)
                    {
                        uartsend[5] = 0xab;
                    }
                    else if(smartcrlpanel[i].perftask[2] == 6)
                    {
                        uartsend[5] = 0xae;
                    }
                    else if(smartcrlpanel[i].perftask[2] == 7)
                    {
                        uartsend[5] = 0xba;
                    }
                    else if(smartcrlpanel[i].perftask[2] == 8)
                    {
                        uartsend[5] = 0xea;
                    }
                }
                //手动场景
                else if(smartcrlpanel[i].type == 4)
                {
                    //id
                    appdata[3] = smartcrlpanel[i].perftask[0];
                    //执行手动场景
                    manualsceope(appdata);
                    return;
                }
                //红外快捷方式
                else if(smartcrlpanel[i].type == 0x06)
                {
                    uartsend[0] = 0x7f;
                    uartsend[1] = 0x06;
                    memcpy(uartsend + 2, smartcrlpanel[i].perftask + 1, 3);
                }
                //窗帘
                else if(smartcrlpanel[i].type == 0x11)
                {
                    uartsend[2] = smartcrlpanel[i].perftask[1];
                    uartsend[4] = smartcrlpanel[i].perftask[2];
                    uartsend[5] = smartcrlpanel[i].perftask[3];
                }
                //灯具
                else if(smartcrlpanel[i].type == 0x31)
                {
                    uartsend[0] = 0x86;
                    uartsend[1] = 0x08;
                    memcpy(uartsend + 2, smartcrlpanel[i].perftask + 1, 5);
                }
                else
                {
                    return;
                }
                UART0_Send(uart_fd, uartsend, uartsend[1]);
                return;
            }
        }
    }
}

//触摸功能
void touchfunction(int slave, byte *touch, byte nodetype)
{
    byte touchkey[5] = {0};
    byte appsend[20] = {0xf0,0x00,0x09};
    byte uartsend[10] = {0x63,0x0e,0xff,0x2b,0xff,0x00};
    //mac
    touchkey[0] = slave;
    touchkey[1] = touch[2];
    //id
    touchkey[2] = (touch[4] & 0x7f);
    //value
    touchkey[3] = touch[5];
    
    if(hostinfo.alarm == 1)
    {
        hostinfo.alarm = 0;
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }

    pr_debug("nodetype:%0x\n", nodetype);
    //上传至app
    appsend[3] = slave;
    appsend[4] = touch[2];
    appsend[5] = nodetype;
    memcpy(appsend + 6, touch + 3, 3);
    UartToApp_Send(appsend, appsend[2]);
 
    pr_debug("触发led校验\n");
    pthread_mutex_lock(&fifo->lock);
    g_led = 1;
    g_led_on = 1;
    pthread_mutex_unlock(&fifo->lock);

    if(touch[4] == 0x09)
    {
        pr_debug("掌拍\n");
        int i = 5;
        for(i = 5; i < (nodedev[touch[2]].nodeinfo[4] + 5); i++)
        {
            //id
            touchkey[2] = i;
            localbuttonshort(touchkey);
        }
    }
    else if(touch[4] == 0x10)
    {
        pr_debug("触摸复位\n");
    }
    //长按
    else if((touch[4] & 0x80) != 0)
    {
        //开关
        if((nodetype > 0) && (nodetype < 0x0f))
        {
            //功能键长按
            if((touchkey[2] >= 1) && (touchkey[2] <= 4))
            {
                funkeypress(touchkey);
            }
        }
        else if(nodetype == 0x71)
        {
            //智能控制面板长按
            //id
            touchkey[2] = touch[4];
            //智能控制面板关联
            smartctlpaneltouch(touchkey);
        }
    }
    else if((touch[4] & 0x80) == 0)
    {
        //开关
        if((nodetype > 0) && (nodetype < 0x0f))
        {
            //功能键短按
            if((touchkey[2] >= 1) && (touchkey[2] <= 4))
            {
                funkeyshort(touchkey);
            }
            //本地键短按
            else if((touchkey[2] >= 5) && (touchkey[2] <= 8))
            {
                localbuttonshort(touchkey);
            }
        }
        else if(nodetype == 0x71)
        {
            //智能控制面板短按
            //id
            touchkey[2] = touch[4];
            //智能控制面板关联
            smartctlpaneltouch(touchkey);
        }
    }
}

//节点数据反馈
void datafeedback(int slave, byte *node)
{
    /*
       int i = 0;
       byte nodestr[30] = {0};
       byte stateapp[20] = {0xf0,0x00,0x08};
       stateapp[3] = slave;
     */
    byte appsend[10] = {0xf0,0x00,0x08};
    byte acknode[20] = {0};
    byte tmpnode[10] = {0};

    memcpy(tmpnode, node, node[1]);
    tmpnode[1] = slave;

    if(nodedev[node[2]].nodeinfo[2] == 0)
    {
        pr_debug("datafeedback-非法节点\n");
        appsend[3] = slave;
        appsend[4] = node[2];
        appsend[5] = 0xff;
        UartToApp_Send(appsend, appsend[2]);
        return;
    }
    pr_debug("nodedev[node[2]].nodeinfo[2]:%0x\n", nodedev[node[2]].nodeinfo[2]);
    pr_debug("node[2]:%0x\n", node[2]);
    switch(node[3])
    {
        case 0x02:
            touchfunction(slave, node, nodedev[node[2]].nodeinfo[2]);
            break;
        case 0x07:
            nodeupdate(slave, node);
            break;
        case 0x0a:
            nodeupdate(slave, node);
            break;
        case 0x0c:
            nodeupdate(slave, node);
            break;
        case 0x0e:
            nodeupdate(slave, node);
            break;
        case 0x0f:
            nodeupdate(slave, node);
            break;
        case 0x10:
            nodeupdate(slave, node);
            break;
        default:
            pr_debug("datafeedback未识别类型\n");
            break;
    }
}

//恢复出厂
void restorefactory()
{
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte mesh[15] = {0x7b,0x0b,0x01,0x02,0x03,0x04,0x01,0x02,0x03,0x01,0x00};
    
    UART0_Send(uart_fd, mesh, 11);
    IRUART0_Send(single_fd, chipsend, chipsend[2]);

    //创建恢复出厂shell文件
    createrestore();
    
    sleep(5);
    pr_debug("恢复出厂等待5秒\n");
    system("sh /data/modsty/restorefactory.sh");
    system("reboot");
}

//从机单片机指令上传
void slavesinglechipup(byte *buf)
{
    byte dhcpstr[10] = {"dhcp1"};

    if(buf[3] == 0x05)
    {
        //切换dhcp
        if(buf[4] == 0x04)
        {
            if(hostinfo.dhcp == 0)
            {
                replace(HOSTSTATUS, dhcpstr, 4);
                hostinfo.dhcp = 1;
                system("/etc/init.d/network restart");
            }
        }
        else if(buf[4] == 0x05)
        {
            byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00,0x3e};
            byte uartsend[12] = {0x7b,0x0b,0x01,0x02,0x03,0x04,0x01,0x02,0x03,0x01,0x97};
            //恢复出厂
            //解散网络
            slaveUartSend(uart_fd, uartsend, uartsend[1]);

            slavechipSend(single_fd, chipsend, chipsend[2]);
            restorefactory();
            sleep(1);
            system("reboot");
        }
        //恢复出厂并切换主从机
        //出厂状态-切换至从机-临时
        else if(buf[4] == 0x06)
        {
            //未组过网==初始状态
            if(hostinfo.mesh == 2)
            {
                byte orimode[20] = {"mode0"};
                replace(HOSTSTATUS, orimode, 4);
                sleep(1);
                exit(0);
            }
        }
    }
    else if(buf[3] == 0x03)
    {
        pr_debug("从机温湿度\n");
        slaveident[master].temperature = buf[4];
        slaveident[master].moderate = buf[5];
    }
}

//震动关联手动场景
void shockpermansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    nodemac[0] = slave;
    nodemac[1] = 0x01;

    for(i = 0; i < 260; i++)
    {
        if(shockbindinfo[i].trimac[1] == 0)
        {
            pr_debug("shockpermansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(shockbindinfo[i].trimac, nodemac, 2))
        {
            //挟持报警
            //id
            appdata[3] = shockbindinfo[i].carmac[0];
            //执行手动场景
            manualsceope(appdata);
            break;
        }
    }
}

//单片机指令上传
void singlechipup(int slave, byte *buf)
{
    int sendlen = 0;
    int i = 0;
    int offset = 0;
    byte vibramac[10] = {0};
    byte dhcpstr[10] = {"dhcp1"};
    byte temoderate[20] = {0xf0,0x00,0x09,0x00,0x00,0x0f,0x09};
    byte appbuf[20] = {0x23,0x00,0x0a};
    double lentemp = 0.0;
    double n = 0.0, precious = 0.0, inter = 0;
    byte irmeshsend[70] = {0};
    byte ifcode[512] = {0};

    int len = ((buf[1] << 8) + buf[2]);
    byte appsend[500] = {0xcb};
    byte appsendc[10] = {0xcc,0x00,0x04,0x00};

    if(buf[3] == 0x01)
    {
        memcpy(appbuf + 3, hostinfo.hostsn, 6);
        appbuf[2] = 0x0c;
        memcpy(appbuf + 9, buf + 4, 7);
        if(2018 <= ((buf[4] << 8) + buf[5]))
        {
            appsettime(appbuf);
        }
    }
    else if(buf[3] == 0x04)
    {
        pr_debug("震动信息\n");
        //震动关联手动场景
        shockpermansce(slave, buf);
        
        //震动报警检查
        vibramac[0] = slave;
        vibramac[1] = 0x01;
        vibrationcheck(vibramac, 4);

        byte cloudsend[20] = {0x06,0x00,0x0f};
        memcpy(cloudsend + 3, buf + 4, 12);
        send(client_fd, cloudsend, cloudsend[2], MSG_NOSIGNAL);
        
        temoderate[3] = slave;
        temoderate[4] = 0x01;
        UartToApp_Send(temoderate, temoderate[2]);
    }
    else if(buf[3] == 0x05)
    {
        memcpy(appbuf + 3, hostinfo.hostsn, 6);
        //组网
        if(buf[4] == 0x01)
        {
            appbuf[9] = 0;
            networking(appbuf);
        }
        else if(buf[4] == 0x02)
        {
        }
        //强制组网授权
        else if(buf[4] == 0x03)
        {
            if(timers[0].startlogo == 1)
            {
                timers[0].triggerlogo = 1;
            }
        }
        //切换dhcp
        else if(buf[4] == 0x04)
        {
            if(hostinfo.dhcp == 0)
            {
                replace(HOSTSTATUS, dhcpstr, 4);
                hostinfo.dhcp = 1;
                system("/etc/init.d/network restart");
            }
        }
        else if(buf[4] == 0x05)
        {
            byte delslave[24] = {0x26,0x00,0x06,0x02,0xff,0xff};
            byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00,0x3e};
            //从机恢复
            hosttoslave(0xff, delslave, delslave[2]);
            slavechipSend(single_fd, chipsend, chipsend[2]);
            
            //主机解散网络
            hostinfo.slavemesh = 1;
            
            //恢复出厂
            restorefactory();
        }
        //恢复出厂并切换主从机
        else if(buf[4] == 0x06)
        {
            //未组过网==初始状态
            if(hostinfo.mesh == 2)
            {
                byte orimode[20] = {"mode0"};
                replace(HOSTSTATUS, orimode, 4);
                sleep(1);
                exit(0);
            }
        }
    }
    else if(buf[3] == 0x03)
    {
        pr_debug("主机温湿度\n");
        byte secnode[8] = {0};
        //主机
        if(slave == 1)
        {
            hostinfo.temperature = buf[4];
            hostinfo.moderate = buf[5];
        }
        //从机
        else
        {
            slaveident[slave].temperature = buf[4];
            slaveident[slave].moderate = buf[5];
        }
        secnode[0] = slave;
        secnode[1] = 0x01;
        secnode[2] = 0x0f;
        secnode[3] = 0x06;
        secnode[4] = buf[4];
        //温度报警
        sensorproblem(secnode);
        
        temoderate[3] = slave;
        temoderate[4] = 0x01;
        temoderate[6] = 0x13;
        temoderate[7] = buf[4];
        temoderate[8] = buf[5];
        UartToApp_Send(temoderate, temoderate[2]);
    }
    //红外
    else if((buf[3] == 0x02) || (buf[3] == 0x06))
    {
        memset(&ircache, 0, sizeof(ircache));
        //mac
        ircache.mac[0] = buf[4];
        ircache.mac[1] = buf[5];
        //len
        ircache.len[0] = (((len - 6) >> 8) & 0xff);
        ircache.len[1] = ((len - 6) & 0xff);
        //来源
        ircache.source = 0x02;
        //压缩-红外码
        ircache.compression = buf[6];
        memcpy(ircache.ircode, buf + 7, len - 8);
        
        ifcode[0] = ircache.len[0];
        ifcode[1] = ircache.len[1];
        ifcode[2] = ircache.source;
        ifcode[3] = ircache.compression;
        memcpy(ifcode + 4, ircache.ircode, len - 8);
        
        lentemp = len - 4;
        n = lentemp / 55;
        precious = modf(n, &inter);
        pr_debug("小数部分 %lf, 整数部分 %lf\n", precious, inter);
        if(precious > 0.0)
        {
            inter += 1;
        }
        
        //发往app
        if(buf[5] == 0x00)
        {
            //发往app指令比单片机上传指令少4字节
            len -= 4;
            appsend[1] = (len >> 8) & 0xff;
            appsend[2] = len & 0xff;
            appsend[3] = 0x00;
            memcpy(appsend + 4, buf + 7, len);
            
            UartToApp_Send(appsend, len);
        }
        //主机存储
        else if(buf[5] == 0x01)
        {
            UartToApp_Send(appsendc, 4);
        }
        else if(buf[5] == 0xff)
        {
            pr_debug("IR学习失败\n");
            appsend[1] = 0x00;
            appsend[2] = 0x04;
            appsend[3] = 0x01;
            UartToApp_Send(appsend, 4);
        }
        //发往节点
        else
        {
            pr_debug("IR发往节点\n");
            for(i = 0; i < inter; i++)
            {
                memset(irmeshsend, 0, sizeof(irmeshsend));
                irmeshsend[0] = 0x7c;
                irmeshsend[1] = 0x3c;
                //节点mac
                irmeshsend[2] = buf[5];
                //当前包数
                irmeshsend[3] = i;
                //红外码
                if(((len - 2) - offset) > 55)
                {
                    memcpy(irmeshsend + 4, ifcode + offset, 55);
                    offset += 55;
                }
                else
                {
                    memcpy(irmeshsend + 4, ifcode + offset, ((len - 2) - offset));
                }
                UART0_Send(uart_fd, irmeshsend, irmeshsend[1]);
            }
            memcpy(sceneack.endbuf, irmeshsend, irmeshsend[1]);
        }
    }
    else if(buf[3] == 0x09)
    {
        //主机
        if(slave == 1)
        {
            hostinfo.enable = buf[4];
        }
        //从机
        else
        {
            slaveident[slave].enable = buf[4];
        }
        temoderate[3] = slave;
        temoderate[4] = 0x01;
        temoderate[6] = 0x0c;
        temoderate[7] = buf[4];
        UartToApp_Send(temoderate, temoderate[2]);
    }
    pr_debug("sendlen:%d\n", sendlen);
}

//控制盒转换应答
void ctlboxconversion(int slave, byte *buf)
{
    if(buf[4] != 0)
    {
        pr_debug("控制盒转换失败\n");
        return;
    }
    byte appsend[10] = {0xf0,0x00,0x09};
    appsend[3] = slave;
    appsend[4] = buf[2];
    //node type
    appsend[5] = nodedev[buf[2]].nodeinfo[2];
    appsend[6] = 0x17;
    memcpy(appsend + 7, buf + 3, 2);
    UartToApp_Send(appsend, appsend[2]);
    
    byte uartsend[20] = {0x63,0x0e};
    byte nodemac[5] = {0};

    uartsend[2] = buf[2];
    uartsend[3] = 0x10;

    nodemac[0] = slave;
    nodemac[1] = buf[2];

    //检索删除区域
    delnodereg(nodemac);
    //检索删除手动场景
    delnodemanualsce(nodemac);
    //检索删除自动场景
    delnodeautosce(nodemac);
    
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//窗帘控制应答0x88
void curtainctlack(int slave, byte *buf)
{
    if(buf[4] != 0)
    {
        pr_debug("窗帘转换失败\n");
        return;
    }
    byte uartsend[20] = {0x63,0x0e};
    byte nodemac[5] = {0};
    byte appsend[10] = {0xf0,0x00,0x09};
    
    appsend[3] = slave;
    appsend[4] = buf[2];
    //node type
    appsend[5] = nodedev[buf[2]].nodeinfo[2];
    appsend[6] = 0x16;
    memcpy(appsend + 7, buf + 3, 2);
    UartToApp_Send(appsend, appsend[2]);
    
    uartsend[2] = buf[2];
    uartsend[3] = 0x10;
    
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    //检索删除区域
    delnodereg(nodemac);
    //检索删除手动场景
    delnodemanualsce(nodemac);
    //检索删除自动场景
    delnodeautosce(nodemac);
    
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//关联执行节点
void aocexecnode(byte *exnode)
{
    int i = 0;
    for(i = 0; i < 6; i++)
    {
        pr_debug("exnode[%d]:%0x\n", i, exnode[i]);
    }

    int type = 0;
    byte uartsend[24] = {0x63,0x0e};
    //mac
    uartsend[2] = exnode[1];
    //type
    uartsend[3] = 0x01;
    //node type
    type = nodedev[exnode[1]].nodeinfo[2];

    if((type > 0) && (type < 0x0f))
    {
        //开关
        uartsend[4] = exnode[2];
        uartsend[5] = exnode[3];
    }
    else if((type > 0x10) && (type < 0x1f))
    {
        //窗帘
        uartsend[4] = exnode[2];
        uartsend[5] = exnode[3];
    }
    else if((type > 0x20) && (type < 0x2f))
    {
        //控制器/插板
        uartsend[4] = exnode[2];
        uartsend[5] = exnode[3];
    }
    else if((type > 0x30) && (type < 0x3f))
    {
        //灯具
        uartsend[0] = 0x86;
        uartsend[1] = 0x08;
        memcpy(uartsend + 2, exnode + 1, 5);
    }
    else
    {
        pr_debug("aocexecnode notype\n");
        return;
    }
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//烟雾关联检查执行手动场景
void smokepermansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(smokebindinfo[i].trimac[1] == 0)
        {
            pr_debug("smokepermansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(smokebindinfo[i].trimac, nodemac, 2))
        {
            if(smokebindinfo[i].triid == buf[4])
            {
                if(smokebindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = smokebindinfo[i].carmac[0];
                    asexecnode[1] = smokebindinfo[i].carmac[1];
                    asexecnode[2] = smokebindinfo[i].carid;
                    memcpy(asexecnode + 3, smokebindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(smokebindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = smokebindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//门磁关联
void doormagnetmansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(doormagnetbindinfo[i].trimac[1] == 0)
        {
            pr_debug("doormagnetmansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(doormagnetbindinfo[i].trimac, nodemac, 2))
        {
            if(doormagnetbindinfo[i].triid == buf[4])
            {
                if(doormagnetbindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = doormagnetbindinfo[i].carmac[0];
                    asexecnode[1] = doormagnetbindinfo[i].carmac[1];
                    asexecnode[2] = doormagnetbindinfo[i].carid;
                    memcpy(asexecnode + 3, doormagnetbindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(doormagnetbindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = doormagnetbindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//水浸关联
void floodingmansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(floodingbindinfo[i].trimac[1] == 0)
        {
            pr_debug("floodingmansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(floodingbindinfo[i].trimac, nodemac, 2))
        {
            if(floodingbindinfo[i].triid == buf[4])
            {
                if(floodingbindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = floodingbindinfo[i].carmac[0];
                    asexecnode[1] = floodingbindinfo[i].carmac[1];
                    asexecnode[2] = floodingbindinfo[i].carid;
                    memcpy(asexecnode + 3, floodingbindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(floodingbindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = floodingbindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//红外活动侦测关联
void iractivitymansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(iractivitybindinfo[i].trimac[1] == 0)
        {
            pr_debug("iractivity check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(iractivitybindinfo[i].trimac, nodemac, 2))
        {
            if(iractivitybindinfo[i].triid == buf[4])
            {
                if(iractivitybindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = iractivitybindinfo[i].carmac[0];
                    asexecnode[1] = iractivitybindinfo[i].carmac[1];
                    asexecnode[2] = iractivitybindinfo[i].carid;
                    memcpy(asexecnode + 3, iractivitybindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(iractivitybindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = iractivitybindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//雨雪关联
void rainsnowmansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(rainsnowbindinfo[i].trimac[1] == 0)
        {
            pr_debug("rainsnowmansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(rainsnowbindinfo[i].trimac, nodemac, 2))
        {
            if(rainsnowbindinfo[i].triid == buf[4])
            {
                if(rainsnowbindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = rainsnowbindinfo[i].carmac[0];
                    asexecnode[1] = rainsnowbindinfo[i].carmac[1];
                    asexecnode[2] = rainsnowbindinfo[i].carid;
                    memcpy(asexecnode + 3, rainsnowbindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(rainsnowbindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = rainsnowbindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//燃气关联
void gasmansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(gasbindinfo[i].trimac[1] == 0)
        {
            pr_debug("gasmansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(gasbindinfo[i].trimac, nodemac, 2))
        {
            if(gasbindinfo[i].triid == buf[4])
            {
                if(gasbindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = gasbindinfo[i].carmac[0];
                    asexecnode[1] = gasbindinfo[i].carmac[1];
                    asexecnode[2] = gasbindinfo[i].carid;
                    memcpy(asexecnode + 3, gasbindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(gasbindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = gasbindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//风速关联
void windspeedmansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(windspeedbindinfo[i].trimac[1] == 0)
        {
            pr_debug("windspeedmansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(windspeedbindinfo[i].trimac, nodemac, 2))
        {
            //大于等于
            if(0x80 == (windspeedbindinfo[i].triid & 0x80))
            {
                pr_debug("大于等于\n");
                if((windspeedbindinfo[i].triid & 0x7f) <= buf[4])
                {
                    if(windspeedbindinfo[i].type == 1)
                    {
                        //关联执行节点
                        asexecnode[0] = windspeedbindinfo[i].carmac[0];
                        asexecnode[1] = windspeedbindinfo[i].carmac[1];
                        asexecnode[2] = windspeedbindinfo[i].carid;
                        memcpy(asexecnode + 3, windspeedbindinfo[i].execstatus, 3);

                        aocexecnode(asexecnode);
                    }
                    //手动场景
                    else if(windspeedbindinfo[i].type == 4)
                    {
                        //挟持报警
                        //id
                        appdata[3] = windspeedbindinfo[i].carmac[0];
                        //执行手动场景
                        manualsceope(appdata);
                    }
                    break;
                }
            }
            //小于
            else if(0 == (windspeedbindinfo[i].triid & 0x80))
            {
                pr_debug("小于\n");
                if((windspeedbindinfo[i].triid & 0x7f) > buf[4])
                {
                    if(windspeedbindinfo[i].type == 1)
                    {
                        //关联执行节点
                        asexecnode[0] = windspeedbindinfo[i].carmac[0];
                        asexecnode[1] = windspeedbindinfo[i].carmac[1];
                        asexecnode[2] = windspeedbindinfo[i].carid;
                        memcpy(asexecnode + 3, windspeedbindinfo[i].execstatus, 3);

                        aocexecnode(asexecnode);
                    }
                    //手动场景
                    else if(windspeedbindinfo[i].type == 4)
                    {
                        //挟持报警
                        //id
                        appdata[3] = windspeedbindinfo[i].carmac[0];
                        //执行手动场景
                        manualsceope(appdata);
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
}

//一氧化碳关联
void carbonmoxdemansce(int slave, byte *buf)
{
    int i = 0;
    byte appdata[24] = {0x62,0x00,0x04};
    byte nodemac[6] = {0};
    byte asexecnode[12] = {0};
    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(carmoxdebindinfo[i].trimac[1] == 0)
        {
            pr_debug("carbonmoxdemansce check over:%d\n", i);
            break;
        }
        //匹配mac
        else if(0 == memcmp(carmoxdebindinfo[i].trimac, nodemac, 2))
        {
            if(carmoxdebindinfo[i].triid == buf[4])
            {
                if(carmoxdebindinfo[i].type == 1)
                {
                    //关联执行节点
                    asexecnode[0] = carmoxdebindinfo[i].carmac[0];
                    asexecnode[1] = carmoxdebindinfo[i].carmac[1];
                    asexecnode[2] = carmoxdebindinfo[i].carid;
                    memcpy(asexecnode + 3, carmoxdebindinfo[i].execstatus, 3);
                    
                    aocexecnode(asexecnode);
                }
                //手动场景
                else if(carmoxdebindinfo[i].type == 4)
                {
                    //挟持报警
                    //id
                    appdata[3] = carmoxdebindinfo[i].carmac[0];
                    //执行手动场景
                    manualsceope(appdata);
                }
                break;
            }
        }
    }
}

//被动式传感器信息
void passivesensorinfo(int slave, byte *buf)
{
    int type = 0;
    byte nodeinfo[12] = {0};
    byte appsend[24] = {0xf0,0x00,0x09};

    type = nodedev[buf[2]].nodeinfo[2];
    
    appsend[3] = slave;
    appsend[4] = buf[2];
    appsend[5] = nodedev[buf[2]].nodeinfo[2];
    memcpy(appsend + 6, buf + 3, 3);

    //更新注册信息
    //status
    nodedev[buf[2]].nodeinfo[5] = buf[4];
    //电量
    nodedev[buf[2]].nodeinfo[6] = buf[5];

    //门磁
    if(type == 0x51)
    {
        if(buf[3] == 0x01)
        {
            //安防
            nodeinfo[0] = slave;
            nodeinfo[1] = buf[2];
            nodeinfo[4] = buf[4];
            smartsecuritycheck(0x51, nodeinfo);

            //门磁关联
            doormagnetmansce(slave, buf);

            //更新自动场景触发条件
            nodeinfo[3] = buf[4];
            refreshconditions(0x51, nodeinfo);
        }
    }
    else if(type == 0x52)
    {
        //水浸
        if(buf[3] == 0x01)
        {
            if(secmonitor[6].enable == 1)
            {
                //安防
                nodeinfo[0] = slave;
                nodeinfo[1] = buf[2];
                nodeinfo[4] = buf[4];
                checkalarmcond(6, 0x52, nodeinfo);
            }
            //水浸关联
            floodingmansce(slave, buf);
        }
    }
    else if(type == 0x53)
    {
        //红外人体感应
        if(buf[3] == 0x01)
        {
            //安防
            nodeinfo[0] = slave;
            nodeinfo[1] = buf[2];
            nodeinfo[4] = buf[4];
            //安防检查
            smartsecuritycheck(7, nodeinfo);
            //更新自动场景条件
            refreshconditions(7, nodeinfo);

            //红外活动侦测关联
            iractivitymansce(slave, buf);
        }
    }
    else if(type == 0x54)
    {
        if(buf[3] == 0x01)
        {
            if(secmonitor[4].enable == 1)
            {
                //安防
                nodeinfo[0] = slave;
                nodeinfo[1] = buf[2];
                nodeinfo[4] = buf[4];
                checkalarmcond(4, 0x54, nodeinfo);
            }
            //烟雾
            smokepermansce(slave, buf);
        }
    }
    else if(type == 0x55)
    {
        if(buf[3] == 0x01)
        {
            //雨雪关联
            rainsnowmansce(slave, buf);
        }
    }
    else if(type == 0x56)
    {
        if(buf[3] == 0x01)
        {
            if(secmonitor[7].enable == 1)
            {
                //安防
                nodeinfo[0] = slave;
                nodeinfo[1] = buf[2];
                nodeinfo[4] = buf[4];
                checkalarmcond(7, 0x56, nodeinfo);
            }
            //燃气关联
            gasmansce(slave, buf);
        }
    }
    else if(type == 0x57)
    {
        if(buf[3] == 0x01)
        {
            //风速关联
            windspeedmansce(slave, buf);
        }
    }
    else if(type == 0x58)
    {
        if(buf[3] == 0x01)
        {
            if(secmonitor[8].enable == 1)
            {
                //安防
                nodeinfo[0] = slave;
                nodeinfo[1] = buf[2];
                nodeinfo[4] = buf[4];
                checkalarmcond(8, 0x58, nodeinfo);
            }
            //一氧化碳关联
            carbonmoxdemansce(slave, buf);
        }
    }
    UartToApp_Send(appsend, appsend[2]);
}

//智能锁检查挟持
void smartlockreserved(int slave, byte *buf)
{
    //检查挟持报警是否打开
    if(0 == secmonitor[5].enable)
    {
        pr_debug("挟持未使能\n");
        return;
    }

    int i = 0;
    byte node[8] = {0};
    node[0] = slave;
    node[1] = buf[2];

    byte appsend[32] = {0xf0,0x00,0x0f};
    //mac
    appsend[3] = slave;
    appsend[4] = buf[2];
    //node type
    appsend[5] = nodedev[buf[2]].nodeinfo[2];
    ///data type
    appsend[6] = 0x0a;
    //value
    appsend[7] = 0x07;

    if((buf[8] > 0) && (buf[8] < 0xff))
    {
        for(i = 0; i < 260; i++)
        {
            if(smartlockinfo[i].lockuserid == 0)
            {
                pr_debug("smartlockreserved check over:%d\n", i);
                break;
            }
            //匹配id
            else if(buf[8] == smartlockinfo[i].lockuserid)
            {
                //是否开启挟持
                if(smartlockinfo[i].reserved == 1)
                {
                    //匹配mac
                    if(0 == memcmp(smartlockinfo[i].mac, appsend + 3, 2))
                    {
                        //挟持报警
                        node[2] = buf[8];
                        checksecurityalarm(5, node);
                        //id
                        appsend[8] = buf[8];
                        UartToApp_Send(appsend, appsend[2]);
                        break;
                    }
                }
            }
        }
    }
    if((buf[10] > 0) && (buf[10] < 0xff))
    {
        for(i = 0; i < 260; i++)
        {
            if(smartlockinfo[i].lockuserid == 0)
            {
                pr_debug("smartlockreserved check over:%d\n", i);
                break;
            }
            //匹配id
            else if(buf[10] == smartlockinfo[i].lockuserid)
            {
                //是否开启挟持
                if(smartlockinfo[i].reserved == 1)
                {
                    //匹配mac
                    if(0 == memcmp(smartlockinfo[i].mac, appsend + 3, 2))
                    {
                        //挟持报警
                        node[2] = buf[10];
                        checksecurityalarm(5, node);
                        //id
                        appsend[8] = buf[10];
                        UartToApp_Send(appsend, appsend[2]);
                        break;
                    }
                }
            }
        }
    }
}

//智能锁关联检查执行手动场景
void smartlockpermansce(int slave, byte *buf)
{
    int i = 0;
    byte nodemac[6] = {0};
    byte appdata[24] = {0x62,0x00,0x04};

    nodemac[0] = slave;
    nodemac[1] = buf[2];

    for(i = 0; i < 260; i++)
    {
        if(smartlockbindinfo[i].triid == 0)
        {
            pr_debug("smartlockreserved check over:%d\n", i);
            break;
        }
        //匹配id
        else if(buf[8] == smartlockbindinfo[i].triid)
        {
            //匹配mac
            if(0 == memcmp(smartlockbindinfo[i].trimac, nodemac, 2))
            {
                //id
                appdata[3] = smartlockbindinfo[i].carmac[0];
                //执行手动场景
                manualsceope(appdata);
            }
        }
        //匹配id
        else if(buf[10] == smartlockbindinfo[i].triid)
        {
            //匹配mac
            if(0 == memcmp(smartlockbindinfo[i].trimac, nodemac, 2))
            {
                //id
                appdata[3] = smartlockbindinfo[i].carmac[0];
                //执行手动场景
                manualsceope(appdata);
            }
        }
    }
}

//检测智能锁用户是否注册
void checksmartlockuserreg(int slave, byte *buf)
{
    int i = 0;
    int mark = 0;
    byte lockuser[36] = {0};
    byte savestr[72] = {0};
    byte appsend[32] = {0xf0,0x00,0x0f};
    //mac
    lockuser[0] = slave;
    lockuser[1] = buf[2];

    if((buf[8] > 0) && (buf[8] < 0xff))
    {
        //开锁方式
        lockuser[2] = buf[5];
        //用户权限
        if(buf[8] < 6)
        {
            lockuser[3] = 0x01;
        }
        else
        {
            lockuser[3] = 0x00;
        }
        //id
        lockuser[5] = buf[8];

        for(i = 0; i < 260; i++)
        {
            if(0 == smartlockinfo[i].mac[0])
            {
                mark = 1;
                break;
            }
            else if(0 == memcmp(smartlockinfo[i].mac, lockuser, 2))
            {
                if(smartlockinfo[i].lockuserid == lockuser[5])
                {
                    break;
                }
            }
        }
        if(mark == 1)
        {
            //存
            HexToStr(savestr, lockuser, 9);
            replace(SMARTLOCK, savestr, 12);

            //mac
            appsend[3] = slave;
            appsend[4] = buf[2];
            //node type
            appsend[5] = nodedev[buf[2]].nodeinfo[2];
            //data type & value
            appsend[6] = 0x20;
            appsend[7] = 0x01;
            appsend[8] = lockuser[2];
            appsend[9] = lockuser[3];
            appsend[11] = lockuser[5];
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    if((buf[10] > 0) && (buf[10] < 0xff))
    {
        //开锁方式
        lockuser[2] = buf[6];
        //用户权限
        if(buf[10] < 6)
        {
            lockuser[3] = 0x01;
        }
        else
        {
            lockuser[3] = 0x00;
        }
        //id
        lockuser[5] = buf[10];

        for(i = 0; i < 260; i++)
        {
            if(0 == smartlockinfo[i].mac[0])
            {
                mark = 2;
                break;
            }
            else if(0 == memcmp(smartlockinfo[i].mac, lockuser, 2))
            {
                if(smartlockinfo[i].lockuserid == lockuser[5])
                {
                    break;
                }
            }
        }
        if(mark == 2)
        {
            //存
            HexToStr(savestr, lockuser, 9);
            replace(SMARTLOCK, savestr, 12);
            
            //mac
            appsend[3] = slave;
            appsend[4] = buf[2];
            //node type
            appsend[5] = nodedev[buf[2]].nodeinfo[2];
            //data type & value
            appsend[6] = 0x20;
            appsend[7] = 0x01;
            appsend[8] = lockuser[2];
            appsend[9] = lockuser[3];
            appsend[11] = lockuser[5];
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    if(mark > 0)
    {
        readsmartlockinfo();
    }
}

//智能锁信息
void smartlock(int slave, byte *buf)
{
    byte appsend[32] = {0xf0,0x00,0x0f};
    byte savehex[24] = {0};
    byte savestr[48] = {0};
    byte appbuf[12] = {0x42,0x00,0x05};
    byte nodelock[24] = {0};

    //mac
    appsend[3] = slave;
    appsend[4] = buf[2];
    //node type
    appsend[5] = nodedev[buf[2]].nodeinfo[2];
    ///data type & value
    memcpy(appsend + 6, buf + 3, 9);

    if(buf[3] == 0x20)
    {
        pr_debug("添加/删除智能锁ID\n");
        //mac
        savehex[0] = slave;
        savehex[1] = buf[2];
        //开锁方式
        //用户权限
        //透传
        //id
        memcpy(savehex + 2, buf + 5, 4);
        //填充3字节0，共9字节
        HexToStr(savestr, savehex, 9);
        
        //删除
        if(buf[4] == 0)
        {
            filecondel(SMARTLOCK, savestr, 12);
            
            //删除智能锁关联id
            //savestr[4] = savestr[10];
            //savestr[5] = savestr[11];
            //filecondel(LOCKRELATED, savestr, 6);
            //读取智能锁关联
            //readlockinfo();
        }
        //新增
        else if(buf[4] == 1)
        {
            replace(SMARTLOCK, savestr, 12);
        }
        readsmartlockinfo();
    }
    else if(buf[3] == 0x0a)
    {
        pr_debug("智能锁报警\n");
        if(buf[4] == 0x04)
        {
            //更新注册信息
            //status
            nodedev[buf[2]].nodeinfo[5] = 0x01;
            pr_debug("智能锁状态-开锁\n");
            
            //处理安防/场景-开锁
            //pr_debug("开锁:%d\n", lock_i);
            //lock_i++;
            
            nodelock[0] = slave;
            nodelock[1] = buf[2];
            nodelock[4] = 0x01;
            smartsecuritycheck(0x41, nodelock);

            //处理挟持
            smartlockreserved(slave, buf);
            //处理手动场景
            smartlockpermansce(slave, buf);

            //检测智能锁用户是否注册
            checksmartlockuserreg(slave, buf);
        }
    }
    else if(buf[3] == 0x21)
    {
        //更新注册信息
        //status
        nodedev[buf[2]].nodeinfo[5] = 0x00;
        
        pr_debug("智能锁状态-关锁\n");
        
        //处理安防/场景-关锁
        //pr_debug("关锁:%d\n", lock_g);
        //lock_g++;
        
        nodelock[0] = slave;
        nodelock[1] = buf[2];
        nodelock[4] = 0x00;
        smartsecuritycheck(0x41, nodelock);
    }
    else if(buf[3] == 0xa1)
    {
        pr_debug("智能锁退网\n");
        appbuf[3] = slave;
        appbuf[4] = buf[2];
        delnodedev(appbuf);
    }
    UartToApp_Send(appsend, appsend[2]);
}

//从机单片机特殊处理指令
void slavemicroproc(int slave, byte *buf)
{
    byte delhex[24] = {0};
    byte delslave[24] = {0};
    
    if(buf[3] == 0x05)
    {
        if(buf[4] == 0x05)
        {
            byte appsend[24] = {0x96,0x00,0x0e,0x02,0x0f};
            memcpy(appsend + 5, slaveident[slave].slavesn, 7);
            appsend[12] = 0x01;
            appsend[13] = 0x00;
            UartToApp_Send(appsend, appsend[2]);
            
            memcpy(delhex, slaveident[slave].slavesn, 7);
            HexToStr(delslave, delhex, 7);
            filecondel(MASSLAINFO, delslave, 12);
            sleep(1);

            pr_debug("delslave:%s\n", delslave);

            delslave[0] = delslave[12];
            delslave[1] = delslave[13];
            filecondel(MESHPW, delslave, 2);
            
            //从机恢复出厂-清除此从机信息
            slavenetworking(slave);

            memset(&slaveident[slave], 0, sizeof(slaveident[slave]));
            
            //
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    else
    {
        singlechipup(slave, buf);
    }
}

//从机心跳处理
int slaveheartbeatproc(byte *sockbuf)
{
    pr_debug("主机记录从机心跳时间\n");
    byte heartbeat[24] = {0xf2,0x00,0x06};
    //记录心跳发生时间
    time(&slaveident[sockbuf[3]].heartbeattime);

    //回复从机心跳
    hosttoslave(sockbuf[3], heartbeat, heartbeat[2]);
}

//从机指令处理
void slaveinproc(byte *buf)
{
    byte slaveinst[1024] = {0};
    memcpy(slaveinst, buf + 4, (((buf[1] << 8) + buf[2]) - 4));

    //记录心跳发生时间
    time(&slaveident[buf[3]].heartbeattime);
    
    switch(slaveinst[0])
    {
        case 0x01://节点信息注册
            pr_debug("节点注册!\n");
            searchnode(buf[3], slaveinst);
            break;
        case 0x02://节点状态反馈
            pr_debug("节点状态反馈!\n");
            statefeedback(buf[3], slaveinst);
            break;    
        case 0x03://节点数据反馈
            pr_debug("节点数据反馈!\n");
            datafeedback(buf[3], slaveinst);
            break;
        case 0x31://单片机上传
            pr_debug("单片机上传\n");
            slavemicroproc(buf[3], slaveinst);
            break;
        case 0x64://区域应答
        case 0x67:
        case 0x70:
            if(0 == memcmp(slaveinst, ackswer, 3))
            {
                //记录配置失败原因
                if(slaveinst[3] != 0)
                {
                    pr_debug("0x64应答配置失败:%0x\n", slaveinst[3]);
                    sceneack.badmac[sceneack.maclen] = buf[3];
                    sceneack.badmac[sceneack.maclen + 1] = slaveinst[2];
                    sceneack.badmac[sceneack.maclen + 2] = ackswer[3];
                    sceneack.badmac[sceneack.maclen + 3] = slaveinst[3];
                    sceneack.maclen += 4;
                }
                answer = 1;
            }
            break;
        case 0x74://初始节点信息
            pr_debug("初始节点信息!\n");
            savenoderawinfo(slaveinst);
            break;
        case 0x75://初始信息上传结束
            pr_debug("初始信息上传结束\n");
            distributionmac(buf[3]);
            break;
        case 0x77://分配mac应答
            pr_debug("分配mac应答!\n");
            if(0 == memcmp(ackswer, slaveinst + 2, 7))
            {
                answer = 1;
            }
            break;
        case 0x79://组网结束
            pr_debug("组网结束\n");
            meshnetover(slaveinst);
            break;
        case 0x7c://发送完整红外码应答
            pr_debug("发送完整红外码应答");
            completeirsendack(slaveinst);
            break;
        case 0x7d:
            pr_debug("存储红外快捷方式应答\n");
            if(0 == memcmp(ackswer, slaveinst, 3))
            {
                answer = 1;
                irquickoperaack(slaveinst);
            }
            break;
            /*
               case 0x7e://发送完整红外码应答
               memcpy(slaveinst, r_buf + temp, 7);
               len = 7;
               temp += 7;
               break;
               case 0x7f://发送完整红外码应答
               memcpy(slaveinst, r_buf + temp, 6);
               len = 6;
               temp += 6;
               break;
             */
        case 0x87:
            pr_debug("电机/负载数量转换\n");
            ctlboxconversion(buf[3], slaveinst);
            break;
        case 0x88:
            pr_debug("窗帘控制应答\n");
            curtainctlack(buf[3], slaveinst);
            break;
        case 0x89:
            smartlock(buf[3], slaveinst);
            break;
        case 0x8a:
            passivesensorinfo(buf[3], slaveinst);
            break;
        case 0x8b:
            break;
        case 0x8c:
            smartctrpanelanswer(buf[3],slaveinst);
            break;
        case 0xe2:
            pr_debug("mesh升级应答");
            if(slaveinst[3] == 1)
            {
                answer = 2;
            }
            else if(slaveinst[3] == 0)
            {
                pr_debug("mesh升级成功\n");
            }
            break;
        case 0xf2:
            //从机心跳
            slaveheartbeatproc(buf);
            break;
        case 0xff:
            pr_debug("超时应答\n");
            answer = 2;
            break;
        default:
            pr_debug("Serial_run unsupported type\n");
            break;
    }
}

//从机透传指令处理
void slavepasscomm(byte *buf)
{
    byte slaveinst[1024] = {0};
    memcpy(slaveinst, buf + 4, (((buf[1] << 8) + buf[2]) - 4));

    //记录心跳发生时间
    time(&slaveident[buf[3]].heartbeattime);

    UartToApp_Send(slaveinst, slaveinst[2]);
}

//智能控制面板应答
void smartctrpanelanswer(int slave, byte *buf)
{
    byte appsend[24] = {0xb7,0x00,0x0e,0x01};
    byte machex[12] = {0};
    byte macstr[24] = {0};

    appsend[3] = buf[11];
    appsend[4] = slave;
    memcpy(appsend + 5, buf + 2, 3);
    
    if((buf[4] == 0x03) || (buf[4] == 0x04))
    {
        appsend[8] = buf[5];
        memcpy(machex, appsend + 4, 10);
        HexToStr(macstr, machex, 10);
    }
    else
    {
        appsend[8] = nodedev[buf[5]].nodeinfo[0];
        memcpy(appsend + 9, buf + 5, 6);
        memcpy(machex, appsend + 4, 10);
        HexToStr(macstr, machex, 10);
    }
    if(buf[11] == 0)
    {
        //成功
        if(buf[4] == 0)
        {
            //清除该键任务
            filecondel(SMARTCRLPANEL, macstr, 6);
            //重新读取
            readsmartctlpanel();
        }
        else if(buf[4] == 0xff)
        {
            //清除所有按键任务
            system("rm -f /data/modsty/smartcrlpanel");
            //清除内存中的按键信息
            memset(&smartcrlpanel, 0, sizeof(smartcrlpanel));
        }
        else
        {
            //新建/修改按键任务
            replace(SMARTCRLPANEL, macstr, 6);
            //重新读取
            readsmartctlpanel();
        }
    }
    UartToApp_Send(appsend, appsend[2]);
}
