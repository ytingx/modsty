#include "appfunction.h"
#include "struct.h"

extern int readlogo;
extern int uart_fd;
extern int g_heartbeat;
extern int softwareversion[10];
extern int answer;
extern int client_fd;
extern int single_fd;
extern int new_fd;

//从机登录验证
int slavelogincheck(int fd, byte *buf)
{
    if(0 == memcmp(buf + 4, slaveident[buf[10]].slavesn, 6))
    {
        if(0 == memcmp(buf + 11, hostinfo.hostsn, 6))
        {
            slaveident[buf[10]].sockfd = fd;
            //版本号
            memcpy(slaveident[buf[10]].version , buf + 18, 4);

            //记录心跳/数据发生时间
            time(&slaveident[buf[10]].heartbeattime);

            send(fd, buf, buf[2], MSG_NOSIGNAL);
            return 0;
        }
    }
    return -1;
}

//帐号检索
int accountcheck(int *authority, byte *account)
{
    int i = 1;
    int ai = 0;
    byte accountpw[64] = {0};
    byte tempaccountpw[64] = {0};

    memcpy(accountpw, account + 3, account[2] - 3);
    pr_debug("hostinfo.bindhost:%d\n", hostinfo.bindhost);
    //未绑定主机-只验证密码
    if(0 == hostinfo.bindhost)
    {
        memcpy(tempaccountpw, accountpw + (accountpw[0] + 2), accountpw[accountpw[0] + 1]);
        
        pr_debug("accinfo[0].pwlen:%d\n", accinfo[0].pwlen);
        for(ai = 0; ai < accinfo[0].pwlen; ai++)
        {
            pr_debug("accinfo[0].password[%d]:%0x\n", ai, accinfo[0].password[ai]);
            pr_debug("tempaccountpw[%d]:%0x\n", ai, tempaccountpw[ai]);
        }
        pr_debug("accountpw[%d]:%d\n", accountpw[0] + 1, accountpw[accountpw[0] + 1]);

        //验证密码长度
        if(accinfo[0].pwlen == accountpw[accountpw[0] + 1])
        {
            if(0 == memcmp(accinfo[0].password, tempaccountpw, accinfo[0].pwlen))
            {
                *authority = 1;
                return 0;
            }
            else
            {
                return 2;
            }
        }
        else
        {
            return 2;
        }
    }
    //验证帐号密码
    else
    {
        for(i = 1; i < 20; i++)
        {
            pr_debug("accinfo[%d].accountlen:%0x\n", i, accinfo[i].accountlen);
            pr_debug("accountpw:%0x\n", accountpw[0]);
            if(accinfo[i].accountlen == 0)
            {
                pr_debug("验证帐号密码完毕\n");
                return 2;
            }
            //对比帐号长度
            else if(accinfo[i].accountlen == accountpw[0])
            {
                pr_debug("对比帐号\n");
                //对比帐号
                if(0 == memcmp(accinfo[i].accountinfo, accountpw + 1, accountpw[0]))
                {
                    //对比密码
                    pr_debug("对比密码\n");
                    pr_debug("accinfo[%d].pwlen:%0x\n", i, accinfo[i].pwlen);
                    int gi = 0;
                    for(gi = 0; gi < accinfo[i].pwlen; gi++)
                    {
                        pr_debug("accinfo[%d].password[%d]:%0x\n", i, gi, accinfo[i].password[gi]);
                        pr_debug("accountpw[%d]:%0x\n", accountpw[0] + 2 + gi, accountpw[accountpw[0] + 2 + gi]);
                    }
                    pr_debug("#####################################\n");
                    //对比密码长度
                    if(accinfo[i].pwlen == accountpw[accountpw[0] + 1])
                    {
                        if(0 == memcmp(accinfo[i].password, accountpw + (accountpw[0] + 2), accinfo[i].pwlen))
                        {
                            *authority = accinfo[i].authority;
                            return 0;
                        }
                        else
                        {
                            return 2;
                        }
                    }
                    else
                    {
                        return 2;
                    }
                }
            }
        }
    }
    return 2;
}

//修改密码 0x11
void changepassword(byte *buf)
{
    int authority = 0;
    byte appsend[32] = {0x81,0x00,0x04,0x00};
    
    byte bindhoststr[64] = {"0561646D696E"};
    byte newpw[20] = {0};
    byte newacc[60] = {20};

    byte newpwstr[40] = {0};
    byte newaccstr[120] = {0};

    memcpy(newpw, buf + (5 + buf[3] + buf[4 + buf[3]]), buf[5 + buf[3] + buf[4 + buf[3]]] + 1);
    
    HexToStr(newpwstr, newpw, buf[5 + buf[3] + buf[4 + buf[3]]] + 1);
    pr_debug("newpwstr:%s\n", newpwstr);
    pr_debug("newpwlen:%d\n", buf[5 + buf[3] + buf[4 + buf[3]]] + 1);
    memcpy(newacc, buf + 3, buf[3] + 1);

    HexToStr(newaccstr, newacc, buf[3] + 1);
    pr_debug("newaccstr:%s\n", newaccstr);

    strcat(newaccstr, newpwstr);
    pr_debug("newaccstr:%s\n", newaccstr);

    if(0 == accountcheck(&authority, buf))
    {
        //未绑定主机-只验证密码
        if(0 == hostinfo.bindhost)
        {
            strcat(bindhoststr, newpwstr);
            bindhoststr[strlen(bindhoststr)] = '0';
            bindhoststr[strlen(bindhoststr)] = '1';
            replace(INITIALPW, bindhoststr, 12);
        }
        else
        {
            if(authority == 0)
            {
                newaccstr[strlen(newaccstr)] = '0';
                newaccstr[strlen(newaccstr)] = '0';
            }
            else if(authority == 1)
            {
                newaccstr[strlen(newaccstr)] = '0';
                newaccstr[strlen(newaccstr)] = '1';
            }
            replace(ACCOUNTPW, newaccstr, (buf[3] + 1) * 2);
        }
    }
    else
    {
        appsend[3] = 0x01;
    }
    if(appsend[3] == 0x00)
    {
        readaccount();
    }
    sendsingle(new_fd, appsend, appsend[2]);
}

//绑定主机0x12
void bindhost(byte *buf)
{
    byte hoststr[50] = {0};
    int authority = 0;
    byte bufstr[100] = {0};
    byte host[20] = {"bindhost01"};
    byte appsend[10] = {0x82,0x00,0x04,0x00};
    byte savehex[50] = {0};
    
    memcpy(savehex, buf + 3, buf[2] - 3);
    savehex[buf[2] - 3] = 0x01;
    HexToStr(bufstr, savehex, buf[2] - 2);

    if(0 == accountcheck(&authority, buf))
    {
        hostinfo.bindhost = 1;

        if(0 == filefind("bindhost", hoststr, HOSTSTATUS, 8))
        {
            if(hoststr[9] == '0')
            {
                replace(ACCOUNTPW, bufstr, (buf[3] + 1) * 2);
                replace(HOSTSTATUS, host, 8);
            }
            else if(hoststr[9] == '1')
            {
                appsend[3] = 0x01;
            }
        }
        else
        {
            replace(ACCOUNTPW, bufstr, (buf[3] + 1) * 2);
            replace(HOSTSTATUS, host, 8);
        }
        //获取帐号信息
        readaccount();
    }
    else
    {
        appsend[3] = 0x01;
    }
    sendsingle(new_fd, appsend, appsend[2]);
}

//绑定主机强制0x13
void forcedbindhost(byte *buf)
{
    int i = 0;
    byte hoststr[50] = {0};
    byte bufstr[100] = {0};
    byte host[20] = {"bindhost01"};
    byte appsend[10] = {0x83,0x00,0x04,0x00};
    byte savehex[50] = {0};
    
    memcpy(savehex, buf + 3, buf[2] - 3);
    savehex[buf[2] - 3] = 0x01;
    HexToStr(bufstr, savehex, buf[2] - 2);

    timers[0].startlogo = 1;
    while(i < 5)
    {
        sleep(1);
        if(timers[0].triggerlogo == 1)
        {
            break;
        }
        i++;
    }
    if(timers[0].triggerlogo == 1)
    {
        system("rm -f /data/modsty/accountpw");
        usleep(100000);
        replace(ACCOUNTPW, bufstr, (buf[3] + 1) * 2);
        replace(HOSTSTATUS, host, 8);
        usleep(200000);
        
        //获取帐号信息
        readaccount();
        hostinfo.bindhost = 1;

    }
    else
    {
        appsend[3] = 1;
    }
    
    memset(&timers[0], 0, sizeof(timers[0]));
    
    pr_debug("sizeof(timers[0]):%d\n", sizeof(timers[0]));

    pr_debug("timers[0].startlogo:%d\n", timers[0].startlogo);
    
    sendsingle(new_fd, appsend, appsend[2]);
}

//解绑主机0x14
void unbindhost(byte *buf)
{
    int authority = 0;
    byte appsend[10] = {0x84,0x00,0x04,0x00};
    byte hostbind[20] = {"bindhost00"};
    
    replace(HOSTSTATUS, hostbind, 8);
    hostinfo.bindhost = 0;
    
    //重新生成主机登录许可
    loginpermission();

    system("rm /data/modsty/accountpw");
    readaccount();
    sendsingle(new_fd, appsend, appsend[2]);
}

//添加子帐号 0x15
void addaubaccount(byte *buf)
{
    int i = 0;
    byte aubaccount[40] = {0};
    byte aubaccountstr[80] = {0};
    byte appsend[10] = {0x85,0x00,0x04,0x00};

    memcpy(aubaccount, buf + 3, buf[2] - 3);
    aubaccount[buf[2] - 3] = 0x02;
    HexToStr(aubaccountstr, aubaccount, buf[2] - 2);

    while(i < 20)
    {
        if(accinfo[i].accountlen == 0)
        {
            break;
        }
        i++;
    }

    if(-1 == contains(aubaccountstr, (buf[3] + 1) * 2, ACCOUNTPW))
    {
        appsend[3] = 0x01;
    }
    else if(i >= 12)
    {
        appsend[3] = 0x02;
    }
    else
    {
        if(-1 == replace(ACCOUNTPW, aubaccountstr, (buf[3] + 1) * 2))
        {
            appsend[3] = 0x03;
        }
    }
    if(appsend[3] == 0x00)
    {
        readaccount();
    }
    sendsingle(new_fd, appsend, appsend[2]);
}

//删除子帐号
void deletesubaccount(byte *buf)
{
    byte aubaccount[40] = {0};
    byte aubaccountstr[80] = {0};
    byte appsend[10] = {0x86,0x00,0x04,0x00};

    memcpy(aubaccount, buf + 3, buf[2] - 3);
    HexToStr(aubaccountstr, aubaccount, buf[2] - 3);

    if(-1 == filecondel(ACCOUNTPW, aubaccountstr, (buf[3] + 1) * 2))
    {
        appsend[3] = 0x01;
    }
    if(appsend[3] == 0x00)
    {
        readaccount();
    }
    sendsingle(new_fd, appsend, appsend[2]);
}

//获取子帐号信息
void getsubaccount(byte *buf)
{
    int i = 1;
    int offset = 4;
    int count = 0;
    byte appsend[128] = {0x87,0x00};
    for(i = 1; i < 20; i++)
    {
        if(accinfo[i].accountlen == 0)
        {
            break;
        }
        else if(accinfo[i].authority == 1)
        {
            continue;
        }
        else
        {

            //帐号长度
            appsend[offset] = accinfo[i].accountlen;
            offset += 1;
            //帐号
            memcpy(appsend + offset, accinfo[i].accountinfo, accinfo[i].accountlen);
            offset += accinfo[i].accountlen;
            //密码长度
            appsend[offset] = accinfo[i].pwlen;
            offset += 1;
            //密码
            memcpy(appsend + offset, accinfo[i].password, accinfo[i].pwlen);
            offset += accinfo[i].pwlen;
            count++;
        }
    }
    appsend[2] = offset;
    appsend[3] = count;
    pr_debug("offset:%d count:%d\n", offset, count);
    sendsingle(new_fd, appsend, appsend[2]);
}

/*查找ID对应结构数组
int findcorresid(int id, int mark)
{
    int i = 0;
    while(i < 50)
    {
        //手动场景
        if(mark == 1)
        {
            if(id == mscecontrol[i].id)
            {
                return i;
            }
            else if(0 == mscecontrol[i].id)
            {
                return -1;
            }
        }
        //自动场景
        else if(mark == 2)
        {
            if(id == ascecontrol[i].id)
            {
                return i;
            }
            else if(0 == ascecontrol[i].id)
            {
                return -1;
            }
        }
        //区域
        else if(mark == 3)
        {
            if(id == regcontrol[i].id)
            {
                return i;
            }
            else if(0 == regcontrol[i].id)
            {
                return -1;
            }
        }
        i++;
    }
    return -1;
}
*/

