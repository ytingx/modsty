#include "pautosce.h"
#include "struct.h"

extern byte  g_autotime[BUF_SIZE];
extern int client_fd;
extern int uart_fd;
extern byte  relevance_temp[10];
extern byte  dhcp_s[10];

//保存自动场景执行状态
void saveautoscestatus()
{
    int i = 1;
    //触发条件偏移
    int type = 0;
    int z = 0;
    byte savestatus[512] = {0};
    byte savestastr[1024] = {0};
    while(i < 11)
    {
        //未使能
        if(ascecontrol[i].enable == 0)
        {
            i++;
            continue;
        }
        //触发条件长度为0
        else if(ascecontrol[i].tricondlen == 0)
        {
            i++;
            continue;
        }
        //执行条件长度为0
        else if(ascecontrol[i].tasklen == 0)
        {
            i++;
            continue;
        }
        
        memset(savestatus, 0, sizeof(savestatus));
        memset(savestastr, 0, sizeof(savestastr));
        //id
        savestatus[0] = ascecontrol[i].id;
        //执行状态
        savestatus[1] = autotri[i].ldentification;
        //上次执行时间
        savestatus[2] = ((autotri[i].lasttime >> 24) & 0xff);
        savestatus[3] = ((autotri[i].lasttime >> 16) & 0xff);
        savestatus[4] = ((autotri[i].lasttime >> 8) & 0xff);
        savestatus[5] = (autotri[i].lasttime & 0xff);

        //存储触发条件
        type = 6;
        for(z = 0; z < 10; z++)
        {
            if(0 < (autotri[i].autotion[z].load[6] + autotri[i].autotion[z].load[7] + autotri[i].autotion[z].load[8]))
            {
                memcpy(savestatus + type, autotri[i].autotion[z].load, 9);
                type += 9;
            }
        }
        for(z = 0; z < 10; z++)
        {
            if(0 < (autotri[i].autotion[z].activity[6] + autotri[i].autotion[z].activity[7] + autotri[i].autotion[z].activity[8]))
            {
                memcpy(savestatus + type, autotri[i].autotion[z].activity, 9);
                type += 9;
            }
        }
        for(z = 0; z < 10; z++)
        {
            if(0 < (autotri[i].autotion[z].magnetic[6] + autotri[i].autotion[z].magnetic[7] + autotri[i].autotion[z].magnetic[8]))
            {
                memcpy(savestatus + type, autotri[i].autotion[z].magnetic, 9);
                type += 9;
            }
        }
        pr_debug("ascecontrol[%d].id:%d-%d\n", i, ascecontrol[i].id, type);
        HexToStr(savestastr, savestatus, type);
        savestastr[strlen(savestastr)] = '\n';
        InsertLine(AUTOSTA, savestastr);
        i++;
    }
    pr_debug("saveautoscestatus完毕\n");
}

//读取自动场景执行状态
void readautoscestatus()
{
    FILE *fd = NULL;
    int i = 1;
    //触发条件偏移
    int type = 6;
    int z = 0;
    
    byte savestatus[512] = {0};
    byte savestastr[1024] = {0};

    if((fd = fopen(AUTOSTA, "r")) == NULL)
    {
        pr_debug("%s: %s\n", AUTOSTA, strerror(errno));
        return;
    }
    pr_debug("readautoscestatus触发条件结构体:%d\n", sizeof(autotri));
    memset(autotri, 0, sizeof(autotri));

    while(fgets(savestastr, 1024, fd))
    {
        StrToHex(savestatus, savestastr, strlen(savestastr) / 2);

        //id
        autotri[savestatus[0]].id = savestatus[0];
        i = savestatus[0];
        //执行状态
        autotri[savestatus[0]].ldentification = savestatus[1];
        ascecontrol[savestatus[0]].status = savestatus[1];
        //上次执行时间
        autotri[savestatus[0]].lasttime = ((savestatus[2] << 24) + (savestatus[3] << 16) + \
                (savestatus[4] << 8) + savestatus[5] + 40);

        pr_debug("autotri[%d].id:%d\n", savestatus[0], autotri[savestatus[0]].id);
        pr_debug("autotri[%d].ldentification:%d\n", savestatus[0], savestatus[1]);
        pr_debug("autotri[%d].lasttime:%ld\n", savestatus[0], autotri[savestatus[0]].lasttime);
        while(1)
        {
            //判断场景信息是否执行完毕
            if(0 == savestatus[type])
            {
                pr_debug("readautoscestatus完毕\n");
                break;
            }
            switch(savestatus[type])
            {
                case 0x01:
                    pr_debug("触发条件01\n");
                    for(z = 0; z < 10; z++)
                    {
                        pr_debug("1\n");
                        if(0 == memcmp(autotri[i].autotion[z].load, savestatus + type, 6))
                        {
                            break;
                        }
                        else if(0 == autotri[i].autotion[z].load[0])
                        {
                            memcpy(autotri[i].autotion[z].load, savestatus + type, 9);
                            pr_debug("i:%d z:%d\n", i, z);
                            break;
                        }
                    }
                    type += 9;
                    break;
                case 0x07:
                    pr_debug("触发条件07\n");
                    for(z = 0; z < 10; z++)
                    {
                        pr_debug("2\n");
                        if(0 == memcmp(autotri[i].autotion[z].activity, savestatus + type, 6))
                        {
                            break;
                        }
                        else if(0 == autotri[i].autotion[z].activity[0])
                        {
                            pr_debug("i:%d z:%d\n", i, z);
                            memcpy(autotri[i].autotion[z].activity, savestatus + type, 9);
                            break;
                        }
                    }
                    type += 9;
                    break;
                case 0x51:
                    pr_debug("触发条件51\n");
                    for(z = 0; z < 10; z++)
                    {
                        pr_debug("2\n");
                        if(0 == memcmp(autotri[i].autotion[z].magnetic, savestatus + type, 6))
                        {
                            break;
                        }
                        else if(0 == autotri[i].autotion[z].magnetic[0])
                        {
                            pr_debug("i:%d z:%d\n", i, z);
                            memcpy(autotri[i].autotion[z].magnetic, savestatus + type, 9);
                            break;
                        }
                    }
                    type += 9;
                    break;
                default:
                    pr_debug("readautoscestatus未识别类型\n");
                    type += 9;
                    break;
            }
        }
        memset(savestatus, 0, sizeof(savestatus));
        memset(savestastr, 0, sizeof(savestastr));
    }
    fclose(fd);
    fd = NULL;
    system("rm -f /data/modsty/autosta");
    pr_debug("所有readautoscestatus获取完毕\n");
}

