#include "pledcheck.h"
#include "struct.h"

extern int uart_fd;
extern int answer;
extern int single_fd;
extern int g_led_on;
extern int g_led;;
extern int g_meshdata;
extern struct cycle_buffer *fifo;

//超时从机的节点置为离线
void slavenodeoffline(int mac)
{
    int i = 2;
    for(i = 2; i < 256; i++)
    {
        if(nodedev[i].nodeinfo[0] == mac)
        {
            nodedev[i].nodeinfo[11] = 0x00;
        }
    }
}

//检查心跳超时
int checkheartbeattimeout()
{
    int i = 2;
    time_t nowtime;
    byte appsend[24] = {0x96,0x00,0x14,0x00,0x0f};

    //获取当前时间
    time(&nowtime);
    for(i = 2; i < 5; i++)
    {
        if(150 < (nowtime - slaveident[i].heartbeattime))
        {
            //离线-上报
            if(slaveident[i].sockfd > 0)
            {
                //超时从机的节点置为离线
                slavenodeoffline(slaveident[i].assignid);

                pr_debug("从机离线上报:%d-%d\n", i, slaveident[i].sockfd);
                close(slaveident[i].sockfd);
                slaveident[i].sockfd = 0;
                slaveident[i].heartbeattime = 0;

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
}

//led指令发送
int Uart_Crc(int fd, byte *send_buf,int data_len)
{
    int i = 0;
    int len = 0;

    time_t timep;
    byte time_buf[50] = {0};
    byte buf[50] = {0};

    //crc
    byte add = 0x00;
    //数据头
    byte head_buf[20] = {0};
    //数据长度
    for(i = 0; i < (data_len - 1); i++)
    {
        add += send_buf[i];
    }
    memcpy(head_buf, send_buf, data_len);
    head_buf[data_len - 1] = add;
    UartSend(fd, head_buf, data_len);
    
    //数据发送标志
    g_meshdata = 1;

    /*
    len = write(fd, head_buf, data_len);
    
    for(i = 0; i < len; i++)
    {
        pr_debug("leduartsend[%d]:%0x\n", i, head_buf[i]);
    }
    */
    /*
    time(&timep);
    strcpy(buf, ctime(&timep));
    HexToStr(time_buf, head_buf, data_len);
    buf[strlen(buf)] = ' ';
    strcat(buf, time_buf);
    buf[strlen(buf)] = '\n';
    InsertLine("/data/modsty/ledlog", buf);
    memset(buf, 0, sizeof(buf));
    */
    /*
    if (len == (data_len))
    {
        return len;
    }     
    else   
    {
        tcflush(fd,TCOFLUSH);
        return FALSE;
    }
    */
}

//同步时钟
void synchronizeclock()
{
    //获取当前时间
    struct tm *ptr;
    time_t lt;
    lt = time(NULL); 
    ptr = localtime(&lt);       
    //pr_debug("minute:%d\n", ptr->tm_min);
    //pr_debug("hour:%d\n", ptr->tm_hour);
    //pr_debug("wday:%d\n", ptr->tm_wday);

    byte chipsend[24] = {0x30,0x00,0x0c,0x02};

    if(0 == getnetstatus())
    {
        chipsend[4] = ((ptr->tm_year + 1900) >> 8) & 0xff;
        chipsend[5] = (ptr->tm_year + 1900) & 0xff;
        chipsend[6] = ptr->tm_mon + 1; 
        chipsend[7] = ptr->tm_mday;
        chipsend[8] = ptr->tm_hour;
        chipsend[9] = ptr->tm_min;
        chipsend[10] = ptr->tm_sec;
        pr_debug("同步时钟-设置\n");
    }
    else
    {
        chipsend[2] = 0x06;
        chipsend[3] = 0x03;
        chipsend[4] = 0x00;
        pr_debug("同步时钟-获取\n");
    }
    IRUART0_Send(single_fd, chipsend, chipsend[2]);
}
//定时器功能
void timerhandle(int signo)
{
    int secmark = -1;
    //pr_debug("定时器\n");
    //震动
    if(timers[1].startlogo == 1)
    {
        pr_debug("震动:%d\n", timers[1].counting);
        ++timers[1].counting;
        if(10 == timers[1].counting)
        {
            pr_debug("10s未满足震动报警条件:%d\n", timers[1].triggerlogo);
            memset(&timers[1], 0, sizeof(timers[1]));
        }
    }
    //添加从机
    if(timers[2].startlogo == 1)
    {
        pr_debug("添加从机:%d\n", timers[2].counting);
        ++timers[2].counting;
        if(20 == timers[2].counting)
        {
            pr_debug("搜索从机结束\n");
            hostinfo.receslave = 0;
            memset(&timers[2], 0, sizeof(timers[2]));
            slavequery();
        }
    }
    //离家安防
    if(timers[3].startlogo == 1)
    {
        pr_debug("离家安防:%d\n", timers[3].counting);
        ++timers[3].counting;
        if(secmontri[1].seccountdown == timers[3].counting)
        {
            pr_debug("离家安防计时结束开始检查布防条件\n");
            secmark = checkfortificcond(1);
            pr_debug("secmark:%d\n", secmark);
            if(0 < secmark)
            {
                secmonitor[1].secmonitor = 1;
                writesecstatus(1, "1", 5);
            }
            secmontri[1].seccountdown = 0;
            memset(&timers[3], 0, sizeof(timers[3]));
        }
    }
    //在家安防
    if(timers[4].startlogo == 1)
    {
        pr_debug("在家安防:%d\n", timers[4].counting);
        ++timers[4].counting;
        if(secmontri[2].seccountdown == timers[4].counting)
        {
            pr_debug("在家安防计时结束开始检查布防条件\n");
            secmark = checkfortificcond(2);
            pr_debug("secmark:%d\n", secmark);
            if(0 < secmark)
            {
                secmonitor[2].secmonitor = 1;
                writesecstatus(2, "1", 5);
            }
            secmontri[2].seccountdown = 0;
            memset(&timers[4], 0, sizeof(timers[4]));
        }
    }
}

//定时器
void starttimer()
{
    pr_debug("启动定时器\n");
    struct itimerval tick;
    signal(SIGALRM, timerhandle);
    memset(&tick, 0, sizeof(tick));
    
    //Timeout to run first time
    tick.it_value.tv_sec = 1;
    tick.it_value.tv_usec = 0;

    //After first, the Interval time for clock
    tick.it_interval.tv_sec = 1;
    tick.it_interval.tv_usec = 0;
    if(setitimer(ITIMER_REAL, &tick, NULL) < 0)
    {
        printf("Set timer failed!\n");
    }
}

//chiptest
void chiptest()
{
    int i = 1;
    byte chipsend[24] = {0x30,0x00,0x06,0x01,0x00};
    while(1)
    {
        chipsend[4] = i;
        usleep(5000);
        IRUART0_Send(single_fd, chipsend, chipsend[2]);
        i++;
        if(i == 0x0f)
        {
            i = 1;
            continue;
        }
    }
}

//led命令执行部分
void *ledcheck(void *a)
{
    //chiptest(); 
    int i = 1;
    int j = 0;
    int h = 0;
    int ledstart = 0;
    int ledlogo = 0;
    int offset = 0;
    
    //本地关联
    byte localhead[24] = {0x01};
    byte localtail[24] = {0x01};
    int localheadvalue = 0;
    int localtailvalue = 0;
    byte uartsend[24] = {0x63,0x0e};

    byte chipsend[24] = {0x30,0x00,0x06,0x08,0x00};
    byte getnodevalue[24] = {0x63,0x0e,0xff,0x11,0xff,0x00};
    byte getnodereg[24] = {0x63,0x0e,0xff,0x10,0x00,0x00};
    byte temoderate[24] = {0x30,0x00,0x06,0x07,0x00};
    byte meshuartstatus[24] = {0x30,0x00,0x06,0x01,0xe1};
    //node value
    int nodevalue = 0;
    //去重相关
    int logo = 0;
    byte rele_temp[24] = {0};
    byte led[24] = {0x63,0x0e};

    //获取所有节点状态
    UART0_Send(uart_fd, getnodereg, getnodereg[1]);
    while(i < 5)
    {
        sleep(1);
        if(answer == 2)
        {
            break;
        }
        i++;
    }
    //同步时钟
    synchronizeclock();

    //获取主机温湿度
    IRUART0_Send(single_fd, temoderate, 6);
    sleep(2);

    //获取所有节点状态
    UART0_Send(uart_fd, getnodevalue, getnodevalue[1]);
    sleep(3);

    //检测主机与MESH通信状态
    if(hostinfo.meshuart != 2)
    {
        IRUART0_Send(single_fd, meshuartstatus, meshuartstatus[2]);
    }

    //定时器
    starttimer();
    
    while(1)
    {
        //检查从机心跳超时
        checkheartbeattimeout();

        //pr_debug("g_led:%d, logo:%d\n", g_led, logo);
        //pr_debug("g_led_on:%d\n", g_led_on);
        //偏移置0
        i = 0;
        //uart指令检查
        if(g_led == 1)
        {
            pthread_mutex_lock(&fifo->lock);
            g_led = 0;
            pthread_mutex_unlock(&fifo->lock);
            sleep(3);
            logo = 1;
            continue;
        }
        else if((logo == 1) && (g_led_on == 1))
        {
            //偏移
            //存放节点mac
            //取出关联信息进行检查
            while(i < 21)
            {
                //pr_debug("regcontrol[%d].tasklen:%d\n", i, regcontrol[i].tasklen);
                //pr_debug("id:%d\n", regcontrol[i].id);
                if(g_led == 1)
                {
                    break;
                }
                //执行长度为0
                if(regcontrol[i].tasklen == 0)
                {
                    i++;
                    continue;
                }
                //无绑定按键
                if((regcontrol[i].bingkeyid == 0) || (regcontrol[i].bingkeymac[1] == 0))
                {
                    i++;
                    continue;
                }
                
                //偏移
                h = 0;
                while(1)
                {
                    //pr_debug("while-1\n");
                    //拆分关联信息
                    memcpy(rele_temp, regcontrol[i].regtask + h, 8);
                    //检查完毕
                    if(h == regcontrol[i].tasklen)
                    {
                        //mac
                        led[2] = regcontrol[i].bingkeymac[1];
                        //type
                        led[3] = 0x04;
                        //node id
                        led[4] = regcontrol[i].bingkeyid;
                        //led value
                        led[5] = 0x00;
                        Uart_Crc(uart_fd, led, led[1]);
                        usleep(500000);
                        break;
                    }
                    //离线
                    if(0 == nodedev[rele_temp[2]].nodeinfo[11])
                    {
                        pr_debug("nodevalue33333:%0x\n", nodedev[rele_temp[2]].nodeinfo[5]);
                        h += 8;
                        continue;
                    }
                    if(nodedev[rele_temp[2]].nodeinfo[2] == 0x31)
                    {
                        if(0 < (nodedev[rele_temp[2]].nodeinfo[5] & 0x7f))
                        {
                            //mac
                            led[2] = regcontrol[i].bingkeymac[1];
                            //type
                            led[3] = 0x04;
                            //node id
                            led[4] = regcontrol[i].bingkeyid;
                            //led value
                            led[5] = 0x01;
                            Uart_Crc(uart_fd, led, led[1]);
                            usleep(500000);
                            ledlogo = 1;
                            break;
                        }
                        else
                        {
                            h += 8;
                        }
                    }
                    else if(((nodedev[rele_temp[2]].nodeinfo[2] > 0x00) && (nodedev[rele_temp[2]].nodeinfo[2] < 0x0f)) || (nodedev[rele_temp[2]].nodeinfo[2] == 0x21))
                    {
                        nodevalue = (nodedev[rele_temp[2]].nodeinfo[5] >> 4);

                        if(0 < nodevalue)
                        {
                            offset = 0;
                            ledlogo = 0;
                            for(j = 0; j < 8; j += 2)
                            {
                                pr_debug("nodevalue:%0x\n", nodevalue);
                                if(0x00 == ((rele_temp[4] >> j) & 0x03))
                                {
                                    pr_debug("rele_temp[4]:%0x\n", rele_temp[4]);
                                    pr_debug("j:%d, offset:%d\n", j, offset);
                                    if(0x01 == ((nodevalue  >> offset) & 0x01))
                                    {
                                        //mac
                                        led[2] = regcontrol[i].bingkeymac[1];
                                        //type
                                        led[3] = 0x04;
                                        //node id
                                        led[4] = regcontrol[i].bingkeyid;
                                        //led value
                                        led[5] = 0x01;
                                        Uart_Crc(uart_fd, led, led[1]);
                                        usleep(500000);
                                        ledlogo = 1;
                                        break;
                                    }
                                }
                                offset++;
                            }
                            if(ledlogo == 1)
                            {
                                break;
                            }
                            else
                            {
                                h += 8;
                            }
                        }
                        else
                        {
                            h += 8;
                        }
                    }
                    else
                    {
                        h += 8;
                    }
                }
                i++;
            }
            logo = 0;
            if(g_led == 1)
            {
                g_led_on = 1;
            }
            else
            {
                g_meshdata = 0;
                g_led_on = 0;
            }
        }
        usleep(1000000);
    }
}
