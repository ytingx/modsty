#include "mesh.h"
#include "struct.h"

extern int uart_fd;
extern int new_fd;
extern int master;
extern byte newnodemac[300];

//存储节点原始信息
void saveorignode(byte *rawmac)
{
    byte orignodestr[30] = {0};
    
    HexToStr(orignodestr, rawmac, 7);
    
    orignodestr[14] = '\n';
    InsertLine(ORIGNODE, orignodestr);
}

//读取节点原始信息
int readorignode()
{
    FILE *fd = NULL;
    int count = 0;
    byte nodestr[30] = {0};

    if((fd = fopen(ORIGNODE, "r")) == NULL)
    {
        pr_debug("%s: %s\n", ORIGNODE, strerror(errno));
        return -1;
    }
    memset(orinode, 0, sizeof(orinode));
    while(fgets(nodestr, 30, fd))
    {
        StrToHex(orinode[count].originfo, nodestr, strlen(nodestr) / 2);
        count++;
    }
    fclose(fd);
    fd = NULL;
    return count;
}

//检索节点原始信息
int checkorignode(byte *rawmac, byte *ackmac)
{
    int i = 0;
    int g = 0;
    for(i = 0; i < 300; i++)
    {
        /*
        for(g = 0; g < 8; g++)
        {
            pr_debug("rawmac[%d]:%0x\n", g, rawmac[g]);
        }
        for(g = 0; g < 8; g++)
        {
            pr_debug("orinode[%d].originfo[%d]:%0x\n", i, g, orinode[i].originfo[g]);
        }
        */
        
        if(0 == memcmp(rawmac + 1, orinode[i].originfo + 1, 6))
        {
            ackmac[0] = orinode[i].originfo[0];
            return -1;
        }
        else if(0 == orinode[i].originfo[0])
        {
            memcpy(orinode[i].originfo, rawmac, 7);
            return 0;
        }
    }
    return 0;
}

//节点原始信息
void savenoderawinfo(byte *rawmac)
{
    int i = 0;
    byte macinfo[10] = {0};
    while(i < 300)
    {
        if(0 == memcmp(nodemac[i].macinfo, rawmac + 2, 7))
        {
            memcpy(nodemac[i].macinfo, rawmac + 2, 7);
            break;
        }
        else if(0x00 == nodemac[i].macinfo[0])
        {
            memcpy(nodemac[i].macinfo, rawmac + 2, 7);
            break;
        }
        i++;
    }
}

//分配节点mac
void distributionmac(int slaveident)
{
    int i = 0;
    int meshcount = 0;
    byte appbuf[10] = {0x42,0x00,0x05};
    int mac = 0;
    byte allocmac[5] = {0};
    byte fillingnode[15] = {0x01,0x0d,0x00,0xff};
    byte newmac[15] = {0x76,0x0b};
    byte node[10] = {0x73,0x05,0x00,0x00,0x00};
    byte orinode[10] = {0};
    //byte getmeshpw[12] = {0x8b,0x0b,0x01};
    //UART0_Send(uart_fd, getmeshpw, getmeshpw[1]);

    //pr_debug("getmeshpw\n");
    //sleep(10);

    //fillingnode[0] = master;
    
    //读取节点原始信息
    readorignode();

    //组网标识
    hostinfo.mesh = 1;
    while(i < 260)
    {
        if(0 != nodemac[i].macinfo[0])
        {
            //检查有无重复配置
            if(-1 == checkorignode(nodemac[i].macinfo, allocmac))
            {
                i++;
                pr_debug("重复节点初始信息\n");
                appbuf[3] = slaveident;
                appbuf[4] = allocmac[0];
                delnodedev(appbuf);
                continue;
            }
            mac = allocatemac();
            pr_debug("mac:%d\n", mac);

            fillingnode[2] = mac;

            //地址已分配
            searchnode(slaveident, fillingnode);
            
            newmac[2] = nodemac[i].macinfo[0];
            newmac[3] = mac;
            memcpy(newmac + 4, nodemac[i].macinfo + 1, 6);

            //原始信息存储
            memcpy(orinode, newmac + 3, 7);
            saveorignode(orinode);

            UART0_Send(uart_fd, newmac, 11);
            
            meshcount++;
        }
        else
        {
            break;
        }
        i++;
    }
    //清除临时数据
    memset(nodemac, 0, sizeof(nodemac));
    memset(orinode, 0, sizeof(orinode));

    //循环获取初始节点信息
    pr_debug("mac over i:%d\n", i);
    if((i > 0) && (meshcount > 0))
    {
        pr_debug("循环获取节点初始信息\n");
        UART0_Send(uart_fd, node, 5);
    }
    else
    {
        if(hostinfo.slavemesh == 1)
        {
            byte mesh[10] = {"mesh0"};
            //hostinfo.mesh = 1;
            replace(HOSTSTATUS, mesh, 4);
        }
        pr_debug("开始组网\n");
        meshnetworking(slaveident);
    }
}

