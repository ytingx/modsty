#include "securitymonitor.h"
#include "struct.h"

extern int client_fd;
extern int uart_fd;

//配置
void securityconfig(byte *buf)
{
    byte security[2048] = {0};
    byte datastr[4096] = {0};
    byte appsend[12] = {0xe0,0x00,0x05};

    int len = ((buf[1] << 8) + buf[2]);
    memcpy(security, buf + 3, len - 3);

    HexToStr(datastr, security, len - 3);

    replace(SECMONITOR, datastr, 2);

    readsecurityconfig(0, security);

    appsend[3] = buf[3];
    UartToApp_Send(appsend, appsend[2]);
}

//写入安防状态
int writesecstatus(int id, byte *data, int sub)
{
    byte secid[12] = {0};
    byte sechex[12] = {0};
    byte secstr[2048] = {0};

    sechex[0] = id;
    HexToStr(secid, sechex, 1);

    pr_debug("secid:%s\n", secid);
    if(0 == filefind(secid, secstr, SECMONITOR, 2))
    {
        secstr[sub] = data[0];
        secstr[strlen(secstr) - 1] = '\0';
        replace(SECMONITOR, secstr, 2);
        return 0;
    }
    return -1;
}

//执行安防动作
void performsecactions(byte *buf)
{
    byte secstr[2048] = {0};
    byte appsend[24] = {0xe1,0x00,0x06};
    byte uartsend[10] = {0x63,0x0e,0xff,0x2b,0xff,0x00};

    if(buf[3] == 0x00)
    {
        if(secmonitor[1].secmonitor == 1)
        {
            secmonitor[1].secmonitor = 0;
            secmonitor[1].status = 0;

            appsend[3] = 0x00;
            appsend[4] = 1;
            if(0 > writesecstatus(1, "0", 5))
            {
                appsend[5] = 1;
            }
        }
        if(secmonitor[2].secmonitor == 1)
        {
            secmonitor[2].secmonitor = 0;
            secmonitor[2].status = 0;
            
            appsend[3] = 0x00;
            appsend[4] = 0x01;
            if(0 > writesecstatus(2, "0", 5))
            {
                appsend[5] = 1;
            }
        }
    }
    else if(buf[3] == 0x01)
    {
        secmonitor[2].secmonitor = 0;
        secmonitor[2].status = 0;

        secmontri[1].seccountdown = 0;
        secmontri[2].seccountdown = 0;
        memset(&timers[3], 0, sizeof(timers[3]));
        memset(&timers[4], 0, sizeof(timers[4]));

        secmonitor[1].secmonitor = 1;

        appsend[3] = 0x01;
        appsend[4] = 0x01;

        if(0 > writesecstatus(buf[3], "1", 5))
        {
            appsend[5] = 1;
        }
        writesecstatus(2, "0", 5);
    }
    else if(buf[3] == 0x02)
    {
        secmonitor[1].secmonitor = 0;
        secmonitor[1].status = 0;
        
        secmontri[1].seccountdown = 0;
        secmontri[2].seccountdown = 0;
        memset(&timers[3], 0, sizeof(timers[3]));
        memset(&timers[4], 0, sizeof(timers[4]));

        secmonitor[2].secmonitor = 1;

        appsend[3] = 0x02;
        appsend[4] = 0x02;
        if(0 > writesecstatus(buf[3], "1", 5))
        {
            appsend[5] = 1;
        }
        writesecstatus(1, "0", 5);
    }
    else if(buf[3] == 0x03)
    {
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("01", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[1].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[1].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
        if(0 == filefind("02", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[2].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[2].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x04)
    {
        //震动报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("03", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[3].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[3].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x05)
    {
        //火灾报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("04", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[4].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[4].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x06)
    {
        //挟持报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("05", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[5].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[5].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x07)
    {
        //漏水报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("06", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[6].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[6].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x08)
    {
        //燃气报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("07", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[7].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[7].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0x09)
    {
        //一氧化碳报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x01;
        if(0 == filefind("08", secstr, SECMONITOR, 2))
        {
            if(buf[4] == 0)
            {
                secstr[3] = '0';
                secmonitor[8].enable = 0;
            }
            else
            {
                secstr[3] = '1';
                secmonitor[8].enable = 1;
            }
            secstr[strlen(secstr) - 1] = '\0';
            replace(SECMONITOR, secstr, 2);
            appsend[5] = 0x00;
        }
    }
    else if(buf[3] == 0xff)
    {
        //停止声光报警
        appsend[3] = buf[3];
        appsend[4] = buf[4];
        appsend[5] = 0x00;
        UART0_Send(uart_fd, uartsend, uartsend[1]);
    }
    UartToApp_Send(appsend, appsend[2]);
}

//读取安防执行条件
void readsecconditions(int id)
{
    int z = 0;
    int type = 0;

    //启动布防条件
    while(type < secmonitor[id].secstartlen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            break;
        }
        switch(secmonitor[id].secstartcond[type])
        {
            case 0x07:
                pr_debug("07\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secstartlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secstartcond[z].activity, secmonitor[id].secstartcond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secstartcond[z].activity[0])
                    {
                        memcpy(secmontri[id].secstartcond[z].activity, secmonitor[id].secstartcond + type, 6);
                        pr_debug("secmontri[%d].secstartcond[%d].activity:%0x\n", id, z, secmontri[id].secstartcond[z].activity[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x11:
                pr_debug("11\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secstartlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secstartcond[z].timer, secmonitor[id].secstartcond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secstartcond[z].timer[0])
                    {
                        memcpy(secmontri[id].secstartcond[z].timer, secmonitor[id].secstartcond + type, 6);
                        pr_debug("secmontri[%d].secstartcond[%d].timer:%0x\n", id, z, secmontri[id].secstartcond[z].timer[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x41:
                pr_debug("41\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secstartlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secstartcond[z].doorlock, secmonitor[id].secstartcond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secstartcond[z].doorlock[0])
                    {
                        memcpy(secmontri[id].secstartcond[z].doorlock, secmonitor[id].secstartcond + type, 6);
                        pr_debug("secmontri[%d].secstartcond[%d].doorlock:%0x\n", id, z, secmontri[id].secstartcond[z].doorlock[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x51:
                pr_debug("51\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secstartlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secstartcond[z].magnetic, secmonitor[id].secstartcond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secstartcond[z].magnetic[0])
                    {
                        memcpy(secmontri[id].secstartcond[z].magnetic, secmonitor[id].secstartcond + type, 6);
                        pr_debug("secmontri[%d].secstartcond[%d].magnetic:%0x\n", id, z, secmontri[id].secstartcond[z].magnetic[2]);
                        break;
                    }
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].secstartcond[type]);
                type = -1;
                break;
        }
    }

    //智能布防条件
    type = 0;
    while(type < secmonitor[id].tricondlen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            break;
        }
        switch(secmonitor[id].tricond[type])
        {
            case 0x07:
                pr_debug("07\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secmontion[z].activity, secmonitor[id].tricond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secmontion[z].activity[0])
                    {
                        memcpy(secmontri[id].secmontion[z].activity, secmonitor[id].tricond + type, 6);
                        pr_debug("secmontri[%d].secmontion[%d].activity:%0x\n", id, z, secmontri[id].secmontion[z].activity[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x11:
                pr_debug("11\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secmontion[z].timer, secmonitor[id].tricond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secmontion[z].timer[0])
                    {
                        memcpy(secmontri[id].secmontion[z].timer, secmonitor[id].tricond + type, 6);
                        pr_debug("secmontri[%d].secmontion[%d].timer:%0x\n", id, z, secmontri[id].secmontion[z].timer[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x41:
                pr_debug("41\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secmontion[z].doorlock, secmonitor[id].tricond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secmontion[z].doorlock[0])
                    {
                        memcpy(secmontri[id].secmontion[z].doorlock, secmonitor[id].tricond + type, 6);
                        pr_debug("secmontri[%d].secmontion[%d].doorlock:%0x\n", id, z, secmontri[id].secmontion[z].doorlock[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x51:
                pr_debug("51\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].tricondlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secmontion[z].magnetic, secmonitor[id].tricond + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secmontion[z].magnetic[0])
                    {
                        memcpy(secmontri[id].secmontion[z].magnetic, secmonitor[id].tricond + type, 6);
                        pr_debug("secmontri[%d].secmontion[%d].magnetic:%0x\n", id, z, secmontri[id].secmontion[z].magnetic[2]);
                        break;
                    }
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].tricond[type]);
                type = -1;
                break;
        }
    }
    //智能撤防条件
    type = 0;
    while(type < secmonitor[id].disarmlen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            break;
        }
        switch(secmonitor[id].disarmtask[type])
        {
            case 0x07:
                pr_debug("07\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].disarmlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secdisarm[z].activity, secmonitor[id].disarmtask + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secdisarm[z].activity[0])
                    {
                        memcpy(secmontri[id].secdisarm[z].activity, secmonitor[id].disarmtask + type, 6);
                        pr_debug("secmontri[%d].secdisarm[%d].activity:%0x\n", id, z, secmontri[id].secdisarm[z].activity[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x11:
                pr_debug("11\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].disarmlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secdisarm[z].timer, secmonitor[id].disarmtask + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secdisarm[z].timer[0])
                    {
                        memcpy(secmontri[id].secdisarm[z].timer, secmonitor[id].disarmtask + type, 6);
                        pr_debug("secmontri[%d].secdisarm[%d].timer:%0x\n", id, z, secmontri[id].secdisarm[z].timer[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x41:
                pr_debug("41\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].disarmlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secdisarm[z].doorlock, secmonitor[id].disarmtask + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secdisarm[z].doorlock[0])
                    {
                        memcpy(secmontri[id].secdisarm[z].doorlock, secmonitor[id].disarmtask + type, 6);
                        pr_debug("secmontri[%d].secdisarm[%d].doorlock:%0x\n", id, z, secmontri[id].secdisarm[z].doorlock[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x51:
                pr_debug("51\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].disarmlen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secdisarm[z].magnetic, secmonitor[id].disarmtask + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secdisarm[z].magnetic[0])
                    {
                        memcpy(secmontri[id].secdisarm[z].magnetic, secmonitor[id].disarmtask + type, 6);
                        pr_debug("secmontri[%d].secdisarm[%d].magnetic:%0x\n", id, z, secmontri[id].secdisarm[z].magnetic[2]);
                        break;
                    }
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].disarmtask[type]);
                type = -1;
                break;
        }
    }
    //报警条件
    type = 0;
    while(type < secmonitor[id].secenfolen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            break;
        }
        switch(secmonitor[id].secenforce[type])
        {
            case 0x06:
                pr_debug("06\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].temperature, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].temperature[0])
                    {
                        memcpy(secmontri[id].secenfo[z].temperature, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].temperature:%0x\n", id, z, secmontri[id].secenfo[z].temperature[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x07:
                pr_debug("07\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].activity, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].activity[0])
                    {
                        memcpy(secmontri[id].secenfo[z].activity, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].activity:%0x\n", id, z, secmontri[id].secenfo[z].activity[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x41:
                pr_debug("11\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].doorlock, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].doorlock[0])
                    {
                        memcpy(secmontri[id].secenfo[z].doorlock, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].doorlock:%0x\n", id, z, secmontri[id].secenfo[z].doorlock[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x51:
                pr_debug("51\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].magnetic, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].magnetic[0])
                    {
                        memcpy(secmontri[id].secenfo[z].magnetic, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].magnetic:%0x\n", id, z, secmontri[id].secenfo[z].magnetic[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x52:
                pr_debug("52\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].flooding, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].flooding[0])
                    {
                        memcpy(secmontri[id].secenfo[z].flooding, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].flooding:%0x\n", id, z, secmontri[id].secenfo[z].flooding[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x54:
                pr_debug("54\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].smoke, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].smoke[0])
                    {
                        memcpy(secmontri[id].secenfo[z].smoke, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].smoke:%0x\n", id, z, secmontri[id].secenfo[z].smoke[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x56:
                pr_debug("56\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].gas, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].gas[0])
                    {
                        memcpy(secmontri[id].secenfo[z].gas, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].gas:%0x\n", id, z, secmontri[id].secenfo[z].gas[2]);
                        break;
                    }
                }
                type += 6;
                break;
            case 0x58:
                pr_debug("58\n");
                for(z = 0; z < 250; z++)
                {
                    if(type >= secmonitor[id].secenfolen)
                    {
                        break;
                    }
                    else if(0 == memcmp(secmontri[id].secenfo[z].carbonmxde, secmonitor[id].secenforce + type, 6))
                    {
                        break;
                    }
                    else if(0 == secmontri[id].secenfo[z].carbonmxde[0])
                    {
                        memcpy(secmontri[id].secenfo[z].carbonmxde, secmonitor[id].secenforce + type, 6);
                        pr_debug("secmontri[%d].secenfo[%d].carbonmxde:%0x\n", id, z, secmontri[id].secenfo[z].carbonmxde[2]);
                        break;
                    }
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].secenforce[type]);
                type = -1;
                break;
        }
    }
}

//配置信息读入内存
void readsectriggercond(byte *secconfig)
{
    int id = 0;
    int offset = 0;

    id = secconfig[0];
    //重置
    memset(&secmonitor[id], 0, sizeof(secmonitor[id]));
    memset(&secmontri[id], 0, sizeof(secmontri[id]));

    pr_debug("secmonitor[id]:%d\n", sizeof(secmonitor[id]));
    pr_debug("secmonitor:%d\n", sizeof(secmonitor));

    pr_debug("secmontri[id]:%d\n", sizeof(secmontri[id]));
    pr_debug("secmontri:%d\n", sizeof(secmontri));

    //id
    secmonitor[id].id = id;
    offset += 1;
    //使能
    secmonitor[id].enable = secconfig[offset];
    offset += 1;
    //当前智能布防状态
    secmonitor[id].secmonitor = secconfig[offset];
    offset += 1;
    //当前布防执行状态
    secmonitor[id].status = secconfig[offset];
    offset += 1;
    //启动布防时间段
    memcpy(secmonitor[id].armingtime, secconfig + offset, 5);
    offset += 15;//10个字节预留
    //布防启动条件长度
    secmonitor[id].secstartlen = ((secconfig[offset] << 8) + secconfig[offset + 1]);
    offset += 2;
    //布防启动条件
    memcpy(secmonitor[id].secstartcond, secconfig + offset, secmonitor[id].secstartlen);
    offset += secmonitor[id].secstartlen;
    //布防条件长度
    secmonitor[id].tricondlen = ((secconfig[offset] << 8) + secconfig[offset + 1]);
    offset += 2;
    //布防条件
    memcpy(secmonitor[id].tricond, secconfig + offset, secmonitor[id].tricondlen);
    offset += secmonitor[id].tricondlen;
    //撤防条件长度
    secmonitor[id].disarmlen = ((secconfig[offset] << 8) + secconfig[offset + 1]);
    offset += 2;
    //撤防条件
    memcpy(secmonitor[id].disarmtask, secconfig + offset, secmonitor[id].disarmlen);
    offset += secmonitor[id].disarmlen;
    //报警条件长度
    secmonitor[id].secenfolen = ((secconfig[offset] << 8) + secconfig[offset + 1]);
    offset += 2;
    //报警条件
    memcpy(secmonitor[id].secenforce, secconfig + offset, secmonitor[id].secenfolen);
    offset += secmonitor[id].secenfolen;
    //报警任务长度
    secmonitor[id].tasklen = ((secconfig[offset] << 8) + secconfig[offset + 1]);
    offset += 2;
    //报警任务
    memcpy(secmonitor[id].regtask, secconfig + offset, secmonitor[id].tasklen);
    offset += secmonitor[id].tasklen;

    pr_debug("offset:%d\n", offset);

    readsecconditions(id);
}

//读取安防配置
void readsecurityconfig(int id, byte *secconfig)
{
    if(id == 0)
    {
        readsectriggercond(secconfig);
    }
    else
    {
        FILE *fd = NULL;
        byte areastr[4096] = {0};
        byte areahex[2048] = {0};
        memset(&secmonitor, 0, sizeof(secmonitor));

        if((fd = fopen(SECMONITOR, "r")) == NULL)
        {
            pr_debug("%s: %s\n", SECMONITOR, strerror(errno));
            fd = NULL;
            return;
        }
        while(fgets(areastr, 4096, fd))
        {
            if(strlen(areastr) > 10)
            {
                StrToHex(areahex, areastr, strlen(areastr) / 2);
                readsectriggercond(areahex);
            }
        }
        fclose(fd);
        fd = NULL;
    }
}

//检查是否满足撤防条件
void checkdisarmcond(int id, int type, byte *node)
{
    int i = 0;
    byte appsend[24] = {0xe1,0x00,0x06,0x00,0x00,0x00};
    appsend[4] = id;
    //活动侦测
    if(type == 0x07)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secdisarm[i].activity + 1, 2))
            {
                if(secmontri[id].secdisarm[i].activity[3] == (node[4] & 0x01))
                {
                    writesecstatus(id, "0", 5);
                    //撤防
                    secmonitor[id].secmonitor = 0;
                    secmonitor[id].status = 0;
                    UartToApp_Send(appsend, appsend[2]);
                    break;
                }
                else
                {
                    break;
                }
            }
            else if(0 == secmontri[id].secdisarm[i].activity[0])
            {
                break;
            }
        }
    }
    //门锁
    else if(type == 0x41)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secdisarm[i].doorlock + 1, 2))
            {
                if(secmontri[id].secdisarm[i].doorlock[3] == node[4])
                {
                    writesecstatus(id, "0", 5);
                    //撤防
                    secmonitor[id].secmonitor = 0;
                    secmonitor[id].status = 0;
                    UartToApp_Send(appsend, appsend[2]);
                    break;
                }
                else
                {
                    break;
                }
            }
            else if(0 == secmontri[id].secdisarm[i].doorlock[0])
            {
                break;
            }
        }
    }
    //门磁
    else if(type == 0x51)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secdisarm[i].magnetic + 1, 2))
            {
                if(secmontri[id].secdisarm[i].magnetic[3] == node[4])
                {
                    writesecstatus(id, "0", 5);
                    //撤防
                    secmonitor[id].secmonitor = 0;
                    secmonitor[id].status = 0;
                    UartToApp_Send(appsend, appsend[2]);
                    break;
                }
                else
                {
                    break;
                }
            }
            else if(0 == secmontri[id].secdisarm[i].magnetic[0])
            {
                break;
            }
        }
    }
}

//安防报警
void checksecurityalarm(int id, byte *node)
{
    int type = 0;
    byte tempcond[64] = {0};
    byte appsend[12] = {0x62,0x00,0x04};
    byte cloudsend[256] = {0xf4,0x00};
    byte uartsend[10] = {0x63,0x0e,0xff,0x2b,0xff,0x01};

    while(type < secmonitor[id].tasklen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            return;
        }
        switch(secmonitor[id].regtask[type])
        {
            case 0x0f:
                if(secmonitor[id].regtask[type + 3] == 0x01)
                {
                    memcpy(tempcond, secmonitor[id].regtask + type, 5);
                    appsend[3] = tempcond[4];
                    //执行手动场景
                    manualsceope(appsend);

                    type += 5;
                }
                else if(secmonitor[id].regtask[type + 3] == 0x08)
                {
                    //温度报警
                    hostinfo.alarm = 1;
                    UART0_Send(uart_fd, uartsend, uartsend[1]);
                    type += 5;
                }
                else if((secmonitor[id].regtask[type + 3] > 0x04) && (secmonitor[id].regtask[type + 3] < 0x08))
                {
                    memcpy(tempcond, secmonitor[id].regtask + type, (secmonitor[id].regtask[type + 4] + 5));
                    //len
                    cloudsend[2] = (10 + tempcond[4]);
                    //mac
                    cloudsend[3] = node[0];
                    cloudsend[4] = node[1];
                    //id
                    cloudsend[5] = id;
                    //type
                    if(tempcond[3] == 0x05)
                    {
                        //短信
                        cloudsend[6] = 0x03;
                    }
                    else if(tempcond[3] == 0x06)
                    {
                        //电话
                        cloudsend[6] = 0x02;
                    }
                    else if(tempcond[3] == 0x07)
                    {
                        //提醒
                        cloudsend[6] = 0x01;
                    }
                    else
                    {
                        cloudsend[6] = tempcond[3];
                    }
                    //触发设备/id
                    if(id == 5)
                    {
                        cloudsend[7] = 0x00;
                        cloudsend[8] = node[2];
                    }
                    else
                    {
                        cloudsend[7] = node[0];
                        cloudsend[8] = node[1];
                    }
                    //telephone len
                    cloudsend[9] = tempcond[4];
                    //telephone/提醒内容
                    memcpy(cloudsend + 10, tempcond + 5, tempcond[4]);
                    UartToApp_Send(cloudsend, cloudsend[2]);
                    type += (secmonitor[id].regtask[type + 4] + 5);
                }
                else
                {
                    type = -1;
                }
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].regtask[type]);
                type = -1;
                break;
        }
    }
}

//检查是否满足报警条件
int checkalarmcond(int id, int type, byte *node)
{
    pr_debug("type:%d\n", type);
    int i = 0;

    for(i = 0; i < 6; i++)
    {
        pr_debug("node[%d]:%0x\n", i, node[i]);
    }
    //温度报警
    if(type == 0x06)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].temperature + 1, 2))
            {
                if(secmontri[id].secenfo[i].temperature[3] <= node[4])
                {
                    //报警
                    pr_debug("满足报警条件:%d\n", i);
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].temperature[0])
            {
                if(i == 0)
                {
                    //用户未配置节点,采用默认68度报警
                    if(node[4] >= 88)
                    {
                        //报警
                        pr_debug("满足报警条件:%d\n", i);
                        checksecurityalarm(id, node);
                        return 0;
                    }
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
        }
    }
    //活动侦测
    else if(type == 0x07)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].activity + 1, 2))
            {
                if(secmontri[id].secenfo[i].activity[3] == (node[4] & 0x01))
                {
                    //报警
                    pr_debug("满足报警条件:%d\n", i);
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].activity[0])
            {
                pr_debug("未满足报警条件:%d\n", i);
                return -1;
            }
        }
    }
    //智能锁
    else if(type == 0x41)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].doorlock + 1, 2))
            {
                if(secmontri[id].secenfo[i].doorlock[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].doorlock[0])
            {
                return -1;
            }
        }
    }
    //门磁
    else if(type == 0x51)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].magnetic + 1, 2))
            {
                if(secmontri[id].secenfo[i].magnetic[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].magnetic[0])
            {
                return -1;
            }
        }
    }
    //水浸
    else if(type == 0x52)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].flooding + 1, 2))
            {
                if(secmontri[id].secenfo[i].flooding[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].flooding[0])
            {
                return -1;
            }
        }
    }
    //烟雾
    else if(type == 0x54)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].smoke + 1, 2))
            {
                if(secmontri[id].secenfo[i].smoke[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].smoke[0])
            {
                return -1;
            }
        }
    }
    //燃气
    else if(type == 0x56)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].gas + 1, 2))
            {
                if(secmontri[id].secenfo[i].gas[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].gas[0])
            {
                return -1;
            }
        }
    }
    //一氧化碳
    else if(type == 0x58)
    {
        for(i = 0; i < 250; i++)
        {
            if(0 == memcmp(node, secmontri[id].secenfo[i].carbonmxde + 1, 2))
            {
                if(secmontri[id].secenfo[i].carbonmxde[3] == node[4])
                {
                    //报警
                    checksecurityalarm(id, node);
                    return 0;
                }
                else
                {
                    pr_debug("未满足报警条件:%d\n", i);
                    return -1;
                }
            }
            else if(0 == secmontri[id].secenfo[i].carbonmxde[0])
            {
                return -1;
            }
        }
    }
    return -1;
}

//检查是否满足设防条件
int checkfortificcond(int id)
{
    int type = 0;
    int nodestatus = 0;
    int nodeenable = 0;
    byte appsend[24] = {0xe1,0x00,0x06,0x00,0x00,0x00};
    appsend[3] = id;

    byte tempcond[24] = {0};

    pr_debug("secmonitor[id].tricondlen:%d\n", secmonitor[id].tricondlen);
    while(type < secmonitor[id].tricondlen)
    {
        if(type == -1)
        {
            pr_debug("未识别的触发条件类型\n");
            return -1;
        }
        switch(secmonitor[id].tricond[type])
        {
            case 0x07:
                memcpy(tempcond, secmonitor[id].tricond + type, 6);
                //节点当前是否在线
                if(nodedev[tempcond[2]].nodeinfo[11] == 1)
                {
                    //nodeenable = nodedev[tempcond[2]].nodeinfo[8] & 0x01; 
                    nodestatus = nodedev[tempcond[2]].nodeinfo[6] & 0x01;

                    if(nodestatus != tempcond[3])
                    {
                        pr_debug("未满足设防条件:%0x\n", tempcond[2]);
                        return -1;
                    }
                }
                type += 6;
                break;
            case 0x51:
                memcpy(tempcond, secmonitor[id].tricond + type, 6);
                nodestatus = nodedev[tempcond[2]].nodeinfo[6] & 0x01;
                if(nodestatus != tempcond[3])
                {
                    pr_debug("未满足设防条件:%0x\n", tempcond[2]);
                    return -1;
                }
                type += 6;
                break;
            default:
                pr_debug("获取触发条件未识别类型:%d-%0x\n", type, secmonitor[id].tricond[type]);
                type = -1;
                break;
        }
    }
    if(type >= 0)
    {
        UartToApp_Send(appsend, appsend[2]);
        return id;
    }
}

//检查是否满足启动布防条件
void checkarmingcond(int id, int type, byte *node)
{
    pr_debug("开始检查启动布防条件\n");
    int i = 0;
    //活动侦测
    if(type == 0x07)
    {
        for(i = 0; i < 2; i++)
        {
            if(0 == memcmp(node, secmontri[id].secstartcond[i].activity + 1, 2))
            {
                if(0x01 == (node[4] & 0x01))
                {
                    //清除启动安防倒计时
                    pr_debug("清除启动安防倒计时:%d\n", id);
                    //在家:3,离家4
                    secmontri[id].seccountdown = 0;
                    memset(&timers[id + 2], 0, sizeof(timers[id + 2]));
                }
                else if(0 == (node[4] & 0x01))
                {
                    //刷新安防倒计时检查
                    pr_debug("刷新安防启动倒计时:%d-%d\n", id, timers[id + 2].counting);
                    secmontri[id].seccountdown = 5;
                    timers[id + 2].startlogo = 1;
                    timers[id + 2].counting = 0;
                }
            }
        }
    }
    //门磁
    else if(type == 0x51)
    {
        for(i = 0; i < 2; i++)
        {
            if(0 == memcmp(node, secmontri[id].secstartcond[i].magnetic + 1, 2))
            {
                if(0x01 == node[4])
                {
                    //清除启动安防倒计时
                    pr_debug("清除启动安防倒计时:%d\n", id);
                    //在家:3,离家4
                    secmontri[id].seccountdown = 0;
                    memset(&timers[id + 2], 0, sizeof(timers[id + 2]));
                }
                else if(0 == node[4])
                {
                    //刷新安防倒计时检查
                    pr_debug("刷新安防启动倒计时:%d-%d\n", id, timers[id + 2].counting);
                    secmontri[id].seccountdown = 35;
                    timers[id + 2].startlogo = 1;
                    timers[id + 2].counting = 0;
                }
            }
        }
    }
}

//智能安防检查
void smartsecuritycheck(int type, byte *node)
{
    pr_debug("智能安防检查\n");
    pr_debug("secmonitor[1].secmonitor:%d\n", secmonitor[1].secmonitor);
    pr_debug("secmonitor[2].secmonitor:%d\n", secmonitor[2].secmonitor);
    
    pr_debug("secmonitor[1].enable:%d\n", secmonitor[1].enable);
    pr_debug("secmonitor[2].enable:%d\n", secmonitor[2].enable);
    //离家布防
    if(secmonitor[1].secmonitor == 1)
    {
        //已经开启布防，检查是否报警/撤防
        if(0 > checkalarmcond(1, type, node))
        {
            if(secmonitor[1].enable == 1)
            {
                //撤防检查
                pr_debug("撤防检查\n");
                checkdisarmcond(1, type, node);
            }
        }
    }
    //在家布防
    else if(secmonitor[2].secmonitor == 1)
    {
        //已经开启布防，检查是否报警/撤防
        if(0 > checkalarmcond(2, type, node))
        {
            if(secmonitor[2].enable == 1)
            {
                //撤防检查
                pr_debug("撤防检查\n");
                checkdisarmcond(2, type, node);
            }
        }
    }
    else
    {
        //检查是否满足启动布防条件
        //优先离家布防
        if(secmonitor[1].enable == 1)
        {
            pr_debug("开始检查是否满足离家智能布防条件:%d\n", secmontri[2].seccountdown);
            if(secmontri[2].seccountdown == 0)
            {
                if(0 != restrictcheck(secmonitor[1].armingtime))
                {
                    checkarmingcond(1, type, node);
                }
                else if(0 == secmonitor[1].armingtime[0])
                {
                    checkarmingcond(1, type, node);
                }
            }
        }
        if(secmonitor[2].enable == 1)
        {
            //离家布防未满足&&未开始倒计时启动布防
            pr_debug("开始检查是否满足在家智能布防条件:%d\n", secmontri[1].seccountdown);
            if(secmontri[1].seccountdown == 0)
            {
                if(0 != restrictcheck(secmonitor[2].armingtime))
                {
                    checkarmingcond(2, type, node);
                }
                else if(0 == secmonitor[2].armingtime[0])
                {
                    checkarmingcond(2, type, node);
                }
            }
        }
    }
}