//重置每天执行一次的场景
void autosceresetregularly()
{
    int i = 1;
    
    struct tm *ptr;
    time_t lt;
    lt = time(NULL);
    ptr = localtime(&lt);

    //pr_debug("ptr->tm_hour:%d\n", ptr->tm_hour);
    //pr_debug("ptr->tm_min:%d\n", ptr->tm_min);
    //pr_debug("hostinfo.autoscereset:%d\n", hostinfo.autoscereset);
    if((hostinfo.autoscereset == 1) && (ptr->tm_hour == 0) && (ptr->tm_min == 0))
    {
        pr_debug("重置自动场景标志\n");
        hostinfo.autoscereset = 2;
    }
    else if(((hostinfo.autoscereset == 2) || (hostinfo.autoscereset == 0)) && (ptr->tm_hour == 0) && (ptr->tm_min == 1))
    {
        pr_debug("重置自动场景:%d\n", hostinfo.autoscereset);
        hostinfo.autoscereset = 1;

        while(i < 11)
        {
            //触发条件长度为0
            if(ascecontrol[i].tricondlen == 0)
            {
                i++;
                continue;
            }
            //执行条件长度为0
            else if(ascecontrol[i].tasklen == 0)
            {
                i++;
                continue;
            }
            //只执行一次的场景-且已经执行
            else if((ascecontrol[i].interval == 0) && (ascecontrol[i].status == 1))
            {
                i++;
                continue;
            }
            else if(ascecontrol[i].interval == 1440)
            {
                ascecontrol[i].status = 0;
                autotri[i].ldentification = 0;
                autotri[i].lasttime = 0;
            }
            i++;
        }
    }
}

//执行自动场景-主从机红外/手动场景
void run_autosceneir(int id)
{
    int type = 0;
    int i = 0;
    int auto_logo = 0;
    byte autosceneir[20] = {0};
    byte cloudsend[32] = {0x09,0x00,0x0b};
    //执行红外
    byte perirfast[10] = {0x65,0x00,0x07};
    //执行手动场景
    byte permanualsce[10] = {0x62,0x00,0x04};

    while(1)
    {
        pr_debug("ascecontrol[id].regtask[%d]:%0x\n", type, ascecontrol[id].regtask[type]);
        //判断场景信息是否执行完毕
        if(ascecontrol[id].tasklen <= type)
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
        switch(ascecontrol[id].regtask[type])
        {
            //指令长度相同，统一处理
            //继电器，小夜灯，背光灯
            case 0x01:
            case 0x11:
            case 0x12:
            case 0x21:
            case 0x31:
                pr_debug("case 0x01\n");
                type += 10;
                break;
                //红外/手动场景
            case 0x0f:
                if(ascecontrol[id].regtask[type + 3] == 0x01)
                {
                    //执行手动场景
                    permanualsce[3] = ascecontrol[id].regtask[type + 4];
                    manualsceope(permanualsce);
                    type += 5;
                }
                else if(ascecontrol[id].regtask[type + 3] == 0x02)
                {
                    //执行主从机红外
                    memcpy(autosceneir, ascecontrol[id].regtask + (type + 5), (ascecontrol[id].regtask[type + 4] * 2));
                    for(i = 0; i < (ascecontrol[id].regtask[type + 4] * 2); i += 2)
                    {
                        perirfast[3] = autosceneir[i];
                        perirfast[4] = autosceneir[i + 1];
                        perirfast[5] = ascecontrol[id].regtask[type];
                        perirfast[6] = ascecontrol[id].regtask[type + 1];
                        //执行红外快捷方式
                        runirquick(perirfast);
                    }
                    type += ((ascecontrol[id].regtask[type + 4] * 2) + 5);
                }
                else if(ascecontrol[id].regtask[type + 3] == 0x03)
                {
                    //执行跨主机场景
                    memcpy(autosceneir, ascecontrol[id].regtask + (type + 4), 30);
                    //主机sn
                    memcpy(cloudsend + 3, autosceneir, 6);
                    //type-04
                    cloudsend[9] = autosceneir[6];
                    //id
                    cloudsend[10] = autosceneir[7];
                    sendsingle(client_fd, cloudsend, cloudsend[2]);
                    type += 30;
                }
                else
                {
                    pr_debug("自动场景不支持的类型\n");
                    auto_logo = 1;
                }
                break;
            default:
                pr_debug("自动场景不支持的类型\n");
                auto_logo = 1;
                break;
        }
    }
}

