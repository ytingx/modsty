#include "puartread.h"
#include "struct.h"

extern int master;
extern int uart_fd;
extern int single_fd;
extern int answer;
extern byte ackswer[15];

void *serial_run(void *a)
{
    int err = -1;
    int len = 0;
    int temp = 0;
    int i = 0;
    int typelen = 0;
    byte r_buf[2048] = {0};
    byte add = 0x00;
    byte rcv_buf[512] = {0};

    //uart
    unsigned loop = 1;
    int maxufd = 0;
    int temp_len = 0;
    int fs_sel = 0;

    fd_set fs_read;
    fd_set tmp_inset;
    
    int fds[2] = {0};
    byte temp_buf[2048] = {0};
    struct timeval tv;

    //打开8266串口，返回文件描述符
    uart_fd = UART0_Open(uart_fd, "/dev/ttysWK2");
    do{
        err = UART0_Init(uart_fd, 115200, 0, 8, 1, 'N');
        pr_debug("Set Port Exactly!\n");
    }while(FALSE == err || FALSE == uart_fd);

    //打开单片机串口，返回文件描述符
    single_fd = UART0_Open(single_fd, "/dev/ttysWK3"); //打开8266串口，返回文件描述符
    do{
        err = UART0_Init(single_fd, 115200, 0, 8, 1, 'N');
        pr_debug("Set Port Exactly!\n");
    }while(FALSE == err || FALSE == single_fd);

    while(1)
    {
        FD_ZERO(&fs_read);
        FD_SET(uart_fd, &fs_read);
        FD_SET(single_fd, &fs_read);
        fds[0] = uart_fd;
        fds[1] = single_fd;
        maxufd = (fds[0] > fds[1]) ? fds[0] : fds[1];
        
        //temp = 0;
        //len = -1;
        tmp_inset = fs_read;
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        fs_sel = select(maxufd + 1, &tmp_inset, NULL, NULL, &tv);
        if(fs_sel)
        {
            pr_debug("fs_sel:%d\n", fs_sel);
            fs_sel = 0;

            for(i = 0; i < 2; i++)
            {
                temp = 0;
                len = 0;
                memset(r_buf, 0, sizeof(r_buf));
                memset(temp_buf, 0, sizeof(temp_buf));
                
                if(FD_ISSET(fds[i], &tmp_inset))
                {
                    while((temp_len = read(fds[i], temp_buf, 2048)) > 0)
                    {
                        memcpy(r_buf + len, temp_buf, temp_len);
                        memset(temp_buf, 0, sizeof(temp_buf));
                        len += temp_len;
                        temp_len = 0;
                        //read_count++;
                        //usleep(850);//GM8135s
                        usleep(1500);
                    }
                    pr_debug("uart_recv:%d\n", len);
                    /*
                    for(i = 0; i < len; i++)
                    {
                        pr_debug("r_buf[%d]:0X%0X\n", i, r_buf[i]);
                    }
                    */
                }
                if(len <= 0)
                {
                    continue;
                }
                while(1)
                {
                    if(r_buf[temp] == 0)
                    {
                        break;
                    }
                    memset(rcv_buf, 0, sizeof(rcv_buf));
                    //指令截取
                    //pr_debug("r_buf[temp]:%0x, %d\n", r_buf[temp], temp);
                    switch(r_buf[temp])
                    {
                        case 0x01://节点信息注册
                            //pr_debug("节点注册!\n");
                            memcpy(rcv_buf, r_buf + temp, 13);
                            len = 13;
                            temp += 13;
                            break;
                        case 0x02://节点状态反馈
                            //pr_debug("节点状态反馈!\n");
                            memcpy(rcv_buf, r_buf + temp, 13);
                            len = 13;
                            temp += 13;
                            break;    
                        case 0x03://节点数据反馈
                            //pr_debug("节点数据反馈!\n");
                            memcpy(rcv_buf, r_buf + temp, 7);
                            len = 7;
                            temp += 7;
                            break;
                        case 0x31://单片机上传
                            memcpy(rcv_buf, r_buf + temp, (r_buf[1] << 8) + r_buf[2]);
                            len = (r_buf[1] << 8) + r_buf[2];
                            temp += len;
                            break;
                        case 0x64://应答
                        case 0x67:
                        case 0x70:
                            memcpy(rcv_buf, r_buf + temp, 5);
                            len = 5;
                            temp += 5;
                            break;
                        case 0x74://初始节点信息上传
                            memcpy(rcv_buf, r_buf + temp, 10);
                            len = 10;
                            temp += 10;
                            break;
                        case 0x75://初始节点上传结束
                            memcpy(rcv_buf, r_buf + temp, 5);
                            len = 5;
                            temp += 5;
                            break;
                        case 0x77://分配mac应答
                            memcpy(rcv_buf, r_buf + temp, 10);
                            len = 10;
                            temp += 10;
                            break;
                        case 0x79://组网结束
                            memcpy(rcv_buf, r_buf + temp, 7);
                            len = 7;
                            temp += 7;
                            break;
                        case 0x7c://发送完整红外码应答
                            memcpy(rcv_buf, r_buf + temp, 6);
                            len = 6;
                            temp += 6;
                            break;
                        case 0x7d://发送完整红外码应答
                            memcpy(rcv_buf, r_buf + temp, 8);
                            len = 8;
                            temp += 8;
                            break;
                        case 0x7e://发送完整红外码应答
                            memcpy(rcv_buf, r_buf + temp, 7);
                            len = 7;
                            temp += 7;
                            break;
                        case 0x7f://发送完整红外码应答
                            memcpy(rcv_buf, r_buf + temp, 6);
                            len = 6;
                            temp += 6;
                            break;
                        case 0x87:
                        case 0x88:
                            memcpy(rcv_buf, r_buf + temp, 6);
                            len = 6;
                            temp += 6;
                            break;
                        case 0x89:
                            memcpy(rcv_buf, r_buf + temp, 13);
                            len = 13;
                            temp += 13;
                            break;
                        case 0x8a:
                            memcpy(rcv_buf, r_buf + temp, 13);
                            len = 13;
                            temp += 13;
                            break;
                        case 0x8b:
                            memcpy(rcv_buf, r_buf + temp, 11);
                            len = 11;
                            temp += 11;
                            break;
                        case 0x8c:
                            memcpy(rcv_buf, r_buf + temp, 13);
                            len = 13;
                            temp += 13;
                            break;
                        case 0xe2:
                            memcpy(rcv_buf, r_buf + temp, 5);
                            len = 5;
                            temp += 5;
                            break;
                        case 0xff:
                            memcpy(rcv_buf, r_buf + temp, 5);
                            len = 5;
                            temp += 5;
                            break;
                        default:
                            pr_debug("r_buf Serial_run unsupported type\n");
                            for(i = 0; i < len; i++)
                            {
                                pr_debug("Receive r_buf[%d]:0X%0X\n", i, r_buf[i]);
                            }
                            break;
                    }
                    if(rcv_buf[0] == 0)
                    {
                        break;
                    }

                    add = 0x00;
                    typelen = 0;
                    
                    if(rcv_buf[0] == 0x31)
                    {
                        typelen = ((rcv_buf[1] << 8) + rcv_buf[2]);
                    }
                    else
                    {
                        typelen = rcv_buf[1];
                    }

                    for(i = 0; i < (typelen - 1); i++)
                    {
                        add += rcv_buf[i]; 
                    }

                    //效验数据
                    if(add != rcv_buf[typelen - 1])
                    {
                        /*
                        int j = 0;
                        for(j = 0; j < typelen; j++)
                        {
                            pr_debug("Bad data[%d]:%0x\n", j, rcv_buf[j]);
                        }
                        time_t timep;
                        byte buf_t[1024] = {0};
                        byte time_buf[1024] = {0};

                        time(&timep);
                        strcpy(buf_t, ctime(&timep));
                        HexToStr(time_buf, rcv_buf, typelen);
                        strncat(buf_t, time_buf, typelen * 2);
                        buf_t[strlen(buf_t)] = '\n';
                        InsertLine("/data/modsty/baduartlog", buf_t);
                        */
                        break;
                    }
                    if(temp > 0)
                    {
                        //记录空闲时间
                        time(&hostinfo.freetime);

                        rcv_buf[temp] = '\0';
                        for(i = 0; i < len; i++)
                        {
                            pr_debug("Receive rcv_buf[%d]:0X%0X\n", i, rcv_buf[i]);
                        }
                        /*
                        time_t timep;
                        byte buf_t[100] = {0};
                        byte time_buf[50] = {0};

                        time(&timep);
                        strcpy(buf_t, ctime(&timep));
                        HexToStr(time_buf, rcv_buf, len);
                        strncat(buf_t, time_buf, len * 2);
                        buf_t[strlen(buf_t)] = '\n';
                        InsertLine("/data/modsty/uartuplog", buf_t);
                        */

                        switch(rcv_buf[0])
                        {
                            case 0x01://节点信息注册
                                pr_debug("节点注册!\n");
                                if(hostinfo.mesh != 2)
                                {
                                    searchnode(master, rcv_buf);
                                }
                                break;
                            case 0x02://节点状态反馈
                                pr_debug("节点状态反馈!\n");
                                if(hostinfo.mesh != 2)
                                {
                                    statefeedback(master, rcv_buf);
                                }
                                break;    
                            case 0x03://节点数据反馈
                                pr_debug("节点数据反馈!\n");
                                if(hostinfo.mesh == 0)
                                {
                                    datafeedback(master, rcv_buf);
                                }
                                break;
                            case 0x31://单片机上传
                                pr_debug("单片机上传\n");
                                singlechipup(master, rcv_buf);
                                break;
                            case 0x64://区域应答
                            case 0x67:
                            case 0x70:
                                if(0 == memcmp(rcv_buf, ackswer, 3))
                                {
                                    //记录配置失败原因
                                    if(rcv_buf[3] != 0)
                                    {
                                        pr_debug("0x64应答配置失败:%0x\n", rcv_buf[3]);
                                        sceneack.badmac[sceneack.maclen] = master;
                                        sceneack.badmac[sceneack.maclen + 1] = rcv_buf[2];
                                        sceneack.badmac[sceneack.maclen + 2] = ackswer[3];
                                        sceneack.badmac[sceneack.maclen + 3] = rcv_buf[3];
                                        sceneack.maclen += 4;
                                    }
                                    answer = 1;
                                }
                                break;
                            case 0x74://初始节点信息
                                pr_debug("初始节点信息!\n");
                                savenoderawinfo(rcv_buf);
                                break;
                            case 0x75://初始信息上传结束
                                pr_debug("初始信息上传结束\n");
                                distributionmac(master);
                                break;
                            case 0x77://分配mac应答
                                pr_debug("分配mac应答!\n");
                                if(0 == memcmp(ackswer, rcv_buf + 2, 7))
                                {
                                    answer = 1;
                                }
                                break;
                            case 0x79://组网结束
                                pr_debug("组网结束\n");
                                meshnetover(rcv_buf);
                                break;
                            case 0x7c://发送完整红外码应答
                                pr_debug("发送完整红外码应答");
                                completeirsendack(rcv_buf);
                                break;
                            case 0x7d:
                                pr_debug("存储红外快捷方式应答\n");
                                if(0 == memcmp(ackswer, rcv_buf, 3))
                                {
                                    answer = 1;
                                    irquickoperaack(rcv_buf);
                                }
                                break;
                                /*
                            case 0x7e://发送完整红外码应答
                                memcpy(rcv_buf, r_buf + temp, 7);
                                len = 7;
                                temp += 7;
                                break;
                            case 0x7f://发送完整红外码应答
                                memcpy(rcv_buf, r_buf + temp, 6);
                                len = 6;
                                temp += 6;
                                break;
                                */
                            case 0x87:
                                pr_debug("电机/负载数量转换\n");
                                ctlboxconversion(master, rcv_buf);
                                break;
                            case 0x88:
                                pr_debug("窗帘控制应答\n");
                                curtainctlack(master, rcv_buf);
                                break;
                            case 0x89:
                                pr_debug("智能锁信息\n");
                                smartlock(master, rcv_buf);
                                break;
                            case 0x8a:
                                pr_debug("被动时传感器信息\n");
                                passivesensorinfo(master, rcv_buf);
                                break;
                            case 0x8b:
                                pr_debug("获取MESH PW应答\n");
                                break;
                            case 0x8c:
                                pr_debug("智能控制面板应答\n");
                                smartctrpanelanswer(master, rcv_buf);
                                break;
                            case 0xe2:
                                pr_debug("mesh升级应答");
                                if(rcv_buf[3] == 1)
                                {
                                    answer = 2;
                                }
                                else if(rcv_buf[3] == 0)
                                {
                                    pr_debug("mesh升级成功\n");
                                }
                                break;
                            case 0xff:
                                pr_debug("超时应答\n");
                                answer = 2;
                                hostinfo.meshuart = 2;
                                break;
                            default:
                                pr_debug("Serial_run unsupported type\n");
                                break;
                        }
                    }
                }
            }
        }
        else
        {
            pr_debug("No data received!\n");
        }
    }
    UART0_Close(uart_fd);
    UART0_Close(single_fd);
}
