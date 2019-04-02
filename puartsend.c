#include "puartsend.h"
#include "struct.h"

extern int master;
extern int uart_fd;
extern int answer;
extern int new_fd;
extern int g_led;
extern int g_led_on;
extern int g_meshdata;
extern byte ackswer[15];
extern struct cycle_buffer *fifo;

//红外存储快捷方式
void storeirshortcuts()
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

//单条自动场景读取
void singlereadautosce(byte *buf)
{
    int i = 0;
    int offset = 0;
    
    i = buf[offset];

    //重置触发条件和当前状态
    pr_debug("autotri[%d]:%d\n", i, sizeof(autotri[i]));
    pr_debug("ascecontrol[%d]:%d\n", i, sizeof(ascecontrol[i]));
    memset(&autotri[i], 0, sizeof(autotri[i]));
    memset(&ascecontrol[i], 0, sizeof(ascecontrol[i]));

    //id
    ascecontrol[i].id = buf[offset];
    offset += 1;
    //pr_debug("autosceid:%0x\n", ascecontrol[i].id);
    //使能
    ascecontrol[i].enable = buf[offset];
    offset += 1;
    //当前状态
    ascecontrol[i].status = buf[offset];
    offset += 1;
    //冷却时间
    ascecontrol[i].interval = (buf[offset] << 8) + buf[offset + 1];
    offset += 2;
    //预留6字节
    offset += 6;
    //触发模式
    ascecontrol[i].trimode = buf[offset];
    offset += 1;
    //限制条件
    memcpy(ascecontrol[i].limitations, buf + offset, 5);
    offset += 5;
    //触发条件长度
    ascecontrol[i].tricondlen = buf[offset];
    offset += 1;
    //触发条件
    memcpy(ascecontrol[i].tricond, buf + offset, ascecontrol[i].tricondlen);
    offset += ascecontrol[i].tricondlen;
    //执行任务长度
    ascecontrol[i].tasklen = (buf[offset] << 8) + buf[offset + 1];
    offset += 2;
    //pr_debug("ascecontrol[%d].tasklen:%d\n", i, ascecontrol[i].tasklen);
    //执行任务数据
    memcpy(ascecontrol[i].regtask, buf + offset, ascecontrol[i].tasklen);
    offset += ascecontrol[i].tasklen;
    //场景名称长度
    ascecontrol[i].namelen = buf[offset];
    offset += 1;
    //手动场景名称
    memcpy(ascecontrol[i].regname, buf + offset, 16);
    offset += 16;
    //手动场景图标id
    ascecontrol[i].iconid = buf[offset];
    //pr_debug("buf[%d]:%0x\n", ((strlen(areastr) / 2) - 1), buf[(strlen(areastr) / 2) - 1]);


    //触发条件偏移
    int type = 0;
    int z = 0;
    int err = 0;
    //获取当前时间
    time_t auto_time = 0;
    struct tm *ptr;
    time_t lt;
    lt = time(NULL); 
    ptr = localtime(&lt);       
    //pr_debug("minute:%d\n", ptr->tm_min);
    //pr_debug("hour:%d\n", ptr->tm_hour);
    //pr_debug("wday:%d\n", ptr->tm_wday);
    while(1)
    {
        //判断场景信息是否执行完毕
        if(type >= ascecontrol[i].tricondlen)
        {
            pr_debug("触发条件存储完毕\n");
            break;
        }
        else if(err == 1)
        {
            pr_debug("未识别的触发条件\n");
            break;
        }
        switch(ascecontrol[i].tricond[type])
        {
            case 0x01:
                pr_debug("触发条件01\n");
                for(z = 0; z < 10; z++)
                {
                    pr_debug("1\n");
                    if(type >= ascecontrol[i].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(autotri[i].autotion[z].load, ascecontrol[i].tricond + type, 6))
                    {
                        type += 6;
                        continue;
                    }
                    else if(0 == autotri[i].autotion[z].load[0])
                    {
                        memcpy(autotri[i].autotion[z].load, ascecontrol[i].tricond + type, 6);
                        //设定条件-开
                        if(1 == (autotri[i].autotion[z].load[3] >> 7))
                        {
                            //检查当前节点状态-开
                            if(0x01 == ((nodedev[autotri[i].autotion[z].load[2]].nodeinfo[5] >> ((autotri[i].autotion[z].load[3] & 0x7f) - 1)) & 0x01))
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].load[6] + autotri[i].autotion[z].load[7] + autotri[i].autotion[z].load[8]))
                                {
                                    pr_debug("开-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[i].autotion[z].load[2]);
                                    autotri[i].autotion[z].load[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].load[7] = ptr->tm_min;
                                    autotri[i].autotion[z].load[8] = ptr->tm_sec;
                                }
                            }
                        }
                        else if(0 == (autotri[i].autotion[z].load[3] >> 7))
                        {
                            //检查当前节点状态-关
                            if(0x00 == ((nodedev[autotri[i].autotion[z].load[2]].nodeinfo[5] >> ((autotri[i].autotion[z].load[3] & 0x7f) - 1)) & 0x01))
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].load[6] + autotri[i].autotion[z].load[7] + autotri[i].autotion[z].load[8]))
                                {
                                    pr_debug("关-刷新auto[%d].autotion[%d].loadmac:%0x\n", z, i, autotri[i].autotion[z].load[2]);
                                    autotri[i].autotion[z].load[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].load[7] = ptr->tm_min;
                                    autotri[i].autotion[z].load[8] = ptr->tm_sec;
                                }
                            }
                        }
                        pr_debug("autotri[%d].autotion[%d].load:%0x\n", i, z, autotri[i].autotion[z].load[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x06:
            case 0x08:
            case 0x11:
            case 0x12:
            case 0x52:
            case 0x54:
            case 0x55:
            case 0x56:
            case 0x57:
            case 0x58:
                pr_debug("触发条件06\n");
                type += 6;
                break;
            case 0x07:
                pr_debug("触发条件07\n");
                for(z = 0; z < 10; z++)
                {
                    pr_debug("1\n");
                    if(type >= ascecontrol[i].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(autotri[i].autotion[z].activity, ascecontrol[i].tricond + type, 6))
                    {
                        type += 6;
                        continue;
                    }
                    else if(0 == autotri[i].autotion[z].activity[0])
                    {
                        memcpy(autotri[i].autotion[z].activity, ascecontrol[i].tricond + type, 6);
                        //设定条件-开
                        if(1 == autotri[i].autotion[z].activity[3])
                        {
                            //检查当前节点状态-开
                            if(0x01 == (nodedev[autotri[i].autotion[z].activity[2]].nodeinfo[6] & 0x01))
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].activity[6] + autotri[i].autotion[z].activity[7] + autotri[i].autotion[z].activity[8]))
                                {
                                    pr_debug("开-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[i].autotion[z].activity[2]);
                                    autotri[i].autotion[z].activity[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].activity[7] = ptr->tm_min;
                                    autotri[i].autotion[z].activity[8] = ptr->tm_sec;
                                }
                            }
                        }
                        else if(0 == autotri[i].autotion[z].activity[3])
                        {
                            //检查当前节点状态-关
                            if(0x00 == (nodedev[autotri[i].autotion[z].activity[2]].nodeinfo[6] & 0x01))
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].activity[6] + autotri[i].autotion[z].activity[7] + autotri[i].autotion[z].activity[8]))
                                {
                                    pr_debug("开-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[i].autotion[z].activity[2]);
                                    autotri[i].autotion[z].activity[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].activity[7] = ptr->tm_min;
                                    autotri[i].autotion[z].activity[8] = ptr->tm_sec;
                                }
                            }
                        }
                        pr_debug("autotri[%d].autotion[%d].activity:%0x\n", i, z, autotri[i].autotion[z].activity[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x51:
                pr_debug("触发条件51\n");
                for(z = 0; z < 10; z++)
                {
                    pr_debug("1\n");
                    if(type >= ascecontrol[i].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(autotri[i].autotion[z].magnetic, ascecontrol[i].tricond + type, 6))
                    {
                        type += 6;
                        continue;
                    }
                    else if(0 == autotri[i].autotion[z].magnetic[0])
                    {
                        memcpy(autotri[i].autotion[z].magnetic, ascecontrol[i].tricond + type, 6);
                        //设定条件-开
                        if(1 == autotri[i].autotion[z].magnetic[3])
                        {
                            //检查当前节点状态-开
                            if(0x01 == nodedev[autotri[i].autotion[z].magnetic[2]].nodeinfo[5])
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].magnetic[6] + autotri[i].autotion[z].magnetic[7] + autotri[i].autotion[z].magnetic[8]))
                                {
                                    pr_debug("开-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[i].autotion[z].magnetic[2]);
                                    autotri[i].autotion[z].magnetic[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].magnetic[7] = ptr->tm_min;
                                    autotri[i].autotion[z].magnetic[8] = ptr->tm_sec;
                                }
                            }
                        }
                        else if(0 == autotri[i].autotion[z].magnetic[3])
                        {
                            //检查当前节点状态-关
                            if(0x00 == nodedev[autotri[i].autotion[z].magnetic[2]].nodeinfo[5])
                            {
                                //检查是否已经开始计时-未计时则开始计时
                                if(0 == (autotri[i].autotion[z].magnetic[6] + autotri[i].autotion[z].magnetic[7] + autotri[i].autotion[z].magnetic[8]))
                                {
                                    pr_debug("开-刷新auto[%d].autotion[%d].activitymac:%0x\n", z, i, autotri[i].autotion[z].magnetic[2]);
                                    autotri[i].autotion[z].magnetic[6] = ptr->tm_hour;
                                    autotri[i].autotion[z].magnetic[7] = ptr->tm_min;
                                    autotri[i].autotion[z].magnetic[8] = ptr->tm_sec;
                                }
                            }
                        }
                        pr_debug("autotri[%d].autotion[%d].activity:%0x\n", i, z, autotri[i].autotion[z].magnetic[2]);
                        break;
                    }
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, ascecontrol[i].tricond[type]);
                err = 1;
                break;
        }
    }
}