//开启mesh组网
void meshnetworking(int slaveident)
{
    int i = 6;
    byte slavemesh[12] = {0};
    byte slavestr[24] = {0};

    byte meshname[20] = {0x78,0x0b,0x6d,0x6f,0x64,0x73};
    byte meshstr[20] = {0};
    byte meshpw[20] = {0};
    byte mesh[20] = {0};

    slavemesh[0] = slaveident;
    HexToStr(slavestr, slavemesh, 1);
    meshstr[0] = slavestr[0];
    meshstr[1] = slavestr[1];

    memset(newnodemac, 0, sizeof(newnodemac));

    if(0 != filefind(slavestr, meshpw, MESHPW, 2))
    {
        //生成随机meshpw
        srand((unsigned)time(NULL));
        for(i = 6; i < 9; i++)
        {
            meshname[i] = rand() % 51 + 50;
        }
        HexToStr(meshpw, meshname, 9);
        memcpy(meshstr + 2, meshpw + 4, 14);
        meshstr[16] = '\n';
        InsertLine(MESHPW, meshstr);
        //进入缓冲区11
        UART0_Send(uart_fd, meshname, 11);
    }
    else
    {
        StrToHex(mesh, meshpw, 8);
        memcpy(meshname + 6, mesh + 5, 3);
        //进入缓冲区11
        UART0_Send(uart_fd, meshname, 11);
    }
}

/*
//关闭mesh组网
void closemeshnet()
{
    int i = 6;
    byte meshname[20] = {0x7a,0x0b,0x6d,0x6f,0x64,0x73};
    byte meshstr[20] = {0};
    byte meshpw[20] = {0};
    byte mesh[20] = {0};

    if(0 != filefind("6D6F6473", meshpw, MESHPW, 8))
    {
        //生成随机meshpw
        srand((unsigned)time(NULL));
        for(i = 6; i < 9; i++)
        {
            meshname[i] = rand() % 51 + 50;
        }
        HexToStr(meshpw, meshname, 9);
        memcpy(meshstr, meshpw + 4, 14);
        InsertLine(HOSTSTATUS, meshstr);
        //进入缓冲区11
        UART0_Send(uart_fd, meshname, 11);
    }
    else
    {
        StrToHex(mesh, meshpw, 7);
        memcpy(meshname + 6, mesh + 4, 3);
        //进入缓冲区11
        UART0_Send(uart_fd, mesh, 11);
    }
}

//解散mesh网络
void disbandedmesh()
{
    int i = 6;

    byte slavemesh[12] = {0};
    byte slavestr[24] = {0};

    byte meshname[20] = {0x7b,0x0b,0x6d,0x6f,0x64,0x73};
    byte meshstr[20] = {0};
    byte meshpw[20] = {0};
    byte mesh[20] = {0};

    slavemesh[0] = slaveident;
    HexToStr(slavestr, slavemesh, 1);
    meshstr[0] = slavestr[0];
    meshstr[1] = slavestr[1];

    if(0 != filefind(slavestr, meshpw, MESHPW, 2))
    {
        //生成随机meshpw
        srand((unsigned)time(NULL));
        for(i = 6; i < 9; i++)
        {
            meshname[i] = rand() % 51 + 50;
        }
        HexToStr(meshpw, meshname, 9);
        memcpy(meshstr + 2, meshpw + 4, 14);
        meshstr[16] = '\n';
        InsertLine(MESHPW, meshstr);
        //进入缓冲区11
        UART0_Send(uart_fd, meshname, 11);
    }
    else
    {
        StrToHex(mesh, meshpw, 8);
        memcpy(meshname + 6, mesh + 5, 3);
        //进入缓冲区11
        UART0_Send(uart_fd, mesh, 11);
    }
}
*/

//生成安防信息
void generatesecinfo()
{
    //byte securityinfo[512] = {"0100000000000000000000000000000000000000000000000000000000\n0200000000000000000000000000000000000000000000000000000000\n"};
    byte audiblealarmsec[1024] = {"010000000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n020000000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n030100000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n040100000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n060100000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n070100000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n080100000000000000000000000000000000000000000000000000000a0f010107000fffff0800\n"};
    byte holdalarmsec[1024] = {"05010000000000000000000000000000000000000000000000000000050f01010700\n"};
    if(-1 == access("/mnt/mtd/secmonitor", F_OK))
    {
        //InsertLine(SECMONITOR, securityinfo);
        InsertLine(SECMONITOR, audiblealarmsec);
        InsertLine(SECMONITOR, holdalarmsec);
    }
}