//修改手动场景配置
void modifymanualsce(int sub, byte *buf)
{
    //存放有效数据
    byte appscene[20] = {0x70,0x0c};
    byte appsend[20] = {0xc0,0x00,0x05,0x00,0x00};
    byte manualsce[BUFFER_SIZE] = {0};
    byte manualsceope_hex[20] = {0};
    byte manualsce_h[20] = {0};
    byte manualsceope_h[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    int type = 0;
    int newtype = 0;
    int tempcount = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("manualscenario_len:%d\n", len);
    
    //新配置截取，从配置开始
    memcpy(manualsce, buf + 9, len - 27);

    while(1)
    {
        if(auto_logo == 1)
        {
            pr_debug("本条执行完毕\n");
            type = 0;
            break;
        }
        //判断场景信息是否执行完毕
        if(mscecontrol[sub].tasklen <= type)
        {
            pr_debug("场景指令发送完毕\n");
            type = 0;
            break;
        }
        auto_logo = 0;
        int y = 0;
        //mscecontrol[sub].regtask[0]场景首个设备类型
        switch(mscecontrol[sub].regtask[type])
        {
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01:%d\n", type);
                memcpy(manualsceope_hex, mscecontrol[sub].regtask + type, 10);
                for(y = 0; y < 10; y++)
                {
                    pr_debug("manualsceope_hex[%d]:%0x\n", y, manualsceope_hex[y]);
                }
                //先用老配置对比新配置
                while(1)
                {
                    pr_debug("auto_logo:%d\n", auto_logo);
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        //type += 5;
                        auto_logo = 0;
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(manualsce[newtype] == 0)
                    {
                        pr_debug("场景指令发送完毕\n");
                        //检索完毕，原配置无对应节点
                        //删除
                        if(auto_logo == 0)
                        {
                            //准备发送指令
                            //id
                            appscene[2] = buf[3];
                            memcpy(appscene + 3, manualsceope_hex + 2, 4);
                            //配置类型
                            appscene[4] = (appscene[4] << 4) + 1;
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            tempcount++;
                        }
                        //type += 6;
                        newtype = 0;
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(manualsce[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_h, manualsce + newtype, 10);
                            int b = 0;
                            for(b = 0; b < 10; b++)
                            {
                                pr_debug("manualsceope_h[%d]:%0x\n", b, manualsceope_h[b]);
                            }
                            //mac相同-修改
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 4))
                            {
                                pr_debug("case 0x01(2)\n");
                                //有改动
                                if(0 != memcmp(manualsceope_h, manualsceope_hex, 10))
                                {
                                    pr_debug("case 0x01(3)\n");
                                    //准备发送指令
                                    //id
                                    appscene[2] = buf[3];
                                    memcpy(appscene + 3, manualsceope_h + 2, 4);
                                    //配置类型
                                    appscene[4] = (appscene[4] << 4);
                                    //发送控制指令
                                    UART0_Send(uart_fd, appscene, appscene[1]);
                                    tempcount++;
                                    
                                    auto_logo = 1;
                                }
                                //无改动
                                else
                                {
                                    auto_logo = 1;
                                }
                            }
                            newtype += 10;
                            break;
                            //主从机红外
                        case 0x0f:
                            if(manualsce[newtype + 3] == 0x02)
                            {
                                newtype += ((manualsce[newtype + 4] * 2) + 5);
                            }
                            else if(manualsce[newtype + 3] == 0x04)
                            {
                                newtype += 5;
                            }
                            else
                            {
                                auto_logo = 1;
                                pr_debug("NO_Type:%d\n", newtype);
                            }
                            break;
                        default:
                            pr_debug("手动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }
                type += 10;
                break;
                //主从机红外
            case 0x0f:
                pr_debug("case 0x01:%d\n", type);
                if(mscecontrol[sub].regtask[type + 3] == 0x02)
                {
                    type += ((mscecontrol[sub].regtask[type + 4] * 2) + 5);
                }
                else if(mscecontrol[sub].regtask[type + 3] == 0x04)
                {
                    type += 5;
                }
                else
                {
                    auto_logo = 1;
                }
                break;
            default:
                pr_debug("手动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }

    type = 0;
    newtype = 0;
    auto_logo = 0;
    while(1)
    {
        //新配置对比老配置
        pr_debug("反向\n");
        if(auto_logo == 1)
        {
            break;
        }
        if(manualsce[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        auto_logo = 0;
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                memcpy(manualsceope_h, manualsce + type, 10);
                while(1)
                {
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        auto_logo = 0;
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(mscecontrol[sub].tasklen <= newtype)
                    {
                        pr_debug("场景指令发送完毕(2)\n");
                        //检索完毕，新配置无对应节点
                        //新增
                        if(auto_logo == 0)
                        {
                            //准备发送指令
                            //id
                            appscene[2] = buf[3];
                            memcpy(appscene + 3, manualsceope_h + 2, 4);
                            //配置类型
                            appscene[4] = (appscene[4] << 4);
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            tempcount++;
                        }
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(mscecontrol[sub].regtask[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_hex, mscecontrol[sub].regtask + newtype, 10);
                            //mac相同
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 4))
                            {
                                auto_logo = 1;
                            }
                            newtype += 10;
                            break;
                            //红外码
                        case 0x0f:
                            if(mscecontrol[sub].regtask[newtype + 3] == 0x02)
                            {
                                newtype += ((mscecontrol[sub].regtask[newtype + 4] * 2) + 5);
                            }
                            else if(mscecontrol[sub].regtask[newtype + 3] == 0x04)
                            {
                                newtype += 5;
                            }
                            else
                            {
                                auto_logo = 1;
                                pr_debug("NO_Type\n");
                            }
                            break;
                        default:
                            pr_debug("手动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }
                type += 10;
                break;
                //红外码
            case 0x0f:
                if(manualsce[type + 3] == 0x02)
                {
                    memcpy(regionaltemp.irtask + regionaltemp.irlen, manualsce + type, ((manualsce[type + 4] * 2) + 5));
                    regionaltemp.irlen += ((manualsce[type + 4] * 2) + 5);

                    type += ((manualsce[type + 4] * 2) + 5);
                }
                else if(manualsce[type + 3] == 0x04)
                {
                    memcpy(regionaltemp.security + regionaltemp.seclen, manualsce + type, 5);
                    regionaltemp.seclen += 5;

                    type += 5;
                }
                else
                {
                    auto_logo = 1;
                    pr_debug("NO_Type\n");
                }
                break;
            default:
                pr_debug("手动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);

    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;

    //手动场景指令头
    memcpy(regionaltemp.reghead, buf + 3, 6);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务
    memcpy(regionaltemp.regassoc, buf + 9, len -27);

    //无配置信息改动
    if(0 == tempcount)
    {
        manscestoreinfo();

        appsend[3] = buf[3];
        UartToApp_Send(appsend, appsend[2]);
    }
}

//新增手动场景配置
void newmanualsce(int id, byte *buf)
{
    //存放有效数据
    byte manualsce[1250] = {0};
    byte manualsceope[1250] = {0};
    byte manualsceope_hex[10] = {0};
    byte manualsce_s[20] = {0};
    byte appsend[20] = {0xc0,0x00,0x05,0x00,0x00};
    byte manualsce_str_a[20] = {0};
    byte manualsce_h[20] = {0};
    byte appscene[20] = {0x70,0x0c};
    byte str[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    int tempcount = 0;
    int type = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("newmanualsce_len:%d\n", len);

    //截取有效数据
    memcpy(manualsce, buf + 9, len - 27);
    //分配ID
    memcpy(manualsceope, buf + 3, len - 3);
    manualsceope[0] = id;
    while(1)
    {
        pr_debug("manualsce[%d]:%0x\n", type, manualsce[type]);
        //判断场景信息是否执行完毕
        if(manualsce[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            pr_debug("default场景指令执行完毕\n");
            break;
        }
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01\n");
                memcpy(manualsceope_hex, manualsce + type, 10);
                //准备发送指令
                appscene[2] = id;
                memcpy(appscene + 3, manualsceope_hex + 2, 4);
                //配置类型
                appscene[4] = ((appscene[4] << 4) & 0xf0);
                //发送控制指令
                UART0_Send(uart_fd, appscene, appscene[1]);
                tempcount++;
                type += 10;
                break;
                //红外码
            case 0x0f:
                pr_debug("case 0x0f\n");
                if(manualsce[type + 3] == 0x02)
                {
                    memcpy(regionaltemp.irtask + regionaltemp.irlen, manualsce + type, ((manualsce[type + 4] * 2) + 5));
                    regionaltemp.irlen += ((manualsce[type + 4] * 2) + 5);
                    type += ((manualsce[type + 4] * 2) + 5);
                }
                else if(manualsce[type + 3] == 0x04)
                {
                    memcpy(regionaltemp.security + regionaltemp.seclen, manualsce + type, 5);
                    regionaltemp.seclen += 5;
                    type += 5;
                }
                else
                {
                    auto_logo = 1;
                    pr_debug("NO_Type\n");
                }
                break;
            default:
                pr_debug("手动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);
    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;
    //指令头
    memcpy(regionaltemp.reghead, manualsceope, 6);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务
    memcpy(regionaltemp.regassoc, manualsceope + 6, len - 27);
    
    //无配置信息改动
    if(0 == tempcount)
    {
        manscestoreinfo();

        appsend[3] = id;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//手动场景配置 0x01
void manualscenario(byte *buf)
{
    //存放有效数据
    byte manualsce[BUFFER_SIZE] = {0};
    byte idhex[5] = {0};
    byte str[2048] = {0};
    byte appsend[20] = {0xc0,0x00,0x05,0x00,0x02};
    byte manualsce_hex[BUFFER_SIZE] = {0};
    int tempid = 0;
    int id = 1;
    
    //初始化应答结构
    memset(&regionaltemp, 0, sizeof(regionaltemp));
    memset(&sceneack, 0, sizeof(sceneack));
    sceneack.headbuf[0] = 0xc0;
    sceneack.headbuf[1] = 0x00;
    sceneack.headbuf[2] = 0x05;
    sceneack.headbuf[4] = 0x00;
    //开始记录
    sceneack.starlog = 1;

    hostinfo.dataversion += 1;
    
    //新配置
    if(buf[3] == 0)
    {
        FILE *fd = NULL;
        if((fd = fopen(MANUALSCE, "a+")) == NULL)
        {
            pr_debug("manualscenario_file open failed\n");
            pr_debug("%s: %s\n", MANUALSCE, strerror(errno));

            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, MANUALSCE, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
            //return;
        }
        //截取有效数据从ID开始
        //memcpy(manualsce, buf + 2, len - 2);
        int effective = 0;
        while(fgets(str, 2048, fd))
        {
            if(strlen(str) < 10)
            {
                str[2] = '\0';
                StrToHex(idhex, str, 1);
                tempid = idhex[0];
                id++;
            }
            else
            {
                effective++;
                id++;
            }
            memset(str, 0, sizeof(str));
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        fclose(fd);
        fd = NULL;
        if(id >= 21)
        {
            id = tempid;
        }
        if((id < 0) && (effective >= 20))
        {
            pr_debug("场景配置已达上限1\n");
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        sceneack.headbuf[3] = id;
        newmanualsce(id, buf);
    }
    //修改配置
    else
    {
        if(mscecontrol[buf[3]].id == 0)
        {
            pr_debug("修改手动场景失败\n");
            appsend[3] = buf[3];
            appsend[4] = 1;
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        sceneack.headbuf[3] = buf[3];
        modifymanualsce(buf[3], buf);
    }
}

//手动场景处理主从机红外
void manualsceopeir(int id)
{
    int offset = 0;
    int i = 0;
    byte manualsce[20] = {0};
    byte perirfast[10] = {0x65,0x00,0x07};
    byte appbuf[12] = {0x71,0x00,0x05};

    while(1)
    {
        if(mscecontrol[id].tasklen <= offset)
        {
            pr_debug("manualsceopeir over:%d...\n", offset);
            break;
        }
        switch(mscecontrol[id].regtask[offset])
        {
            //开关
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                offset += 10;
                break;
            //主从机红外
            case 0x0f:
                if(mscecontrol[id].regtask[offset + 3] == 0x02)
                {
                    //取出红外快捷方式索引
                    memcpy(manualsce, mscecontrol[id].regtask + (offset + 5), (mscecontrol[id].regtask[offset + 4] * 2));
                    for(i = 0; i < (mscecontrol[id].regtask[offset + 4] * 2); i += 2)
                    {
                        perirfast[3] = manualsce[i];
                        perirfast[4] = manualsce[i + 1];
                        perirfast[5] = mscecontrol[id].regtask[offset];
                        perirfast[6] = mscecontrol[id].regtask[offset + 1];
                        //执行红外快捷方式
                        runirquick(perirfast);
                    }
                    offset += ((mscecontrol[id].regtask[offset + 4] * 2) + 5);
                }
                else if(mscecontrol[id].regtask[offset + 3] == 0x04)
                {
                    //处理安防
                    appbuf[3] = mscecontrol[id].regtask[offset + 4];
                    performsecactions(appbuf);
                    offset += 5;
                }
                else
                {
                    pr_debug("manualsceopeir nottype...\n");
                    return;
                }
                break;
            default:
                pr_debug("manualsceopeir nottype...\n");
                return;
        }
    }
}

//手动场景操作 0x62
void manualsceope(byte *buf)
{
    byte scenerun[10] = {0x71,0x04,0x00,0x00};
    byte appsend[10] = {0xd2,0x00,0x04,0x00};
    if((buf[3] == mscecontrol[buf[3]].id) && (mscecontrol[buf[3]].tasklen > 0))
    {
        //区域执行命令(广播) 0x65
        //id
        scenerun[2] = buf[3];
        UART0_Send(uart_fd, scenerun, scenerun[1]);

        //执行主从机红外
        manualsceopeir(buf[3]);
        //id
        appsend[3] = buf[3];
    }
    else
    {
        appsend[3] = 0;
    }
    UartToApp_Send(appsend, 4);
}

//删除场景
void delmanualscenario(byte *buf)
{
    if(buf[3] <= 0)
    {
        return;
    }
    byte scenedel[15] = {0x70,0x0c,0x00,0xff,0x00};
    byte delmanualscenario_str[10] = {0};
    byte delmanualscenario_buf[6] = {0};
    byte appsend[10] = {0xc2,0x00,0x05,0x00,0x00};
    HexToStr(delmanualscenario_str, buf, buf[2]);
    memcpy(delmanualscenario_buf, delmanualscenario_str + 6, 2);
    
    if(1 == replace(MANUALSCE, delmanualscenario_buf, 2))
    {
        hostinfo.dataversion += 1;
        //删除区域(广播)
        scenedel[2] = buf[3];
        UART0_Send(uart_fd, scenedel, scenedel[1]);

        appsend[3] = buf[3];
        UartToApp_Send(appsend, 5);
        //
        readmansce();
    }
    else
    {
        //失败
        appsend[3] = buf[3];
        appsend[4] = 0x01;
        UartToApp_Send(appsend, 5);
    }
}


//修改自动场景配置
void automodifymanualsce(int sub, byte *buf)
{
    //存放有效数据
    int pr_i = 0;
    byte appscene[20] = {0x67,0x0c};
    byte appsend[10] = {0xc3,0x00,0x05,0x00,0x00};
    byte manualsce[BUFFER_SIZE] = {0};
    byte manualsceope_hex[10] = {0};
    byte manualsce_h[20] = {0};
    byte manualsceope_h[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    int type = 0;
    int tempcount = 0;
    int newtype = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("manualscenario_len:%d\n", len);

    //新配置截取，从配置开始
    memcpy(manualsce, buf + (23 + buf[20]), len - (18 + 23 + buf[20]));
    while(1)
    {
        pr_debug("type(1):%d\n", type);
        pr_debug("ascecontrol[sub].tasklen(1):%d\n", ascecontrol[sub].tasklen);
        //判断场景信息是否执行完毕
        if(auto_logo == 1)
        {
            pr_debug("本条执行完毕\n");
            break;
        }
        if(ascecontrol[sub].tasklen <= type)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        auto_logo = 0;
        switch(ascecontrol[sub].regtask[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01:%d\n", type);
                memcpy(manualsceope_hex, ascecontrol[sub].regtask + type, 10);
                int y = 0;
                for(y = 0; y < 10; y++)
                {
                    pr_debug("manualsceope_hex[%d]:%0x\n", y, manualsceope_hex[y]);
                }
                while(1)
                {
                    pr_debug("auto_logo:%d\n", auto_logo);
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        //type += 5;
                        auto_logo = 0;
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(manualsce[newtype] == 0)
                    {
                        pr_debug("场景指令发送完毕\n");
                        //检索完毕，原配置无对应节点
                        //删除
                        if(auto_logo == 0)
                        {
                            //准备发送指令
                            //id
                            appscene[2] = buf[3];
                            memcpy(appscene + 3, manualsceope_hex + 2, 7);
                            //配置类型
                            appscene[4] = (appscene[4] << 4) + 1;
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            tempcount++;
                            for(pr_i = 0; pr_i < appscene[1]; pr_i++)
                            {
                                pr_debug("appscene1[%d]:%0x\n", pr_i, appscene[pr_i]);
                            }
                        }
                        //type += 5;
                        newtype = 0;
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(manualsce[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_h, manualsce + newtype, 10);
                            int b = 0;
                            for(b = 0; b < 9; b++)
                            {
                                pr_debug("manualsceope_h[%d]:%0x\n", b, manualsceope_h[b]);
                            }
                            //mac相同-修改
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 4))
                            {
                                pr_debug("case 0x01(2)\n");
                                //有改动
                                if(0 != memcmp(manualsceope_h, manualsceope_hex, 9))
                                {
                                    pr_debug("case 0x01(3)\n");
                                    //准备发送指令
                                    //id
                                    appscene[2] = buf[3];
                                    memcpy(appscene + 3, manualsceope_h + 2, 7);
                                    //配置类型
                                    appscene[4] = (appscene[4] << 4);
                                    //发送控制指令
                                    UART0_Send(uart_fd, appscene, appscene[1]);
                                    tempcount++;
                                    for(pr_i = 0; pr_i < appscene[1]; pr_i++)
                                    {
                                        pr_debug("appscene2[%d]:%0x\n", pr_i, appscene[pr_i]);
                                    }

                                }
                                //无改动
                                auto_logo = 1;
                            }
                            newtype += 10;
                            break;
                            //红外码/手动场景
                        case 0x0f:
                            //手动场景
                            if(manualsce[newtype + 3] == 0x01)
                            {
                                newtype += 5;
                            }
                            //红外
                            else if(manualsce[newtype + 3] == 0x02)
                            {
                                newtype += ((manualsce[newtype + 4] * 2) + 5);
                            }
                            else if(manualsce[newtype + 3] == 0x03)
                            {
                                newtype += 30;
                            }
                            else
                            {
                                auto_logo = 1;
                            }
                            break;
                        default:
                            pr_debug("手动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }

                type += 10;
                break;
                //红外码
            case 0x0f:
                pr_debug("case 0x0f:%d\n", type);
                if(ascecontrol[sub].regtask[type + 3] == 0x01)
                {
                    type += 5;
                }
                else if(ascecontrol[sub].regtask[type + 3] = 0x02)
                {
                    type += ((ascecontrol[sub].regtask[type + 4] * 2) + 5);
                }
                else if(ascecontrol[sub].regtask[type + 3] == 0x03)
                {
                    type += 30;
                }
                else
                {
                    auto_logo = 1;
                }
                break;
            default:
                pr_debug("自动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }

    type = 0;
    newtype = 0;
    auto_logo = 0;
    while(1)
    {
        //判断场景信息是否执行完毕
        pr_debug("反向\n");
        if(manualsce[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            break;
        }
        auto_logo = 0;
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                memcpy(manualsceope_h, manualsce + type, 10);
                while(1)
                {
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        auto_logo = 0;
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(ascecontrol[sub].tasklen <= newtype)
                    {
                        pr_debug("场景指令发送完毕(2)\n");
                        //检索完毕，新配置无对应节点
                        //新增
                        if(auto_logo == 0)
                        {
                            //准备发送指令
                            //id
                            appscene[2] = buf[3];
                            memcpy(appscene + 3, manualsceope_h + 2, 7);
                            //配置类型
                            appscene[4] = (appscene[4] << 4);
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            tempcount++;
                            for(pr_i = 0; pr_i < appscene[1]; pr_i++)
                            {
                                pr_debug("appscene3[%d]:%0x\n", pr_i, appscene[pr_i]);
                            }
                        }
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(ascecontrol[sub].regtask[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_hex, ascecontrol[sub].regtask + newtype, 10);
                            //mac相同
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 4))
                            {
                                auto_logo = 1;
                            }
                            newtype += 10;
                            break;
                            //红外码
                        case 0x0f:
                            if(ascecontrol[sub].regtask[newtype + 3] == 0x01)
                            {
                                newtype += 5;
                            }
                            else if(ascecontrol[sub].regtask[newtype + 3] == 0x02)
                            {
                                newtype += ((ascecontrol[sub].regtask[newtype + 4] * 2) + 5);
                            }
                            else if(ascecontrol[sub].regtask[newtype + 3] == 0x03)
                            {
                                newtype += 30;
                            }
                            else
                            {
                                auto_logo = 1;
                            }
                            break;
                        default:
                            pr_debug("自动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }
                type += 10;
                break;
                //红外码
            case 0x0f:
                pr_debug("0x0f\n");
                if(manualsce[type + 3] == 0x01)
                {
                    memcpy(regionaltemp.manualsce + regionaltemp.manlen, manualsce + type, 5);
                    regionaltemp.manlen += 5;
                    type += 5;
                }
                else if(manualsce[type + 3] == 0x02)
                {
                    memcpy(regionaltemp.irtask + regionaltemp.irlen, manualsce + type, ((manualsce[type + 4] * 2) + 5));
                    regionaltemp.irlen += ((manualsce[type + 4] * 2) + 5);
                    type += ((manualsce[type + 4] * 2) + 5);
                }
                else if(manualsce[type + 3] == 0x03)
                {
                    memcpy(regionaltemp.crosshostsce + regionaltemp.crosshostlen, manualsce + type, 30);
                    regionaltemp.crosshostlen += 30;
                    type += 30;
                }
                else
                {
                    auto_logo = 1;
                }
                break;
            default:
                pr_debug("自动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);
    
    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;
    //指令头
    memcpy(regionaltemp.reghead, buf + 3, 17);
    //触发条件长度
    regionaltemp.tricondlen = buf[20];
    //触发条件
    memcpy(regionaltemp.tricond, buf + 21, buf[20]);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务长度
    regionaltemp.tasklen[0] = buf[21 + buf[20]];
    regionaltemp.tasklen[1] = buf[22 + buf[20]];
    //执行任务
    memcpy(regionaltemp.regassoc, buf + (23 + buf[20]), len - (18 + 23 + buf[20]));
    if(0 == tempcount)
    {
        autoscestoreinfo();
        appsend[3] = buf[3];
        UartToApp_Send(appsend, appsend[2]);
    }
}

//新增自动场景配置
void newautomanualsce(int id, byte *buf)
{
    //存放有效数据
    byte manualsce[2048] = {0};
    byte manualsceope_hex[10] = {0};
    byte manualsce_s[20] = {0};
    byte manualsce_str_a[20] = {0};
    byte manualsce_h[20] = {0};
    byte appsend[10] = {0xc3,0x00,0x05,0x00,0x00};
    byte appscene[20] = {0x67,0x0c};
    byte str[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    int tempcount = 0;
    int type = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("newmanualsce_len:%d\n", len);

    //截取有效数据
    memcpy(manualsce, buf + (23 + buf[20]), len - (18 + 23 + buf[20]));
    while(1)
    {
        pr_debug("manualsce[%d]:%0x\n", type, manualsce[type]);
        //判断场景信息是否执行完毕
        if(manualsce[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            pr_debug("场景指令执行完毕\n");
            break;
        }
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01\n");
                memcpy(manualsceope_hex, manualsce + type, 10);
                //准备发送指令
                appscene[2] = id;
                memcpy(appscene + 3, manualsceope_hex + 2, 7);
                //配置类型
                appscene[4] = (appscene[4] << 4);
                //发送控制指令
                UART0_Send(uart_fd, appscene, appscene[1]);
                tempcount++;
                type += 10;
                break;
                //红外码
            case 0x0f:
                pr_debug("case 0x0f\n");
                if(manualsce[type + 3] == 0x01)
                {
                    memcpy(regionaltemp.manualsce + regionaltemp.manlen, manualsce + type, 5);
                    regionaltemp.manlen += 5;
                    type += 5;
                }
                else if(manualsce[type + 3] == 0x02)
                {
                    memcpy(regionaltemp.irtask + regionaltemp.irlen, manualsce + type, ((manualsce[type + 4] * 2) + 5));
                    regionaltemp.irlen += ((manualsce[type + 4] * 2) + 5);
                    type += ((manualsce[type + 4] * 2) + 5);
                }
                else if(manualsce[type + 3] == 0x03)
                {
                    memcpy(regionaltemp.crosshostsce + regionaltemp.crosshostlen, manualsce + type, 30);
                    regionaltemp.crosshostlen += 30;
                    type += 30;
                }
                else
                {
                    auto_logo = 1;
                    pr_debug("NO_Type\n");
                }
                break;
            default:
                pr_debug("自动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);
    
    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;
    //指令头
    memcpy(regionaltemp.reghead, buf + 3, 17);
    regionaltemp.reghead[0] = id;
    //触发条件长度
    regionaltemp.tricondlen = buf[20];
    //触发条件
    memcpy(regionaltemp.tricond, buf + 21, buf[20]);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务长度
    regionaltemp.tasklen[0] = buf[21 + buf[20]];
    regionaltemp.tasklen[1] = buf[22 + buf[20]];
    //执行任务
    memcpy(regionaltemp.regassoc, buf + (23 + buf[20]), len - (18 + 23 + buf[20]));
    
    if(0 == tempcount)
    {
        autoscestoreinfo();
        appsend[3] = id;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//自动场景配置 0x53  0x67
void autoscene(byte *buf)
{
    //存放有效数据
    byte manualsce[BUFFER_SIZE] = {0};
    byte str[2048] = {0};
    byte idhex[5] = {0};
    byte manualsce_hex[BUFFER_SIZE] = {0};
    byte appsend[10] = {0xc3,0x00,0x05,0x00,0x02};
    int tempid = 0;
    int id = 1;

    //初始化应答结构
    memset(&sceneack, 0, sizeof(sceneack));
    memset(&regionaltemp, 0, sizeof(regionaltemp));
    sceneack.headbuf[0] = 0xc3;
    sceneack.headbuf[1] = 0x00;
    sceneack.headbuf[2] = 0x05;
    sceneack.headbuf[4] = 0x00;

    //开始记录
    sceneack.starlog = 1;
    hostinfo.dataversion += 1;

    //新配置
    if(buf[3] == 0)
    {
        FILE *fd = NULL;
        if((fd = fopen(AUTOSCE, "a+")) == NULL)
        {
            pr_debug("AUTOSCE_file open failed\n");
            pr_debug("%s: %s\n", AUTOSCE, strerror(errno));
            
            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, AUTOSCE, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
            //return;
        }
        //截取有效数据从ID开始
        //memcpy(manualsce, buf + 2, len - 2);
        int effective = 0;
        while(fgets(str, 2048, fd))
        {
            if(strlen(str) < 5)
            {
                str[2] = '\0';
                StrToHex(idhex, str, 1);
                tempid = idhex[0];
                id++;
            }
            else
            {
                effective++;
                id++;
            }
            memset(str, 0, sizeof(str));
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        fclose(fd);
        fd = NULL;
        if(id >= 11)
        {
            id = tempid;
        }
        if((id == 0) && (effective >= 10))
        {
            pr_debug("自动场景配置已达上限1\n");
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        sceneack.headbuf[3] = id;
        newautomanualsce(id, buf);
    }
    //修改配置
    else
    {
        if(ascecontrol[buf[3]].id == 0)
        {
            pr_debug("修改自动场景失败\n");
            appsend[3] = buf[3];
            appsend[4] = 1;
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        sceneack.headbuf[3] = buf[3];
        automodifymanualsce(buf[3], buf);
    }
}

//获取自动场景状态0x44
void getautoscestatus(byte *buf)
{
    int i = 1;
    time_t nowtime = 0;
    time(&nowtime);
    byte appsend[10] = {0xb4,0x00,0x08};
    while(i < 11)
    {
        //空场景
        if(ascecontrol[i].id == 0)
        {
            i++;
            continue;
        }
        else
        {
            //id
            appsend[3] = i;
            //enable
            appsend[4] = ascecontrol[i].enable;
            //status
            appsend[5] = autotri[i].ldentification;
            //剩余冷却时间
            appsend[6] = (((ascecontrol[i].interval - ((nowtime - autotri[i].lasttime) / 60)) >> 8) & 0xff);
            appsend[7] = ((ascecontrol[i].interval - ((nowtime - autotri[i].lasttime) / 60)) & 0xff);
            UartToApp_Send(appsend, appsend[2]);
        }
        i++;
    }
}

//自动场景使能 0x61
void autoscene_enable(byte *buf)
{
    byte appsend[10] = {0xd1,0x00,0x05};
    byte autoscnen_enable_hex[5] = {0};
    byte autoscene_enable_str[3000] = {0};
    byte autoscestr[3000] = {0};
    memcpy(autoscnen_enable_hex, buf + 3, 2);
    HexToStr(autoscene_enable_str, autoscnen_enable_hex, 2);

    hostinfo.dataversion += 1;
    
    if(0 == filefind(autoscene_enable_str, autoscestr, AUTOSCE, 2))
    {
        autoscestr[strlen(autoscestr) -1] = '\0';
        pr_debug("autoscestr:%s\n", autoscestr);
        pr_debug("strlen(autoscestr):%d\n", strlen(autoscestr));
        if(strlen(autoscestr) > 10)
        {
            memcpy(autoscene_enable_str + 4, autoscestr + 4, (strlen(autoscestr) - 4));
            
            pr_debug("autoscene_enable_str:%s\n", autoscene_enable_str);
            pr_debug("strlen(autoscene_enable_str):%d\n", strlen(autoscene_enable_str));

            //启用
            if(buf[4] == 1)
            {
                //如果已经启用
                if(ascecontrol[buf[3]].enable == 1)
                {
                    //清除冷却标识
                    autotri[buf[3]].ldentification = 0;
                    //重置触发条件计时
                }
                else
                {
                    ascecontrol[buf[3]].enable = 1;
                    autotri[buf[3]].ldentification = 0;
                    replace(AUTOSCE, autoscene_enable_str, 2);
                }
            }
            //禁用
            else if(buf[4] == 0)
            {
                //如果已经启用
                if(ascecontrol[buf[3]].enable == 1)
                {
                    ascecontrol[buf[3]].enable = 0;
                    replace(AUTOSCE, autoscene_enable_str, 2);
                }
            }
            //读取自动场景信息
            //readautosce();
            //读取触发条件
            //readtriggercond();

            appsend[3] = buf[3];
            appsend[4] = buf[4];
            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//删除自动场景 0x54
void delautoscene(byte *buf)
{
    if(buf[3] <= 0)
    {
        return;
    }
    byte scenedel[15] = {0x67,0x0c,0x00,0xff,0x00};
    byte delmanualscenario_str[10] = {0};
    byte delmanualscenario_buf[6] = {0};
    byte appsend[10] = {0xc4,0x00,0x05,0x00,0x00};
    HexToStr(delmanualscenario_str, buf, buf[2]);
    memcpy(delmanualscenario_buf, delmanualscenario_str + 6, 2);
    
    if(1 == replace(AUTOSCE, delmanualscenario_buf, 2))
    {
        hostinfo.dataversion += 1;
        //删除区域(广播)
        scenedel[2] = buf[3];
        UART0_Send(uart_fd, scenedel, scenedel[1]);

        appsend[3] = buf[3];
        UartToApp_Send(appsend, 5);
        //
        readautosce();
    }
    else
    {
        //失败
        appsend[3] = buf[3];
        appsend[4] = 0x01;
        UartToApp_Send(appsend, 5);
    }
}

//测试智能锁
void whilesmartlock(byte *uartsend)
{
    //byte uartsend[24] = {0x89,0x0c,0x03,0x01,0x21,0x43,0x65,0xff,0xff,0xff,0x00,0x5f};
    int i = 0;
    while(i < 100)
    {
        UART0_Send(uart_fd, uartsend, uartsend[1]);
        i++;
        pr_debug("测试智能锁:%d\n", i);
        sleep(10);
    }
}

//App开关本地 操作 单指令控制 0x60  0x63
void appsw(byte *buf)
{
    byte uartsend[24] = {0x63,0x0e};
    byte chipuartsend[24] = {0x30,0x00,0x09,0x09};

    if(buf[5] == 0x16)
    {
        //id
        uartsend[0] = 0x88;
        //len
        uartsend[1] = 0x06;
        //mac
        uartsend[2] = buf[4];
        //type
        uartsend[3] = buf[6];
        //value
        uartsend[4] = buf[7];
    }
    else if(buf[5] == 0x17)
    {
        //id
        uartsend[0] = 0x87;
        //len
        uartsend[1] = 0x06;
        //mac
        uartsend[2] = buf[4];
        //type
        uartsend[3] = 0x01;
        //value
        uartsend[4] = buf[7];
    }
    //灯具
    else if(buf[5] == 0x31)
    {
        //id
        uartsend[0] = 0x86;
        //len
        uartsend[1] = 0x08;
        //mac
        uartsend[2] = buf[4];
        memcpy(uartsend + 3, buf + 6, 4);
    }
    //锁
    else if(buf[5] == 0x41)
    {
        //id
        uartsend[0] = 0x89;
        //len
        uartsend[1] = 0x0c;
        //mac
        uartsend[2] = buf[4];
        memcpy(uartsend + 3, buf + 6, 7);
        //whilesmartlock(uartsend);
    }
    else
    {
        //主从机使能
        if((buf[4] == 0x01) && (buf[5] == 0x0c))
        {
            chipuartsend[4] = buf[3];
            chipuartsend[5] = buf[4];
            chipuartsend[6] = buf[6];
            chipuartsend[7] = buf[7];
            IRUART0_Send(single_fd, chipuartsend, chipuartsend[2]);
            return;
        }
        else
        {
            uartsend[1] = 0x0e;
            memcpy(uartsend + 2, buf + 4, 4);
        }
    }
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//本地按键关联，一个按键最多关联1项 0x43
void localassociation(byte *buf)
{
    byte localass_hex[20] = {0};
    byte localass_str[40] = {0};
    
    int locallogo = 0;
    byte appsend[10] = {0xb3,0x00,0x04,0x00};

    byte localfirst[10] = {0};
    byte localseco[10] = {0};
    
    byte findstr[20] = {0};
    memcpy(localass_hex, buf + 4, 6);
    HexToStr(localass_str, localass_hex, 6);
    pr_debug("localass_str:%s\n", localass_str);
    //首个
    memcpy(localfirst, localass_str, 6);
    memcpy(localseco, localass_str + 6, 6);
    hostinfo.dataversion += 1;
    //修改/删除
    if(0 == filefindstr(localfirst, findstr, LOCALASS))
    {
        locallogo = 1;
        pr_debug("首-findstr:%s\n", findstr);
        //新增/修改
        if(buf[3] == 0)
        {
            if(0 == memcmp(localass_str, findstr, 6))
            {
                //替换
                replace(LOCALASS, localass_str, 6);
            }
            else
            {
                //删除
                filecondel(LOCALASS, findstr, 6);
                //替换
                replace(LOCALASS, localass_str, 6);
            }
        }
        //删除
        else if(buf[3] == 1)
        {
            filecondel(LOCALASS, findstr, 6);
        }
    }
    localass_str[12] = '\0';
    //次
    if(0 == filefindstr(localseco, findstr, LOCALASS))
    {
        locallogo = 1;
        pr_debug("次-findstr:%s\n", findstr);
        //新增/修改
        if(buf[3] == 0)
        {
            if(0 == memcmp(localass_str, findstr, 6))
            {
                //替换
                replace(LOCALASS, localass_str, 6);
            }
            else
            {
                //删除
                filecondel(LOCALASS, findstr, 6);
                //替换
                pr_debug("4:%s\n", localass_str);
                replace(LOCALASS, localass_str, 6);
            }
        }
        //删除
        else if(buf[3] == 1)
        {
            //删除
            filecondel(LOCALASS, findstr, 6);
        }
    }
    //新增
    localass_str[12] = '\0';
    if((locallogo == 0) && (buf[3] == 0))
    {
        pr_debug("新增-localass_str:%s\n", localass_str);
        //替换
        pr_debug("localass_str1:%s\n", localass_str);
        replace(LOCALASS, localass_str, 6);
    }
    readlocalass();
    UartToApp_Send(appsend, appsend[2]);
}

//主机命名
int hostnaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte hostname[64] = {"hostname"};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname + 8, tempnamestr + 8, (buf[2] * 2) - 9);

    if(1 == replace(HOSTSTATUS, hostname, 8))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//节点命名
int nodenaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 9);

    if(1 == replace(DEVNAME, hostname, 6))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//负载命名
int loadnaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 9);

    if(1 == replace(DEVNAME, hostname, 6))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//红外设备命名
int irdevnaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte irdevstr[1024] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 10);

    pr_debug("hostname:%s\n", hostname);
    if(-1 == filefind(hostname, irdevstr, IRDEV, 2))
    {
        return 1;
    }
    pr_debug("irdevstr:%s\n", irdevstr);
    pr_debug("strlen(irdevstr):%d\n", strlen(irdevstr));
    memcpy(irdevstr + (strlen(irdevstr) - 35), hostname + 2, 34);
    pr_debug("irdevstr:%s\n", irdevstr);
    irdevstr[strlen(irdevstr) - 1] = '\0';
    if(1 == replace(IRDEV, irdevstr, 2))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//红外快捷方式命名
int irfastnaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte irdevstr[1024] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, 4);

    if(-1 == filefind(hostname, irdevstr, IRFAST, 4))
    {
        return 1;
    }
    memcpy(irdevstr + (strlen(irdevstr) - 68), tempnamestr + 12, 68);
    if(1 == replace(IRFAST, irdevstr, 4))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//区域命名
int areanaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte irdevstr[2048] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 8);

    if(-1 == filefind(hostname, irdevstr, REGASSOC, 2))
    {
        return 1;
    }
    memcpy(irdevstr + (strlen(irdevstr) - 36), hostname + 2, 36);
    if(1 == replace(REGASSOC, irdevstr, 2))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//手动场景命名
int manualscenaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte irdevstr[2048] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 8);

    if(-1 == filefind(hostname, irdevstr, MANUALSCE, 2))
    {
        return 1;
    }
    memcpy(irdevstr + (strlen(irdevstr) - 36), hostname + 2, 36);
    if(1 == replace(MANUALSCE, irdevstr, 2))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//自动场景命名
int autonaming(byte *buf)
{
    int i = 0;
    byte tempnamestr[128] = {0};
    byte irdevstr[2048] = {0};
    byte hostname[64] = {0};

    HexToStr(tempnamestr, buf, buf[2]);
    memcpy(hostname, tempnamestr + 8, (buf[2] * 2) - 8);

    if(-1 == filefind(hostname, irdevstr, AUTOSCE, 2))
    {
        return 1;
    }
    memcpy(irdevstr + (strlen(irdevstr) - 36), hostname + 2, 36);
    if(1 == replace(AUTOSCE, irdevstr, 2))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//命名集合
void namingcollection(byte *buf)
{
    byte appsend[10] = {0xa0,0x00,0x05};

    appsend[4] = buf[3];
    hostinfo.dataversion += 1;
    switch(buf[3])
    {
        case 0x01:
            pr_debug("主机命名\n");
            appsend[3] = hostnaming(buf);
            break;
        case 0x02:
            pr_debug("节点命名\n");
            appsend[3] = nodenaming(buf);
            break;
        case 0x03:
            pr_debug("负载命名\n");
            appsend[3] = loadnaming(buf);
            break;
        case 0x04:
            pr_debug("红外设备命名\n");
            appsend[3] = irdevnaming(buf);
            break;
        case 0x05:
            pr_debug("区域命名\n");
            appsend[3] = areanaming(buf);
            break;
        case 0x06:
            pr_debug("手动场景命名\n");
            appsend[3] = manualscenaming(buf);
            break;
        case 0x07:
            pr_debug("自动场景命名\n");
            appsend[3] = autonaming(buf);
            break;
        case 0x08:
            pr_debug("红外快捷方式命名\n");
            appsend[3] = irfastnaming(buf);
            break;
        default:
            pr_debug("namingcollection unsupported type\n");
            break;
    }
    UartToApp_Send(appsend, appsend[2]);
}

//同步从机
void synchronizeslave()
{
    int i = 2;
    byte appsend[24] = {0xa1,0x00,0x16,0x0f,0x0f};
    for(i = 2; i < 5; i++)
    {
        if(slaveident[i].assignid > 1)
        {
            //sn6
            memcpy(appsend + 5, slaveident[i].slavesn, 6);
            //mac2
            appsend[11] = slaveident[i].assignid;
            appsend[12] = 0x01;
            //在线/离线
            appsend[13] = slaveident[i].sockfd;
            //版本号
            memcpy(appsend + 14, slaveident[i].version, 4);
            //温度
            appsend[18] = slaveident[i].temperature;
            //湿度
            appsend[19] = slaveident[i].moderate;
            //使能
            appsend[20] = slaveident[i].enable;
            //mesh版本
            appsend[21] = slaveident[i].meshversion;
            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//同步节点
void synchronizenode()
{
    int i = 2;
    int count = 0;
    int len = 0;
    byte appsend[20] = {0xa1,0x00,0x10,0x01};

    for(i = 2; i < 256; i++)
    {
        if(nodedev[i].nodeinfo[2] == 0)
        {
            //pr_debug("节点信息同步-保留mac:%d\n", i);
        }
        else if(nodedev[i].nodeinfo[2] == 0xff)
        {
            //pr_debug("节点信息同步-保留ffmac:%d\n", i);
        }
        else
        {
            memcpy(appsend + 4, nodedev[i].nodeinfo, 12);
            count++;
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    pr_debug("节点信息同步完成:%d\n", count);
}

//节点名称同步
void synchronizenodename()
{
    FILE *fd = NULL;
    int i = 0;
    byte appsend[128]  = {0xa1,0x00};
    byte irstr[256] = {0};
    byte irhex[128] = {0};

    appsend[3] = 0x10;

    if((fd = fopen(DEVNAME, "r")) == NULL)
    {
        pr_debug("%s: %s\n", DEVNAME, strerror(errno));
        return;
    }
    while(fgets(irstr, 256, fd))
    {
        StrToHex(irhex, irstr, strlen(irstr) / 2);
        //len
        appsend[2] = (strlen(irstr) / 2) + 4;
        memcpy(appsend + 4, irhex, strlen(irstr) / 2);

        UartToApp_Send(appsend, appsend[2]);
    }
    fclose(fd);
    fd = NULL;
}

//同步本地按键关联
void synchronizelocalass()
{
    int i = 0;
    int len = 0;
    byte appsend[20] = {0xa1,0x00,0x0a,0x11};

    for(i = 0; i < 50; i++)
    {
        if(localasoction[i].bingkey[1] == 0)
        {
            pr_debug("本地按键关联同步完成:%d\n", i);
            break;
        }
        else
        {
            memcpy(appsend + 4, localasoction[i].bingkey, 6);
            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//同步红外设备信息
void synchronizeirdev()
{
    FILE *fd = NULL;
    int i = 0;
    byte appsend[128]  = {0xa1,0x00};
    byte irstr[256] = {0};
    byte irhex[128] = {0};

    appsend[3] = 0x02;

    if((fd = fopen(IRDEV, "r")) == NULL)
    {
        pr_debug("%s: %s\n", IRDEV, strerror(errno));
        return;
    }
    while(fgets(irstr, 256, fd))
    {
        if(strlen(irstr) < 20)
        {
            continue;
        }
        StrToHex(irhex, irstr, strlen(irstr) / 2);
        //len
        appsend[2] = (strlen(irstr) / 2) + 4;
        memcpy(appsend + 4, irhex, strlen(irstr) / 2);

        UartToApp_Send(appsend, appsend[2]);
    }
    fclose(fd);
    fd = NULL;
}

//同步区域0xa1
void synchronizereg()
{
    int i = 1;
    int z = 0;
    int offset = 4;
    byte appsend[2048] = {0xa1,0x00,0x00,0x03};

    for(i = 1; i < 21; i++)
    {
        if(regcontrol[i].id == 0)
        {
            continue;
        }
        offset = 4;
        //id
        appsend[offset] = regcontrol[i].id;
        offset += 1;
        //绑定按键和id
        appsend[offset] = regcontrol[i].bingkeymac[0];
        appsend[offset + 1] = regcontrol[i].bingkeymac[1];
        appsend[offset + 2] = regcontrol[i].bingkeyid;
        offset += 3;
        //执行任务长度
        appsend[offset] = ((regcontrol[i].tasklen >> 8) & 0xff);
        appsend[offset + 1] = (regcontrol[i].tasklen & 0xff);
        offset += 2;
        //执行任务
        memcpy(appsend + offset, regcontrol[i].regtask, regcontrol[i].tasklen);
        offset += regcontrol[i].tasklen;
        //区域名称-长度
        appsend[offset] = regcontrol[i].namelen;
        offset += 1;
        memcpy(appsend + offset, regcontrol[i].regname, 16);
        offset += 16;
        //图标id
        appsend[offset] = regcontrol[i].iconid;
        offset += 1;
        //len
        appsend[1] = ((offset >> 8) & 0xff);
        appsend[2] = (offset & 0xff);
        //appsend
        UartToApp_Send(appsend, offset);
        /*
           pr_debug("synchronizereg\n");
           for(z = 0; z < (appsend[1] << 8) + appsend[2]; z++)
           {
           pr_debug("%.2x", appsend[z]);
           }
           pr_debug("\n");
         */
    }
    pr_debug("区域同步完毕\n");
}

//同步手动场景
void synchronizemansce()
{
    int i = 1;
    int offset = 4;
    byte appsend[2048] = {0xa1,0x00,0x00,0x04};

    for(i = 1; i < 21; i++)
    {
        if(mscecontrol[i].id == 0)
        {
            //pr_debug("空的手动场景\n");
        }
        else
        {
            offset = 4;
            //id
            appsend[offset] = mscecontrol[i].id;
            offset += 1;
            //绑定按键和id
            appsend[offset] = mscecontrol[i].bingkeymac[0];
            appsend[offset + 1] = mscecontrol[i].bingkeymac[1];
            appsend[offset + 2] = mscecontrol[i].bingkeyid;
            offset += 3;
            //执行任务长度
            appsend[offset] = ((mscecontrol[i].tasklen >> 8) & 0xff);
            appsend[offset + 1] = (mscecontrol[i].tasklen & 0xff);
            offset += 2;
            //执行任务
            memcpy(appsend + offset, mscecontrol[i].regtask, mscecontrol[i].tasklen);
            offset += mscecontrol[i].tasklen;
            //区域名称-长度
            appsend[offset] = mscecontrol[i].namelen;
            offset += 1;
            memcpy(appsend + offset, mscecontrol[i].regname, 16);
            offset += 16;
            //图标id
            appsend[offset] = mscecontrol[i].iconid;
            offset += 1;
            //len
            appsend[1] = ((offset >> 8) & 0xff);
            appsend[2] = (offset & 0xff);
            //appsend
            UartToApp_Send(appsend, offset);
        }
    }
    pr_debug("手动场景同步完毕\n");
}

//同步自动场景
void synchronizeautosce()
{
    int i = 1;
    int offset = 4;
    byte appsend[2048] = {0xa1,0x00,0x00,0x05};

    for(i = 1; i < 11; i++)
    {
        if(ascecontrol[i].id == 0)
        {
            //pr_debug("空的自动场景\n");
        }
        else
        {
            offset = 4;
            pr_debug("autosce_appsend[4]:%0x\n", appsend[4]);
            //id
            appsend[offset] = ascecontrol[i].id;
            offset += 1;
            //使能
            appsend[offset] = ascecontrol[i].enable;
            offset += 1;
            //当前状态
            appsend[offset] = ascecontrol[i].status;
            offset += 1;
            //循环时间
            appsend[offset] = ((ascecontrol[i].interval >> 8) & 0xff);
            appsend[offset + 1] = (ascecontrol[i].interval & 0xff);
            offset += 2;
            //预留6字节
            offset += 6;
            //触发模式
            appsend[offset] = ascecontrol[i].trimode;
            offset += 1;
            //限制条件
            memcpy(appsend + offset, ascecontrol[i].limitations, 5);
            offset += 5;
            //触发条件长度
            appsend[offset] = ascecontrol[i].tricondlen;
            offset += 1;
            //触发条件
            memcpy(appsend + offset, ascecontrol[i].tricond, ascecontrol[i].tricondlen);
            offset += ascecontrol[i].tricondlen;
            //执行任务长度
            appsend[offset] = ((ascecontrol[i].tasklen >> 8) & 0xff);
            appsend[offset + 1] = (ascecontrol[i].tasklen & 0xff);
            offset += 2;
            //////////////////////////////////
            //执行任务
            memcpy(appsend + offset, ascecontrol[i].regtask, ascecontrol[i].tasklen);
            offset += ascecontrol[i].tasklen;
            //区域名称-长度
            appsend[offset] = ascecontrol[i].namelen;
            offset += 1;
            memcpy(appsend + offset, ascecontrol[i].regname, 16);
            offset += 16;
            //图标id
            appsend[offset] = ascecontrol[i].iconid;
            offset += 1;
            //len
            appsend[1] = ((offset >> 8) & 0xff);
            appsend[2] = (offset & 0xff);
            //appsend
            UartToApp_Send(appsend, offset);
        }
    }
    pr_debug("自动场景同步完毕:%d\n", i);
}

//同步红外快捷方式
void synchronizeirfast()
{
    FILE *fd = NULL;
    int i = 0;
    byte appsend[128]  = {0xa1,0x00};
    byte irstr[1024] = {0};
    byte irhex[512] = {0};

    //len
    appsend[2] =  0x29;
    appsend[3] = 0x06;

    if((fd = fopen(IRFAST, "r")) == NULL)
    {
        pr_debug("%s: %s\n", IRFAST, strerror(errno));
        return;
    }
    while(fgets(irstr, 1024, fd))
    {
        if(strlen(irstr) < 20)
        {
            continue;
        }
        StrToHex(irhex, irstr, strlen(irstr) / 2);
        
        //拼接索引和mac
        memcpy(appsend + 4, irhex, 4);
        //不上传图标id
        memcpy(appsend + 8, irhex + ((strlen(irstr) / 2) - 34), 33);

        UartToApp_Send(appsend, appsend[2]);
    }
    fclose(fd);
    fd = NULL;
}

//同步安防信息
void synchronizedsecinfo()
{
    int i = 1;
    int offset = 4;
    byte appsend[2048] = {0xa1,0x00,0x0,0x07};

    for(i = 1; i < 9; i++)
    {
        if(secmonitor[i].id == 0)
        {
            continue;
        }
        offset = 4;
        //id
        appsend[offset] = i;
        offset += 1;
        //使能
        appsend[offset] = secmonitor[i].enable;
        offset += 1;
        //当前智能布防状态
        appsend[offset] = secmonitor[i].secmonitor;
        offset += 1;
        //当前布防执行状态
        appsend[offset] = secmonitor[i].status;
        offset += 1;
        //启动布防时间段
        memcpy(appsend + offset, secmonitor[i].armingtime, 5);
        offset += 5;
        //10字节预留
        offset += 10;
        //布防启动条件长度
        appsend[offset] = ((secmonitor[i].secstartlen >> 8) & 0xff);
        appsend[offset + 1] = (secmonitor[i].secstartlen & 0xff);
        offset += 2;
        //布防启动条件
        memcpy(appsend + offset, secmonitor[i].secstartcond, secmonitor[i].secstartlen);
        offset += secmonitor[i].secstartlen;
        //布防条件长度
        appsend[offset] = ((secmonitor[i].tricondlen >> 8) & 0xff);
        appsend[offset + 1] = (secmonitor[i].tricondlen & 0xff);
        offset += 2;
        //布防条件
        memcpy(appsend + offset, secmonitor[i].tricond, secmonitor[i].tricondlen);
        offset += secmonitor[i].tricondlen;
        //撤防条件长度
        appsend[offset] = ((secmonitor[i].disarmlen >> 8) & 0xff);
        appsend[offset + 1] = (secmonitor[i].disarmlen & 0xff);
        offset += 2;
        //撤防条件
        memcpy(appsend + offset, secmonitor[i].disarmtask, secmonitor[i].disarmlen);
        offset += secmonitor[i].disarmlen;
        //报警条件长度
        appsend[offset] = ((secmonitor[i].secenfolen >> 8) & 0xff);
        appsend[offset + 1] = (secmonitor[i].secenfolen & 0xff);
        offset += 2;
        //报警条件
        memcpy(appsend + offset, secmonitor[i].secenforce, secmonitor[i].secenfolen);
        offset += secmonitor[i].secenfolen;
        //报警任务长度
        appsend[offset] = ((secmonitor[i].tasklen >> 8) & 0xff);
        appsend[offset + 1] = (secmonitor[i].tasklen & 0xff);
        offset += 2;
        //报警任务
        memcpy(appsend + offset, secmonitor[i].regtask, secmonitor[i].tasklen);
        offset += secmonitor[i].tasklen;
        //len
        appsend[1] = ((offset >> 8) & 0xff);
        appsend[2] = (offset & 0xff);
        //appsend
        UartToApp_Send(appsend, offset);
    }
    pr_debug("安防信息同步完毕\n");
}

//同步智能锁
void synchronizesmartlock()
{
    FILE *fd = NULL;
    int i = 0;
    byte appsend[64]  = {0xa1,0x00};
    byte irstr[128] = {0};
    byte irhex[64] = {0};

    appsend[3] = 0x12;

    if((fd = fopen(SMARTLOCK, "r")) == NULL)
    {
        pr_debug("%s: %s\n", SMARTLOCK, strerror(errno));
        return;
    }
    while(fgets(irstr, 128, fd))
    {
        StrToHex(irhex, irstr, (strlen(irstr) / 2));
        
        //拼接索引和mac
        memcpy(appsend + 4, irhex, (strlen(irstr) / 2));
        appsend[2] = (4 + (strlen(irstr) / 2));
        UartToApp_Send(appsend, appsend[2]);
    }
    fclose(fd);
    fd = NULL;
    pr_debug("智能锁同步完毕\n");
}

//同步智能控制面板
void synchronizesmartctrpane()
{
    int i = 0;
    byte appsend[24] = {0xa1,0x00,0x0e,0x14};

    for(i = 0; i < 20; i++)
    {
        if(smartcrlpanel[i].trimac[1] == 0)
        {
            pr_debug("智能控制面板同步完成:%d\n", i);
            break;
        }
        else
        {
            //mac
            appsend[4] = smartcrlpanel[i].trimac[0];
            appsend[5] = smartcrlpanel[i].trimac[1];
            //id
            appsend[6] = smartcrlpanel[i].triid;
            //type
            appsend[7] = smartcrlpanel[i].type;
            //执行任务
            memcpy(appsend + 8, smartcrlpanel[i].perftask, 6);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//同步关联信息
void synchronizerelatedinfo()
{
    int i = 0;
    byte appsend[20] = {0xa1,0x00,0x0f,0x13};

    for(i = 0; i < 260; i++)
    {
        if(gesturebindinfo[i].trimac[1] == 0)
        {
            pr_debug("手势关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x01;
            //mac
            appsend[5] = gesturebindinfo[i].trimac[0];
            appsend[6] = gesturebindinfo[i].trimac[1];
            //id
            appsend[7] = gesturebindinfo[i].triid;
            //type
            appsend[8] = gesturebindinfo[i].type;
            //mac
            appsend[9] = gesturebindinfo[i].carmac[0];
            appsend[10] = gesturebindinfo[i].carmac[1];
            //id
            appsend[11] = gesturebindinfo[i].carid;
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(smartlockbindinfo[i].trimac[1] == 0)
        {
            pr_debug("智能锁关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x02;
            //mac
            appsend[5] = smartlockbindinfo[i].trimac[0];
            appsend[6] = smartlockbindinfo[i].trimac[1];
            //id
            appsend[7] = smartlockbindinfo[i].triid;
            //type
            appsend[8] = smartlockbindinfo[i].type;
            //mac
            appsend[9] = smartlockbindinfo[i].carmac[0];
            appsend[10] = smartlockbindinfo[i].carmac[1];
            //id
            appsend[11] = smartlockbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, smartlockbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(shockbindinfo[i].trimac[1] == 0)
        {
            pr_debug("震动关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x03;
            //mac
            appsend[5] = shockbindinfo[i].trimac[0];
            appsend[6] = shockbindinfo[i].trimac[1];
            //id
            appsend[7] = shockbindinfo[i].triid;
            //type
            appsend[8] = shockbindinfo[i].type;
            //mac
            appsend[9] = shockbindinfo[i].carmac[0];
            appsend[10] = shockbindinfo[i].carmac[1];
            //id
            appsend[11] = shockbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, shockbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(smokebindinfo[i].trimac[1] == 0)
        {
            pr_debug("烟雾关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x04;
            //mac
            appsend[5] = smokebindinfo[i].trimac[0];
            appsend[6] = smokebindinfo[i].trimac[1];
            //id
            appsend[7] = smokebindinfo[i].triid;
            //type
            appsend[8] = smokebindinfo[i].type;
            //mac
            appsend[9] = smokebindinfo[i].carmac[0];
            appsend[10] = smokebindinfo[i].carmac[1];
            //id
            appsend[11] = smokebindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, smokebindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(doormagnetbindinfo[i].trimac[1] == 0)
        {
            pr_debug("门磁关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x05;
            //mac
            appsend[5] = doormagnetbindinfo[i].trimac[0];
            appsend[6] = doormagnetbindinfo[i].trimac[1];
            //id
            appsend[7] = doormagnetbindinfo[i].triid;
            //type
            appsend[8] = doormagnetbindinfo[i].type;
            //mac
            appsend[9] = doormagnetbindinfo[i].carmac[0];
            appsend[10] = doormagnetbindinfo[i].carmac[1];
            //id
            appsend[11] = doormagnetbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, doormagnetbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(floodingbindinfo[i].trimac[1] == 0)
        {
            pr_debug("水浸关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x06;
            //mac
            appsend[5] = floodingbindinfo[i].trimac[0];
            appsend[6] = floodingbindinfo[i].trimac[1];
            //id
            appsend[7] = floodingbindinfo[i].triid;
            //type
            appsend[8] = floodingbindinfo[i].type;
            //mac
            appsend[9] = floodingbindinfo[i].carmac[0];
            appsend[10] = floodingbindinfo[i].carmac[1];
            //id
            appsend[11] = floodingbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, floodingbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(iractivitybindinfo[i].trimac[1] == 0)
        {
            pr_debug("红外活动侦测关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x07;
            //mac
            appsend[5] = iractivitybindinfo[i].trimac[0];
            appsend[6] = iractivitybindinfo[i].trimac[1];
            //id
            appsend[7] = iractivitybindinfo[i].triid;
            //type
            appsend[8] = iractivitybindinfo[i].type;
            //mac
            appsend[9] = iractivitybindinfo[i].carmac[0];
            appsend[10] = iractivitybindinfo[i].carmac[1];
            //id
            appsend[11] = iractivitybindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, iractivitybindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(rainsnowbindinfo[i].trimac[1] == 0)
        {
            pr_debug("雨雪关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x08;
            //mac
            appsend[5] = rainsnowbindinfo[i].trimac[0];
            appsend[6] = rainsnowbindinfo[i].trimac[1];
            //id
            appsend[7] = rainsnowbindinfo[i].triid;
            //type
            appsend[8] = rainsnowbindinfo[i].type;
            //mac
            appsend[9] = rainsnowbindinfo[i].carmac[0];
            appsend[10] = rainsnowbindinfo[i].carmac[1];
            //id
            appsend[11] = rainsnowbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, rainsnowbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(gasbindinfo[i].trimac[1] == 0)
        {
            pr_debug("燃气关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x09;
            //mac
            appsend[5] = gasbindinfo[i].trimac[0];
            appsend[6] = gasbindinfo[i].trimac[1];
            //id
            appsend[7] = gasbindinfo[i].triid;
            //type
            appsend[8] = gasbindinfo[i].type;
            //mac
            appsend[9] = gasbindinfo[i].carmac[0];
            appsend[10] = gasbindinfo[i].carmac[1];
            //id
            appsend[11] = gasbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, gasbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(windspeedbindinfo[i].trimac[1] == 0)
        {
            pr_debug("风速关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x0a;
            //mac
            appsend[5] = windspeedbindinfo[i].trimac[0];
            appsend[6] = windspeedbindinfo[i].trimac[1];
            //id
            appsend[7] = windspeedbindinfo[i].triid;
            //type
            appsend[8] = windspeedbindinfo[i].type;
            //mac
            appsend[9] = windspeedbindinfo[i].carmac[0];
            appsend[10] = windspeedbindinfo[i].carmac[1];
            //id
            appsend[11] = windspeedbindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, windspeedbindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
    for(i = 0; i < 260; i++)
    {
        if(carmoxdebindinfo[i].trimac[1] == 0)
        {
            pr_debug("一氧化碳关联同步完成:%d\n", i);
            break;
        }
        else
        {
            //type
            appsend[4] = 0x0b;
            //mac
            appsend[5] = carmoxdebindinfo[i].trimac[0];
            appsend[6] = carmoxdebindinfo[i].trimac[1];
            //id
            appsend[7] = carmoxdebindinfo[i].triid;
            //type
            appsend[8] = carmoxdebindinfo[i].type;
            //mac
            appsend[9] = carmoxdebindinfo[i].carmac[0];
            appsend[10] = carmoxdebindinfo[i].carmac[1];
            //id
            appsend[11] = carmoxdebindinfo[i].carid;
            //节点执行状态
            memcpy(appsend + 12, carmoxdebindinfo[i].execstatus, 3);

            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//信息同步 0x31
void node_up(byte *buf)
{
    pr_debug("信息上传\n");
    byte synover[10] = {0xa1,0x00,0x05,0xff,0x00};
    byte synstart[10] = {0xa1,0x00,0x05,0x00};

    synstart[4] = buf[3];
    UartToApp_Send(synstart, synstart[2]);
    
    synover[4] = hostinfo.dataversion;
    switch(buf[3])
    {
        case 0x01:
            pr_debug("节点同步\n");
            synchronizeslave();
            synchronizenode();
            synchronizenodename();
            synchronizelocalass();
            synchronizesmartlock();
            synchronizerelatedinfo();
            break;
        case 0x02:
            pr_debug("红外设备同步\n");
            synchronizeirdev();
            break;
        case 0x03:
            pr_debug("区域同步\n");
            synchronizereg();
            break;
        case 0x04:
            pr_debug("手动场景同步\n");
            synchronizemansce();
            break;
        case 0x05:
            pr_debug("自动场景同步\n");
            synchronizeautosce();
            getautoscestatus(buf);
            break;
        case 0x06:
            pr_debug("红外快捷方式同步\n");
            synchronizeirfast();
            break;
        case 0x07:
            pr_debug("安防信息同步\n");
            synchronizedsecinfo();
            break;
        case 0x12:
            pr_debug("智能锁同步\n");
            synchronizesmartlock();
            break;
        case 0x13:
            pr_debug("联动信息\n");
            synchronizerelatedinfo();
            break;
        case 0x14:
            pr_debug("智能控制面板同步\n");
            synchronizesmartctrpane();
            break;
        case 0xff:
            synchronizeslave();
            synchronizenode();
            synchronizesmartlock();
            synchronizerelatedinfo();
            synchronizesmartctrpane();
            synchronizenodename();
            synchronizelocalass();
            synchronizeirdev();
            synchronizereg();
            synchronizemansce();
            synchronizeautosce();
            getautoscestatus(buf);
            synchronizeirfast();
            synchronizedsecinfo();
            break;
        default:
            pr_debug("node_up unsupported type\n");
            break;
    }
    UartToApp_Send(synover, synover[2]);
}

//修改区域配置
void modifyregation(int sub, byte *buf)
{
    int pr_i = 0;
    //存放有效数据
    byte appscene[20] = {0x64,0x0c};
    byte appsend[10] = {0xc5,0x00,0x05,0x00,0x00};
    byte manualsce[1250] = {0};
    byte manualsceope[1250] = {0};
    byte manualsceope_hex[10] = {0};
    byte manualsce_h[20] = {0};
    byte manualsceope_h[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    //按键标志
    int oldkey_logo = 0;
    int key_logo = 0;

    int type = 0;
    int newtype = 0;
    int tempcount = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("manualscenario_len:%d\n", len);

    //新配置截取，从配置开始
    memcpy(manualsce, buf + 9, len - 27);
    //原配置截取，从配置开始
    memcpy(manualsceope, regcontrol[sub].regtask, regcontrol[sub].tasklen);
    while(1)
    {
        //判断场景信息是否执行完毕
        if(manualsceope[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            break;
        }
        auto_logo = 0;
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsceope[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01:%d\n", type);
                memcpy(manualsceope_hex, manualsceope + type, 8);
                int y = 0;
                /*
                for(y = 0; y < 6; y++)
                {
                    pr_debug("manualsceope_hex[%d]:%0x\n", y, manualsceope_hex[y]);
                }
                */
                while(1)
                {
                    pr_debug("auto_logo:%d\n", auto_logo);
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        //type += 5;
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(manualsce[newtype] == 0)
                    {
                        pr_debug("场景指令发送完毕\n");
                        //检索完毕，原配置无对应节点
                        //删除
                        if(auto_logo == 0)
                        {
                            appscene[2] = buf[3];
                            //配置标识-删除
                            appscene[3] = 0x01;
                            //mac
                            appscene[4] = manualsceope_hex[2];
                            appscene[5] = 0x00;
                            
                            if(appscene[4] == regcontrol[sub].bingkeymac[1])
                            {
                                appscene[5] = regcontrol[sub].bingkeyid;
                                oldkey_logo = 1;
                                //执行状态
                                memcpy(appscene + 6, manualsceope_hex + 3, 2);
                            }
                            else
                            {
                                //执行状态
                                memcpy(appscene + 6, manualsceope_hex + 3, 2);
                            }
                            if(appscene[4] == buf[5])
                            {
                                appscene[3] = 0x00;
                                appscene[5] = buf[6];
                                appscene[6] = 0xaa;
                                appscene[7] = 0xaa;
                                key_logo = 1;
                            }
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            for(pr_i = 0; pr_i < 9; pr_i++)
                            {
                                pr_debug("appscene1[%d]:%0x\n", pr_i, appscene[pr_i]);
                            }
                            tempcount++;
                        }
                        //type += 5;
                        newtype = 0;
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(manualsce[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_h, manualsce + newtype, 8);
                            /*
                            int b = 0;
                            for(b = 0; b < 6; b++)
                            {
                                pr_debug("manualsceope_h[%d]:%0x\n", b, manualsceope_h[b]);
                            }
                            */
                            //mac相同-修改
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 3))
                            {
                                pr_debug("case 0x01(2)\n");
                                //有改动
                                if(0 != memcmp(manualsceope_h, manualsceope_hex, 5))
                                {
                                    pr_debug("case 0x01(3)\n");
                                    //准备发送指令
                                    //id
                                    appscene[2] = buf[3];
                                    //配置标识-新增
                                    appscene[3] = 0x00;
                                    //mac
                                    appscene[4] = manualsceope_h[2];
                                    if(appscene[4] == regcontrol[sub].bingkeymac[1])
                                    {
                                        oldkey_logo = 1;
                                    }
                                    //与新绑定按键mac进行对比
                                    if(appscene[4] == buf[5])
                                    {
                                        appscene[5] = buf[6];
                                        key_logo = 1;
                                    }
                                    else
                                    {
                                        appscene[5] = 0x00;
                                    }
                                    //执行状态
                                    memcpy(appscene + 6, manualsceope_h + 3, 2);
                                    //发送控制指令
                                    UART0_Send(uart_fd, appscene, appscene[1]);
                                    for(pr_i = 0; pr_i < 9; pr_i++)
                                    {
                                        pr_debug("appscene2[%d]:%0x\n", pr_i, appscene[pr_i]);
                                    }
                                    tempcount++;

                                    auto_logo = 1;
                                }
                                //无改动
                                else
                                {
                                    //检查是否删除绑定按键
                                    if((manualsceope_h[2] == regcontrol[sub].bingkeymac[1]) && (buf[5] != regcontrol[sub].bingkeymac[1]))
                                    {
                                            pr_debug("case 0x01(4)\n");
                                            //准备发送指令
                                            //id
                                            appscene[2] = buf[3];
                                            //配置标识-新增
                                            appscene[3] = 0x00;
                                            //mac
                                            appscene[4] = manualsceope_h[2];
                                            appscene[5] = 0x00;
                                            //执行状态
                                            memcpy(appscene + 6, manualsceope_h + 3, 2);
                                            //发送控制指令
                                            UART0_Send(uart_fd, appscene, appscene[1]);
                                    for(pr_i = 0; pr_i < 9; pr_i++)
                                    {
                                        pr_debug("appscene3[%d]:%0x\n", pr_i, appscene[pr_i]);
                                    }
                                            
                                            oldkey_logo = 1;
                                            //key_logo = 1;
                                            tempcount++;
                                    }
                                    else if(manualsceope_h[2] == buf[5])
                                    {
                                        pr_debug("case 0x01(5)\n");
                                        //准备发送指令
                                        //id
                                        appscene[2] = buf[3];
                                        //配置标识-新增
                                        appscene[3] = 0x00;
                                        //mac
                                        appscene[4] = manualsceope_h[2];
                                        appscene[5] = buf[6];
                                        //执行状态
                                        memcpy(appscene + 6, manualsceope_h + 3, 2);
                                        //发送控制指令
                                        UART0_Send(uart_fd, appscene, appscene[1]);
                                    for(pr_i = 0; pr_i < 9; pr_i++)
                                    {
                                        pr_debug("appscene4[%d]:%0x\n", pr_i, appscene[pr_i]);
                                    }

                                        //oldkey_logo = 1;
                                        key_logo = 1;
                                        tempcount++;
                                    }
                                    auto_logo = 1;
                                }
                            }
                            newtype += 8;
                            break;
                        default:
                            pr_debug("手动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }

                type += 8;
                break;
            default:
                pr_debug("手动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }

    type = 0;
    newtype = 0;
    auto_logo = 0;
    while(1)
    {
        //判断场景信息是否执行完毕
        pr_debug("反向:%d\n", type);
        if(manualsce[type] == 0)
        {
            pr_debug("场景指令发送完毕\n");
            //新按键绑定-新增
            if((key_logo != 1) && (buf[5] > 0) && (buf[5] < 0xff))
            {
                pr_debug("key_logo发送1\n");
                //准备发送指令
                appscene[2] = buf[3];
                appscene[3] = 0x00;
                //mac
                appscene[4] = buf[5];
                //按键id
                appscene[5] = buf[6];
                //保持
                appscene[6] = 0xaa;
                appscene[7] = 0xaa;
                UART0_Send(uart_fd, appscene, appscene[1]);
                for(pr_i = 0; pr_i < 9; pr_i++)
                {
                    pr_debug("appscene5[%d]:%0x\n", pr_i, appscene[pr_i]);
                }
                tempcount++;
            }
            //删除旧绑定按键
            if((oldkey_logo != 1) && (regcontrol[sub].bingkeymac[1] > 0) && (regcontrol[sub].bingkeymac[1] < 0xff) && (regcontrol[sub].bingkeymac[1] != buf[5]))
            {
                pr_debug("key_logo发送1\n");
                //准备发送指令
                appscene[2] = regcontrol[sub].id;
                appscene[3] = 0x01;
                //mac
                appscene[4] = regcontrol[sub].bingkeymac[1];
                //按键id
                appscene[5] = regcontrol[sub].bingkeyid;
                //保持
                appscene[6] = 0xaa;
                appscene[7] = 0xaa;
                UART0_Send(uart_fd, appscene, appscene[1]);
                for(pr_i = 0; pr_i < 9; pr_i++)
                {
                    pr_debug("appscene6[%d]:%0x\n", pr_i, appscene[pr_i]);
                }
                tempcount++;
            }
            break;
        }
        auto_logo = 0;
        newtype = 0;
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                memcpy(manualsceope_h, manualsce + type, 8);
                while(1)
                {
                    if(auto_logo == 1)
                    {
                        pr_debug("本条执行完毕\n");
                        newtype = 0;
                        break;
                    }
                    //判断场景信息是否执行完毕
                    if(manualsceope[newtype] == 0)
                    {
                        pr_debug("场景指令发送完毕(2)\n");
                        //检索完毕，新配置无对应节点
                        //新增
                        if(auto_logo == 0)
                        {
                            //准备发送指令
                            //id
                            appscene[2] = buf[3];
                            //配置标识-新增
                            appscene[3] = 0x00;
                            //mac
                            appscene[4] = manualsceope_h[2];
                            if(appscene[4] == regcontrol[sub].bingkeymac[1])
                            {
                                oldkey_logo = 1;
                            }
                            //新
                            if(appscene[4] == buf[5])
                            {
                                appscene[5] = buf[6];
                                key_logo = 1;
                            }
                            else
                            {
                                appscene[5] = 0x00;
                            }
                            //执行状态
                            memcpy(appscene + 6, manualsceope_h + 3, 2);
                            //发送控制指令
                            UART0_Send(uart_fd, appscene, appscene[1]);
                            for(pr_i = 0; pr_i < 9; pr_i++)
                            {
                                pr_debug("appscene7[%d]:%0x\n", pr_i, appscene[pr_i]);
                            }
                            tempcount++;
                        }
                        break;
                    }
                    //根据device Type拷贝指令长度
                    //manualsceope[0]场景首个设备类型
                    switch(manualsceope[newtype])
                    {
                        //指令长度相同，统一处理
                        //继电器，小夜灯，背光灯
                        case 0x01:
                        case 0x11:
                        case 0x12:
                        case 0x21:
                        case 0x31:
                            memcpy(manualsceope_hex, manualsceope + newtype, 8);
                            //mac相同
                            if(0 == memcmp(manualsceope_h, manualsceope_hex, 3))
                            {
                                auto_logo = 1;
                            }
                            newtype += 8;
                            break;
                        default:
                            pr_debug("手动场景不支持的类型\n");
                            auto_logo = 1;
                            break;
                    }
                }
                type += 8;
                break;
            default:
                pr_debug("手动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);
    
    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;
    //指令头
    memcpy(regionaltemp.reghead, buf + 3, 6);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务
    memcpy(regionaltemp.regassoc, buf + 9, len - 27);

    if(0 == tempcount)
    {
        storeinfo();
        appsend[3] = buf[3];
        UartToApp_Send(appsend, appsend[2]);
    }
}

//新增区域配置
void newregassociation(int id, byte *buf)
{
    //存放有效数据
    byte manualsce[1250] = {0};
    byte manualsceope[1250] = {0};
    byte manualsceope_hex[10] = {0};
    byte manualsce_s[20] = {0};
    byte manualsce_str_a[20] = {0};
    byte manualsce_h[20] = {0};
    byte appscene[20] = {0x64,0x0c};
    byte str[BUFFER_SIZE] = {0};
    int auto_logo = 0;
    int type = 0;
    int key_logo = 0;
    int tempcount = 0;
    int len = (buf[1] << 8) + buf[2];
    pr_debug("newrega_len:%d\n", len);

    //截取有效数据
    memcpy(manualsce, buf + 9, len - 27);
    //分配ID
    memcpy(manualsceope, buf + 3, len - 3);
    manualsceope[0] = id;
    while(1)
    {
        pr_debug("manualsce[%d]:%0x\n", type, manualsce[type]);
        //判断场景信息是否执行完毕
        if(manualsce[type] == 0)
        {
            pr_debug("区域指令发送完毕\n");
            if((key_logo != 1) && (buf[5] > 0) && (buf[5] < 0xff))
            {
                //准备发送指令
                appscene[2] = id;
                //memcpy(appscene + 3, manualsceope_hex + 2, 3);
                //配置标识-新增
                appscene[3] = 0x00;
                //mac
                appscene[4] = buf[5];
                appscene[5] = buf[6];
                //保持
                appscene[6] = 0xaa;
                appscene[7] = 0xaa;
                UART0_Send(uart_fd, appscene, appscene[1]);
                tempcount++;
            }
            break;
        }
        if(auto_logo == 1)
        {
            pr_debug("区域指令执行完毕\n");
            break;
        }
        //根据device Type拷贝指令长度
        //manualsceope[0]场景首个设备类型
        switch(manualsce[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01\n");
                memcpy(manualsceope_hex, manualsce + type, 8);
                //准备发送指令
                appscene[2] = id;
                //memcpy(appscene + 3, manualsceope_hex + 2, 3);
                //配置标识-新增
                appscene[3] = 0x00;
                //mac
                appscene[4] = manualsceope_hex[2];
                //按键绑定
                if(appscene[4] == buf[5])
                {
                    appscene[5] = buf[6];
                    key_logo = 1;
                }
                else
                {
                    appscene[5] = 0x00;
                }
                //执行状态
                memcpy(appscene + 6, manualsceope_hex + 3, 2);
                //appscene[4] &= 0xf0;
                //发送控制指令
                UART0_Send(uart_fd, appscene, appscene[1]);
                tempcount++;
                type += 8;
                break;
            default:
                pr_debug("区域不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    //记录最后一条配置
    memcpy(sceneack.endbuf, appscene, 12);
    /*
    byte manualsce_str[BUFFER_SIZE] = {0};
    HexToStr(manualsce_str, manualsceope, len - 3);
    replace(REGASSOC, manualsce_str, 2);
    readareainfo();
    */
    regionaltemp.len = len - 3;
    regionaltemp.count = tempcount;
    //指令头
    memcpy(regionaltemp.reghead, manualsceope, 6);
    //regname
    memcpy(regionaltemp.regname, buf + (len - 18), 18);
    //执行任务
    memcpy(regionaltemp.regassoc, manualsceope + 6, len - 24);
}

//区域功能配置(最多64个) 0x55  0x64
void regassociation(byte *buf)
{
    byte charid[5] = {0};
    byte idbuf[5] = {0};
    //byte tempcharid[10] = {0};
    //存放有效数据
    byte manualsce[1024] = {0};
    byte idhex[5] = {0};
    byte str[2048] = {0};
    byte manualsce_hex[1024] = {0};
    byte appsend[10] = {0xc5,0x00,0x05,0x00,0x01};
    int tempid = 0;
    int id = 1;
    int idlogo = 0;

    //初始化应答结构
    memset(&sceneack, 0, sizeof(sceneack));
    memset(&regionaltemp, 0, sizeof(regionaltemp));
    
    sceneack.headbuf[0] = 0xc5;
    sceneack.headbuf[1] = 0x00;
    sceneack.headbuf[2] = 0x05;
    sceneack.headbuf[4] = 0x00;

    //开始记录
    sceneack.starlog = 1;
    hostinfo.dataversion += 1;

    //新配置
    if(buf[3] == 0)
    {
        FILE *fd = NULL;
        if((fd = fopen(REGASSOC, "a+")) == NULL)
        {
            pr_debug("%s: %s\n", REGASSOC, strerror(errno));
            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, REGASSOC, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
            //return;
        }
        //截取有效数据从ID开始
        //memcpy(manualsce, buf + 2, len - 2);
        int effective = 4;
        while(fgets(str, 2048, fd))
        {
            if(strlen(str) < 10)
            {
                str[2] = '\0';
                StrToHex(idhex, str, 1);
                tempid = idhex[0];
                id++;
            }
            else
            {
                effective++;
                id++;
            }
            memset(str, 0, sizeof(str));
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        fclose(fd);
        fd = NULL;
        if(id >= 21)
        {
            id = tempid;
        }
        if((id < 5) && (effective >= 20))
        {
            pr_debug("区域配置已达上限1\n");
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        pr_debug("tempid:%d, id:%d\n", tempid, id);
        sceneack.headbuf[3] = id;
        newregassociation(id, buf);
    }
    //修改配置
    else
    {
        sceneack.headbuf[3] = buf[3];
        if(regcontrol[buf[3]].id == 0)
        {
            if((buf[3] > 0) && (buf[3] < 5))
            {
                newregassociation(buf[3], buf);
            }
            else
            {
                pr_debug("修改区域失败\n");
                UartToApp_Send(appsend, appsend[2]);
                return;
            }
        }

        modifyregation(buf[3], buf);
    }
}

//app区域操作 0x63  0x65
void regoperation(byte *buf)
{
    byte scenerun[10] = {0x65,0x0c,0x00,0x00,0x00};
    byte appsend[10] = {0xd3,0x00,0x04,0x00};
    if((buf[3] == regcontrol[buf[3]].id) && (regcontrol[buf[3]].tasklen > 0))
    {
        //区域执行命令(广播) 0x65
        //id
        scenerun[2] = buf[3];
        //mac
        if((buf[3] >= 1) && (buf[3] <= 4))
        {
            scenerun[3] = 0xff;
        }
        else
        {
            scenerun[3] = regcontrol[buf[3]].bingkeymac[1];
        }
        //node id
        scenerun[4] = regcontrol[buf[3]].bingkeyid;
        memcpy(scenerun + 5, buf + 4, 4);
        UART0_Send(uart_fd, scenerun, scenerun[1]);

        UartToApp_Send(appsend, 4);
    }
    else
    {
        appsend[3] = 1;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//app删除区域0x57  0x66
void appdelarea(byte *buf)
{
    if(buf[3] <= 0)
    {
        return;
    }
    byte scenedel[15] = {0x64,0x0c,0x00,0x01,0xff};
    byte delmanualscenario_str[10] = {0};
    byte delmanualscenario_buf[6] = {0};
    byte appsend[10] = {0xc7,0x00,0x05,0x00,0x00};
    HexToStr(delmanualscenario_str, buf, buf[2]);
    memcpy(delmanualscenario_buf, delmanualscenario_str + 6, 2);
    
    if(1 == replace(REGASSOC, delmanualscenario_buf, 2))
    {
        hostinfo.dataversion += 1;
        //删除区域(广播)
        scenedel[2] = buf[3];
        UART0_Send(uart_fd, scenedel, scenedel[1]);

        appsend[3] = buf[3];
        UartToApp_Send(appsend, 5);
        //
        readareainfo();
    }
    else
    {
        //失败
        appsend[3] = buf[3];
        appsend[4] = 0x01;
        UartToApp_Send(appsend, 5);
    }
}

//节点状态查询 0x18
void nodequery(byte *buf, char *FileName)
{
    FILE *fd = NULL;
    byte nodequery_send[10] = {0x19,0x06};
    byte nodequery_str[30] = {0};
    byte nodequery_hex[15] = {0};
    if((fd = fopen(FileName, "r")) == NULL)
    {
        pr_debug("nodequery_FILENAME_file open failed\n");
        byte logsend[256] = {0xf3,0x00};
        byte logbuf[256] = {0};
        sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, FileName, strerror(errno));
        logsend[2] = (3 + strlen(logbuf));
        memcpy(logsend + 3, logbuf, strlen(logbuf));
        send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
        return;
    }
    if(buf[0] == 0x18)//节点
    {
        while(fgets(nodequery_str, 30, fd))
        { 
            StrToHex(nodequery_hex, nodequery_str, strlen(nodequery_str) / 2);
            //类型和mac是否一致
            if((buf[2] == nodequery_hex[4]) && (buf[3] == nodequery_hex[0]) && (buf[4] == nodequery_hex[1]))
            {
                //节点状态反馈（应答）0x19
                memcpy(nodequery_send + 2, buf + 2, 3);
                nodequery_send[5] = nodequery_hex[6];
                UartToApp_Send(nodequery_send, 6);
                break;
            }
            else
            {
                continue;
            }
        }
    }
    else if(buf[0] == 0x1a)//温度 0x1b
    {
        while(fgets(nodequery_str, 30, fd))
        { 
            StrToHex(nodequery_hex, nodequery_str, strlen(nodequery_str) / 2);
            //mac是否一致
            if(buf[4] == nodequery_hex[0])
            {
                //节点状态反馈（应答）0x1b
                nodequery_send[0] = 0x1b;
                nodequery_send[2] = 0x00;
                nodequery_send[3] = 0x01;
                nodequery_send[4] = buf[4];
                nodequery_send[5] = nodequery_hex[2];
                UartToApp_Send(nodequery_send, 6);
                break;
            }
            else
            {
                continue;
            }
        }
    
    }
    else if(buf[0] == 0x1c)//地震 0x1d
    {
        while(fgets(nodequery_str, 30, fd))
        { 
            StrToHex(nodequery_hex, nodequery_str, strlen(nodequery_str) / 2);
            //类型和mac是否一致
            if(buf[4] == nodequery_hex[0])
            {
                //节点状态反馈（应答）0x1d
                nodequery_send[0] = 0x1d;
                nodequery_send[1] = 0x08;
                nodequery_send[2] = 0x00;
                nodequery_send[3] = 0x01;
                nodequery_send[4] = buf[4];
                nodequery_send[5] = nodequery_hex[1];
                nodequery_send[6] = nodequery_hex[2];
                if((nodequery_send[5] > 0) || (nodequery_send[6] > 0))
                {
                    nodequery_send[7] = 0x01;
                }
                else if((nodequery_send[5] == 0) && (nodequery_send[6] == 0))
                {
                    nodequery_send[7] == 0x00;
                }
                UartToApp_Send(nodequery_send, 8);
                break;
            }
            else
            {
                continue;
            }
        }
    
    }
    else if(buf[0] == 0x1e)//人体感应 0x1f
    {
        while(fgets(nodequery_str, 30, fd))
        { 
            StrToHex(nodequery_hex, nodequery_str, strlen(nodequery_str) / 2);
            //类型和mac是否一致
            if(buf[4] == nodequery_hex[0])
            {
                //节点状态反馈（应答）0x1f
                nodequery_send[0] = 0x1f;
                nodequery_send[1] = 0x06;
                nodequery_send[2] = 0x00;
                nodequery_send[3] = 0x01;
                nodequery_send[4] = buf[4];
                if(0x02 == (nodequery_hex[2] & 0x02))
                {
                    nodequery_send[5] = 0x01;
                }
                else if(0x00 == (nodequery_hex[2] & 0x02))
                {
                    nodequery_send[5] = 0x00;
                }
                UartToApp_Send(nodequery_send, 6);
                break;
            }
            else
            {
                continue;
            }
        }
    
    }
    else if(buf[0] == 0x20)//光感 0x21
    {
        while(fgets(nodequery_str, 30, fd))
        { 
            StrToHex(nodequery_hex, nodequery_str, strlen(nodequery_str) / 2);
            //类型和mac是否一致
            if(buf[4] == nodequery_hex[0])
            {
                //节点状态反馈（应答）0x21
                nodequery_send[0] = 0x21;
                nodequery_send[1] = 0x06;
                nodequery_send[2] = 0x00;
                nodequery_send[3] = 0x01;
                nodequery_send[4] = buf[4];
                nodequery_send[5] = nodequery_hex[1];
                UartToApp_Send(nodequery_send, 6);
                break;
            }
            else
            {
                continue;
            }
        }    
    }
    fclose(fd);
    fd = NULL;
}

//设置开关光感阀门值 0x22
void lightvalve(byte *buf)
{
    byte lightvalve_h[10] = {0};
    byte lightvalve_s[10] = {0};
    byte lightvalve_send[10] = {0x23,0x06};
    byte lightvalve_usend[10] = {0x31,0x15};
    memcpy(lightvalve_h, buf + 4, 2);
    HexToStr(lightvalve_s, lightvalve_h, 2);
    memcpy(lightvalve_send + 2, buf + 2, 3);
    if(1 == replace(THRESHOLD, lightvalve_s, 2))
    {
        //开关光感阀值应答 0x23
        lightvalve_send[5] = 0x01;
        UartToApp_Send(lightvalve_send, 6);

        lightvalve_usend[2] = buf[4];
        lightvalve_usend[3] = 0x00;
        lightvalve_usend[4] = buf[5];
        UART0_Send(uart_fd, lightvalve_usend, 5);
    }
    else
    {
        lightvalve_send[5] = 0x00;
        UartToApp_Send(lightvalve_send, 6);
    }
}

//获取主机软件/硬件版本 0x20
void obtainshvers(byte *buf)
{
    byte appsend[20] = {0x90,0x00,0x0d};
    memcpy(appsend + 3, softwareversion, 4);
    appsend[7] = hostinfo.bindhost;
    appsend[8] = hostinfo.temperature;
    appsend[9] = hostinfo.moderate;
    appsend[10] = hostinfo.dataversion;
    appsend[11] = hostinfo.enable;
    appsend[12] = hostinfo.meshversion;

    UartToApp_Send(appsend, appsend[2]);
}

//设置手势场景 0x27
void gesturescene(byte *buf)
{
    byte gesturescene_hex[15] = {0};
    byte gesturescene_str[30] = {0};
    byte gesturescene_send[15] = {0x31,0x16};
    memcpy(gesturescene_hex, buf + 2, buf[1] - 2);

    //目的mac
    gesturescene_send[2] = buf[7];
    //网下消息
    gesturescene_send[3] = 0x01;
    //主mac
    gesturescene_send[4] = buf[3];
    //driver id
    gesturescene_send[5] = buf[4];
    //driver type 0x02:开关切换
    gesturescene_send[6] = 0x02;
    //key id
    gesturescene_send[7] = buf[8];
    //key value
    if((buf[5] == 0x02) || (buf[5] == 0x06) || (buf[5] == 0x09))
    {
        gesturescene_send[8] = buf[9];
        UART0_Send(uart_fd, gesturescene_send, 9);
    }
    else if(buf[5] == 0x10)
    {
        if((buf[8] == 0x00) || (buf[8] == 0x01) || (buf[8] == 0x02))
        {
            gesturescene_send[8] = buf[9];
            UART0_Send(uart_fd, gesturescene_send, 9);
        }
        else if(buf[8] == 0x03)
        {
            gesturescene_send[8] = buf[9];
            gesturescene_send[9] = buf[10];
            gesturescene_send[10] = buf[11];
            UART0_Send(uart_fd, gesturescene_send, 11);
        }
    }
    //存储
    HexToStr(gesturescene_str, gesturescene_hex, buf[1] - 2);
    replace(GESTURE, gesturescene_str, 8);
}

//删除手势场景 0x28
void gesturescenedel(byte *buf)
{
    byte gesturescenedel_hex[10] = {0};
    byte gesturescenedel_str[20] = {0};
    byte gesturescenedel_send[10] = {0x31,0x18,0xff,0x01};
    
    memcpy(gesturescenedel_hex, buf + 2, buf[1] - 2);
    HexToStr(gesturescenedel_str, gesturescenedel_hex, buf[1] - 2);
    filecondel(GESTURE, gesturescenedel_str, 8);

    gesturescenedel_send[4] = buf[3];
    gesturescenedel_send[5] = buf[4];
    UART0_Send(uart_fd, gesturescenedel_send, 6);
}

//app灯具控制 0x29
void lightingcontrol(byte *buf)
{
    byte lightingcontrol_hex[10] = {0x31,0x10};
    memcpy(lightingcontrol_hex + 2, buf + 4, buf[1] - 4);
    //根据ID决定发送长度 0x03 = RGB
    if(buf[5] == 0x03)
    {
        UART0_Send(uart_fd, lightingcontrol_hex, 7);
    }
    else
    {
        UART0_Send(uart_fd, lightingcontrol_hex, 5);
    }
}

//app节点注册信息上传请求 0x2f 0x30
void appnodeupask(byte *buf)
{
    pr_debug("节点信息上传\n");
    FILE* fp = NULL;
    int len = -1;
    int logo = 1;
    byte buf_str[30] = {0};
    byte buf_h[15] = {0};
    byte buf_hex[10] = {0x30};
    
    //上传所有节点信息
    if((fp = fopen(NODEID, "r")) == NULL)
    {
        pr_debug("node_up_FILENAME文件打开失败\n");
        logo = -1;
    }
    if(logo == 1)
    {
        while(fgets(buf_str, 30, fp))
        {
            StrToHex(buf_h, buf_str, strlen(buf_str) / 2);
            buf_hex[1] = ((strlen(buf_str) / 2) + 2);
            memcpy(buf_hex + 2, buf_h, strlen(buf_str) / 2);
            len = UartToApp_Send(buf_hex, ((strlen(buf_str) / 2) + 2));

            int temp_i = 0;
            for(temp_i = 0; temp_i < len; temp_i++)
            {
                pr_debug("0x2f上传节点信息至app[%d]:%0x\n", temp_i, buf_hex[temp_i]);
            }
            //sleep(1);
        }
        fclose(fp);
    }
    fp = NULL;

    /*
    byte appnodeupask_h[10] = {0};
    byte appnodeupask_s[10] = {0};
    byte appnodeupask_str[30] = {0};
    byte appnodeupask_hex[15] = {0};
    byte appnodeupask_send[10] = {0x30,0x09};

    //取出mac
    memcpy(appnodeupask_h, buf + 2, 2);
    HexToStr(appnodeupask_s, appnodeupask_h, 2);

    if(0 == (filefind(appnodeupask_s, appnodeupask_str, FILENAME, 4)))
    {
        StrToHex(appnodeupask_hex, appnodeupask_str, strlen(appnodeupask_str) / 2);
        
        memcpy(appnodeupask_send + 2, appnodeupask_hex, 7);
        send(new_fd, appnodeupask_send, 9, 0);
    }
    else
    {
        pr_debug("appnodeupask_未找到相应节点\n");
    }
    */
}
//设置时间
int set_system_time(struct tm ptime)
{
    struct tm *stime;
    struct timeval tv;
    time_t timep,timeq;
    int rec;

    time(&timep);
    stime = localtime(&timep);

    stime->tm_sec = ptime.tm_sec;
    stime->tm_min = ptime.tm_min;
    stime->tm_hour = ptime.tm_hour;
    stime->tm_mday = ptime.tm_mday;
    stime->tm_mon = ptime.tm_mon - 1;
    stime->tm_year = ptime.tm_year - 1900;

    /*
    pr_debug("stime->tm_sec:%d\n", stime->tm_sec);
    pr_debug("stime->tm_min:%d\n", stime->tm_min);
    pr_debug("stime->tm_hour:%d\n", stime->tm_hour);
    pr_debug("stime->tm_mday:%d\n", stime->tm_mday);
    pr_debug("stime->tm_mon:%d\n", stime->tm_mon);
    pr_debug("stime->tm_year:%d\n", stime->tm_year);
    */
    timeq = mktime(stime);
    tv.tv_sec = (long)timeq;
    //pr_debug("the second: %ld\n",tv.tv_sec);
    tv.tv_usec = 0;

    rec = settimeofday(&tv,NULL);
    if(rec < 0)
    {
        pr_debug("settimeofday failed!\n");
        return -1;
    }
    else
    {
        pr_debug("Set system time ok!\n");
        return 0;
    }
    stime = NULL;
} 

//app同步网关时间 0x31,0x22
void appsettime(byte *buf)
{
    byte appsettime_send[10] = {0x92,0x00,0x04};
    byte chipsend[20] = {0x30,0x00,0x0c,0x02};
        
    //准备数据
    struct tm tm;
    tm.tm_sec = buf[15];
    tm.tm_min = buf[14];
    tm.tm_hour = buf[13];
    tm.tm_mday = buf[12];
    tm.tm_mon = buf[11];
    tm.tm_year = ((buf[9] << 8) + buf[10]);
    
    //对比mac是否一致
    if(0 == memcmp(buf + 3, hostinfo.hostsn, 6))
    {
        //设置时间-成功
        if(0 == (set_system_time(tm)))
        {
            appsettime_send[3] = 0x00;
        }
        else//失败
        {
            appsettime_send[3] = 0x01;
        }
    }
    else
    {
        appsettime_send[3] = 0x01;
    }
    if(buf[0] == 0x22)
    {
        UartToApp_Send(appsettime_send, appsettime_send[2]);
        
        if(appsettime_send[3] == 0)
        {
            chipsend[4] = ((tm.tm_year + 1900) >> 8) & 0xff;
            chipsend[5] = (tm.tm_year + 1900) & 0xff;
            chipsend[6] = tm.tm_mon + 1; 
            chipsend[7] = tm.tm_mday;
            chipsend[8] = tm.tm_hour;
            chipsend[9] = tm.tm_min;
            chipsend[10] = tm.tm_sec;
            IRUART0_Send(single_fd, chipsend, chipsend[2]);
        }
    }
}

//主（从）机重启
void appreboot(byte *buf)
{
    byte appreboot_mac[10] = {0};
    byte appreboot_local_mac[10] = {0};
    memcpy(appreboot_mac, buf + 2, 6);
    get_mac(appreboot_local_mac);
    if(0 == memcmp(appreboot_mac, appreboot_local_mac, 6))
    {
        system("reboot");
    }
    else
    {
    
    }
}

//手动场景重命名0x53,0x54,0x55
void remanualscenario(byte *buf)
{
    byte remanual_h[512] = {0};
    byte remanual_s[1024] = {0};
    memcpy(remanual_h, buf + 2, buf[1] - 2);
    HexToStr(remanual_s, remanual_h, buf[1] - 2);
    if(buf[0] == 0x53)
    {
        replace(MANUALSCE, remanual_s, 2);
    }
    else if(buf[0] == 0x54)
    {
        replace(AUTOSCE, remanual_s, 2);
    }
    else if(buf[0] == 0x55)
    {
        replace(REGASSOC, remanual_s, 2);
    }
}

//设备命名0x52
void devicenaming(byte *buf)
{
    byte devname_h[64] = {0};
    byte devname_s[128] = {0};
    memcpy(devname_h, buf + 2, buf[1] - 2);
    HexToStr(devname_s, devname_h, buf[1] - 2);
    replace(DEVNAME, devname_s, 10);
}

//APP添加设置网下开关区域设置(下发命令) 0x58
void netassociation(byte *buf)
{
    int i = 0;
    int logo = 1;
    int offset = 0;
    int temp = 0;
    byte netassociation[256] = {0};
    byte savehex[256] = {0};
    byte sendapp[5] = {0x59,0x03};
    //存放发送指令
    byte unicastscene[20] = {0x31,0x16};
    //截取配置信息
    memcpy(netassociation, buf + 7, buf[6] * 5);
    for(i = 0; i < buf[6]; i++)
    {
        //场景配置命令(单播) 9.19添加
        //目的mac
        unicastscene[2] = netassociation[offset + 1];
        //Message source
        unicastscene[3] = 0x01;
        //mast mac
        unicastscene[4] = buf[3];
        //mast id
        if((buf[5] > 8) || (buf[5] < 1))
        {
            logo = 0;
            break;
        }
        unicastscene[5] = buf[5];
        //driver type 0x02:开/关切换
        unicastscene[6] = 0x02;
        //key id
        if((netassociation[offset + 3] > 8) || (netassociation[offset + 3] < 5))
        {
            logo = 0;
            break;
        }
        unicastscene[7] = netassociation[offset + 3];
        //key value
        unicastscene[8] = 0x01;
        //发送
        UART0_Send(uart_fd, unicastscene, 9);

        //指令组合
        savehex[temp] = netassociation[offset + 1];
        savehex[temp + 1] = netassociation[offset + 3];
        savehex[temp + 2] = netassociation[offset + 4];
        //偏移
        temp += 3;
        offset += 5;
    }
    if(logo == 0)
    {
        sendapp[2] = 0x02;
        UartToApp_Send(sendapp, 3);
        pr_debug("netassociation Configuration failed\n");
        return;
    }
    //存
    byte saveassoc[256] = {0};
    byte saveassoc_str[512] = {0};
    saveassoc[0] = buf[3];
    saveassoc[1] = buf[5];
    saveassoc[2] = 0x00;
    memcpy(saveassoc + 3, savehex, buf[6] * 3);
    memcpy(saveassoc + ((buf[6] * 3) + 3), buf + (buf[1] - 17), 17);

    HexToStr(saveassoc_str, saveassoc, (buf[6] * 3) + 20);
    replace(RELEVANCE, saveassoc_str, 4);
    sendapp[2] = 0x01;
    UartToApp_Send(sendapp, 3);
}

//APP删除网下开关区域设置(下发命令) 0x70
void delnetassociation(byte *buf)
{
    byte delnetassociation [5] = {0};
    byte del_str[10] = {0};
    byte senddel[5] = {0x71,0x03};
    delnetassociation[0] = buf[3];
    delnetassociation[1] = buf[5];
    HexToStr(del_str, delnetassociation, 2);

    //场景删除   9.19
    if(-1 == (contains(del_str, 4, RELEVANCE)))
    {
        byte scenesdel[10] = {0x31,0x18,0xff,0x01};
        memcpy(scenesdel + 4, delnetassociation, 2);
        //发送
        //删除该条关联
        filecondel(RELEVANCE, del_str, 4);
        UART0_Send(uart_fd, scenesdel, 6);

        senddel[2] = 0x01;
    }
    else
    {
        senddel[2] = 0x02;
    }
    UartToApp_Send(senddel, 3);
}

void slavenetworking(int mac)
{
    int i = 2;
    byte appsend[20] = {0x42,0x00,0x05};
    
    //删除从机红外
    byte slavemac[12] = {0};
    slavemac[0] = mac;
    slavemac[1] = 0x01;
    //检索删除红外设备
    delnodeirdev(slavemac);
    usleep(50000);
    //检索删除红外快捷方式
    delnodeirfast(slavemac);
    usleep(50000);

    for(i = 2; i < 256; i++)
    {
        if(nodedev[i].nodeinfo[2] == 0)
        {
            //pr_debug("节点信息同步-保留mac:%d\n", i);
        }
        else if(nodedev[i].nodeinfo[2] == 0xff)
        {
            //pr_debug("节点信息同步-保留ffmac:%d\n", i);
        }
        else if(nodedev[i].nodeinfo[0] == mac)
        {
            appsend[3] = mac;
            appsend[4] = nodedev[i].nodeinfo[1];
            delnodedev(appsend);
            usleep(100000);
        }
    }
}

//APP组网/解散网络
void networking(byte *buf)
{
    //hostinfo.mesh = 1;
    byte mesh[10] = {"mesh2"};
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    hostinfo.dataversion += 1;
    if(0 == memcmp(buf + 3, hostinfo.hostsn, 6))
    {
        hostinfo.slavemesh = 1;
    }
    else
    {
        int i = 2;
        for(i = 2; i < 5; i++)
        {
            if(0 == memcmp(buf + 3, slaveident[i].slavesn, 6))
            {
                hostinfo.slavemesh = slaveident[i].assignid;
            }
        }
    }
    pr_debug("hostinfo.slavemesh:%d\n", hostinfo.slavemesh);

    if(buf[9] == 0)
    {
        byte node[10] = {0x73,0x05,0x00,0x00,0x00};
        UART0_Send(uart_fd, node, 5);
    }
    else if(buf[9] == 1)
    {
        byte meshsend[15] = {0x7b,0x0b,0x01,0x02,0x03,0x04,0x01,0x02,0x03,0x01,0x00};
        UART0_Send(uart_fd, meshsend, 11);
        if(hostinfo.slavemesh == 1)
        {
            replace(HOSTSTATUS, mesh, 4);
            IRUART0_Send(single_fd, chipsend, chipsend[2]);

            //创建恢复出厂shell文件
            createrestore();

            sleep(3);
            system("sh /data/modsty/system_reset.sh");
            system("reboot");
        }
        else
        {
            slavenetworking(hostinfo.slavemesh);
        }
    }
}

//APP上“设备全开”和“设备全关”功能(主机下发命令) 0x63
void groupcontrol(byte *buf)
{
    byte groupcontrol[10] = {0x31,0x23,0xff,0xff};
    groupcontrol[4] = buf[4];
    UART0_Send(uart_fd, groupcontrol, 5);
}

//APP上设置开关温度误差 0x68
void tempererror(byte *buf)
{
    byte sendtemper[10] = {0x69,0x05,0x01};
    byte tempererror[10] = {0x31,0x24};
    //MAC
    tempererror[2] = buf[3];
    //id
    tempererror[3] = buf[5];
    //value
    tempererror[4] = buf[6];
    UART0_Send(uart_fd, tempererror, 5);

    sendtemper[3] = buf[3];
    sendtemper[4] = 0x01;
    UartToApp_Send(sendtemper, 5);
}

//背光灯调光 0x11
void backlight(byte *buf)
{
    byte backlight[10] = {0x31,0x09};
    //MAC
    backlight[2] = buf[3];
    //id value
    memcpy(backlight + 3, buf + 4, 2);
    UART0_Send(uart_fd, backlight, 5);
}

//APP上设置网下区域关联手动场景(APP下发命令) 0x64
void appscene(byte *buf)
{
    byte appscene[10] = {0};
    byte appscene_str[15] = {0};
    byte sendappscene[10] = {0x65,0x03,0x01};

    memcpy(appscene, buf + 3, 4);
    HexToStr(appscene_str, appscene, 4);

    if(1 == replace(APPSCENE, appscene_str, 6))
    {
        UartToApp_Send(sendappscene, 3);
    }
    else
    {
        sendappscene[2] = 0x02;
        UartToApp_Send(sendappscene, 3);
    }
}

//App删除网下区域关联手动场景 0x72
void delappscene(byte *buf)
{
    byte delappscene[5] = {0};
    byte delappscene_s[10] = {0};
    byte senddelapp[10] = {0x63,0x03,0x01};

    memcpy(delappscene, buf + 3, 3);
    HexToStr(delappscene_s, delappscene, 3);

    if(1 == filecondel(APPSCENE, delappscene_s, 6))
    {
        UartToApp_Send(senddelapp, 3);
    }
    else
    {
        senddelapp[2] = 0x02;
        UartToApp_Send(senddelapp, 3);
    }
}

//APP自动场景的状态作为另外一个自动场景触发条件设置0x66
void autotrigger(byte *buf)
{
    byte autotrigger[5] = {0};
    byte autotrigger_s[10] = {0};
    byte sendautotri[10] = {0x67,0x03,0x01};

    memcpy(autotrigger, buf + 2, 3);
    HexToStr(autotrigger_s, autotrigger, 3);

    if(1 == replace(AUTOTRIGGER, autotrigger_s, 6))
    {
        UartToApp_Send(sendautotri, 3);
    }
    else
    {
        sendautotri[2] = 0x02;
        UartToApp_Send(sendautotri, 3);
    }
}

//删除 APP自动场景的状态作为另外一个自动场景触发条件设置 0x73
void delautotri(byte *buf)
{
    byte delautotri[5] = {0};
    byte delautotri_s[10] = {0};
    byte senddelautri[5] = {0x74,0x03,0x01};

    memcpy(delautotri, buf + 2, 2);
    HexToStr(delautotri_s, delautotri, 2);

    if(1 == filecondel(AUTOTRIGGER, delautotri_s, 4))
    {
        UartToApp_Send(senddelautri, 3);
    }
    else
    {
        senddelautri[2] = 0x02;
        UartToApp_Send(senddelautri, 3);
    }
}

//app删除开关 0x6a
void deletenode(byte *buf)
{
    byte delnode_h[5] = {0};
    byte delnode_s[10] = {0};
    byte deluart[10] = {0x31,0x26,0x00,0x00,0x00};
    byte delsend[10] = {0};

    memcpy(delnode_h, buf + 2, 3);
    HexToStr(delnode_s, delnode_h, 3);
    deluart[2] = delnode_h[1];
    UART0_Send(uart_fd, deluart, 5);

    memcpy(delsend, buf, 5);
    delsend[1] = 0x06;
    if(1 == filecondel(NODEID, delnode_s, 4))
    {
        delsend[5] = 0x01;
    }
    else
    {
        delsend[5] = 0x00;
    }
    UartToApp_Send(delsend, 6);
}

//完整红外码发送 0x58
void completeirsend(byte *buf)
{
    int result = 0;
    int i = 0;
    int offset = 0;
    double lentemp = 0.0;
    double n = 0.0, precious = 0.0, inter = 0;
    //当前包数
    int len = (buf[1] << 8) + buf[2];
    byte ifcode[512] = {0};
    byte appsend[10] = {0xc8,0x00,0x04,0x00};
    byte irsend[512] = {0x30};
    byte irmeshsend[70] = {0};

    //len
    irsend[1] = (((len + 2) >> 8) & 0xff);
    irsend[2] = ((len + 2) & 0xff);
    
    //type
    irsend[3] = 0x04;
    //ir
    memcpy(irsend + 4, buf + 3, len - 3);

    //完整红外码长度
    ifcode[0] = ((len - 4) >> 8) & 0xff;
    ifcode[1] = (len - 4) & 0xff;
    //来源
    ifcode[2] = 0x01;
    //压缩+红外码
    memcpy(ifcode + 3, buf + 5, len - 5);
    
    lentemp = len - 2;
    n = lentemp / 55;
    pr_debug("n:%lf\n", n);
    
    //缓存
    memset(&ircache, 0, sizeof(ircache));
    //mac
    ircache.mac[0] = buf[3];
    ircache.mac[1] = buf[4];

    ircache.len[0] = ifcode[0];
    ircache.len[1] = ifcode[1];

    ircache.source = ifcode[2];
    
    ircache.compression = ifcode[3];

    memcpy(ircache.ircode, ifcode + 4, len - 6);

    precious = modf(n, &inter);
    pr_debug("小数部分 %lf, 整数部分 %lf\n", precious, inter);
    if(precious > 0.0)
    {
        inter += 1;
    }

    //目的：主机/从机
    if(buf[4] == 0x01)
    {
        result = IRUART0_Send(single_fd, irsend, len + 2);
        if(result == len + 2)
        {
            UartToApp_Send(appsend, 4);
        }
        else
        {
            appsend[3] = 0x01;
            UartToApp_Send(appsend, 4);
        }
    }
    //节点
    else
    {
        pr_debug("inter:%lf\n", inter);
        for(i = 0; i < inter; i++)
        {
            pr_debug("(len - 2) -offset:%d\n", (len - 2) - offset);
            memset(irmeshsend, 0, sizeof(irmeshsend));
            irmeshsend[0] = 0x7c;
            irmeshsend[1] = 0x3c;
            //节点mac
            irmeshsend[2] = buf[4];
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

//完整红外码发送应答 0xc8
void completeirsendack(byte *buf)
{
    byte appsend[10] = {0xc8,0x00,0x04};
    appsend[3] = buf[4];
    UartToApp_Send(appsend, appsend[2]);
}

//配置红外设备 0x59
void configirdev(byte *buf)
{
    int id = 1;
    int tempid = 0;
    byte idhex[5] = {0};
    int len = (buf[1] << 8) + buf[2];
    byte appsend[10] = {0xc9,0x00,0x05,0x00,0x00};
    byte str[2048] = {0};
    byte save[256] = {0};

    memcpy(save, buf + 3, len - 3);
    hostinfo.dataversion += 1;

    if(buf[3] == 0)
    {
        FILE *fd = NULL;
        if((fd = fopen(IRDEV, "a+")) == NULL)
        {
            pr_debug("IRDEV_file open failed\n");
            pr_debug("%s: %s\n", IRDEV, strerror(errno));
            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, IRDEV, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
            //return;
        }
        //截取有效数据从ID开始
        //memcpy(manualsce, buf + 2, len - 2);
        while(fgets(str, 2048, fd))
        {
            if(strlen(str) < 10)
            {
                str[2] = '\0';
                StrToHex(idhex, str, 1);
                id = idhex[0];
                break;
            }
            else
            {
                id++;
            }
            memset(str, 0, sizeof(str));
        }
        fclose(fd);
        fd = NULL;
        if(id > 50)
        {
            appsend[5] = 1;
            UartToApp_Send(appsend, appsend[2]);
            return;
        }

        //ID
        save[0] = id;
        memset(str, 0, sizeof(str));
        HexToStr(str, save, len - 3);
        replace(IRDEV, str, 2);

        appsend[3] = id;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//删除红外设备 0x5a
void delirdev(byte *buf)
{
    byte charid[5] = {0};
    byte charidtemp[5] = {0};
    byte appsend[10] = {0xca,0x00,0x05,0x00,0x00};
    hostinfo.dataversion += 1;
    
    appsend[3] = buf[3];
    if(buf[3] <= 0)
    {
        pr_debug("delirdev:错误ID%d\n", buf[3]);
        appsend[4] = 0x01;
        UartToApp_Send(appsend, 5);
        return;
    }
    
    charidtemp[0] = buf[3];
    HexToStr(charid, charidtemp, 1);
    
    if(1 == replace(IRDEV, charid, 2))
    {
        UartToApp_Send(appsend, 5);
    }
    else
    {
        appsend[4] = 0x01;
        UartToApp_Send(appsend, 5);
    }
}

//红外-一键匹配 0x5b
void irmatch(byte *buf)
{
    int result = 0;
    byte irsend[10] = {0x30,0x00,0x07,0x05,0x00,0x00};
    
    result = IRUART0_Send(single_fd, irsend, 7);
    pr_debug("IR-5B-result:%d\n", result);
}

//红外-智能学习 0x5c
void irlntelearn(byte *buf)
{
    int result = 0;
    byte irsend[10] = {0x30,0x00,0x07,0x06,0x00,0x00};

    memcpy(irsend + 4, buf + 3, 2);
    result = IRUART0_Send(single_fd, irsend, 7);
    pr_debug("IR-5C-result:%d\n", result);
}

//主机存储红外快捷方式
void hoststorageirfast()
{
    int irlen = 0;

    int addlen = 0;
    byte appsend[10] = {0xcd,0x00,0x06};
    byte savebuf[500] = {0};
    byte savestr[1000] = {0};

    irlen = (ircache.len[0] << 8) + ircache.len[1];
    pr_debug("开始存储红外快捷方式:%d\n", irlen);

    //索引
    savebuf[0] = ircache.id[0];
    savebuf[1] = ircache.id[1];
    //mac
    savebuf[2] = ircache.mac[0];
    savebuf[3] = ircache.mac[1];
    //len
    savebuf[4] = ircache.len[0];
    savebuf[5] = ircache.len[1];
    //source
    savebuf[6] = ircache.source;
    //压缩
    savebuf[7] = ircache.compression;
    addlen = 8;
    //红外码
    memcpy(savebuf + addlen, ircache.ircode, irlen - 2);
    addlen += (irlen - 2);
    //名称长度-1
    savebuf[addlen] = ircache.namelen;
    addlen += 1;
    //名称和图标id-33
    memcpy(savebuf + addlen, ircache.name, 33);
    addlen += 33;

    HexToStr(savestr, savebuf, addlen);
    pr_debug("savestr:%s\n", savestr);
    replace(IRFAST, savestr, 4);
    memset(&ircache, 0, sizeof(ircache));

    appsend[3] = savebuf[0];
    appsend[4] = savebuf[1];
    appsend[5] = 0x00;
    UartToApp_Send(appsend, appsend[2]);
}

//添加红外快捷操作 0x5d
void irquickopera(byte *buf)
{
    int id = 1;
    byte tempname[50] = {0};
    byte tempid[10] = {0};
    int g_tempid = 0;
    int len = (buf[1] << 8) + buf[2];
    byte str[1000] = {0};
    byte uartsend[10] = {0x7d,0x08};
    byte appsend[10] = {0xcd,0x00,0x06,0x00,0x00,0x00};
    hostinfo.dataversion += 1;
    
    if((0 == buf[3] + buf[4]) && (0 != ircache.mac[1]))
    {
        FILE *fd = NULL;
        if((fd = fopen(IRFAST, "a+")) == NULL)
        {
            pr_debug("IRFAST_file open failed\n");
            pr_debug("%s: %s\n", IRFAST, strerror(errno));
            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, IRFAST, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
            //return;
        }
        //截取有效数据从ID开始
        //memcpy(manualsce, buf + 2, len - 2);
        while(fgets(str, 1024, fd))
        {
            if(strlen(str) < 10)
            {
                pr_debug("irfastid:%s\n", str);
                StrToHex(tempid, str, 2);
                id = (tempid[0] << 8) + tempid[1];
                break;
            }
            else
            {
                id++;
            }
        }
        fclose(fd);
        fd = NULL;

        pr_debug("irfast:%d\n", id);
        if(id > 100)
        {
            appsend[5] = 1;
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        ircache.id[0] = ((id >> 8) & 0xff);
        ircache.id[1] = (id & 0xff);
        //暂存名称
        ircache.namelen = buf[9];
        memcpy(ircache.name, buf + (len - 33), 33);

        if(buf[6] == 0x01)
        {
            hoststorageirfast();
        }
        else
        {
            uartsend[2] = buf[6];
            uartsend[3] = ircache.id[0];
            uartsend[4] = ircache.id[1];
            uartsend[5] = buf[7];
            uartsend[6] = buf[8];

            //记录最后一条配置
            //memset(&sceneack, 0, sizeof(sceneack));
            //memcpy(sceneack.endbuf, uartsend, uartsend[1]);

            UART0_Send(uart_fd, uartsend, uartsend[1]);
        }
    }
    else
    {
        appsend[5] = 1;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//添加红外快捷操作应答 0xcd
void irquickoperaack(byte *buf)
{
    byte appsend[10] = {0xcd,0x00,0x06,0x00,0x00};
    appsend[5] = buf[6];
    if(buf[6] == 0)
    {
        storeirshortcuts();
    }
    else
    {
        UartToApp_Send(appsend, appsend[2]);
    }
}

//删除红外快捷操作 0x5e
void delirquick(byte *buf)
{
    byte charid[5] = {0};
    byte tempid[5] = {0};
    byte uartsend[10] = {0x7e,0x06};
    byte appsend[10] = {0xce,0x00,0x06,0x00,0x00,0x00};
    
    appsend[3] = buf[3];
    appsend[4] = buf[4];
    hostinfo.dataversion += 1;

    if((buf[3] == 0) && (buf[4] == 0))
    {
        pr_debug("delirquick:错误ID\n");
        appsend[5] = 0x01;
        UartToApp_Send(appsend, appsend[2]);
        return;
    }
    
    memcpy(tempid, buf + 3, 2);
    HexToStr(charid, tempid, 2);
    
    if(1 == replace(IRFAST, charid, 4))
    {
        if(buf[6] > 1)
        {
            uartsend[2] = buf[6];
            uartsend[3] = buf[3];
            uartsend[4] = buf[4];
            UART0_Send(uart_fd, uartsend, uartsend[1]);
        }
        
        UartToApp_Send(appsend, appsend[2]);
    }
    else
    {
        appsend[5] = 0x01;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//删除红外快捷操作应答 0xce
//void delirquickack(byte *buf)

//执行红外快捷操作 0x65
void runirquick(byte *buf)
{
    int len = 0;
    int result = 0;
    byte charid[5] = {0};
    byte tempid[5] = {0};
    byte irstr[1024] = {0};
    byte irhex[512] = {0};
    byte uartsend[10] = {0x7f,0x06};
    byte irsend[512] = {0x30,0x00,0x00,0x04};
    byte appsend[10] = {0xd5,0x00,0x04,0x00};
    
    if((buf[3] == 0) && (buf[4] == 0))
    {
        pr_debug("delirquick:错误ID\n");
        appsend[3] = 0x01;
        UartToApp_Send(appsend, 4);
        return;
    }
    
    memcpy(tempid, buf + 3, 2);
    HexToStr(charid, tempid, 2);
    
    pr_debug("runirquick ID:%s\n", charid);
    if(0 == filefind(charid, irstr, IRFAST, 4))
    {
        StrToHex(irhex, irstr, strlen(irstr) / 2);
        //单片机
        if(irhex[3] == 1)
        {
            //mac
            irsend[4] = irhex[2];
            irsend[5] = irhex[3];
            //压缩方式和红外码
            len = ((irhex[4] << 8) + irhex[5]);
            //////////////////
            len -= 2;
            /////////////////
            irsend[6] = irhex[7];
            memcpy(irsend + 7, irhex + 8, len);
            //len-其他数据(8) + 红外码
            irsend[1] = (((len + 8) >> 8) & 0xff);
            irsend[2] = ((len + 8) & 0xff);

            result = IRUART0_Send(single_fd, irsend, len + 8);
            pr_debug("IR-D5-result:%d\n", result);
        }
        //节点
        else
        {
            uartsend[2] = irhex[3];
            uartsend[3] = irhex[0];
            uartsend[4] = irhex[1];
            UART0_Send(uart_fd, uartsend, uartsend[1]);
        }
        UartToApp_Send(appsend, 4);
    }
    else
    {
        appsend[3] = 0x01;
        UartToApp_Send(appsend, 4);
    }
}

//检索删除节点
void delnode(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    byte uartsend[10] = {0x63,0x0e,0x00,0x0e};
    HexToStr(nodemacstr, nodemac, 2);
    replace(NODEID, nodemacstr, 4);

    if(nodedev[nodemac[1]].nodeinfo[1] != 0)
    {
        memset(nodedev[nodemac[1]].nodeinfo, 0, sizeof(nodedev[nodemac[1]].nodeinfo));
        //保留mac
        nodedev[nodemac[1]].nodeinfo[0] = nodemac[0];
        nodedev[nodemac[1]].nodeinfo[1] = nodemac[1];
    }
    uartsend[2] = nodemac[1];
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//检索删除节点原始信息
void delorinode(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    byte nodemachex[5] = {0};
    nodemachex[0] = nodemac[1];
    HexToStr(nodemacstr, nodemachex, 1);
    filecondel(ORIGNODE, nodemacstr, 2);
}

//检索删除智能锁
void delsmartlock(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);
    filecondel(SMARTLOCK, nodemacstr, 4);
}

//检索删除节点名称
void deldevname(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);
    filecondel(DEVNAME, nodemacstr, 4);
}

//检索删除智能控制面板关联信息
void delsmartcrlpane(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);

    filecondel(SMARTCRLPANEL, nodemacstr, 4);
    //读取智能控制面板关联
    readsmartctlpanel();
}

//检索删除关联信息
void delassociatedinfo(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);
    
    //震动
    filecondel(SHAKE, nodemacstr, 4);
    //烟雾
    filecondel(SMOKERELATED, nodemacstr, 4);
    //智能锁关联
    filecondel(LOCKRELATED, nodemacstr, 4);
    //手势识别
    filecondel(GESTURE, nodemacstr, 4);
    //门磁
    filecondel(DOORMAGNET, nodemacstr, 4);
    //水浸
    filecondel(FLOODING, nodemacstr, 4);
    //红外活动侦测
    filecondel(IRACTIVITY, nodemacstr, 4);
    //雨雪
    filecondel(RAINSNOW, nodemacstr, 4);
    //燃气
    filecondel(GASINFO, nodemacstr, 4);
    //风速
    filecondel(WINDSPEED, nodemacstr, 4);
    //一氧化碳
    filecondel(CARMOXDE, nodemacstr, 4);
    
    //读取智能锁关联
    readlockinfo();
    //读取手势关联
    readgestureinfo();
    //读取烟雾关联
    readsmokeinfo();
    //读取震动关联
    readshakeinfo();
    //读取门磁关联
    readdoormagnetinfo();
    //读取水浸关联
    readfloodinginfo();
    //读取红外活动侦测关联
    readiractivityinfo();
    //读取雨雪关联
    readrainsnowinfo();
    //读取燃气关联
    readgasinfo();
    //读取风速关联
    readwindspeed();
    //读取一氧化碳关联
    readcarmoxde();
}

//检索删除区域
void delnodereg(byte *nodemac)
{
    int i = 1;
    int offset = 0;
    int type = 0;
    int newtype = 0;
    byte tempreg[1500] = {0};
    byte regstr[3000] = {0};

    while(i < 21)
    {
        if(regcontrol[i].id == 0)
        {
            i++;
            continue;
        }
        //删除的节点= 绑定按键
        if(0 == memcmp(nodemac, regcontrol[i].bingkeymac, 2))
        {
            regcontrol[i].bingkeymac[0] = 0;
            regcontrol[i].bingkeymac[1] = 0;
            regcontrol[i].bingkeyid = 0;
        }
        type = 0;
        newtype = 0;
        offset = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < regcontrol[i].tasklen)
        {
            if(0 == memcmp(regcontrol[i].regtask + (type + 1), nodemac, 2))
            {
                type += 8;
            }
            else
            {
                memcpy(tempreg + newtype, regcontrol[i].regtask + type, 5);
                newtype += 8;
                type += 8;
            }
        }
        pr_debug("delnodereg_type:%d\n", type);
        pr_debug("delnodereg_newtype:%d\n", newtype);

        if(newtype < type)
        {
            //更新区域
            regcontrol[i].tasklen = newtype;
            memset(regcontrol[i].regtask, 0, sizeof(regcontrol[i].regtask));
        
            pr_debug("sizeof(regcontrol[%d].regtask):%d\n", i, sizeof(regcontrol[i].regtask));
            memcpy(regcontrol[i].regtask, tempreg, regcontrol[i].tasklen);

            memset(tempreg, 0, sizeof(tempreg));
            //拼接
            //id
            tempreg[offset] = regcontrol[i].id;
            offset += 1;
            //绑定按键mac
            tempreg[offset] = regcontrol[i].bingkeymac[0];
            tempreg[offset + 1] = regcontrol[i].bingkeymac[1];
            offset += 2;
            //绑定按键id
            tempreg[offset] = regcontrol[i].bingkeyid;
            offset += 1;
            tempreg[offset] = ((newtype >> 8) & 0xff);
            tempreg[offset + 1] = (newtype & 0xff);
            offset += 2;
            //执行任务
            memcpy(tempreg + offset, regcontrol[i].regtask, newtype);
            offset += newtype;
            //区域名称长度
            tempreg[offset] = regcontrol[i].namelen;
            offset += 1;
            //区域名称
            memcpy(tempreg + offset, regcontrol[i].regname, 16);
            offset += 16;
            //区域图标
            tempreg[offset] = regcontrol[i].iconid;
            offset += 1;

            HexToStr(regstr, tempreg, offset);
            replace(REGASSOC, regstr, 2);
        }
        i++;
    }
}

//删除本地关联
void dellocalass(byte *nodemac)
{
    byte localass_str[20] = {0};
    byte findstr[20] = {0};

    HexToStr(localass_str, nodemac, 2);
    pr_debug("dellocalass_str:%s\n", localass_str);

    while(0 == filefindstr(localass_str, findstr, LOCALASS))
    {
        filecondel(LOCALASS, findstr, 6);
    }
    readlocalass();
}

//检索删除手动场景
void delnodemanualsce(byte *nodemac)
{
    int i = 1;
    int err = 0;
    int offset = 0;
    int type = 0;
    int newtype = 0;
    byte tempreg[1500] = {0};
    byte regstr[3000] = {0};

    while(i < 21)
    {
        if(mscecontrol[i].id == 0)
        {
            i++;
            continue;
        }
        //删除的节点= 绑定按键
        if(0 == memcmp(nodemac, mscecontrol[i].bingkeymac, 2))
        {
            mscecontrol[i].bingkeymac[0] = 0;
            mscecontrol[i].bingkeymac[1] = 0;
            mscecontrol[i].bingkeyid = 0;
        }
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < mscecontrol[i].tasklen)
        {
            if(err == 1)
            {
                pr_debug("未识别的类型，退出\n");
                break;
            }
            switch(mscecontrol[i].regtask[type])
            {
                //指令长度相同，统一处理
                //继电器，小夜灯，背光灯
                case 0x01:
                case 0x11:
                case 0x12:
                case 0x21:
                case 0x31:
                    pr_debug("case 0x01_delnodemanualsce\n");
                    if(0 == memcmp(mscecontrol[i].regtask + (type + 1), nodemac, 2))
                    {
                        type += 10;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, mscecontrol[i].regtask + type, 5);
                        newtype += 10;
                        type += 10;
                    }
                    break;
                    //红外码
                case 0x0f:
                    if(mscecontrol[i].regtask[type + 3] == 0x01)
                    {
                        memcpy(tempreg + newtype, mscecontrol[i].regtask + type, 5);
                        newtype += 5;
                        type += 5;
                    }
                    else if(mscecontrol[i].regtask[type + 3] == 0x02)
                    {
                        memcpy(tempreg + newtype, mscecontrol[i].regtask + type, ((mscecontrol[i].regtask[type + 4] * 2) + 5));
                        newtype += ((mscecontrol[i].regtask[type + 4] * 2) + 5);
                        type += ((mscecontrol[i].regtask[type + 4] * 2) + 5);
                    }
                    break;
                default:
                    pr_debug("自动场景不支持的类型\n");
                    err = 1;
                    break;
            }
        }
        pr_debug("delnodereg_type:%d\n", type);
        pr_debug("delnodereg_newtype:%d\n", newtype);

        if(newtype < type)
        {
            //更新手动场景
            mscecontrol[i].tasklen = newtype;
            memset(mscecontrol[i].regtask, 0, sizeof(mscecontrol[i].regtask));
        
            pr_debug("sizeof(mscecontrol[%d].regtask):%d\n", i, sizeof(mscecontrol[i].regtask));
            memcpy(mscecontrol[i].regtask, tempreg, mscecontrol[i].tasklen);

            memset(tempreg, 0, sizeof(tempreg));
            //拼接
            //id
            tempreg[offset] = mscecontrol[i].id;
            offset += 1;
            //绑定按键mac
            tempreg[offset] = mscecontrol[i].bingkeymac[0];
            tempreg[offset + 1] = mscecontrol[i].bingkeymac[1];
            offset += 2;
            //绑定按键id
            tempreg[offset] = mscecontrol[i].bingkeyid;
            offset += 1;
            tempreg[offset] = ((newtype >> 8) & 0xff);
            tempreg[offset + 1] = (newtype & 0xff);
            offset += 2;
            //执行任务
            memcpy(tempreg + offset, mscecontrol[i].regtask, newtype);
            offset += newtype;
            //区域名称长度
            tempreg[offset] = mscecontrol[i].namelen;
            offset += 1;
            //区域名称
            memcpy(tempreg + offset, mscecontrol[i].regname, 16);
            offset += 16;
            //区域图标
            tempreg[offset] = mscecontrol[i].iconid;
            offset += 1;

            HexToStr(regstr, tempreg, offset);
            replace(MANUALSCE, regstr, 2);
        }
        i++;
    }
}

//检索删除安防
void delsmartsecurity(byte *nodemac)
{
    int i = 1;
    int err = 0;
    int offset = 0;
    int type = 0;
    int modifylogo = 0;
    int newtype = 0;
    byte tempreg[2000] = {0};
    byte regstr[4000] = {0};

    while(i < 3)
    {
        modifylogo = 0;
        if(secmonitor[i].id == 0)
        {
            i++;
            continue;
        }
        //检索启动布防条件
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < secmonitor[i].secstartlen)
        {
            if(err == 1)
            {
                pr_debug("未识别的触发条件\n");
                break;
            }
            switch(secmonitor[i].secstartcond[type])
            {
                //人体感应
                case 0x07:
                case 0x51:
                    pr_debug("delnodeautosce_触发条件\n");
                    if(0 == memcmp(secmonitor[i].secstartcond + (type + 1), nodemac, 2))
                    {
                        type += 6;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, secmonitor[i].secstartcond + type, 6);
                        newtype += 6;
                        type += 6;
                    }
                    break;
                default:
                    pr_debug("场景不支持的触发条件\n");
                    err = 1;
                    break;
            }
        }
        if(newtype < type)
        {
            modifylogo = 1;
            secmonitor[i].secstartlen = newtype;
            memset(secmonitor[i].secstartcond, 0, sizeof(secmonitor[i].secstartcond));
            memcpy(secmonitor[i].secstartcond, tempreg, newtype);
        }

        //检索布防条件
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < secmonitor[i].tricondlen)
        {
            if(err == 1)
            {
                pr_debug("未识别的触发条件\n");
                break;
            }
            switch(secmonitor[i].tricond[type])
            {
                //人体感应
                case 0x07:
                case 0x51:
                    pr_debug("delnodeautosce_触发条件\n");
                    if(0 == memcmp(secmonitor[i].tricond + (type + 1), nodemac, 2))
                    {
                        type += 6;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, secmonitor[i].tricond + type, 6);
                        newtype += 6;
                        type += 6;
                    }
                    break;
                default:
                    pr_debug("场景不支持的触发条件\n");
                    err = 1;
                    break;
            }
        }
        if(newtype < type)
        {
            modifylogo = 1;
            secmonitor[i].tricondlen = newtype;
            memset(secmonitor[i].tricond, 0, sizeof(secmonitor[i].tricond));
            memcpy(secmonitor[i].tricond, tempreg, newtype);
        }

        //检索撤防条件
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < secmonitor[i].disarmlen)
        {
            if(err == 1)
            {
                pr_debug("未识别的触发条件\n");
                break;
            }
            switch(secmonitor[i].disarmtask[type])
            {
                //人体感应
                case 0x07:
                case 0x41:
                case 0x51:
                    pr_debug("delnodeautosce_触发条件\n");
                    if(0 == memcmp(secmonitor[i].disarmtask + (type + 1), nodemac, 2))
                    {
                        type += 6;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, secmonitor[i].disarmtask + type, 6);
                        newtype += 6;
                        type += 6;
                    }
                    break;
                default:
                    pr_debug("场景不支持的触发条件\n");
                    err = 1;
                    break;
            }
        }
        if(newtype < type)
        {
            modifylogo = 1;
            secmonitor[i].disarmlen = newtype;
            memset(secmonitor[i].disarmtask, 0, sizeof(secmonitor[i].disarmtask));
            memcpy(secmonitor[i].disarmtask, tempreg, newtype);
        }

        //检索报警条件
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < secmonitor[i].secenfolen)
        {
            if(err == 1)
            {
                pr_debug("未识别的触发条件\n");
                break;
            }
            switch(secmonitor[i].secenforce[type])
            {
                //人体感应
                case 0x07:
                case 0x41:
                case 0x51:
                    pr_debug("delnodeautosce_触发条件\n");
                    if(0 == memcmp(secmonitor[i].secenforce + (type + 1), nodemac, 2))
                    {
                        type += 6;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, secmonitor[i].secenforce + type, 6);
                        newtype += 6;
                        type += 6;
                    }
                    break;
                default:
                    pr_debug("场景不支持的触发条件\n");
                    err = 1;
                    break;
            }
        }
        if(newtype < type)
        {
            modifylogo = 1;
            secmonitor[i].secenfolen = newtype;
            memset(secmonitor[i].secenforce, 0, sizeof(secmonitor[i].secenforce));
            memcpy(secmonitor[i].secenforce, tempreg, newtype);
        }

        if(modifylogo == 1)
        {
            memset(tempreg, 0, sizeof(tempreg));
            //更新安防
            //拼接
            offset = 0;
            //id
            tempreg[offset] = i;
            offset += 1;
            //使能
            tempreg[offset] = secmonitor[i].enable;
            offset += 1;
            //当前智能布防状态
            tempreg[offset] = secmonitor[i].secmonitor;
            offset += 1;
            //当前布防执行状态
            tempreg[offset] = secmonitor[i].status;
            offset += 1;
            //启动布防时间段
            memcpy(tempreg + offset, secmonitor[i].armingtime, 5);
            offset += 5;
            //10字节预留
            offset += 10;
            //布防启动条件长度
            tempreg[offset] = ((secmonitor[i].secstartlen >> 8) & 0xff);
            tempreg[offset + 1] = (secmonitor[i].secstartlen & 0xff);
            offset += 2;
            //布防启动条件
            memcpy(tempreg + offset, secmonitor[i].secstartcond, secmonitor[i].secstartlen);
            offset += secmonitor[i].secstartlen;
            //布防条件长度
            tempreg[offset] = ((secmonitor[i].tricondlen >> 8) & 0xff);
            tempreg[offset + 1] = (secmonitor[i].tricondlen & 0xff);
            offset += 2;
            //布防条件
            memcpy(tempreg + offset, secmonitor[i].tricond, secmonitor[i].tricondlen);
            offset += secmonitor[i].tricondlen;
            //撤防条件长度
            tempreg[offset] = ((secmonitor[i].disarmlen >> 8) & 0xff);
            tempreg[offset + 1] = (secmonitor[i].disarmlen & 0xff);
            offset += 2;
            //撤防条件
            memcpy(tempreg + offset, secmonitor[i].disarmtask, secmonitor[i].disarmlen);
            offset += secmonitor[i].disarmlen;
            //报警条件长度
            tempreg[offset] = ((secmonitor[i].secenfolen >> 8) & 0xff);
            tempreg[offset + 1] = (secmonitor[i].secenfolen & 0xff);
            offset += 2;
            //报警条件
            memcpy(tempreg + offset, secmonitor[i].secenforce, secmonitor[i].secenfolen);
            offset += secmonitor[i].secenfolen;
            //报警任务长度
            tempreg[offset] = ((secmonitor[i].tasklen >> 8) & 0xff);
            tempreg[offset + 1] = (secmonitor[i].tasklen & 0xff);
            offset += 2;
            //报警任务
            memcpy(tempreg + offset, secmonitor[i].regtask, secmonitor[i].tasklen);
            offset += secmonitor[i].tasklen;

            HexToStr(regstr, tempreg, offset);
            replace(SECMONITOR, regstr, 2);
        }
        i++;
    }
}

//检索删除自动场景
void delnodeautosce(byte *nodemac)
{
    int i = 1;
    int err = 0;
    int offset = 0;
    int type = 0;
    int newtype = 0;
    int modifylogo = 0;
    byte tempreg[2000] = {0};
    byte regstr[4000] = {0};

    while(i < 11)
    {
        modifylogo = 0;
        if(ascecontrol[i].id == 0)
        {
            i++;
            continue;
        }
        //检索触发条件
        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        while(type < ascecontrol[i].tricondlen)
        {
            if(err == 1)
            {
                pr_debug("未识别的触发条件\n");
                break;
            }
            switch(ascecontrol[i].tricond[type])
            {
                //指令长度相同，统一处理
                //人体感应，光感，温度，继电器
                case 0x01:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x12:
                case 0x51:
                case 0x52:
                case 0x54:
                case 0x55:
                case 0x56:
                case 0x57:
                case 0x58:
                    pr_debug("delnodeautosce_触发条件\n");
                    if(0 == memcmp(ascecontrol[i].tricond + (type + 1), nodemac, 2))
                    {
                        type += 6;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, ascecontrol[i].tricond + type, 6);
                        newtype += 6;
                        type += 6;
                    }
                    break;
                case 0x11:
                    memcpy(tempreg + newtype, ascecontrol[i].tricond + type, 6);
                    newtype += 6;
                    type += 6;
                    break;
                default:
                    pr_debug("场景不支持的触发条件\n");
                    err = 1;
                    break;
            }
        }
        if(newtype < type)
        {
            modifylogo = 1;
            ascecontrol[i].tricondlen = newtype;
            memset(ascecontrol[i].tricond, 0, sizeof(ascecontrol[i].tricond));
            memcpy(ascecontrol[i].tricond, tempreg, newtype);
        }

        type = 0;
        newtype = 0;
        offset = 0;
        err = 0;
        memset(regstr, 0, sizeof(regstr));
        //检索执行任务
        while(type < ascecontrol[i].tasklen)
        {
            if(err == 1)
            {
                pr_debug("未识别的类型\n");
                break;
            }
            switch(ascecontrol[i].regtask[type])
            {
                //指令长度相同，统一处理
                //继电器，小夜灯，背光灯
                case 0x01:
                case 0x11:
                case 0x12:
                case 0x21:
                case 0x31:
                    pr_debug("case 0x01_delnodeautosce\n");
                    if(0 == memcmp(ascecontrol[i].regtask + (type + 1), nodemac, 2))
                    {
                        type += 10;
                    }
                    else
                    {
                        memcpy(tempreg + newtype, ascecontrol[i].regtask + type, 9);
                        newtype += 10;
                        type += 10;
                    }
                    break;
                    //红外码
                case 0x0f:
                    if(ascecontrol[i].regtask[type + 3] == 0x01)
                    {
                        memcpy(tempreg + newtype, ascecontrol[i].regtask + type, 5);
                        newtype += 5;
                        type += 5;
                    }
                    else if(ascecontrol[i].regtask[type + 3] == 0x02)
                    {
                        memcpy(tempreg + newtype, ascecontrol[i].regtask + type, ((ascecontrol[i].regtask[type + 4] * 2) + 5));
                        newtype += ((ascecontrol[i].regtask[type + 4] * 2) + 5);
                        type += ((ascecontrol[i].regtask[type + 4] * 2) + 5);
                    }
                    else if(ascecontrol[i].regtask[type + 3] == 0x03)
                    {
                        memcpy(tempreg + newtype, ascecontrol[i].regtask + type, 30);
                        newtype += 30;
                        type += 30;
                    }
                    else
                    {
                        pr_debug("自动场景不支持的类型\n");
                        err = 1;
                    }
                    break;
                default:
                    pr_debug("自动场景不支持的类型\n");
                    err = 1;
                    break;
            }
        }
        pr_debug("delnodereg_type:%d\n", type);
        pr_debug("delnodereg_newtype:%d\n", newtype);

        if(newtype < type)
        {
            modifylogo = 1;
            //更新手动场景
            ascecontrol[i].tasklen = newtype;
            memset(ascecontrol[i].regtask, 0, sizeof(ascecontrol[i].regtask));
        
            pr_debug("sizeof(ascecontrol[%d].regtask):%d\n", i, sizeof(ascecontrol[i].regtask));
            memcpy(ascecontrol[i].regtask, tempreg, ascecontrol[i].tasklen);
        }

        if(modifylogo == 1)
        {
            memset(tempreg, 0, sizeof(tempreg));
            //拼接
            //id
            tempreg[offset] = ascecontrol[i].id;
            offset += 1;
            //使能
            tempreg[offset] = ascecontrol[i].enable;
            offset += 1;
            //当前状态
            tempreg[offset] = ascecontrol[i].status;
            offset += 1;
            //冷却时间
            tempreg[offset] = ((ascecontrol[i].interval >> 8) & 0xff);
            tempreg[offset + 1] = (ascecontrol[i].interval & 0xff);
            offset += 2;
            //预留6字节
            offset += 6;
            //触发模式
            tempreg[offset] = ascecontrol[i].trimode;
            offset += 1;
            //限制条件
            memcpy(tempreg + offset, ascecontrol[i].limitations, 5);
            offset += 5;
            //触发条件长度
            tempreg[offset] = ascecontrol[i].tricondlen;
            offset += 1;
            //触发条件
            memcpy(tempreg + offset, ascecontrol[i].tricond, ascecontrol[i].tricondlen);
            offset += ascecontrol[i].tricondlen;
            //执行任务长度
            tempreg[offset] = ((newtype >> 8) & 0xff);
            tempreg[offset + 1] = (newtype & 0xff);
            offset += 2;
            //执行任务
            memcpy(tempreg + offset, ascecontrol[i].regtask, newtype);
            offset += newtype;
            //区域名称长度
            tempreg[offset] = ascecontrol[i].namelen;
            offset += 1;
            //区域名称
            memcpy(tempreg + offset, ascecontrol[i].regname, 16);
            offset += 16;
            //区域图标
            tempreg[offset] = ascecontrol[i].iconid;
            offset += 1;

            HexToStr(regstr, tempreg, offset);
            replace(AUTOSCE, regstr, 2);
        }
        i++;
    }
}
//ir文件内容删除 1:irdev 2:irfast
int irfilecondel(int type, char *FileName, byte *str, int len)
{
    FILE *fin = NULL, *ftp = NULL;
    byte irstr[20] = {0};
    byte rep[2500] = {0};
    fin = fopen(FileName, "r");
    ftp = fopen("/data/modsty/irtmp", "w");
    if((fin == NULL) || (ftp == NULL))
    {
        pr_debug("filecondel:file open failed\n");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        return -1;
    }
    while(fgets(rep, 2500, fin))
    {
        if(type == 1)
        {
            if(0 == strncmp(str, rep + 2, len))
            {
                memcpy(irstr, rep, 2);
                irstr[2] = '\n';
                fputs(irstr, ftp);
            }
            else
            {
                fputs(rep, ftp);
            }
        }
        else if(type == 2)
        {
            if(0 == strncmp(str, rep + 4, len))
            {
                memcpy(irstr, rep, 4);
                irstr[4] = '\n';
                fputs(irstr, ftp);
            }
            else
            {
                fputs(rep, ftp);
            }
        }
    }
    fclose(fin);
    fclose(ftp);
    fin = NULL;
    ftp = NULL;
    remove(FileName);
    rename("/data/modsty/irtmp", FileName);
    return 1;
}
//检索删除红外设备
void delnodeirdev(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);
    irfilecondel(1, IRDEV, nodemacstr, 4);
}

//检索删除红外快捷方式
void delnodeirfast(byte *nodemac)
{
    byte nodemacstr[10] = {0};
    HexToStr(nodemacstr, nodemac, 2);
    irfilecondel(2, IRFAST, nodemacstr, 4);
}

//单个节点注册信息查询0x40
void singlenodeinfo(byte *buf)
{
    byte nodemacstr[10] = {0};
    byte uartsend[10] = {0x63,0x0e,0x00,0x10};
    
    hostinfo.mesh = buf[4];

    uartsend[2] = buf[4];
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}

//删除节点
void delnodedev(byte *buf)
{
    byte nodemac[5] = {0};
    byte appsend[10] = {0xb2,0x00,0x06,0x00};
    memcpy(nodemac, buf + 3, 2);

    hostinfo.dataversion += 1;

    //检索删除节点
    delnode(nodemac);
    //检索删除节点名称
    deldevname(nodemac);
    //删除智能锁
    delsmartlock(nodemac);
    //检索删除节点原始信息
    delorinode(nodemac);
    //检索删除关联信息
    delassociatedinfo(nodemac);
    //检索删除智能控制面板关联信息
    delsmartcrlpane(nodemac);
    //检索删除区域
    delnodereg(nodemac);
    //检索删除按键关联
    dellocalass(nodemac);
    //检索删除手动场景
    delnodemanualsce(nodemac);
    //检索删除安防
    delsmartsecurity(nodemac);
    //检索删除自动场景
    delnodeautosce(nodemac);
    //检索删除红外设备
    delnodeirdev(nodemac);
    //检索删除红外快捷方式
    delnodeirfast(nodemac);

    appsend[4] = buf[3];
    appsend[5] = buf[4];
    UartToApp_Send(appsend, appsend[2]);
}

//获取网关地址并检查
int getgateway(byte *gw)
{
    FILE *fp = NULL;
    byte buf[512] = {0};
    byte ping[256] = {"ping "};
    byte cmd[128] = {"route"};
    byte gateway[30] = {0};
    byte *tmp;

    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return -1;
    }

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        tmp = buf;
        while(*tmp && isspace(*tmp))
            ++ tmp;
        if(strncmp(tmp, "default", strlen("default")) == 0)
            break;
    }
    sscanf(buf, "%*s%s", gateway);
    pr_debug("default gateway:%s\n", gateway);
    strncpy(gw, gateway, strlen(gateway));

    pclose(fp);
    fp = NULL;

    memset(cmd, 0, sizeof(cmd));
    int i = 0;
    strcat(ping, gateway);
    pr_debug("ping:%s\n", ping);
    strcat(ping, " -c 1");
    pr_debug("ping1:%s\n", ping);
    if((fp = popen(ping, "r")) != NULL)
    {
        while(i < 2)
        {
            fgets(cmd, sizeof(cmd), fp);
            i++;
        }
        pclose(fp);
        fp = NULL;
    }

    pr_debug("%d\n", strlen(cmd));
    pr_debug("%s", cmd);
    /*
    for(i = 0; i < strlen(cmd); i++)
    {
        pr_debug("cmd[%d]:%c\n", i, cmd[i]);
    }
    */

    if(strnlen(cmd, 100) > 45)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//检查网口状态
//0有线1无线-1错误
int netportinspection()
{
    FILE* fp = NULL;
    byte cmd[128] = {"swconfig dev rt305x show | grep port:0"};
    if((fp = popen(cmd, "r")) != NULL)
    {
        fgets(cmd, sizeof(cmd), fp);
        pclose(fp);
        fp = NULL;
    }
    if(cmd[19] == 'u')
    {
        return 0;
    }
    else if(cmd[19] == 'd')
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

//获取网络状态
int getnetstatus()
{
    FILE *fp = NULL;
    byte ping[128] = {"ping www.baidu.com -c 1"};
    byte cmd[128] = {0};

    int i = 0;
    if((fp = popen(ping, "r")) != NULL)
    {
        while(i < 2)
        {
            fgets(cmd, sizeof(cmd), fp);
            i++;
        }
        pclose(fp);
        fp = NULL;
    }

    pr_debug("%d\n", strlen(cmd));
    pr_debug("%s", cmd);
    /*
    for(i = 0; i < strlen(cmd); i++)
    {
        pr_debug("cmd[%d]:%c\n", i, cmd[i]);
    }
    */

    if(strnlen(cmd, 100) > 45)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//app心跳应答0xf1
void appheartbeat()
{
    byte appsend[10] = {0xf1,0x00,0x03};
    appsend[3] = hostinfo.dataversion;
    UartToApp_Send(appsend, appsend[2]);
}

//配置wifi
void wificonfig(byte *buf)
{
    byte oriapsta[20] = {"apsta0"};
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte wifissid[128] = {"uci set wireless.sta.ssid='"};
    byte wifikey[128] = {"uci set wireless.sta.key='"};
    byte wifiencryption[128] = {"uci set wireless.sta.encryption=psk2"};

    //存储wifi帐号密码
    byte localmacstr[128] = {0};
    byte localmachex[64] = {0};

    memcpy(localmachex, hostinfo.hostsn, 6);
    memcpy(localmachex + 6, buf + 9, buf[2] - 9);
    HexToStr(localmacstr, localmachex, 6 + (buf[2] - 9));
    //拼接mac+wifi
    pr_debug("localmacstr:%s\n", localmacstr);
    replace(WIFIPW, localmacstr, 12);

    byte appsend[10] = {0x95,0x00,0x04,0x00};

    //copy ssid
    memcpy(wifissid + 27, buf + 10, buf[9]);
    wifissid[strlen(wifissid)] = '\'';
    pr_debug("wifissid:%s\n", wifissid);
    //copy key
    memcpy(wifikey + 26, buf + (11 + buf[9]), buf[10 + buf[9]]);
    wifikey[strlen(wifikey)] = '\'';
    pr_debug("wifikey:%s\n", wifikey);

    //发往从机
    hosttoslave(0xff, buf, buf[2]);

    if(hostinfo.apsta == 1)
    {
        replace(HOSTSTATUS, oriapsta, 5);
        pr_debug("Wificonfig_STA MODE\n");
        system("cp -f /data/modsty/sta/etc/config/* /etc/config/");

        system(wifissid);
        system(wifikey);
        system(wifiencryption);
        system("uci commit");
        IRUART0_Send(single_fd, chipsend, chipsend[2]);
        sleep(2);
        UartToApp_Send(appsend, appsend[2]);
        saveautoscestatus();
        system("reboot");
    }
    else
    {
        system(wifissid);
        system(wifikey);
        system(wifiencryption);
        system("uci commit");
        //system("wifi");
        UartToApp_Send(appsend, appsend[2]);
    }
}

//获取mac和序列号
void getmacsn()
{
    byte buf[65600] = {0};
    system("dd if=/dev/mtd2 of=/tmp/factory.bin");

    FILE* fp = fopen("/tmp/factory.bin","rb+");
    if(fp == NULL)
    {
        pr_debug("getmacsn:file open failed\n");
        pr_debug("%s: %s\n", "tmp/factory.bin", strerror(errno));
        return;
    }
    
    fread(buf, 65536, 1, fp);
    
    //wifimac
    memcpy(hostinfo.wifimac, buf + 4, 6);

    //wanmac
    memcpy(hostinfo.wanmac, buf + 40, 6);

    if(hostinfo.mode == 1)
    {
        //hostsn
        hostinfo.hostsn[0] = buf[32772];
        hostinfo.hostsn[1] = buf[32773];
        hostinfo.hostsn[2] = buf[32774];
        hostinfo.hostsn[3] = buf[32775];
        hostinfo.hostsn[4] = buf[32776];
        hostinfo.hostsn[5] = buf[32777];
        hostinfo.hostsn[6] = 0x01;
    }
    else
    {
        //hostsn
        hostinfo.slavesn[0] = buf[32772];
        hostinfo.slavesn[1] = buf[32773];
        hostinfo.slavesn[2] = buf[32774];
        hostinfo.slavesn[3] = buf[32775];
        hostinfo.slavesn[4] = buf[32776];
        hostinfo.slavesn[5] = buf[32777];
    }

    fclose(fp);
    fp = NULL;
}

unsigned short crc16 (unsigned char *pD, int len)
{
    static unsigned short poly[2]={0, 0xa001};
    unsigned short crc = 0xffff;
    int i,j;
    for(j=len; j>0; j--)
    {
        unsigned char ds = *pD++;
        for(i=0; i<8; i++)
        {
            crc = (crc >> 1) ^ poly[(crc ^ ds ) & 1];
            ds = ds >> 1;
        }
    }
    return crc;
}

int MESH_UART0_Send(int fd, byte *send_buf, int data_len)
{
    int i = 0;
    int len = 0;
    //crc
    byte add = 0x00;
    //数据头
    byte head_buf[100] = {0};
    //数据长度
    head_buf[1] = data_len;
    for(i = 0; i < data_len - 1; i++)
    {
        add += send_buf[i];
    }
    memcpy(head_buf, send_buf, data_len);
    head_buf[data_len - 1] = add;
    
    //for(i = 0; i < data_len; i++)
    //{
        //pr_debug("MESH_Send_buf[%d]:%0x\n", i, head_buf[i]);
    //}
    
    len = write(fd, head_buf, data_len);

    if (len == data_len)
    {
        return len;
    }     
    else   
    {
        tcflush(fd,TCOFLUSH);
        return FALSE;
    }
}

//mesh升级
void meshupgrade(int nodetype)
{
    byte appsend[10] = {0xa5,0x00,0x04,0x02};
    byte databuf[65600] = {0};

    int i = 0;
    int offset = 0;
    long int datalen = 0;
    double lentemp = 0.0;
    double n = 0.0, precious = 0.0, inter = 0;
    int crclen = 0;
    byte crcdata[24] = {0};
    byte uartsend[32] = {0};
    byte meshover[10] = {0xe1,0x05,0x01,0x00};

    if(nodetype == 0)
    {
        meshover[3] = 0;
    }
    else
    {
        meshover[3] = 1;
    }

    FILE* fp = fopen("/tmp/meshbin", "rb+");
    if(fp == NULL)
    {
        pr_debug("meshupgrade:file open failed\n");
        pr_debug("%s: %s\n", "tmp/meshbin", strerror(errno));
        return;
    }
    
    memset(databuf, 0xff, sizeof(databuf));
    fread(databuf, 65536, 1, fp);
    
    sleep(1);
    fclose(fp);
    fp = NULL;

    datalen = (databuf[27] << 24) + (databuf[26] << 16) + (databuf[25] << 8) + databuf[24];
    pr_debug("meshupgrade_datalen:%ld\n", datalen);
    
    n = (double)datalen / 16;
    precious = modf(n, &inter);
    pr_debug("小数部分 %lf, 整数部分 %lf\n", precious, inter);
    if(precious > 0.0)
    {
        inter += 1;
    }
    pr_debug("meshupgrade_datacount:%lf\n", inter);
    
    answer = 0;
    for(i = 0; i < inter; i++)
    {
        if(answer == 2)
        {
            UartToApp_Send(appsend, appsend[2]);
            return;
        }
        memset(uartsend, 0xff, sizeof(uartsend));
        //id
        uartsend[0] = 0xe0;
        //len
        uartsend[1] = 0x19;
        //mac
        uartsend[2] = 0x01;
        //ota_type
        uartsend[3] = 0x00;
        //index
        uartsend[4] = i & 0xff; 
        uartsend[5] = (i >> 8) & 0xff;
        //ota data
        memcpy(uartsend + 6, databuf + offset, 16);
        //crc_ota_data
        memcpy(crcdata, uartsend + 4, 18);
        crclen = crc16(crcdata, 18);
        uartsend[22] = crclen & 0xff;
        uartsend[23] = (crclen >> 8) & 0xff;
        offset += 16;

        MESH_UART0_Send(uart_fd, uartsend, uartsend[1]);
        usleep(40000);
    }
    MESH_UART0_Send(uart_fd, meshover, meshover[1]);
    pr_debug("mesh send over\n");
}

void appgetmacsn(byte *buf)
{
    byte appsend[32] = {0x99,0x00,0x15};

    //wifimac
    memcpy(appsend + 3, hostinfo.wifimac, 6);
    //wanmac
    memcpy(appsend + 9, hostinfo.wanmac, 6);
    //hostsn
    memcpy(appsend + 15, hostinfo.hostsn, 6);

    UartToApp_Send(appsend, appsend[2]);
}

//修改mac和序列号
void modifymacsn(byte *macsn)
{
    if(macsn[2] != 21)
    {
        return;
    }
    if(hostinfo.mesh != 2)
    {
        return;
    }
    if(hostinfo.bindhost != 0)
    {
        return;
    }

    byte buf[65600] = {0};
    byte appsend[10] = {0x9a,0x00,0x04,0x00};
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00};

    FILE* fp = fopen("/tmp/factory.bin","rb+");
    if(fp == NULL)
    {
        pr_debug("modifymacsn:file open failed\n");
        pr_debug("%s: %s\n", "tmp/factory.bin", strerror(errno));
        appsend[3] = 0x01;
        UartToApp_Send(appsend, appsend[2]);
        return;
    }
    FILE* fptmp = fopen("/tmp/rf.bin","wb+");
    if(fptmp == NULL)
    {
        pr_debug("modifymacsn:file open failed\n");
        pr_debug("%s: %s\n", "tmp/rf.bin", strerror(errno));
        appsend[3] = 0x01;
        UartToApp_Send(appsend, appsend[2]);
        return;
    }
    
    fread(buf, 65536, 1, fp);
    
    //wifimac
    memcpy(buf + 4, macsn + 3, 6);

    //wanmac
    memcpy(buf + 40, macsn + 9, 6);

    //hostsn
    buf[32772] = macsn[15];
    buf[32773] = macsn[16];
    buf[32774] = macsn[17];
    buf[32775] = macsn[18];
    buf[32776] = macsn[19];
    buf[32777] = macsn[20];

    fwrite(buf, 65536, 1, fptmp);

    fclose(fp);
    fclose(fptmp);

    fp = NULL;
    fptmp = NULL;

    UartToApp_Send(appsend, appsend[2]);
    IRUART0_Send(single_fd, chipsend, chipsend[2]);
    
    sleep(2);
    system("mtd -r write /tmp/rf.bin factory");
}

//设置IP获取方式
void networkconfig(byte *buf)
{
    byte appsend[10] = {0x91,0x00,0x04,0x01};
    byte dhcp_str[10] = {"dhcp1"};
    int mark = 0;
    //主机
    if(0 == memcmp(buf + 3, hostinfo.hostsn, 6))
    {
        appsend[3] = 0x00;
        mark = 1;
    }
    else
    {
        //从机
        int i = 2;
        for(i = 2; i < 5; i++)
        {
            if(0 == memcmp(buf + 3, slaveident[i].slavesn, 6))
            {
                if(slaveident[i].sockfd > 0)
                {
                    appsend[3] = 0x00;
                    mark = i;
                }
                else
                {
                    appsend[3] == 0x01;
                }
                break;
            }
        }
    }
    UartToApp_Send(appsend, appsend[2]);

    if((appsend[3] == 0x00) && (mark > 1))
    {
        //发往从机
        hosttoslave(mark, buf, buf[2]);
    }
    //主机
    else if((appsend[3] == 0x00) && (mark == 1))
    {
        if(buf[9] == 0)
        {
            replace(HOSTSTATUS, dhcp_str, 4);
            system("rm -f /data/modsty/ipaddr.sh");
            system("/etc/init.d/network restart");
        }
        else if(buf[9] == 1)
        {
            FILE *fd = NULL;
            int fplen = 0;
            byte ip_addr[256] = {0};
            if((fd = fopen(IPADDR, "w")) == NULL)
            {
                pr_debug("%s: %s\n", IPADDR, strerror(errno));
                byte logsend[256] = {0xf3,0x00};
                byte logbuf[256] = {0};
                sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, IPADDR, strerror(errno));
                logsend[2] = (3 + strlen(logbuf));
                memcpy(logsend + 3, logbuf, strlen(logbuf));
                send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
                exit(-1);
            }
            snprintf(ip_addr, sizeof(ip_addr), "ifconfig br-lan %d.%d.%d.%d netmask %d.%d.%d.%d\nroute add default gw %d.%d.%d.%d\n", buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17], buf[18], buf[19], buf[20], buf[21]);

            fplen = fprintf(fd, "%s", ip_addr);
            pr_debug("fplen:%d\n", fplen);
            fclose(fd);
            fd = NULL;

            //执行修改指令
            modifynetwork();
        }
    }
}

//mesh升级检查0x35
void appmeshupgrade(byte *buf)
{
    byte appsend[10] = {0x07};
    memcpy(appsend + 1, buf + 1, 6);

    send(client_fd, appsend, appsend[2], MSG_NOSIGNAL);
}

//mesh升级验证
void meshupmd5check(byte *buf)
{
    byte appsend[10] = {0xa5,0x00,0x04,0x00};
    if(buf[3] == 0)
    {
        UartToApp_Send(appsend, appsend[2]);
        return;
    }
    int i = 0;
    int result = -1;
    byte wgetstr[128] = {"wget -P /tmp/ "};
    byte md5[32] = {0};
    memcpy(wgetstr + 14, buf + 21, buf[2] - 21);

    pr_debug("wgetstr:%s\n", wgetstr);
    system(wgetstr);
    pr_debug("下载mesh升级文件\n");
    sleep(5);
    result = MD5File("/tmp/meshbin", md5);
    if(result == 0)
    {
        for(i = 0; i < 16; i++)
        {
            pr_debug("md5[%d]:%0x\n", i, md5[i]);
        }
        if(0 == memcmp(buf + 5, md5, 16))
        {
            appsend[3] = 1;
            UartToApp_Send(appsend, appsend[2]);
            meshupgrade(buf[4]);
        }
        else
        {
            appsend[3] = 2;
            UartToApp_Send(appsend, appsend[2]);
        }
    }
    else
    {
        appsend[3] = 2;
        UartToApp_Send(appsend, appsend[2]);
    }
}

//查询从机
void slavequery()
{
    int i = 2;
    byte appsend[24] = {0x96,0x00,0x14,0x00,0x0f};
    for(i = 2; i < 5; i++)
    {
        if(slaveident[i].assignid > 1)
        {
            //sn6
            memcpy(appsend + 5, slaveident[i].slavesn, 6);
            //mac2
            appsend[11] = slaveident[i].assignid;
            appsend[12] = 0x01;
            //在线/离线
            appsend[13] = slaveident[i].sockfd;
            //版本号
            memcpy(appsend + 14, slaveident[i].version, 4);
            //温度
            appsend[18] = slaveident[i].temperature;
            //湿度
            appsend[19] = slaveident[i].moderate;
            
            UartToApp_Send(appsend, appsend[2]);
        }
    }
}

//删除从机
void delslave(byte *buf)
{
    byte appsend[24] = {0x96,0x00,0x0e,0x02,0x0f};
    byte delhex[24] = {0};
    byte delslave[24] = {0};

    memcpy(delhex, slaveident[buf[4]].slavesn, 7);
    HexToStr(delslave, delhex, 7);
    filecondel(MASSLAINFO, delslave, 12);
    sleep(1);

    pr_debug("delslave:%s\n", delslave);

    delslave[0] = delslave[12];
    delslave[1] = delslave[13];
    filecondel(MESHPW, delslave, 2);

    slavenetworking(buf[4]);

    memcpy(appsend + 5, slaveident[buf[4]].slavesn, 7);
    appsend[12] = 0x01;
    appsend[13] = 0x00;

    //发往从机
    hosttoslave(buf[4], buf, buf[2]);

    UartToApp_Send(appsend, appsend[2]);
    memset(&slaveident[buf[4]], 0, sizeof(slaveident[buf[4]]));
}

//从机操作0x26
void slavefunction(byte *buf)
{
    if(buf[3] == 0)
    {
        //查询从机
        slavequery();
    }
    else if(buf[3] == 1)
    {
        //添加从机
        hostinfo.receslave = 1;
        timers[2].startlogo = 1;

        pr_debug("开始搜索从机...\n");
    }
    else if(buf[3] == 2)
    {
        //删除从机
        delslave(buf);
    }
}

//从机修改IP获取方式
int slavemodifyipmethod(byte *buf)
{
    byte dhcp_str[10] = {"dhcp1"};
    byte ip_addr[256] = {0};
    if(buf[9] == 0)
    {
        replace(HOSTSTATUS, dhcp_str, 4);
        system("rm -f /data/modsty/ipaddr.sh");
        system("/etc/init.d/network restart");
    }
    else if(buf[9] == 1)
    {
        FILE *fd = NULL;
        int fplen = 0;
        if((fd = fopen(IPADDR, "w")) == NULL)
        {
            pr_debug("%s: %s\n", IPADDR, strerror(errno));
            byte logsend[256] = {0xf3,0x00};
            byte logbuf[256] = {0};
            sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, IPADDR, strerror(errno));
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
            exit(-1);
        }
        snprintf(ip_addr, sizeof(ip_addr), "ifconfig br-lan %d.%d.%d.%d netmask %d.%d.%d.%d\nroute add default gw %d.%d.%d.%d\n", buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17], buf[18], buf[19], buf[20], buf[21]);

        fplen = fprintf(fd, "%s", ip_addr);
        pr_debug("fplen:%d\n", fplen);
        fclose(fd);
        fd = NULL;

        //执行修改指令
        modifynetwork();
    }
}

int slavereceinstruct(byte *sockbuf)
{
    int data_len = 0;
    byte sendbuf[1024] = {0};
    byte chipsend[10] = {0x30,0x00,0x06,0x08,0x00,0x3e};
    byte uartsend[12] = {0x7b,0x0b,0x01,0x02,0x03,0x04,0x01,0x02,0x03,0x01,0x97};

    data_len = (((sockbuf[1] << 8) + sockbuf[2]) - 4);
    memcpy(sendbuf, sockbuf + 4, data_len);

    if(sendbuf[0] == 0x05)
    {
        pr_debug("从机程序升级\n");
        versionvalidation(sendbuf);
    }
    else if(sendbuf[0] == 0x21)
    {
        pr_debug("从机修改ip获取方式\n");
        slavemodifyipmethod(sendbuf);
    }
    else if(sendbuf[0] == 0x25)
    {
        //配置wifi
        wificonfig(sendbuf);
    }
    else if(sendbuf[0] == 0x26)
    {
        //删除从机
        //恢复出厂
        slaveUartSend(uart_fd, uartsend, uartsend[1]);

        slavechipSend(single_fd, chipsend, chipsend[2]);
        
        restorefactory();
        sleep(3);
        system("reboot");
    }
    else if(sendbuf[0] == 0x27)
    {
        //获取从机网络信息
        getnetworkinfo(sendbuf);
    }
    else if(sendbuf[0] == 0x30)
    {
        slavechipSend(single_fd, sendbuf, data_len);
    }
    else if(sendbuf[0] == 0x78)
    {
        //组网
        byte mesh[10] = {"mesh0"};
        hostinfo.mesh = 1;
        replace(HOSTSTATUS, mesh, 4);
        slaveUartSend(uart_fd, sendbuf, data_len);
    }
    else if(sendbuf[0] == 0x7b)
    {
        //解散网络
        byte mesh[10] = {"mesh2"};
        hostinfo.mesh = 2;
        replace(HOSTSTATUS, mesh, 4);
        slaveUartSend(uart_fd, sendbuf, data_len);
        sleep(3);
        exit(0);
    }
    else if(sendbuf[0] == 0xf2)
    {
        //从机心跳
        g_heartbeat = 0;
    }
    else
    {
        slaveUartSend(uart_fd, sendbuf, data_len);
    }
    return 0;
}

//智能锁命名检查
int smartlockname(byte *buf, byte *smartlock)
{
    int i = 0;
    int offset = 0;
    byte areastr[128] = {0};

    while(i < 260)
    {
        if(smartlockinfo[i].lockuserid == 0)
        {
            pr_debug("(1)i:%d\n", i);
            return -1;
        }
        //对比id
        else if(buf[6] == smartlockinfo[i].lockuserid)
        {
            pr_debug("(2)i:%d\n", i);
            //对比mac
            if(0 == memcmp(buf + 4, smartlockinfo[i].mac, 2))
            {
                pr_debug("(3)i:%d\n", i);
                //mac
                smartlock[offset] = buf[4];
                smartlock[offset + 1] = buf[5];
                offset += 2;
                //开锁方式
                smartlock[offset] = smartlockinfo[i].method;
                offset += 1;
                //用户权限
                smartlock[offset] = smartlockinfo[i].authority;
                offset += 1;
                //透传-1
                smartlock[offset] = smartlockinfo[i].penetrate;
                offset += 1;
                //id
                smartlock[offset] = smartlockinfo[i].lockuserid;
                offset += 1;

                //挟持标志
                if(buf[3] == 2)
                {
                    smartlock[offset] = smartlockinfo[i].reserved;
                }
                else if(buf[3] == 3)
                {
                    smartlock[offset] = buf[7];
                }
                offset += 1;
                //保留字节-2
                smartlock[offset] = smartlockinfo[i].retention[0];
                smartlock[offset + 1] = smartlockinfo[i].retention[1];
                offset += 2;
                if(buf[3] == 2)
                {
                    //名称长度-1
                    smartlock[offset] = buf[7];
                    offset += 1;
                    //名称-16
                    memcpy(smartlock + offset, buf + 8, 16);
                }
                else if(buf[3] == 3)
                {
                    smartlock[offset] = smartlockinfo[i].namelen;
                    offset += 1;
                    memcpy(smartlock + offset, smartlockinfo[i].name, 16);
                }
                else
                {
                    return -1;
                }
                offset += 16;
                HexToStr(areastr, smartlock, offset);
                replace(SMARTLOCK, areastr, 12);
                //读取智能锁用户信息
                readsmartlockinfo();

                pr_debug("i:%d\n", i);

                return 1;
            }
        }
        i++;
    }
    return -1;
}

//智能锁管理0x45
void smartlockmanage(byte *buf)
{
    int mark = 0;
    byte appsend[64] = {0xb5,0x00,0x1f,0x01};
    byte savehex[64] = {0};
    hostinfo.dataversion += 1;
    if((buf[3] == 2) || (buf[3] == 3))
    {
        appsend[4] = 0x01;
        mark = smartlockname(buf, savehex);
        pr_debug("mark:%d\n", mark);
        if(0 < mark)
        {
            appsend[4] = 0x00;
            memcpy(appsend + 5, savehex, 26);
        }
    }
    if(appsend[4] == 0)
    {
        appsend[2] = 0x1f;
    }
    else if(appsend[4] == 1)
    {
        appsend[2] = 0x05;
    }
    UartToApp_Send(appsend, appsend[2]);
}

//手势关联
void gesturerelated(byte *data)
{
    byte gesturestr[32] = {0};
    byte delrela[24] = {0};
    
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(GESTURE, gesturestr, 6);
    }
    else
    {
        if(data[2] == 0)
        {
            //节点/区域
            filecondel(GESTURE, gesturestr, 4);
            replace(GESTURE, gesturestr, 4);
        }
        else
        {
            //手动场景
            //删除节点/区域
            memcpy(delrela, gesturestr, 4);
            delrela[4] = '0';
            delrela[5] = '0';
            filecondel(GESTURE, delrela, 6);
            //替换相同方向的手势
            replace(GESTURE, gesturestr, 6);
        }
    }
    //读取手势关联
    readgestureinfo();
}

//智能锁关联
void smartlockrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(LOCKRELATED, gesturestr, 6);
    }
    else
    {
        replace(LOCKRELATED, gesturestr, 6);
    }
    //读取智能锁关联
    readlockinfo();
}

//震动关联
void shakerelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(SHAKE, gesturestr, 6);
    }
    else
    {
        replace(SHAKE, gesturestr, 6);
    }
    //读取震动关联
    readshakeinfo();
}

//烟雾传感器关联
void smokerelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(SMOKERELATED, gesturestr, 6);
    }
    else
    {
        replace(SMOKERELATED, gesturestr, 6);
    }
    //读取烟雾关联
    readsmokeinfo();
}

//门磁关联
void doormagnetrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(DOORMAGNET, gesturestr, 6);
    }
    else
    {
        replace(DOORMAGNET, gesturestr, 6);
    }
    //读取门磁关联
    readdoormagnetinfo();
}

//水浸关联
void floodingrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(FLOODING, gesturestr, 6);
    }
    else
    {
        replace(FLOODING, gesturestr, 6);
    }
    //读取水浸关联
    readfloodinginfo();
}

//红外活动侦测关联
void iractivityrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(IRACTIVITY, gesturestr, 6);
    }
    else
    {
        replace(IRACTIVITY, gesturestr, 6);
    }
    //读取红外活动侦测关联
    readiractivityinfo();
}

//雨雪关联
void rainsnowrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(RAINSNOW, gesturestr, 6);
    }
    else
    {
        replace(RAINSNOW, gesturestr, 6);
    }
    //读取雨雪关联
    readrainsnowinfo();
}

//燃气关联
void gasrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(GASINFO, gesturestr, 6);
    }
    else
    {
        replace(GASINFO, gesturestr, 6);
    }
    //读取燃气关联
    readgasinfo();
}

//风速关联
void windspeedrelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(WINDSPEED, gesturestr, 5);
    }
    else
    {
        replace(WINDSPEED, gesturestr, 5);
    }
    //读取风速关联
    readwindspeed();
}

//一氧化碳关联
void carmoxderelated(byte *data)
{
    byte gesturestr[32] = {0};
    HexToStr(gesturestr, data, 10);

    pr_debug("gesturestr:%s\n", gesturestr);

    if(data[3] == 0)
    {
        pr_debug("取消绑定\n");
        filecondel(CARMOXDE, gesturestr, 6);
    }
    else
    {
        replace(CARMOXDE, gesturestr, 6);
    }
    //读取一氧化碳关联
    readcarmoxde();
}

//关联管理0x46
void relatedmanage(byte *buf)
{
    byte reladata[64] = {0};
    byte appsend[24] = {0xb6,0x00,0x04,0x00};
    memcpy(reladata, buf + 4, buf[2] - 4);
    hostinfo.dataversion += 1;
    if(buf[3] == 0x01)
    {
        //手势
        gesturerelated(reladata);
    }
    else if(buf[3] == 0x02)
    {
        //智能锁
        smartlockrelated(reladata);
    }
    else if(buf[3] == 0x03)
    {
        //震动
        shakerelated(reladata);
    }
    else if(buf[3] == 0x04)
    {
        //烟雾
        smokerelated(reladata);
    }
    else if(buf[3] == 0x05)
    {
        //门磁关联
        doormagnetrelated(reladata);
    }
    else if(buf[3] == 0x06)
    {
        //水浸关联
        floodingrelated(reladata);
    }
    else if(buf[3] == 0x07)
    {
        //红外活动侦测关联
        iractivityrelated(reladata);
    }
    else if(buf[3] == 0x08)
    {
        //雨雪关联
        rainsnowrelated(reladata);
    }
    else if(buf[3] == 0x09)
    {
        //燃气关联
        gasrelated(reladata);
    }
    else if(buf[3] == 0x0a)
    {
        //风速关联
        windspeedrelated(reladata);
    }
    else if(buf[3] == 0x0b)
    {
        //一氧化碳关联
        carmoxderelated(reladata);
    }
    UartToApp_Send(appsend, appsend[2]);
}

//获取ip和子网掩码
int get_local_ip(char *ip)
{
    int rc = 0;
    byte netgw[128] = {0};
    byte ipv4[128] = {0};
    byte mask[16] = {0};
    byte ifname[128] = {0};
    struct sockaddr_in *addr = NULL;

    if(0 != getgateway(netgw))
    {
        return -1;
    }
    if(hostinfo.wifisw == 0)
    {
        strcpy(ifname, "br-lan");
    }
    else if(hostinfo.wifisw == 1)
    {
        strcpy(ifname, "apcli0");
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    /* 0. create a socket */
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd == -1)
    {
        return -1;
    }
    /* 1. set type of address to retrieve : IPv4 */
    ifr.ifr_addr.sa_family = AF_INET;

    /* 2. copy interface name to ifreq structure */
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    /* 3. get the IP address */
    if ((rc = ioctl(fd, SIOCGIFADDR, &ifr)) != 0)
    {
        close(fd);
        return -1;
    }
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(ipv4, inet_ntoa(addr->sin_addr), sizeof(ipv4));

    /* 4. get the mask */
    if ((rc = ioctl(fd, SIOCGIFNETMASK, &ifr)) != 0)
    {
        close(fd);
        return -1;
    }

    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(mask, inet_ntoa(addr->sin_addr), sizeof(mask));

    /* 5. display */
    printf("IFNAME:IPv4:MASK\n");
    printf("%s:%s:%s\n", ifname, ipv4, mask);
    
    int i = 0, j = 0, k = 0;
    byte ipmask[64] = {0};
    byte substr[12] = {0};

    strcat(ipv4, ".");
    strcat(ipv4, mask);
    strcat(ipv4, ".");
    strcat(ipv4, netgw);
    strcat(ipv4, ".");

    for(j = 0; j < strlen(ipv4); j++)
    {
        if(ipv4[j] == '.')
        {
            //pr_debug("substr:%s\n", substr);
            ipmask[i] = atoi(substr);
            //pr_debug("ipmask[%d]:%d\n", i, ipmask[i]);
            i++;
            k = 0;
            memset(substr, 0, sizeof(substr));
        }
        else
        {
            substr[k] = ipv4[j];
            //pr_debug("ipv4[%d]:%c\n", j, ipv4[j]);
            k++;
        }
    }
    for(i = 0; i < 12; i++)
    {
        pr_debug("ipmask[%d]:%d\n", i, ipmask[i]);
    }

    memcpy(ip, ipmask, 12);

    /* 6. close the socket */
    close(fd);
    return rc;
}

//获取主从机网络信息0x27
void getnetworkinfo(byte *buf)
{
    int offset = 0;
    byte appsend[128] = {0x97,0x00,0x00,0x01};
    byte macstr[24] = {0};
    byte machex[12] = {0};
    byte wifipw[128] = {0};
    byte ipmask[24] = {0};

    HexToStr(macstr, hostinfo.hostsn, 6);

    //type
    appsend[4] = buf[9];

    if(buf[9] == 1)
    {
        if(0 == filefind(macstr, wifipw, WIFIPW, 12))
        {
            StrToHex(machex, wifipw, (strlen(wifipw) / 2));
            //成功
            appsend[3] = 0x00;
            offset = ((strlen(wifipw) / 2) - 6);
            pr_debug("offset:%d--------------\n", offset);
            memcpy(appsend + 5, machex + 6, offset);
            //len
            appsend[2] = (offset + 5);
        }
        else
        {
            appsend[2] = 0x05;
        }
    }
    else if(buf[9] == 2)
    {
        if(hostinfo.mode == 1)
        {
            int mark = 0;
            int slmaster = 0;
            //主机
            if(0 == memcmp(buf + 3, hostinfo.hostsn, 6))
            {
                mark = 1;
            }
            else
            {
                //从机
                int i = 2;
                for(i = 2; i < 5; i++)
                {
                    if(0 == memcmp(buf + 3, slaveident[i].slavesn, 6))
                    {
                        if(slaveident[i].sockfd > 0)
                        {
                            slmaster = 0x00;
                            mark = i;
                        }
                        else
                        {
                            slmaster == 0x01;
                        }
                        break;
                    }
                }
            }
            if((slmaster == 0x00) && (mark > 1))
            {
                //发往从机
                hosttoslave(mark, buf, buf[2]);
                return;
            }
            else if((slmaster == 0x00) && (mark == 1))
            {
                if(0 == get_local_ip(ipmask))
                {
                    appsend[3] = 0x00;
                    if(hostinfo.dhcp == 0)
                    {
                        appsend[5] = 0x01;
                    }
                    else
                    {
                        appsend[5] = 0x00;
                    }
                    memcpy(appsend + 6, ipmask, 12);
                    appsend[2] = 18;
                }
                else
                {
                    appsend[2] = 0x05;
                }
            }
        }
        else
        {
            //获取从机ip
            if(0 == get_local_ip(ipmask))
            {
                appsend[3] = 0x00;
                if(hostinfo.dhcp == 0)
                {
                    appsend[5] = 0x01;
                }
                else
                {
                    appsend[5] = 0x00;
                }
                memcpy(appsend + 6, ipmask, 12);
                appsend[2] = 18;
            }
            else
            {
                appsend[2] = 0x05;
            }
            //发往主机
            slavepenetrate(appsend[2], appsend);
            return;
        }
    }
    else if(buf[9] == 3)
    {
        struct tm *ptr;
        time_t lt;
        lt = time(NULL);
        ptr = localtime(&lt);

        appsend[5] = (((ptr->tm_year + 1900) >> 8) & 0xff);
        appsend[6] = ((ptr->tm_year + 1900) & 0xff);
        appsend[7] = (ptr->tm_mon + 1);
        appsend[8] = ptr->tm_mday;
        appsend[9] = ptr->tm_hour;
        appsend[10] = ptr->tm_min;
        appsend[11] = ptr->tm_sec;
        appsend[3] = 0x00;
        appsend[2] = 12;
    }
    UartToApp_Send(appsend, appsend[2]);
}

//智能控制面板配置0x47
void smartcontrolpanel(byte *buf)
{
    byte uartsend[24] = {0x8c,0x0c};
    
    memcpy(uartsend + 2, buf + 4, 3);
    
    if((buf[6] == 0x03) || (buf[6] == 0x04))
    {
        //场景/区域id
        uartsend[5] = buf[7];
    }
    else
    {
        memcpy(uartsend + 5, buf + 8, 5);
    }
    UART0_Send(uart_fd, uartsend, uartsend[1]);
}