//自动场景存储信息
void autoscestoreinfo()
{
    int type = 0;
    int addlen = 0;
    int auto_logo = 0;
    int savelogo = 1;
    int savetype = 0;
    int i = 0;
    byte tempreg[10] = {0};
    byte savebuf[1300] = {0};
    byte savestr[2600] = {0};

    int badtype = 0;
    byte regbadmac[5] = {0};
    
    for(i = 0; i < (regionaltemp.count * 5); i++)
    {
        pr_debug("regionaltemp.regassoc[%d]:%0x\n", i, regionaltemp.regassoc[i]);
    }

    pr_debug("sceneack.maclen:%d\n", sceneack.maclen);
    
    while(1)
    {
        badtype = 0;
        savelogo = 1;
        //判断场景信息是否执行完毕
        if(regionaltemp.regassoc[type] == 0)
        {
            pr_debug("storeinfo指令完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            break;
        }
        //根据device Type拷贝指令长度
        switch(regionaltemp.regassoc[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("1\n");
                memcpy(tempreg, regionaltemp.regassoc + type, 10);
                for(i = 0; i < (sceneack.maclen / 4); i++)
                {
                    memcpy(regbadmac, sceneack.badmac + badtype, 4);
                    if(0 == memcmp(tempreg + 1, regbadmac, 2))
                    {
                        pr_debug("2\n");
                        if((regbadmac[2] & 0x0f) == 0x00)
                        {
                            savelogo = 0;
                        }
                        break;
                    }
                    badtype += 4;
                }
                type += 10;
                if(savelogo == 1)
                {
                    memcpy(savebuf + savetype, tempreg, 10);
                    savetype += 10;
                }
                break;
                //主机-红外/手动场景
            case 0x0f:
                //手动场景
                if(regionaltemp.regassoc[type + 3] == 0x01)
                {
                    type += 5;
                }
                //红外
                else if(regionaltemp.regassoc[type + 3] == 0x02)
                {
                    type += ((regionaltemp.regassoc[type + 4] * 2) + 5);
                }
                //跨主机场景
                else if(regionaltemp.regassoc[type + 3] == 0x03)
                {
                    type += 30;
                }
                else
                {
                    auto_logo = 1;
                }
                break;
            default:
                auto_logo = 1;
                pr_debug("不支持的类型\n");
                break;
        }
    }
    for(i = 0; i < savetype; i++)
    {
        pr_debug("savebuf[%d]:%0x\n", i, savebuf[i]);
    }
    pr_debug("regionaltemp.manlen:%d\n", regionaltemp.manlen);
    pr_debug("regionaltemp.irlen:%d\n", regionaltemp.irlen);
    if((savetype > 0) || (regionaltemp.manlen > 0) || (regionaltemp.irlen > 0) || (regionaltemp.crosshostlen > 0))
    {
        //拼接头+触发条件
        regionaltemp.reghead[17] = regionaltemp.tricondlen;
        addlen += 18;
        memcpy(regionaltemp.reghead + addlen, regionaltemp.tricond, regionaltemp.tricondlen);
        addlen += regionaltemp.tricondlen;
        //拼接执行任务长度
        //节点执行任务长度+手动场景长度+主机红外长度+跨主机场景长度
        regionaltemp.tasklen[0] = ((savetype + regionaltemp.manlen + regionaltemp.irlen + regionaltemp.crosshostlen) >> 8) & 0xff;
        regionaltemp.tasklen[1] = (savetype + regionaltemp.manlen + regionaltemp.irlen + + regionaltemp.crosshostlen) & 0xff;
        memcpy(regionaltemp.reghead + addlen, regionaltemp.tasklen, 2);
        addlen += 2;
        //拼接执行任务-手动场景-红外-跨主场景
        memcpy(regionaltemp.reghead + addlen, regionaltemp.manualsce, regionaltemp.manlen);
        addlen += regionaltemp.manlen;
        memcpy(regionaltemp.reghead + addlen, regionaltemp.irtask, regionaltemp.irlen);
        addlen += regionaltemp.irlen;
        memcpy(regionaltemp.reghead + addlen, regionaltemp.crosshostsce, regionaltemp.crosshostlen);
        addlen += regionaltemp.crosshostlen;
        //拼接执行任务
        memcpy(regionaltemp.reghead + addlen, savebuf, savetype);
        addlen += savetype;
        //拼接名称
        memcpy(regionaltemp.reghead + addlen, regionaltemp.regname, 18);
        addlen += 18;
        HexToStr(savestr, regionaltemp.reghead, addlen);
        
        pr_debug("savestr:%s\n", savestr);
        
        replace(AUTOSCE, savestr, 2);

        singlereadautosce(regionaltemp.reghead);
    }
}

//读取手动场景信息
void singlereadmansce(byte *buf)
{
    int i = 0;
    int offset = 0;

    i = buf[offset];
    memset(&mscecontrol[i], 0, sizeof(mscecontrol[i]));
    //id
    mscecontrol[i].id = buf[offset];
    offset += 1;
    //绑定按键mac
    mscecontrol[i].bingkeymac[0] = buf[offset];
    mscecontrol[i].bingkeymac[1] = buf[offset + 1];
    offset += 2;
    //绑定按键id
    mscecontrol[i].bingkeyid = buf[offset];
    offset += 1;
    //执行任务长度
    mscecontrol[i].tasklen = (buf[offset] << 8) + buf[offset + 1];
    offset += 2;
    //执行任务数据
    memcpy(mscecontrol[i].regtask, buf + offset, mscecontrol[i].tasklen);
    offset += mscecontrol[i].tasklen;
    //手动场景名称长度
    mscecontrol[i].namelen = buf[offset];
    offset += 1;
    //手动场景名称
    memcpy(mscecontrol[i].regname, buf + offset, 16);
    offset += 16;
    //手动场景图标id
    mscecontrol[i].iconid = buf[offset];
}

//手动场景存储信息
void manscestoreinfo()
{
    int type = 0;
    int addlen = 0;
    int auto_logo = 0;
    int savelogo = 1;
    int savetype = 0;
    int i = 0;
    byte tempreg[10] = {0};
    byte savebuf[1300] = {0};
    byte savestr[2600] = {0};

    int badtype = 0;
    byte regbadmac[5] = {0};
    
    for(i = 0; i < (regionaltemp.count * 10); i++)
    {
        pr_debug("regionaltemp.regassoc[%d]:%0x\n", i, regionaltemp.regassoc[i]);
    }

    pr_debug("手动场景sceneack.maclen:%d\n", sceneack.maclen);
    
    while(1)
    {
        pr_debug("手动场景while\n");
        badtype = 0;
        savelogo = 1;
        //判断场景信息是否执行完毕
        if(regionaltemp.regassoc[type] == 0)
        {
            pr_debug("storeinfo指令完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            pr_debug("auto_logo指令完毕\n");
            break;
        }
        //根据device Type拷贝指令长度
        switch(regionaltemp.regassoc[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("1\n");
                memcpy(tempreg, regionaltemp.regassoc + type, 10);
                for(i = 0; i < (sceneack.maclen / 4); i++)
                {
                    memcpy(regbadmac, sceneack.badmac + badtype, 4);
                    if(0 == memcmp(tempreg + 1, regbadmac, 2))
                    {
                        pr_debug("2\n");
                        if((regbadmac[2] & 0x0f) == 0x00)
                        {
                            savelogo = 0;
                        }
                        break;
                    }
                    badtype += 4;
                }
                type += 10;
                if(savelogo == 1)
                {
                    pr_debug("3\n");
                    memcpy(savebuf + savetype, tempreg, 10);
                    savetype += 10;
                }
                break;
                //红外码
            case 0x0f:
                if(regionaltemp.regassoc[type + 3] == 0x02)
                {
                    type += ((regionaltemp.regassoc[type + 4] * 2) + 5);
                }
                else if(regionaltemp.regassoc[type + 3] == 0x04)
                {
                    type += 5;
                }
                else
                {
                    auto_logo = 1;
                }
                break;
            default:
                pr_debug("区域不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    for(i = 0; i < savetype; i++)
    {
        pr_debug("savebuf[%d]:%0x\n", i, savebuf[i]);
    }
    if((savetype > 0) || (regionaltemp.irlen > 0) || (regionaltemp.seclen > 0))
    {
        memcpy(regionaltemp.reghead + 6, regionaltemp.irtask, regionaltemp.irlen);
        //节点执行任务长度+主机红外长度
        regionaltemp.reghead[4] = ((savetype + regionaltemp.irlen + regionaltemp.seclen) >> 8) & 0xff;
        regionaltemp.reghead[5] = (savetype +  regionaltemp.irlen + regionaltemp.seclen) & 0xff;
        addlen = (6 + regionaltemp.irlen);

        memcpy(regionaltemp.reghead + addlen, regionaltemp.security, regionaltemp.seclen);
        addlen += regionaltemp.seclen;
        
        memcpy(regionaltemp.reghead + addlen, savebuf, savetype);
        addlen += savetype;
        memcpy(regionaltemp.reghead + addlen, regionaltemp.regname, 18);
        addlen += 18;
        HexToStr(savestr, regionaltemp.reghead, addlen);
        replace(MANUALSCE, savestr, 2);

        singlereadmansce(regionaltemp.reghead);
    }
}

//读取单个区域关联信息
void singlereadareainfo(byte *buf)
{
    int i = 0;
    int offset = 0;

    i = buf[offset];
    memset(&regcontrol[i], 0, sizeof(regcontrol[i]));
    
    //id
    regcontrol[i].id = buf[offset];
    offset += 1;
    //绑定按键mac
    regcontrol[i].bingkeymac[0] = buf[offset];
    regcontrol[i].bingkeymac[1] = buf[offset + 1];
    offset += 2;
    //绑定按键id
    regcontrol[i].bingkeyid = buf[offset];
    offset += 1;
    //执行任务长度
    regcontrol[i].tasklen = (buf[offset] << 8) + buf[offset + 1];
    offset += 2;
    //执行任务数据
    memcpy(regcontrol[i].regtask, buf + offset, regcontrol[i].tasklen);
    offset += regcontrol[i].tasklen;
    //区域名称长度
    regcontrol[i].namelen = buf[offset];
    offset += 1;
    //区域名称
    memcpy(regcontrol[i].regname, buf + offset, 16);
    offset += 16;
    //区域图标id
    regcontrol[i].iconid = buf[offset];
    //pr_debug("readareainfo:%d\n", i);
}

//区域存储信息
void storeinfo()
{
    int type = 0;
    int auto_logo = 0;
    //int addlen = 0;
    int savelogo = 1;
    int savetype = 0;
    int i = 0;
    int offset = 0;
    byte tempreg[10] = {0};
    byte savebuf[1300] = {0};
    byte savestr[2600] = {0};

    int badtype = 0;
    byte regbadmac[5] = {0};
    
    for(i = 0; i < (regionaltemp.count * 8); i++)
    {
        pr_debug("regionaltemp.regassoc[%d]:%0x\n", i, regionaltemp.regassoc[i]);
    }

    pr_debug("sceneack.maclen:%d\n", sceneack.maclen);
    while(1)
    {
        badtype = 0;
        savelogo = 1;
        //判断场景信息是否执行完毕
        if(regionaltemp.regassoc[type] == 0)
        {
            pr_debug("storeinfo指令完毕\n");
            break;
        }
        if(auto_logo == 1)
        {
            break;
        }
        //根据device Type拷贝指令长度
        switch(regionaltemp.regassoc[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("1\n");
                memcpy(tempreg, regionaltemp.regassoc + type, 8);
                for(i = 0; i < (sceneack.maclen / 4); i++)
                {
                    memcpy(regbadmac, sceneack.badmac + badtype, 4);
                    if(0 == memcmp(tempreg + 1, regbadmac, 2))
                    {
                        pr_debug("2\n");
                        if(regbadmac[2] == 0x00)
                        {
                            savelogo = 0;
                        }
                        break;
                    }
                    badtype += 4;
                }
                type += 8;
                if(savelogo == 1)
                {
                    memcpy(savebuf + savetype, tempreg, 8);
                    savetype += 8;
                }
                break;
            default:
                pr_debug("区域不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
    for(i = 0; i < savetype; i++)
    {
        pr_debug("savebuf[%d]:%0x\n", i, savebuf[i]);
    }
    if(savetype > 0)
    {
        //执行任务长度
        regionaltemp.reghead[4] = ((savetype >> 8) & 0xff);
        regionaltemp.reghead[5] = (savetype & 0xff);

        memcpy(regionaltemp.reghead + 6, savebuf, savetype);
        offset = (6 + savetype);
        memcpy(regionaltemp.reghead + offset, regionaltemp.regname, 18);
        offset += 18;
        HexToStr(savestr, regionaltemp.reghead, offset);
        replace(REGASSOC, savestr, 2);
        
        singlereadareainfo(regionaltemp.reghead);
    }
}

int slaveuartsend(byte *sendbuf, int datalen)
{
    int slavemark = 0;
    
    //从机标识
    if(sendbuf[0] == 0x64)
    {
        slavemark = nodedev[sendbuf[4]].nodeinfo[0];
    }
    else if((sendbuf[0] == 0x67) || (sendbuf[0] == 0x70))
    {
        slavemark = nodedev[sendbuf[3]].nodeinfo[0];
    }
    else if((sendbuf[0] == 0x65) || (sendbuf[0] == 0x68) || (sendbuf[0] == 0x71))
    {
        slavemark = 0xff;
    }
    else if((sendbuf[0] > 0x72) && (sendbuf[0] < 0x7c))
    {
        //组网相关指令
        if((sendbuf[0] == 0x7b) && (hostinfo.slavemesh == 1))
        {
            slavemark = 0xff;
        }
        else
        {
            slavemark = hostinfo.slavemesh;
        }
        pr_debug("slavemark:%d\n", slavemark);
    }
    else
    {
        slavemark = nodedev[sendbuf[2]].nodeinfo[0];
    }
    
    if(slavemark != 1)
    {
        /*
        int i = 0;
        for(i = 0; i < (datalen + 4); i++)
        {
            pr_debug("slavemark[%d]:%0x\n", i, slavemark[i]);
        }
        */
        //发送至从机
        hosttoslave(slavemark, sendbuf, datalen);
    }
    return slavemark;
}

int slaveUartSend(int fd, byte *send_buf,int data_len)
{
    int i = 0;
    int len = 0;
    for(i = 0; i < data_len; i++)
    {
        pr_debug("UART0_Send_buf[%d]:%0x\n", i, send_buf[i]);
    }
    len = write(fd, send_buf, data_len);

    //指令发送检查
    /*
    if(0 == memcmp(rele_check, send_buf, 7))
    {
        a_config = 0;
    }
    */
    /*
    time_t timep;
    byte time_buf[128] = {0};
    byte buf[256] = {0};
    time(&timep);
    strcpy(buf, ctime(&timep));
    HexToStr(time_buf, send_buf, data_len);
    strncat(buf, time_buf, data_len * 2);
    buf[strlen(buf)] = '\n';
    //pr_debug("uartlog:%s\n", buf);
    InsertLine("/mnt/mtd/uartlog", buf);
    memset(buf, 0, sizeof(buf));
    */
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

int UartSend(int fd, byte *send_buf,int data_len)
{
    int i = 0;
    int mark = 0;
    int len = 0;
    
    for(i = 0; i < data_len; i++)
    {
        pr_debug("UART0_Send_buf[%d]:%0x\n", i, send_buf[i]);
    }
    
    mark = slaveuartsend(send_buf, data_len);
    pr_debug("mark:%d\n", mark);
    if((mark <= 1) || (mark == 0xff))
    {
        len = write(fd, send_buf, data_len);
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
    //指令发送检查
    /*
    if(0 == memcmp(rele_check, send_buf, 7))
    {
        a_config = 0;
    }
    */
    /*
    //time_t timep;
    //byte time_buf[128] = {0};
    //byte buf[256] = {0};
    time(&timep);
    strcpy(buf, ctime(&timep));
    HexToStr(time_buf, send_buf, data_len);
    strncat(buf, time_buf, data_len * 2);
    buf[strlen(buf)] = '\n';
    //pr_debug("uartlog:%s\n", buf);
    InsertLine("/mnt/mtd/uartlog", buf);
    memset(buf, 0, sizeof(buf));
    */
}

void UartSplit()
{
    byte buf[2500] = {0};
    int uart_logo = 0;
    int irlogo = 0;
    int fifo_len = 0;

    int i = 0;
    int j = 0;
    int scenelen = 0;
    //memset(buf, 0, sizeof(buf));
    //取出缓冲区内所有数据
    pthread_mutex_lock(&fifo->lock);
    fifo_len = fifo_get(buf, sizeof(buf));
    pthread_mutex_unlock(&fifo->lock);

    struct readdata
    {
        byte buf[100];
    };
    struct readdata readbuf[200];
    int struct_conut = 0;
    memset(readbuf, 0, sizeof(readbuf));
    /*
    for(i = 0; i < fifo_len; i++)
    {
        pr_debug("buf[%d]:%0x\n", i, buf[i]);
    }
    */
    if(fifo_len > 0)
    {
        pr_debug("UartSplit_fifo_len:%d\n", fifo_len);
        int offset = 0;
        byte uartsend[100] = {0};
        //pr_debug("buf[10]:%0x\n", buf[10]);
        while(1)
        {
            if(0 == (buf[offset]))
            {
                pr_debug("UartSend信息处理完毕\n");
                break;
            }
            if(1 == uart_logo)
            {
                pr_debug("UartSend信息处理完毕\n");
                break;
            }

            switch(buf[offset])
            {
                case 0x63:
                    memcpy(uartsend, buf + offset, 14);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 14)) || (readbuf[i].buf[3] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 14);
                            break;
                        }
                    }
                    offset += 14;
                    break;
                case 0x64:
                    memcpy(uartsend, buf + offset, 12);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 12)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 12);
                            break;
                        }
                    }
                    offset += 12;
                    break;
                case 0x65:
                    memcpy(uartsend, buf + offset, 12);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 12)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 12);
                            break;
                        }
                    }
                    offset += 12;
                    break;
                    //自动场景配置
                case 0x67:
                    memcpy(uartsend, buf + offset, 12);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 12)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 12);
                            break;
                        }
                    }
                    offset += 12;
                    break;
                    //自动场景执行
                case 0x68:
                    memcpy(uartsend, buf + offset, 4);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 3)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 4);
                            break;
                        }
                    }
                    offset += 4;
                    break;
                case 0x70:
                    memcpy(uartsend, buf + offset, 12);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 12)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 12);
                            break;
                        }
                    }
                    offset += 12;
                    break;
                case 0x71:
                    memcpy(uartsend, buf + offset, 4);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 3)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 4);
                            break;
                        }
                    }
                    offset += 4;
                    break;
                case 0x73:
                    memcpy(uartsend, buf + offset, 5);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 5)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 5);
                            break;
                        }
                    }
                    offset += 5;
                    break;
                case 0x76://分配mac
                case 0x78://组网
                case 0x7a://关闭组网
                case 0x7b://解散mesh
                    memcpy(uartsend, buf + offset, 11);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 11)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 11);
                            break;
                        }
                    }
                    //pr_debug("readbuf[i].buf[10]:%0x\n", readbuf[i].buf[10]);
                    offset += 11;
                    break;
                case 0x7c://完整红外码发送
                    memcpy(uartsend, buf + offset, buf[offset + 1]);
                    //UartSend(uart_fd, uartsend, 10);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, uartsend[1])) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, uartsend[1]);
                            break;
                        }
                    }
                    pr_debug("buf[%d]:%0x\n", offset + 1, buf[offset + 1]);
                    offset += uartsend[1];
                    break;
                case 0x7d://建立红外快捷方式
                    memcpy(uartsend, buf + offset, buf[offset + 1]);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, uartsend[1])) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, uartsend[1]);
                            break;
                        }
                    }
                    offset += uartsend[1];
                    break;
                case 0x7e://建立红外快捷方式
                case 0x7f:
                    memcpy(uartsend, buf + offset, buf[offset + 1]);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, uartsend[1])) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, uartsend[1]);
                            break;
                        }
                    }
                    offset += uartsend[1];
                    break;
                case 0x86:
                    memcpy(uartsend, buf + offset, 8);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 8)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 8);
                            break;
                        }
                    }
                    offset += 8;
                    break;
                case 0x87:
                case 0x88:
                    memcpy(uartsend, buf + offset, 6);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 6)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 6);
                            break;
                        }
                    }
                    offset += 6;
                    break;
                case 0x89:
                    memcpy(uartsend, buf + offset, 13);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 13)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 13);
                            break;
                        }
                    }
                    offset += 13;
                    break;
                case 0x8b:
                    memcpy(uartsend, buf + offset, 11);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 11)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 11);
                            break;
                        }
                    }
                    offset += 11;
                    break;
                case 0x8c:
                    memcpy(uartsend, buf + offset, 12);
                    for(i = 0; i < 200; i++)
                    {
                        if((0 == memcmp(uartsend, readbuf[i].buf, 12)) || (readbuf[i].buf[0] == 0))
                        {
                            memcpy(readbuf[i].buf, uartsend, 12);
                            break;
                        }
                    }
                    offset += 12;
                    break;
                default:
                    pr_debug("default:%0x\n", buf[offset + 3]);
                    for(i = 0; i < fifo_len; i++)
                    {
                        pr_debug("default%0x\n", buf[i]);
                    }
                    uart_logo = 1;
                    pr_debug("UartSend不支持的类型\n");
                    break;

            }
        }
        while(1)
        {
            if(readbuf[j].buf[0] == 0)
            {
                pr_debug("readbuf[%d]信息处理完毕\n", j);
                break;
            }
            
            if(g_meshdata == 1)
            {
                //led监控标识
                pthread_mutex_lock(&fifo->lock);
                g_meshdata = 0;
                g_led = 1;
                pthread_mutex_unlock(&fifo->lock);
                usleep(500000);
            }

            answer = 0;
            switch(readbuf[j].buf[0])
            {
                case 0x63:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x64:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    ackswer[0] = 0x64;
                    ackswer[1] = 0x05;
                    //mac
                    ackswer[2] = readbuf[j].buf[4];
                    //配置标识
                    ackswer[3] = readbuf[j].buf[3];
                    break;
                case 0x65:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                    //自动场景配置
                case 0x67:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    ackswer[0] = 0x67;
                    ackswer[1] = 0x05;
                    //mac
                    ackswer[2] = readbuf[j].buf[3];
                    //配置类型
                    ackswer[3] = readbuf[j].buf[4];
                    break;
                case 0x68:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x70:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    ackswer[0] = 0x70;
                    ackswer[1] = 0x05;
                    //mac
                    ackswer[2] = readbuf[j].buf[3];
                    //配置类型
                    ackswer[3] = readbuf[j].buf[4];
                    break;
                case 0x71:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x73:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x76:
                case 0x78:
                case 0x7a:
                case 0x7b:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    //pr_debug("readbuf[j].buf[0]:%0x\n", readbuf[j].buf[0]);
                    if(readbuf[j].buf[0] == 0x76)
                    {
                        memcpy(ackswer, readbuf[j].buf + 3, 7);
                    }
                    break;
                case 0x7c:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    ackswer[0] = 0x7c;
                    ackswer[1] = 0x06;
                    //mac
                    ackswer[2] = readbuf[j].buf[2];
                    //状态-成功
                    ackswer[3] = 0;
                    break;
                case 0x7d:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    ackswer[0] = 0x7d;
                    ackswer[1] = 0x08;
                    //mac
                    ackswer[2] = readbuf[j].buf[2];
                    //状态-成功
                    ackswer[3] = 0;
                    break;
                case 0x7e:
                case 0x7f:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x86:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                case 0x87:
                case 0x88:
                case 0x89:
                case 0x8b:
                case 0x8c:
                    UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                    break;
                default:
                    pr_debug("default:%0x\n", buf[offset + 3]);
                    for(i = 0; i < fifo_len; i++)
                    {
                        pr_debug("default%0x\n", buf[i]);
                    }
                    uart_logo = 1;
                    pr_debug("(2)UartSend不支持的类型\n");
                    break;

            }
            //led监控标识
            pthread_mutex_lock(&fifo->lock);
            g_led = 1;
            pthread_mutex_unlock(&fifo->lock);

            //红外
            if(readbuf[j].buf[0] == 0x7d)
            {
                int ackcount = 0;
                int slcount = 0;
                irlogo = 0;
                while(ackcount < 3)
                {
                    //led监控标识
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    pthread_mutex_unlock(&fifo->lock);

                    usleep(200000);
                    if(ackcount == 2)
                    {
                        answer = 0;
                        pr_debug("ackcount(3):%d\n", ackcount);
                        pr_debug("0x7c配置无应答\n");
                        break;
                    }
                    else if(answer == 1)
                    {
                        answer = 0;
                        irlogo = 1;
                        break;
                    }
                    else
                    {
                        slcount++;

                        if((slcount == 40) && (ackcount == 0))
                        {
                            answer = 0;
                            pr_debug("ackcount(1):%d\n", ackcount);
                            ackcount++;
                        }
                        else if((slcount == 80) && (ackcount == 1))
                        {
                            answer = 0;
                            pr_debug("ackcount(2):%d\n", ackcount);
                            ackcount++;
                        }
                    }
                }
            }
            //删除整条区域/场景
            else if(((readbuf[j].buf[0] == 0x64) || (readbuf[j].buf[0] == 0x67) || (readbuf[j].buf[0] == 0x70)) && (readbuf[j].buf[4] == 0xff))
            {
                int ackcount = 0;
                int slcount = 0;
                while(ackcount < 3)
                {
                    //led监控标识
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    pthread_mutex_unlock(&fifo->lock);

                    usleep(200000);
                    if(ackcount == 2)
                    {
                        answer = 0;
                        pr_debug("ackcount(3):%d\n", ackcount);
                        pr_debug("删除整条超时应答\n");
                        break;
                    }
                    else if(answer == 2)
                    {
                        answer = 0;
                        pr_debug("删除整条超时应答\n");
                        break;
                    }
                    else
                    {
                        slcount++;

                        if((slcount == 10) && (ackcount == 0))
                        {
                            answer = 0;
                            pr_debug("ackcount(1):%d\n", ackcount);
                            ackcount++;
                        }
                        else if((slcount == 20) && (ackcount == 1))
                        {
                            answer = 0;
                            pr_debug("ackcount(2):%d\n", ackcount);
                            ackcount++;
                        }
                    }
                }
            }
            //区域
            else if((readbuf[j].buf[0] == 0x64) && (readbuf[j].buf[4] != 0xff))
            {
                int ackcount = 0;
                int slcount = 0;
                while(ackcount < 3)
                {
                    //led监控标识
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    pthread_mutex_unlock(&fifo->lock);
                    
                    pr_debug("answer(1):%d\n", answer);
                    
                    usleep(200000);
                    if(ackcount == 2)
                    {
                        answer = 0;
                        pr_debug("ackcount(3):%d\n", ackcount);
                        pr_debug("0x64配置无应答\n");
                        //主机标识
                        sceneack.badmac[sceneack.maclen] = nodedev[readbuf[j].buf[4]].nodeinfo[0];
                        //mac
                        sceneack.badmac[sceneack.maclen + 1] = readbuf[j].buf[4];
                        //配置标识
                        sceneack.badmac[sceneack.maclen + 2] = readbuf[j].buf[3];
                        //失败-超时
                        sceneack.badmac[sceneack.maclen + 3] = 0x03;
                        sceneack.maclen += 4;
                        break;
                    }
                    else if(answer == 1)
                    {
                        pr_debug("answer(2):%d\n", answer);

                        answer = 0;
                        break;
                    }
                    else if(answer == 2)
                    {
                        answer = 0;
                        pr_debug("超时重发\n");
                        UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                        ackcount++;
                    }
                    else
                    {
                        slcount++;
                        
                        if((slcount == 40) && (ackcount == 0))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(1):%d\n", ackcount);
                            ackcount++;
                        }
                        else if((slcount == 80) && (ackcount == 1))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(2):%d\n", ackcount);
                            ackcount++;
                        }
                    }
                    /*
                    time_t timep;
                    byte ack_time_buf[50] = {0};
                    byte ackbuf[50] = {0};

                    time(&timep);
                    strcpy(ackbuf, ctime(&timep));
                    HexToStr(ack_time_buf, readbuf[j].buf, readbuf[j].buf[1]);
                    //ackbuf[strlen(ackbuf)] = ' ';
                    strncat(ackbuf, ack_time_buf, 18);
                    ackbuf[strlen(ackbuf)] = '\n';
                    InsertLine("/mnt/mtd/acklog", ackbuf);
                    //memset(ackbuf, 0, sizeof(ackbuf));
                    */
                }
            }
            //自动场景
            else if((readbuf[j].buf[0] == 0x67) && (readbuf[j].buf[3] != 0xff))
            {
                int ackcount = 0;
                int slcount = 0;
                while(ackcount < 3)
                {
                    //led监控标识
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    pthread_mutex_unlock(&fifo->lock);
                    
                    pr_debug("answer(1):%d\n", answer);
                    
                    usleep(200000);
                    if(ackcount == 2)
                    {
                        answer = 0;
                        pr_debug("ackcount(3):%d\n", ackcount);
                        pr_debug("0x67配置无应答\n");
                        //主机标识
                        sceneack.badmac[sceneack.maclen] = nodedev[readbuf[j].buf[3]].nodeinfo[0];
                        //mac
                        sceneack.badmac[sceneack.maclen + 1] = readbuf[j].buf[3];
                        //配置标识
                        sceneack.badmac[sceneack.maclen + 2] = readbuf[j].buf[4];
                        //失败-超时
                        sceneack.badmac[sceneack.maclen + 3] = 0x03;
                        sceneack.maclen += 4;
                        break;
                    }
                    else if(answer == 1)
                    {
                        pr_debug("answer(2):%d\n", answer);

                        answer = 0;
                        break;
                    }
                    else if(answer == 2)
                    {
                        answer = 0;
                        pr_debug("超时重发\n");
                        UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                        ackcount++;
                    }
                    else
                    {
                        slcount++;
                        
                        if((slcount == 40) && (ackcount == 0))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(1):%d\n", ackcount);
                            ackcount++;
                        }
                        else if((slcount == 80) && (ackcount == 1))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(2):%d\n", ackcount);
                            ackcount++;
                        }
                    }
                    /*
                    time_t timep;
                    byte ack_time_buf[50] = {0};
                    byte ackbuf[50] = {0};

                    time(&timep);
                    strcpy(ackbuf, ctime(&timep));
                    HexToStr(ack_time_buf, readbuf[j].buf, readbuf[j].buf[1]);
                    //ackbuf[strlen(ackbuf)] = ' ';
                    strncat(ackbuf, ack_time_buf, 16);
                    ackbuf[strlen(ackbuf)] = '\n';
                    InsertLine("/mnt/mtd/acklog", ackbuf);
                    //memset(ackbuf, 0, sizeof(ackbuf));
                    */
                }
            }
            //手动场景
            else if((readbuf[j].buf[0] == 0x70) && (readbuf[j].buf[3] != 0xff))
            {
                int ackcount = 0;
                int slcount = 0;
                while(ackcount < 3)
                {
                    //led监控标识
                    pthread_mutex_lock(&fifo->lock);
                    g_led = 1;
                    pthread_mutex_unlock(&fifo->lock);
                    
                    pr_debug("answer(1):%d\n", answer);
                    
                    usleep(200000);
                    if(ackcount == 2)
                    {
                        answer = 0;
                        pr_debug("ackcount(3):%d\n", ackcount);
                        pr_debug("0x70配置无应答\n");
                        //主机标识
                        sceneack.badmac[sceneack.maclen] = nodedev[readbuf[j].buf[3]].nodeinfo[0];
                        //mac
                        sceneack.badmac[sceneack.maclen + 1] = readbuf[j].buf[3];
                        //配置标识
                        sceneack.badmac[sceneack.maclen + 2] = readbuf[j].buf[4];
                        //失败-超时
                        sceneack.badmac[sceneack.maclen + 3] = 0x03;
                        sceneack.maclen += 4;
                        break;
                    }
                    else if(answer == 1)
                    {
                        pr_debug("answer(2):%d\n", answer);

                        answer = 0;
                        break;
                    }
                    else if(answer == 2)
                    {
                        answer = 0;
                        pr_debug("超时重发\n");
                        UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                        ackcount++;
                    }
                    else
                    {
                        slcount++;
                        
                        if((slcount == 40) && (ackcount == 0))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(1):%d\n", ackcount);
                            ackcount++;
                        }
                        else if((slcount == 80) && (ackcount == 1))
                        {
                            answer = 0;
                            UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                            pr_debug("ackcount(2):%d\n", ackcount);
                            ackcount++;
                        }
                    }
                    /*
                    time_t timep;
                    byte ack_time_buf[50] = {0};
                    byte ackbuf[50] = {0};

                    time(&timep);
                    strcpy(ackbuf, ctime(&timep));
                    HexToStr(ack_time_buf, readbuf[j].buf, readbuf[j].buf[1]);
                    //ackbuf[strlen(ackbuf)] = ' ';
                    strncat(ackbuf, ack_time_buf, 16);
                    ackbuf[strlen(ackbuf)] = '\n';
                    InsertLine("/mnt/mtd/acklog", ackbuf);
                    //memset(ackbuf, 0, sizeof(ackbuf));
                    */
                }
            }
            else if(readbuf[j].buf[0] == 0x76)
            {
                int ackcount = 0;
                int slcount = 0;
                //pr_debug("answer:%d\n", answer);
                while(ackcount < 2)
                {
                    usleep(100000);
                    slcount++;

                    if(answer == 1)
                    {
                        //pr_debug("answer(3):%d\n", answer);
                        answer = 0;
                        break;
                    }

                    if((slcount == 8) && (ackcount == 0))
                    {
                        answer = 0;
                        UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                        //pr_debug("ackcount(1):%d\n", ackcount);
                        ackcount++;
                    }
                    else if((slcount == 16) && (ackcount == 1))
                    {
                        answer = 0;
                        UartSend(uart_fd, readbuf[j].buf, readbuf[j].buf[1]);
                        //pr_debug("ackcount(2):%d\n", ackcount);
                        ackcount++;
                    }
                    else
                    {
                        continue;
                    }

                    /*
                    time_t timep;
                    byte ack_time_buf[50] = {0};
                    byte ackbuf[50] = {0};

                    time(&timep);
                    strcpy(ackbuf, ctime(&timep));
                    HexToStr(ack_time_buf, readbuf[j].buf, readbuf[j].buf[1]);
                    //ackbuf[strlen(ackbuf)] = ' ';
                    strncat(ackbuf, ack_time_buf, 22);
                    ackbuf[strlen(ackbuf)] = '\n';
                    InsertLine("/mnt/mtd/acklog", ackbuf);
                    //memset(ackbuf, 0, sizeof(ackbuf));
                    */
                }
            }
            else
            {
                //usleep(320000);
                usleep(500000);
            }
            if(0 == memcmp(readbuf[j].buf, sceneack.endbuf, 7))
            {
                pr_debug("指令发送完毕,准备应答\n");
                pr_debug("sceneack.maclen:%d\n", sceneack.maclen);
                pr_debug("regionaltemp.count:%d\n", regionaltemp.count);
                if(readbuf[j].buf[0] != 0x7c)
                {
                    scenelen = sceneack.maclen + 5;
                    sceneack.headbuf[1] = ((scenelen >> 8) & 0xff);
                    sceneack.headbuf[2] = (scenelen & 0xff);
                    //失败个数-指令个数
                    if(sceneack.maclen == (regionaltemp.count * 4))
                    {
                        sceneack.headbuf[4] = 0x01;
                    }
                    memcpy(sceneack.headbuf + 5, sceneack.badmac, sceneack.maclen);
                    UartToApp_Send(sceneack.headbuf, scenelen);
                }

                //存储
                if(readbuf[j].buf[0] == 0x64)
                {
                    storeinfo();
                }
                else if(readbuf[j].buf[0] == 0x67)
                {
                    autoscestoreinfo();
                }
                else if(readbuf[j].buf[0] == 0x70)
                {
                    manscestoreinfo();
                }
                /*
                else if(readbuf[j].buf[0] == 0x7d)
                {
                    if(irlogo == 1)
                    {
                        storeirshortcuts();
                    }
                }
                */
                //
                //memset(&sceneack, 0, sizeof(sceneack));
            }
            j++;
        }
    }
}

void *uart_run(void *arg)
{
    while(1)
    {
        //uart指令检查
        UartSplit();
        usleep(100000);
    }
}