//组网结束
void meshnetover(byte *buf)
{
    byte appsend[10] = {0x93,0x00,0x05,0x01,0x00};

    byte reghex[1024] = {0x04,0xff,0xff,0x04};
    byte regstr[2048] = {0};
    int i = 2;
    int offset = 6;
    hostinfo.dataversion += 1;
    //首次组网
    if(regcontrol[4].id == 0)
    {
        //生成默认区域1234
        for(i = 2; i < 260; i++)
        {
            if(nodedev[i].nodeinfo[2] == 0)
            {
                pr_debug("节点信息同步-删除保留mac\n");
            }
            else if(nodedev[i].nodeinfo[2] == 0xff)
            {
                pr_debug("节点信息同步-分配保留mac\n");
            }
            else if((nodedev[i].nodeinfo[2] > 0x00) && (nodedev[i].nodeinfo[2] < 0x0f))
            {
                reghex[offset] = 0x01;
                reghex[offset + 1] = nodedev[i].nodeinfo[0];
                reghex[offset + 2] = nodedev[i].nodeinfo[1];
                reghex[offset + 3] = 0x55;
                reghex[offset + 4] = 0x00;
                offset += 8;
            }
        }
        reghex[4] = (((offset - 6) >> 8) & 0xff);
        reghex[5] = ((offset - 6) & 0xff);
        offset += 18;
    }
    //多次组网
    else
    {
        int j = 0;
        offset = 0;
        memset(reghex, 0, sizeof(reghex));
        for(j = 0; j < 260; j++)
        {
            pr_debug("newnodemac[%d]:%0x\n", j, newnodemac[j]);
            if(newnodemac[j] == 0)
            {
                break;
            }
            reghex[offset] = 0x01;
            reghex[offset + 1] = nodedev[newnodemac[j]].nodeinfo[0];
            reghex[offset + 2] = newnodemac[j];
            reghex[offset + 3] = 0x55;
            reghex[offset + 4] = 0x00;
            offset += 8;
        }
        pr_debug("regcontrol[4].tasklen:%d\n", regcontrol[4].tasklen);
        pr_debug("offset:%d\n", offset);
        //regcontrol[4].tasklen += offset;
        memcpy(regcontrol[4].regtask + regcontrol[4].tasklen, reghex, offset);
        regcontrol[4].tasklen += offset;
        //
        offset = 0;
        memset(reghex, 0, sizeof(reghex));

        //id
        reghex[offset] = regcontrol[4].id;
        offset += 1;
        //绑定按键mac
        reghex[offset] = regcontrol[4].bingkeymac[0];
        reghex[offset + 1] = regcontrol[4].bingkeymac[1];
        offset += 2;
        //绑定按键id
        reghex[offset] = regcontrol[4].bingkeyid;
        offset += 1;
        //执行任务长度
        reghex[offset] = ((regcontrol[4].tasklen >> 8) & 0xff);
        reghex[offset + 1] = (regcontrol[4].tasklen & 0xff);
        offset += 2;
        //执行任务
        memcpy(reghex + offset, regcontrol[4].regtask, regcontrol[4].tasklen);
        offset += regcontrol[4].tasklen;
        //区域名称长度
        reghex[offset] = regcontrol[4].namelen;
        offset += 1;
        //区域名称
        memcpy(reghex + offset, regcontrol[4].regname, 16);
        offset += 16;
        //区域图标
        reghex[offset] = regcontrol[4].iconid;
        offset += 1;
        //
    }
    HexToStr(regstr, reghex, offset);
    replace(REGASSOC, regstr, 2);
    sleep(1);
    memset(newnodemac, 0, sizeof(newnodemac));

    //readareainfo();
    singlereadareainfo(reghex);
    
    if(hostinfo.slavemesh == 1)
    {
        hostinfo.mesh = 0;
    }
    else
    {
        byte configstr[64] = {0};
        //mesh状态
        if(0 == filefind("mesh", configstr, HOSTSTATUS, 4))
        {
            hostinfo.mesh = configstr[4] - '0';
        }
        else
        {
            hostinfo.mesh = 2;
        }
    }

    //生成安防信息
    generatesecinfo();
    //读取安防信息
    readsecurityconfig(1, "1");

    UartToApp_Send(appsend, 5);
}