//执行自动场景
void run_autoscene(int id, int triclen, int trimode, byte *tricond)
{
    byte nodestatus[20] = {0};
    //负载
    int node_id = 0;
    int node_load = 0;
    //活动侦测
    int node_enable = 0;
    int node_activity = 0;

    int node_value = 0;
    byte appsend[10] = {0xb4,0x00,0x08};

    //获取当前时间
    time_t auto_time = 0;
    struct tm *ptr;
    time_t lt;
    lt = time(NULL); 
    ptr = localtime(&lt);       
    //pr_debug("minute:%d\n", ptr->tm_min);
    //pr_debug("hour:%d\n", ptr->tm_hour);
    //pr_debug("wday:%d\n", ptr->tm_wday);

    //存储触发条件相关数据
    int typea = 0;
    byte autoscene_h[20] = {0};
    //满足任意一个条件
    int condition = 0;
    //pr_debug("自动场景序列号：%d\n", id);
    while(1)
    {
        if(trimode == 0)
        {
            //pr_debug("满足任意条件——自动场景\n");
            while(1)
            {
                //判断场景信息是否执行完毕
                if(typea == triclen)
                {
                    //pr_debug("触发条件执行完毕\n");
                    break;
                }
                switch(tricond[typea])
                {
                    //指令长度相同，统一处理
                    //人体感应，光感，温度，继电器
                    case 0x01:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("继电器检测\n");
                        //获取当前状态-在线
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            pr_debug("继电器检测-在线\n");
                            //用户设定id
                            node_id = autoscene_h[3] & 0x7f;
                            //用户设定负载状态
                            node_load = (autoscene_h[3] >> 7);

                            node_value = ((nodedev[autoscene_h[2]].nodeinfo[5] >> (node_id - 1)) & 0x01);
                            //设定条件状态为关&当前状态为关
                            if((node_load == 0) && (node_value == 0))
                            {
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].load[0])
                                    {
                                        break;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].load, 6))
                                    {
                                        //未记录时间
                                        if(0 == (autotri[id].autotion[z].load[6] + autotri[id].autotion[z].load[7] + autotri[id].autotion[z].load[8]))
                                        {
                                            break;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].load[6] * 60) * 60) + (autotri[id].autotion[z].load[7] * 60)\
                                                        + autotri[id].autotion[z].load[8]))\
                                                > ((autotri[id].autotion[z].load[4] << 8) + autotri[id].autotion[z].load[5]))
                                        {
                                            condition = 1;
                                            pr_debug("e4_1\n");
                                            break;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                            //状态为是，当前状态为开
                            else if((node_load == 1) && (node_value == 1))
                            {
                                pr_debug("满足任意条件-状态-是\n");
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].load[0])
                                    {
                                        pr_debug("未获取触发条件\n");
                                        break;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].load, 6))
                                    {
                                        pr_debug("开始检查触发条件\n");
                                        //未记录时间
                                        if(0 == (autotri[id].autotion[z].load[6] + autotri[id].autotion[z].load[7] + autotri[id].autotion[z].load[8]))
                                        {
                                            pr_debug("负载触发条件未记录时间\n");
                                            break;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].load[6] * 60) * 60) + (autotri[id].autotion[z].load[7] * 60)\
                                                        + autotri[id].autotion[z].load[8]))\
                                                > ((autotri[id].autotion[z].load[4] << 8) + autotri[id].autotion[z].load[5]))
                                        {
                                            condition = 1;
                                            pr_debug("e4_2\n");
                                            break;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        typea += 6;
                        break;
                    case 0x06:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("温度检测\n");

                        //主从机
                        if(autoscene_h[2] == 1)
                        {
                            if(autoscene_h[3] == 0x00)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] >= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] >= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] <= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] <= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                            }
                        }
                        //节点
                        else if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            //温度未使能
                            if(0 == (nodedev[autoscene_h[2]].nodeinfo[8] & 0x04))
                            {
                                pr_debug("温度未使能\n");
                                typea += 6;
                                break;
                            }
                            if(autoscene_h[3] == 0x00)
                            {
                                if(autoscene_h[4] >= nodedev[autoscene_h[2]].nodeinfo[7])
                                {
                                    condition = 1;
                                    pr_debug("e3_1\n");
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                if(autoscene_h[4] <= nodedev[autoscene_h[2]].nodeinfo[7])
                                {
                                    condition = 1;
                                    pr_debug("e3_2\n");
                                }
                            }
                        }
                        typea += 6;
                        break;
                    case 0x07:
                        memcpy(autoscene_h, tricond + typea, 6);

                        //获取当前状态
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1) || (nodedev[autoscene_h[2]].nodeinfo[2] == 0x53))
                        {
                            pr_debug("条件检查0：人体感应\n");
                            if((nodedev[autoscene_h[2]].nodeinfo[2] > 0) && (nodedev[autoscene_h[2]].nodeinfo[2] < 0x0f))
                            {
                                //是否使能
                                node_enable = nodedev[autoscene_h[2]].nodeinfo[8] & 0x01;
                                //当前状态
                                node_activity = nodedev[autoscene_h[2]].nodeinfo[6] & 0x01;
                            }
                            else if(nodedev[autoscene_h[2]].nodeinfo[2] == 0x53)
                            {
                                //独立式传感器
                                node_enable = 1;
                                node_activity = nodedev[autoscene_h[2]].nodeinfo[5];
                            }
                            else
                            {
                                typea += 6;
                                break;
                            }
                            //活动侦测未使能
                            if(node_enable == 0)
                            {
                                pr_debug("活动侦测未使能\n");
                                typea += 6;
                                break;
                            }
                            //设定为否-当前状态为否
                            else if((autoscene_h[3] == 0x00) && (node_activity == 0))
                            {
                                pr_debug("人体感应-否\n");
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].activity[0])
                                    {
                                        pr_debug("无对应条件id:%d-%d\n", id, z);
                                        break;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].activity, 6))
                                    {
                                        //未出现人体感应条件成立
                                        if(0 == (autotri[id].autotion[z].activity[6] + autotri[id].autotion[z].activity[7] + autotri[id].autotion[z].activity[8]))
                                        {
                                            pr_debug("人体感应-否-未记录时间\n");
                                            break;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].activity[6] *60) * 60) + \
                                                        (autotri[id].autotion[z].activity[7] * 60)\
                                                        + autotri[id].autotion[z].activity[8])) > \
                                                ((autoscene_h[4] << 8) + autoscene_h[5]))
                                        {
                                            condition = 1;
                                            pr_debug("未出现人体感应条件成立\n");
                                            break;
                                        }
                                        else
                                        {
                                            pr_debug("未出现人体感应条件不成立\n");
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        pr_debug("#################32\n");
                                    }
                                }
                            }
                            //状态为是
                            else if((autoscene_h[3] == 0x01) && (node_activity == 1))
                            {
                                pr_debug("人体感应——是\n");
                                //存储时间
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].activity[0])
                                    {
                                        break;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].activity, 6))
                                    {
                                        if(0 == (autotri[id].autotion[z].activity[6] + autotri[id].autotion[z].activity[7] + autotri[id].autotion[z].activity[8]))
                                        {
                                            pr_debug("未记录时间0x07\n");
                                            break;
                                        }
                                        //出现人体感应条件成立
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].activity[6] *60) * 60) + \
                                                        (autotri[id].autotion[z].activity[7] * 60)\
                                                        + autotri[id].autotion[z].activity[8])) > \
                                                ((autoscene_h[4] << 8) + autoscene_h[5]))
                                        {
                                            condition = 1;
                                            pr_debug("出现人体感应条件成立\n");
                                            break;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        typea += 6;
                        break;
                    case 0x08:
                        pr_debug("满足任意条件-光感检查\n");
                        memcpy(autoscene_h, tricond + typea, 6);
                        
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            //活动侦测未使能
                            if(0 == (nodedev[autoscene_h[2]].nodeinfo[8] & 0x02))
                            {
                                pr_debug("光感未使能\n");
                                typea += 6;
                                break;
                            }
                            nodestatus[6] = ((nodedev[autoscene_h[2]].nodeinfo[6] >> 4) & 0x0f);
                            if(autoscene_h[3] == 0x00)
                            {
                                if(autoscene_h[4] >= nodestatus[6])
                                {
                                    condition = 1;
                                    pr_debug("光检测1\n");
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                if(autoscene_h[4] <= nodestatus[6])
                                {
                                    condition = 1;
                                    pr_debug("光检测2\n");
                                }
                            }
                        }
                        typea += 6;
                        break;
                    case 0x11:
                        pr_debug("满足任意条件-定时\n");
                        //pr_debug("minute:%d\n", ptr->tm_min);
                        //pr_debug("hour:%d\n", ptr->tm_hour);
                        //pr_debug("wday:%d\n", ptr->tm_wday);

                        memcpy(autoscene_h, tricond + typea, 6);
                        //pr_debug("autoscene_h[1]:%d\n", autoscene_h[1]);
                        //pr_debug("autoscene_h[3]:%d\n", autoscene_h[3]);
                        //pr_debug("autoscene_h[4]:%d\n", autoscene_h[4]);
                        //0为周日
                        if((ptr->tm_wday == 0) && ((autoscene_h[1] & 0x01) == 0x01))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周日\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 1) && ((autoscene_h[1] & 0x02) == 0x02))
                        {
                            //本日自动场景执行
                            pr_debug("满足任意条件-定时-周1\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 2) && ((autoscene_h[1] & 0x04) == 0x04))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周2\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 3) && ((autoscene_h[1] & 0x08) == 0x08))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周3\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 4) && ((autoscene_h[1] & 0x10) == 0x10))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周4\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 5) && ((autoscene_h[1] & 0x20) == 0x20))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周5\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        else if((ptr->tm_wday == 6) && ((autoscene_h[1] & 0x40) == 0x40))
                        {
                            //本日无自动场景执行
                            pr_debug("满足任意条件-定时-周6\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                        }
                        typea += 6;
                        break;
                    case 0x12:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("湿度检测\n");

                        //主从机
                        if(autoscene_h[2] == 1)
                        {
                            if(autoscene_h[3] == 0x00)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] >= hostinfo.moderate)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] >= slaveident[autoscene_h[1]].moderate)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] <= hostinfo.moderate)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] <= slaveident[autoscene_h[1]].moderate)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                }
                            }
                        }
                        typea += 6;
                        break;
                    case 0x51:
                        memcpy(autoscene_h, tricond + typea, 6);

                        pr_debug("条件检查0:门磁\n");

                        //是否使能
                        //node_enable = nodedev[autoscene_h[2]].nodeinfo[8] & 0x01;
                        //当前状态
                        node_activity = nodedev[autoscene_h[2]].nodeinfo[5];

                        //活动侦测未使能
                        //if(node_enable == 0)
                        //{
                        //pr_debug("活动侦测未使能\n");
                        //typea += 6;
                        //break;
                        //}

                        //设定为否-当前状态为否
                        if((autoscene_h[3] == 0x00) && (node_activity == 0))
                        {
                            pr_debug("人体感应-否\n");
                            int z = 0;
                            for(z = 0; z < 10; z++)
                            {
                                if(0 == autotri[id].autotion[z].magnetic[0])
                                {
                                    pr_debug("无对应条件id:%d-%d\n", id, z);
                                    break;
                                }
                                else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].magnetic, 6))
                                {
                                    //未出现人体感应条件成立
                                    if(0 == (autotri[id].autotion[z].magnetic[6] + autotri[id].autotion[z].magnetic[7] + autotri[id].autotion[z].magnetic[8]))
                                    {
                                        pr_debug("人体感应-否-未记录时间\n");
                                        break;
                                    }
                                    else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                - (((autotri[id].autotion[z].magnetic[6] *60) * 60) + \
                                                    (autotri[id].autotion[z].magnetic[7] * 60)\
                                                    + autotri[id].autotion[z].magnetic[8])) > \
                                            ((autoscene_h[4] << 8) + autoscene_h[5]))
                                    {
                                        condition = 1;
                                        pr_debug("未出现人体感应条件成立\n");
                                        break;
                                    }
                                    else
                                    {
                                        pr_debug("未出现人体感应条件不成立\n");
                                        break;
                                    }
                                }
                                else
                                {
                                    pr_debug("#################32\n");
                                }
                            }
                        }
                        //状态为是
                        else if((autoscene_h[3] == 0x01) && (node_activity == 1))
                        {
                            pr_debug("人体感应——是\n");
                            //存储时间
                            int z = 0;
                            for(z = 0; z < 10; z++)
                            {
                                if(0 == autotri[id].autotion[z].magnetic[0])
                                {
                                    break;
                                }
                                else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].magnetic, 6))
                                {
                                    if(0 == (autotri[id].autotion[z].magnetic[6] + autotri[id].autotion[z].magnetic[7] + autotri[id].autotion[z].magnetic[8]))
                                    {
                                        pr_debug("未记录时间0x07\n");
                                        break;
                                    }
                                    //出现人体感应条件成立
                                    else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                - (((autotri[id].autotion[z].magnetic[6] *60) * 60) + \
                                                    (autotri[id].autotion[z].magnetic[7] * 60)\
                                                    + autotri[id].autotion[z].magnetic[8])) > \
                                            ((autoscene_h[4] << 8) + autoscene_h[5]))
                                    {
                                        condition = 1;
                                        pr_debug("出现人体感应条件成立\n");
                                        break;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        typea += 6;
                        break;
                    //风速
                    case 0x57:
                        pr_debug("满足任意条件-风速检查\n");
                        memcpy(autoscene_h, tricond + typea, 6);
                        if(autoscene_h[3] == 0x00)
                        {
                            if(autoscene_h[4] >= nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("风速1\n");
                            }
                        }
                        else if(autoscene_h[3] == 0x01)
                        {
                            if(autoscene_h[4] <= nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("风速2\n");
                            }
                        }
                        typea += 6;
                        break;
                        //独立传感器
                        //水浸/烟雾/雨雪/燃气
                    case 0x52:
                    case 0x54:
                    case 0x55:
                    case 0x56:
                    case 0x58:
                        pr_debug("满足任意条件-独立式传感器\n");
                        memcpy(autoscene_h, tricond + typea, 6);
                        
                        //限制类型0x50-0x5f
                        if((nodedev[autoscene_h[2]].nodeinfo[2] > 0x50) && (nodedev[autoscene_h[2]].nodeinfo[2] < 0x5f))
                        {
                            if(autoscene_h[3] == nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("独立式传感器1\n");
                            }
                        }
                        typea += 6;
                        break;
                    default:
                        pr_debug("未识别的智能场景触发条件\n");
                        break;
                }
            }
            //pr_debug("condition1:%d\n", condition);

            if(condition == 0)
            {
                //pr_debug("未满足任何触发条件，本条自动场景执行结束\n");
                return;
            }
            //满足一个条件，开始执行自动场景
            if(condition == 1)
            {
                //自动场景执行命令(广播)0x20
                byte scenerun[10] = {0x68,0x04};
                //获取场景id
                scenerun[2] = id;
                UART0_Send(uart_fd, scenerun, 4);

                //执行主从机红外/手动场景
                run_autosceneir(id);

                //执行标志
                ascecontrol[id].status = 1;
                autotri[id].ldentification = 1;
                time(&auto_time);
                autotri[id].lasttime = auto_time;

                appsend[3] = id;
                appsend[4] = ascecontrol[id].enable;
                appsend[5] = autotri[id].ldentification;
                //剩余冷却时间
                appsend[6] = ((ascecontrol[id].interval >> 8) & 0xff);
                appsend[7] = (ascecontrol[id].interval & 0xff);
                UartToApp_Send(appsend, appsend[2]);

                return;
            }
        }
        //满足所有条件
        else if(trimode == 1)
        {
            //pr_debug("满足所有条件执行\n");
            while(1)
            {
                //pr_debug("typea:%d\n", typea);
                //pr_debug("triclen:%d\n", triclen);
                //判断场景信息是否执行完毕
                if(typea == triclen)
                {
                    pr_debug("满足所有-触发条件执行完毕\n");
                    break;
                }
                switch(tricond[typea])
                {
                    //指令长度相同，统一处理
                    //人体感应，光感，温度，继电器
                    case 0x01:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("继电器检测\n");
                        //获取当前状态
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            //用户设定id
                            node_id = autoscene_h[3] & 0x7f;
                            //用户设定负载状态
                            node_load = (autoscene_h[3] >> 7);

                            node_value = ((nodedev[autoscene_h[2]].nodeinfo[5] >> (node_id - 1)) & 0x01);
                            pr_debug("autosce-继电器-node_id:%d\n", node_id);
                            pr_debug("autosce-继电器-node_load:%d\n", node_load);
                            pr_debug("autosce-继电器-node_value:%d\n", node_value);
                            //设定条件状态为关&当前状态为关
                            if((node_load == 0) && (node_value == 0))
                            {
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].load[0])
                                    {
                                        return;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].load, 6))
                                    {
                                        if(0 == (autotri[id].autotion[z].load[6] + autotri[id].autotion[z].load[7] + autotri[id].autotion[z].load[8]))
                                        {
                                            return;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].load[6] * 60) * 60) + (autotri[id].autotion[z].load[7] * 60)\
                                                        + autotri[id].autotion[z].load[8]))\
                                                > ((autotri[id].autotion[z].load[4] << 8) + autotri[id].autotion[z].load[5]))
                                        {
                                            pr_debug("e4_1\n");
                                            break;
                                        }
                                        else
                                        {
                                            return;
                                        }
                                    }
                                }
                            }
                            //状态为是，当前状态为开
                            else if((node_load == 1) && (node_value == 1))
                            {
                                //pr_debug("继电器检测:%s\n", run_autoscene_str);
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].load[0])
                                    {
                                        return;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].load, 6))
                                    {
                                        pr_debug("继电器\n");
                                        if(0 == (autotri[id].autotion[z].load[6] + autotri[id].autotion[z].load[7] + autotri[id].autotion[z].load[8]))
                                        {
                                            pr_debug("继电器未记录时间\n");
                                            return;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].load[6] * 60) * 60) + (autotri[id].autotion[z].load[7] * 60)\
                                                        + autotri[id].autotion[z].load[8]))\
                                                > ((autotri[id].autotion[z].load[4] << 8) + autotri[id].autotion[z].load[5]))
                                        {
                                            condition = 1;
                                            pr_debug("e4_2\n");
                                            break;
                                        }
                                        else
                                        {
                                            pr_debug("继电器其他错误\n");
                                            return;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x06:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("温度检测\n");

                        //主从机
                        if(autoscene_h[2] == 1)
                        {
                            if(autoscene_h[3] == 0x00)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] >= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] >= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] <= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] <= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                        //节点
                        else if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            //温度未使能
                            if(0 == (nodedev[autoscene_h[2]].nodeinfo[8] & 0x04))
                            {
                                pr_debug("温度未使能\n");
                                typea += 6;
                                return;
                            }
                            if(autoscene_h[3] == 0x00)
                            {
                                if(autoscene_h[4] >= nodedev[autoscene_h[2]].nodeinfo[7])
                                {
                                    condition = 1;
                                    pr_debug("e3_1\n");
                                }
                                else
                                {
                                    return;
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                if(autoscene_h[4] <= nodedev[autoscene_h[2]].nodeinfo[7])
                                {
                                    condition = 1;
                                    pr_debug("e3_2\n");
                                }
                                else
                                {
                                    return;
                                }
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x07:
                        memcpy(autoscene_h, tricond + typea, 6);
                        
                        //获取当前状态
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1) || (nodedev[autoscene_h[2]].nodeinfo[2] == 0x53))
                        {
                            pr_debug("条件检查：人体感应\n");
                            if((nodedev[autoscene_h[2]].nodeinfo[2] > 0) && (nodedev[autoscene_h[2]].nodeinfo[2] < 0x0f))
                            {
                                //是否使能
                                node_enable = nodedev[autoscene_h[2]].nodeinfo[8] & 0x01;
                                //当前状态
                                node_activity = nodedev[autoscene_h[2]].nodeinfo[6] & 0x01;
                            }
                            else if(nodedev[autoscene_h[2]].nodeinfo[2] == 0x53)
                            {
                                //独立式传感器
                                node_enable = 1;
                                node_activity = nodedev[autoscene_h[2]].nodeinfo[5];
                            }
                            else
                            {
                                typea += 6;
                                return;
                            }
                            //活动侦测未使能
                            if(node_enable == 0)
                            {
                                typea += 6;
                                return;
                            }
                            //设定为否-当前状态为否
                            else if((autoscene_h[3] == 0x00) && (node_activity == 0))
                            {
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].activity[0])
                                    {
                                        return;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].activity, 6))
                                    {
                                        //未出现人体感应条件成立
                                        if(0 == (autotri[id].autotion[z].activity[6] + autotri[id].autotion[z].activity[7] + autotri[id].autotion[z].activity[8]))
                                        {
                                            return;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].activity[6] *60) * 60) + \
                                                        (autotri[id].autotion[z].activity[7] * 60)\
                                                        + autotri[id].autotion[z].activity[8])) > \
                                                ((autoscene_h[4] << 8) + autoscene_h[5]))
                                        {
                                            condition = 1;
                                            pr_debug("未出现人体感应条件成立\n");
                                            break;
                                        }
                                        else
                                        {
                                            return;
                                        }
                                    }
                                }
                            }
                            //状态为是
                            else if((autoscene_h[3] == 0x01) && (node_activity == 1))
                            {
                                pr_debug("人体感应——是\n");
                                //存储时间
                                int z = 0;
                                for(z = 0; z < 10; z++)
                                {
                                    if(0 == autotri[id].autotion[z].activity[0])
                                    {
                                        return;
                                    }
                                    else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].activity, 6))
                                    {
                                        //出现人体感应条件成立
                                        if(0 == (autotri[id].autotion[z].activity[6] + autotri[id].autotion[z].activity[7] + autotri[id].autotion[z].activity[8]))
                                        {
                                            return;
                                        }
                                        else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                    - (((autotri[id].autotion[z].activity[6] *60) * 60) + \
                                                        (autotri[id].autotion[z].activity[7] * 60)\
                                                        + autotri[id].autotion[z].activity[8])) > \
                                                ((autoscene_h[4] << 8) + autoscene_h[5]))
                                        {
                                            condition = 1;
                                            pr_debug("出现人体感应条件成立\n");
                                            break;
                                        }
                                        else
                                        {
                                            return;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x08:
                        pr_debug("满足所有条件-光感\n");
                        memcpy(autoscene_h, tricond + typea, 6);
                        
                        if((nodedev[autoscene_h[2]].nodeinfo[11] == 1))
                        {
                            //活动侦测未使能
                            if(0 == (nodedev[autoscene_h[2]].nodeinfo[8] & 0x02))
                            {
                                pr_debug("光感未使能\n");
                                typea += 6;
                                return;
                            }
                            nodestatus[6] = ((nodedev[autoscene_h[2]].nodeinfo[6] >> 4) & 0x0f);
                            if(autoscene_h[3] == 0x00)
                            {
                                if(autoscene_h[4] >= nodestatus[6])
                                {
                                    condition = 1;
                                    pr_debug("光检测1\n");
                                }
                                else
                                {
                                    return;
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                if(autoscene_h[4] <= nodestatus[6])
                                {
                                    condition = 1;
                                    pr_debug("光检测2\n");
                                }
                                else
                                {
                                    return;
                                }
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x11:
                        memcpy(autoscene_h, tricond + typea, 6);
                        //0为周日
                        if((ptr->tm_wday == 0) && ((autoscene_h[1] & 0x01) == 0x01))
                        {
                            //本日无自动场景执行
                            pr_debug("周无自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 1) && ((autoscene_h[1] & 0x02) == 0x02))
                        {
                            //本日自动场景执行
                            pr_debug("周一自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 2) && ((autoscene_h[1] & 0x04) == 0x04))
                        {
                            //本日无自动场景执行
                            pr_debug("周二自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 3) && ((autoscene_h[1] & 0x08) == 0x08))
                        {
                            //本日无自动场景执行
                            pr_debug("周三自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 4) && ((autoscene_h[1] & 0x10) == 0x10))
                        {
                            //本日无自动场景执行
                            pr_debug("周四自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 5) && ((autoscene_h[1] & 0x20) == 0x20))
                        {
                            //本日无自动场景执行
                            pr_debug("周五自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if((ptr->tm_wday == 6) && ((autoscene_h[1] & 0x40) == 0x40))
                        {
                            //本日无自动场景执行
                            pr_debug("周六自动场景执行\n");
                            if((autoscene_h[3] == ptr->tm_hour) && (autoscene_h[4] == ptr->tm_min))
                            {
                                condition = 1;
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x12:
                        memcpy(autoscene_h, tricond + typea, 6);
                        pr_debug("湿度检测\n");

                        //主从机
                        if(autoscene_h[2] == 1)
                        {
                            if(autoscene_h[3] == 0x00)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] >= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] >= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                            else if(autoscene_h[3] == 0x01)
                            {
                                //主机
                                if(autoscene_h[1] == 1)
                                {
                                    if(autoscene_h[4] <= hostinfo.temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                                //从机
                                else
                                {
                                    if(autoscene_h[4] <= slaveident[autoscene_h[1]].temperature)
                                    {
                                        condition = 1;
                                        pr_debug("e3_1\n");
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x51:
                        memcpy(autoscene_h, tricond + typea, 6);

                        pr_debug("条件检查:门磁\n");

                        //是否使能
                        //node_enable = nodedev[autoscene_h[2]].nodeinfo[8] & 0x01;
                        //当前状态
                        node_activity = nodedev[autoscene_h[2]].nodeinfo[5];

                        //活动侦测未使能
                        //if(node_enable == 0)
                        //{
                        //typea += 6;
                        //return;
                        //}
                        //设定为否-当前状态为否
                        if((autoscene_h[3] == 0x00) && (node_activity == 0))
                        {
                            int z = 0;
                            for(z = 0; z < 10; z++)
                            {
                                if(0 == autotri[id].autotion[z].magnetic[0])
                                {
                                    return;
                                }
                                else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].magnetic, 6))
                                {
                                    //未出现人体感应条件成立
                                    if(0 == (autotri[id].autotion[z].magnetic[6] + autotri[id].autotion[z].magnetic[7] + autotri[id].autotion[z].magnetic[8]))
                                    {
                                        return;
                                    }
                                    else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                - (((autotri[id].autotion[z].magnetic[6] *60) * 60) + \
                                                    (autotri[id].autotion[z].magnetic[7] * 60)\
                                                    + autotri[id].autotion[z].magnetic[8])) > \
                                            ((autoscene_h[4] << 8) + autoscene_h[5]))
                                    {
                                        condition = 1;
                                        pr_debug("未出现人体感应条件成立\n");
                                        break;
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                        //状态为是
                        else if((autoscene_h[3] == 0x01) && (node_activity == 1))
                        {
                            pr_debug("人体感应——是\n");
                            //存储时间
                            int z = 0;
                            for(z = 0; z < 10; z++)
                            {
                                if(0 == autotri[id].autotion[z].magnetic[0])
                                {
                                    return;
                                }
                                else if(0 == memcmp(autoscene_h, autotri[id].autotion[z].magnetic, 6))
                                {
                                    //出现人体感应条件成立
                                    if(0 == (autotri[id].autotion[z].magnetic[6] + autotri[id].autotion[z].magnetic[7] + autotri[id].autotion[z].magnetic[8]))
                                    {
                                        return;
                                    }
                                    else if(((((ptr->tm_hour * 60) * 60) + (ptr->tm_min * 60) + ptr->tm_sec)\
                                                - (((autotri[id].autotion[z].magnetic[6] *60) * 60) + \
                                                    (autotri[id].autotion[z].magnetic[7] * 60)\
                                                    + autotri[id].autotion[z].magnetic[8])) > \
                                            ((autoscene_h[4] << 8) + autoscene_h[5]))
                                    {
                                        condition = 1;
                                        pr_debug("出现人体感应条件成立\n");
                                        break;
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    //风速
                    case 0x57:
                        pr_debug("满足所有条件-风速\n");
                        memcpy(autoscene_h, tricond + typea, 6);
                        if(autoscene_h[3] == 0x00)
                        {
                            if(autoscene_h[4] >= nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("风速1\n");
                            }
                            else
                            {
                                return;
                            }
                        }
                        else if(autoscene_h[3] == 0x01)
                        {
                            if(autoscene_h[4] <= nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("风速2\n");
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    case 0x52:
                    case 0x54:
                    case 0x55:
                    case 0x56:
                    case 0x58:
                        pr_debug("满足任意条件-独立式传感器\n");
                        memcpy(autoscene_h, tricond + typea, 6);

                        //限制类型0x50-0x5f
                        if((nodedev[autoscene_h[2]].nodeinfo[2] > 0x50) && (nodedev[autoscene_h[2]].nodeinfo[2] < 0x5f))
                        {
                            if(autoscene_h[3] == nodedev[autoscene_h[2]].nodeinfo[5])
                            {
                                condition = 1;
                                pr_debug("独立式传感器1\n");
                            }
                            else
                            {
                                return;
                            }
                        }
                        else
                        {
                            return;
                        }
                        typea += 6;
                        break;
                    default:
                        pr_debug("未识别的智能场景触发条件\n");
                        return;
                }
            }
            //自动场景执行命令(广播)0x20
            byte scenerun[10] = {0x68,0x04};
            //获取场景id
            scenerun[2] = id;
            UART0_Send(uart_fd, scenerun, 4);
            
            //执行主从机红外/手动场景
            run_autosceneir(id);

            //执行标志
            ascecontrol[id].status = 1;
            autotri[id].ldentification = 1;
            time(&auto_time);
            autotri[id].lasttime = auto_time;
            
            appsend[3] = id;
            appsend[4] = ascecontrol[id].enable;
            appsend[5] = autotri[id].ldentification;
            //剩余冷却时间
            appsend[6] = ((ascecontrol[id].interval >> 8) & 0xff);
            appsend[7] = (ascecontrol[id].interval & 0xff);
            UartToApp_Send(appsend, appsend[2]);
            
            return;
        }
    }
}

//读取触发条件
int readtriggercond()
{
    int i = 1;
    //触发条件偏移
    int type = 0;
    int z = 0;
    while(i < 11)
    {
        //触发条件长度为0
        if(ascecontrol[i].tricondlen == 0)
        {
            i++;
            continue;
        }
        //执行条件长度为0
        else if(ascecontrol[i].tasklen == 0)
        {
            i++;
            continue;
        }
        //只执行一次的场景-且已经执行
        else if((ascecontrol[i].interval == 0) && (ascecontrol[i].status == 1))
        {
            i++;
            continue;
        }
        //存储触发条件
        type = 0;
        while(1)
        {
            //判断场景信息是否执行完毕
            if(type >= ascecontrol[i].tricondlen)
            {
                pr_debug("触发条件存储完毕\n");
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
                            pr_debug("autotri[%d].autotion[%d].magnetic:%0x\n", i, z, autotri[i].autotion[z].magnetic[2]);
                            break;
                        }
                    }
                    type += 6;
                    break;
                default:
                    pr_debug("获取触发条件未识别类型:%d-%0x\n", type, ascecontrol[i].tricond[type]);
                    return type;
            }
        }
        i++;
    }
    pr_debug("所有触发条件获取完毕\n");
    return i;
}

//传感器信息存储
/*
 * type:1,综合数据
 *      2,温度
 *      3,震动
 *      4,光感
//返回值:0成功，-1失败
int seninfostorage(int type, byte *sensor)
{
    int i = 0;
    for(i = 0; i < 256; i++)
    {
        if(type == 1)
        {
            if((0 == memcmp(sensor, sensdata[i].synthesize, 2)) || (sensdata[i].synthesize[0] == 0))
            {
                memcpy(sensdata[i].synthesize, sensor, 3);
                return 0;
            }
        }
        else if(type == 2)
        {
            if((0 == memcmp(sensor, sensdata[i].temperature, 2)) || (sensdata[i].temperature[0] == 0))
            {
                memcpy(sensdata[i].temperature, sensor, 3);
                return 0;
            }
        }
        else if(type == 3)
        {
            if((0 == memcmp(sensor, sensdata[i].shake, 2)) || (sensdata[i].shake[0] == 0))
            {
                memcpy(sensdata[i].shake, sensor, 3);
                return 0;
            }
        }
        else if(type == 4)
        {
            if((0 == memcmp(sensor, sensdata[i].light, 2)) || (sensdata[i].light[0] == 0))
            {
                memcpy(sensdata[i].light, sensor, 3);
                return 0;
            }
        }
        else
        {
            break;
        }
    }
    return -1;
}

//传感器信息查询
 * type:1,综合数据
 *      2,温度
 *      3,震动
 *      4,光感
//返回值:0成功，-1失败
int seninfoinquire(int type, byte *sensor, byte *psensor)
{
    int i = 0;
    for(i = 0; i < 256; i++)
    {
        if(type == 1)
        {
            if(0 == memcmp(sensor, sensdata[i].synthesize, 2))
            {
                memcpy(psensor, sensdata[i].synthesize, 3);
                return 0;
            }
            else if(sensdata[i].synthesize[0] == 0)
            {
                return -1;
            }
        }
        else if(type == 2)
        {
            if(0 == memcmp(sensor, sensdata[i].temperature, 2))
            {
                memcpy(psensor, sensdata[i].temperature, 3);
                return 0;
            }
            else if(sensdata[i].temperature[0] == 0)
            {
                return -1;
            }
        }
        else if(type == 3)
        {
            if(0 == memcmp(sensor, sensdata[i].shake, 2))
            {
                memcpy(psensor, sensdata[i].shake, 3);
                return 0;
            }
            else if(sensdata[i].shake[0] == 0)
            {
                return -1;
            }
        }
        else if(type == 4)
        {
            if(0 == memcmp(sensor, sensdata[i].light, 2))
            {
                memcpy(psensor, sensdata[i].light, 3);
                return 0;
            }
            else if(sensdata[i].light[0] == 0)
            {
                return -1;
            }
        }
        else
        {
            break;
        }
    }
    return -1;
}

//检测网络连接
static int networkcheck()
{
    struct ethtool_value
    {
        __uint32_t cmd;
        __uint32_t data;
    };
    
    struct ethtool_value edata;
    int fd = -1, err = 0;
    struct ifreq ifr;
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "eth0");
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("Cannot get control socket");
        return -1;
    }

    edata.cmd = 0x0000000a;
    ifr.ifr_data = (caddr_t)&edata;
    err = ioctl(fd, 0x8946, &ifr);
    //printf("edata.data:%lu\n", edata.data);
    if (err == 0)
    {
        //fprintf(stdout, "Link detected: %s\n", edata.data ? "yes":"no");
        close(fd);
        return edata.data;
    } 
    else if (errno != EOPNOTSUPP)
    {
        close(fd);
        perror("Cannot get link status");
        return -1;
    }
}
*/

int compareInt(int a , int b)
{
    return a >= b ? 1 : 0;
}

//限制条件检索
int restrictcheck(byte *condition)
{
    //获取当前时间
    time_t auto_time = 0;
    struct tm *ptr;
    time_t lt;
    lt = time(NULL);
    ptr = localtime(&lt);

    //pr_debug("minute:%d\n", ptr->tm_min);
    //pr_debug("hour:%d\n", ptr->tm_hour);
    //pr_debug("wday:%d\n", ptr->tm_wday)
    int now_time = ptr->tm_hour * 60 + ptr->tm_min;
    int start_time = condition[1] * 60 + condition[2];
    int end_time = condition[3] * 60 + condition[4];

    if(end_time >= start_time)
    {
        //不跨天
        if(((condition[0] >> ptr->tm_wday) & 0x01) == 1)
        {
            //当天受限
            return compareInt(compareInt(now_time,start_time) + compareInt(end_time,now_time),2) ;
        }
        else
        {
            return 0 ;
        }
    }

    int pro_day = ptr->tm_wday == 0 ? 6 : (ptr->tm_wday -1 );//昨天

    if(((condition[0] >> pro_day) & 0x01) == 1 || ((condition[0] >> ptr->tm_wday) & 0x01) == 1)//昨天受限或者当天受限
    {
        int next_day = ptr->tm_wday == 6 ? 0 : (ptr->tm_wday +1 );//后天
        
        if(((condition[0] >> pro_day) & 0x01) == 0 )//昨天不受限，只判断当天到0点受限时间
        {
            return compareInt(now_time,start_time);
        }
        if(((condition[0] >> next_day) & 0x01) == 0 )//昨天受限
        {
            if(((condition[0] >> ptr->tm_wday) & 0x01) == 1)//明天受限，判断整天
            {
                return compareInt(now_time,start_time) + compareInt(end_time,now_time) ;
            }
            else//明天不受限，只判断0点到开始时间
            {
                return compareInt(end_time,now_time);
            }
        }
        if(((condition[0] >> pro_day) & 0x01) == 1 && ((condition[0] >> next_day) & 0x01) == 1 )//前后都受限
        {
            if(((condition[0] >> ptr->tm_wday) & 0x01) == 0)//但是今天不受限
            {
                return compareInt(end_time,now_time);
            }
            else//今天受限
            {
                return compareInt(now_time,start_time) + compareInt(end_time,now_time);
            }
        }

    }
    return 0;
}

//自动场景
void *timer_run(void *a)
{
    int autotime = 0;
    int current = 0;
    int type = 0;
    byte timer_run_h[20] = {0};
    byte timer_run_swap[20] = {0};
    byte timer_run_s[20] = {0};

    byte timer_enable[10] = {0};

    //自动场景触发标识
    byte autotriident_id[5] = {0};
    int auto_id = 0;
    
    //struct autotrident autrid[15];
    //memset(ascecontrol, 0, sizeof(ascecontrol));

    //读取自动场景信息
    //readautosce();
    
    //读取触发条件
    //readtriggercond();

    while(1)
    {
        /*
        if(ascecontrol[0].id == 0)
        {
            pr_debug("No Scene...\n");
            sleep(10);
            continue;
        }
        */
        //pr_debug("准备执行自动场景\n");
        usleep(500000);
        int num = 1;
        while(num < 11)
        {
            type = 0;
            //所有场景执行完毕,循环执行
            if(ascecontrol[num].id == 0)
            {
                num++;
                continue;
            }
            //未使能
            else if(ascecontrol[num].enable == 0)
            {
                //pr_debug("自动场景%d未使能\n", ascecontrol[num].id);
                num++;
                continue;
            }
            //只执行一次的场景-且已经执行
            else if((ascecontrol[num].interval == 0) && (autotri[num].ldentification == 1))
            {
                //pr_debug("自动场景%d已执行\n", ascecontrol[num].id);
                num++;
                continue;
            }
            //触发条件长度为0
            else if(ascecontrol[num].tricondlen == 0)
            {
                pr_debug("自动场景%d触发条件为0\n", ascecontrol[num].id);
                num++;
                continue;
            }
            //执行任务长度为0
            else if(ascecontrol[num].tasklen == 0)
            {
                pr_debug("自动场景%d执行长度为0\n", ascecontrol[num].id);
                num++;
                continue;
            }
            //限制日期检查
            else if(0 != restrictcheck(ascecontrol[num].limitations))
            {
                pr_debug("自动场景%d限制时间内\n", ascecontrol[num].id);
                num++;
                continue;
            }
            //获取场景id
            auto_id = ascecontrol[num].id;
            //检查时间
            if(autotri[auto_id].ldentification == 1)
            {
                time_t nowtime = 0;
                time(&nowtime);
                //pr_debug("ascecontrol[%d].interval:%0x\n", num, ascecontrol[num].interval * 60);
                //pr_debug("nowtime:%0x autotri[%d].lasttime:%0x\n", nowtime, auto_id, autotri[auto_id].lasttime);
                if((nowtime - autotri[auto_id].lasttime) >= (ascecontrol[num].interval * 60))
                {
                    pr_debug("%d冷却完成:%ld\n", auto_id, nowtime - autotri[auto_id].lasttime);
                    autotri[auto_id].ldentification = 0;
                }
                else
                {
                    pr_debug("%d冷却中:%ld\n", auto_id, nowtime - autotri[auto_id].lasttime);
                    num++;
                    continue;
                }
            }
            run_autoscene(ascecontrol[num].id, ascecontrol[num].tricondlen, ascecontrol[num].trimode, ascecontrol[num].tricond);
            usleep(300000);
            num++;
        }
    }
}
