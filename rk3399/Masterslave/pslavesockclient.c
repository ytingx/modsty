#include "pslavesockclient.h"
#include "struct.h"
#include "md5c.h"

extern int new_fd;
//extern int client_fd;
extern int slavesockfd;
extern int uart_fd;
extern int single_fd;
extern int g_heartbeat;
extern int master;
extern const byte softwareversion[10];

static int tempcount = 0;
static int coldcount = 0;

//从机上传串口数据
int slaveupuartdata(int len, byte *socksend)
{
    int i = 0;
    if(len != send(slavesockfd, socksend, len, MSG_NOSIGNAL))
    {
        perror("send");
        pr_debug("slaveupuartdata 数据发送失败\n");
    }
    else
    {
        for(i = 0; i < len; i++)
        {
            pr_debug("slaveupuartdata[%d]:%0x\n", i, socksend[i]);
        }
        return len;
    }
}

int slabroadcast(struct sockaddr_in *seraddr)
{
    int sock = -1;
    byte slavestr[64] = {0};
    byte slavehex[32] = {0};
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }
    const int opt = -1;
    int nb = 0;
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt));//设置套接字类型
    if(nb == -1)
    {
        return -1;
    }
    struct sockaddr_in addrto;
    memset(&addrto, 0, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);//套接字地址为广播地址
    addrto.sin_port = htons(9999);//套接字广播端口号为9999
    int nlen = sizeof(addrto);
    byte msg[64] = {0x60,0x00,0x12,0x01};
    
    //从机序列号
    memcpy(msg + 4, hostinfo.slavesn, 7);
    //绑定的主机序列号
    memcpy(msg + 11, hostinfo.hostsn, 7);
    
    int ret = sendto(sock, msg, msg[2], 0, (struct sockaddr*)&addrto, nlen);//向广播地址发布消息
    if(ret < 0)
    {
        pr_debug("sendto fall\n");
        return -1;
    }
    else 
    {
        printf("sendto ok\n");
    }

    int count = 0;
    byte buffer[128] = {0};
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        printf("socket option  SO_RCVTIMEO not support\n");
        return -1;
    }
    if((count = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&addrto, &nlen)) < 5)
    {
        printf("recvfrom err:%d\n", count);
        return -1;
    }
    
    //count = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&addrto, &nlen);
    printf("Server IP is%s\n",inet_ntoa(addrto.sin_addr));
    printf("Server Send Port:%d\n",ntohs(addrto.sin_port));
    printf("count:%d\n", count);

    if(msg[10] == 0)
    {
        //新从机保存绑定信息
        pr_debug("新从机保存绑定信息:%d\n", buffer[10]);
        if(buffer[10] > 1)
        {
            memcpy(slavehex, buffer + 4, 14);
            memcpy(hostinfo.slavesn, buffer + 4, 7);
            memcpy(hostinfo.hostsn, buffer + 11, 7);
            HexToStr(slavestr, slavehex, 14);
            replace(MASSLAINFO, slavestr, 12);
        }
    }
    seraddr->sin_port = 6001;
    seraddr->sin_addr = addrto.sin_addr;
    return 0;
}

//从机crc校验
int slaveuartcrc(int fd, byte *send_buf, int data_len)
{
    int i = 0;
    int len = 0;
    //crc
    byte add = 0x00;
    //数据头
    byte head_buf[500] = {0};
    
    for(i = 0; i < data_len - 1; i++)
    {
        add += send_buf[i];
    }
    memcpy(head_buf, send_buf, data_len);
    head_buf[data_len - 1] = add;

    slavechipSend(fd, head_buf, data_len);
}

//发送心跳
int slavesendheartbeat()
{
    byte heartbeat[24] = {0xf7,0x00,0x0a,0x00,0xf2,0x00,0x06};
    heartbeat[3] = master;

    time_t timep;
   
    byte chipsend[10] = {0x30,0x00,0x06,0x01};
    byte route[128] = {"route add default gw "};
    byte gw[20] = {0};
    byte interface[10] = {" br-lan"};

    //网络-link
    //有线
    if(0 == netportinspection())
    {
        hostinfo.wifirestart = 0;
        if(0 == getgateway(gw))
        {
            //连接成功
            chipsend[4] = 0x07;
            if(hostinfo.wifisw == 1)
            {
                //关闭wifi设置gw
                strcat(route, gw);
                strcat(route, interface);
                pr_debug("wifi down:%s\n", route);
                system(route);
                
                system("wifi down");
                hostinfo.wifisw = 0;
            }
        }
    }
    //无线
    else
    {
        if(0 == getgateway(gw))
        {
            hostinfo.wifirestart = 0;
            chipsend[4] = 0x05;
        }
        hostinfo.wifisw = 1;
    }
    slaveuartcrc(single_fd, chipsend, 6);
    usleep(10000);

    if(slavesockfd > 0)
    {
        if(heartbeat[2] != send(slavesockfd, heartbeat, heartbeat[2], MSG_NOSIGNAL))
        {
            perror("send");
            pr_debug("主机心跳发送失败\n");
            close(slavesockfd);
            slavesockfd = -1;
            return -1;
        }
        else
        {
            time(&timep);
            pr_debug("[%s]第%d次发送主机心跳\n", ctime(&timep), coldcount);
            coldcount++;
            g_heartbeat = 1;
        }
    }
    else
    {
        pr_debug("主机停止服务\n");
        close(slavesockfd);
        slavesockfd = -1;
        return -1;
    }
    return 0;
}

//LED初始化 0:云连接成功 1:云连接失败
void slaveledinitialization(int mark)
{
    pr_debug("Led initialization...\n");
    byte irsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte chipsend[10] = {0x30,0x00,0x06,0x01};
    byte apsta[20] = {"apsta0"};
    byte hoststr[20] = {0};
    byte route[128] = {"route add default gw "};
    byte gw[20] = {0};
    byte interface[10] = {" br-lan"};
    
    //主从机-power
    if(0 == filefind("mode", hoststr, HOSTSTATUS, 4))
    {
        if(1 == (hoststr[4] - '0'))
        {
            //主机
            chipsend[4] = 0x01;
        }
        else
        {
            //从机
            chipsend[4] = 0x02;
        }
        slaveuartcrc(single_fd, chipsend, 6);
        usleep(10000);
    }
    //网络-link
    //有线
    if(0 == netportinspection())
    {
        hostinfo.wifirestart = 0;
        pr_debug("0 == netportinspection()\n");
        if(0 == getgateway(gw))
        {
            //连接成功
            chipsend[4] = 0x07;
            if(hostinfo.apsta == 1)
            {
                pr_debug("系统重启-切换网络模式-STA..\n");
                system("cp -f /data/modsty/sta/etc/config/* /etc/config/");
                replace(HOSTSTATUS, apsta, 5);
                slaveuartcrc(single_fd, irsend, irsend[2]);
                saveautoscestatus();
                sleep(3);
                system("reboot");
            }
            else if(hostinfo.wifisw == 1)
            {
                //关闭wifi设置gw
                strcat(route, gw);
                strcat(route, interface);
                pr_debug("wifi down:%s\n", route);
                system(route);
                
                system("wifi down");
                hostinfo.wifisw = 0;
            }
        }
        else
        {
            pr_debug("有线连接网管失败\n");
            chipsend[4] = 0x06;
        }
    }
    //无线
    else
    {
        if(0 == getgateway(gw))
        {
            hostinfo.wifirestart = 0;
            chipsend[4] = 0x05;
        }
        else
        {
            if((hostinfo.apsta == 0) && (hostinfo.wifirestart == 0))
            {
                hostinfo.wifirestart = 1;
                system("wifi");
            }
            chipsend[4] = 0x04;
        }
        hostinfo.wifisw = 1;
    }
    slaveuartcrc(single_fd, chipsend, 6);
    usleep(10000);

    //云连接-成功
    if(mark == 0)
    {
        if(hostinfo.dhcp == 1)
        {
            chipsend[4] = 0x0a;
        }
        else
        {
            chipsend[4] = 0x08;
        }
    }
    else
    {
        if(hostinfo.dhcp == 1)
        {
            chipsend[4] = 0x0b;
        }
        else
        {
            chipsend[4] = 0x09;
        }
    }
    slaveuartcrc(single_fd, chipsend, 6);
}

//程序升级
void slaveupgrade(int sockfd)
{
    pr_debug("发送升级验证\n");
    byte update[10] = {0x04,0x00,0x07};
    memcpy(update + 3, softwareversion, 4);
    send(sockfd, update, update[2], MSG_NOSIGNAL);
}

//版本验证
void slaveversionvalidation(byte *buf)
{
    if(buf[3] == 0)
    {
        return;
    }
    int i = 0;
    int result = -1;
    byte wgetstr[128] = {"wget -P /tmp/ "};
    byte md5[32] = {0};
    memcpy(wgetstr + 14, buf + 20, buf[2] - 20);

    pr_debug("wgetstr:%s\n", wgetstr);
    system(wgetstr);
    pr_debug("下载升级文件\n");
    sleep(5);
    result = MD5File("/tmp/protocol", md5);
    if(result == 0)
    {
        for(i = 0; i < 16; i++)
        {
            pr_debug("md5[%d]:%0x\n", i, md5[i]);
        }
        if(0 == memcmp(buf + 4, md5, 16))
        {
            //system("rm -f /data/modsty/protocol");
            system("cp /tmp/protocol /data/modsty/");
            sleep(1);
            system("chmod 777 /data/modsty/protocol");
            saveautoscestatus();
            byte logsend[128] = {0xf3,0x00};
            byte logbuf[64] = {"Host upgraded successfully"};
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            send(slavesockfd, logsend, logsend[2], MSG_NOSIGNAL);
            sleep(2);
            pr_debug("升级重启程序\n");
            exit(0);
        }
    }
}

int slavepwverification(int sockfd,  byte *account)
{
    int authority = 0;
    byte appsend[32] = {0x80,0x00,0x0b};

    //登录验证
    appsend[3] = accountcheck(&authority, account);
    //mac
    memcpy(appsend + 4, hostinfo.hostsn, 6);
    //权限
    appsend[10] = authority;
    int i = 0;
    pr_debug("登录结果\n");
    for(i = 0; i < appsend[2]; i++)
    {
        pr_debug("appsend[%d]:%0x\n", i, appsend[i]);
    }
    send(sockfd, appsend, appsend[2], MSG_NOSIGNAL);
    return appsend[3];
}

int slaveconnect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
    if(connect(sockfd, addr, alen) == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//同步从机数据
void syncslavedata()
{
    byte getnodevalue[24] = {0x63,0x0e,0xff,0x11,0xff,0x00};
    byte getnodereg[24] = {0x63,0x0e,0xff,0x10,0x00,0x00};
    byte temoderate[24] = {0x30,0x00,0x06,0x07,0x00};

    getnodevalue[13] = 0x80;
    getnodereg[13] = 0x80;

    //获取所有节点状态
    slaveUartSend(uart_fd, getnodereg, getnodereg[1]);
    sleep(3);
    //获取主机温湿度
    slaveuartcrc(single_fd, temoderate, 6);
    sleep(1);
    //获取所有节点状态
    slaveUartSend(uart_fd, getnodevalue, getnodevalue[1]);
    sleep(3);
}

int slaveprint_uptime(int sockfd)
{
    int n = 0;
    int i = 0;
    int slaveack = 0;
    int heartlogo = 0;
    int pwerrcount = 0;
    int recvSize = 0;
    byte recvBuf[BUFLEN] = {0};
    byte buf[BUFLEN] = {0};
    byte password[50] = {0xf5,0x00,0x16,0x01};
    byte revbuf[2048] = {0};

    //从机序列号
    memcpy(password + 4, hostinfo.slavesn, 7);
    //绑定的主机序列号
    memcpy(password + 11, hostinfo.hostsn, 7);
    //从机版本
    memcpy(password + 18, softwareversion, 4);
    
    //验证帐号密码
    if(send(sockfd, password, password[2], MSG_NOSIGNAL) == -1)
    {
        perror("fail to send datas.\n");
    }
    for(i = 0; i < password[2]; i++)
    {
        printf("Server.password[%d]:%0x\n", i, password[i]);
    }
    while(1)
    {
        if((recvSize = recv(sockfd, recvBuf, BUFLEN, 0)) == -1)
        {
            perror("fail to receive datas.\n");
            return -1;
        }
        pr_debug("recvSize:%d\n", recvSize);
        
        if(recvSize == 0)
        {
            if(slaveack == 10)
            {
                return -1;
            }
            sleep(1);
            slaveack++;
            pr_debug("从机登录等待应答超时\n");
            continue;
        }

        for(i = 0; i < recvSize; i++)
        {
            printf("Server.modsty[%d]:%0x\n", i, recvBuf[i]);
        }
        if(recvBuf[10] > 1)
        {
            if(memcmp(recvBuf + 4, hostinfo.slavesn, 6) == 0)
            {
                pr_debug("从机登录验证成功\n");
                if(hostinfo.slavesn[6] == 0)
                {
                    //存储主从机绑定信息
                    byte bindinfohex[24] = {0};
                    byte bindinfostr[48] = {0};
                    memcpy(bindinfohex, recvBuf + 4, 14);
                    HexToStr(bindinfostr, bindinfohex, 14);
                    replace(MASSLAINFO, bindinfostr, 12);
                }
                master = recvBuf[10];
                break;
            }
            else
            {
                pr_debug("从机登录验证失败\n");
                return -1;
            }
        }
        else
        {
            pr_debug("从机获取标识失败\n");
            return -1;
        }
    }
    
    //同步从机数据
    syncslavedata();
    
    int recv_num = 0;

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    //LED检查
    slaveledinitialization(0);
    
    //程序升级
    //upgrade(sockfd);
    
    while(1)
    {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        timeout.tv_sec = 60;

        switch(select(sockfd + 1, &fds, NULL, NULL, &timeout))
        {
            case -1:
                perror("select error");
                pr_debug("psockclient error\n");
                break;
            case 0:
                pr_debug("Client NO data received\n");
                if(g_heartbeat == 1)
                {
                    pr_debug("心跳未收到应答\n");
                    pr_debug("第%d次重连云服务器..\n", tempcount);
                    tempcount++;
                    close(slavesockfd);
                    slavesockfd = -1;
                    g_heartbeat = 0;
                    return -1;
                }
                //心跳
                heartlogo = slavesendheartbeat();
                if(heartlogo == -1)
                {
                    close(slavesockfd);
                    slavesockfd = -1;
                    return -1;
                }
                break;
            default:
                if(FD_ISSET(sockfd, &fds))
                {
                    if(((recv_num = recv(sockfd, revbuf, 2048, 0)) == -1) || (recv_num == 0))
                    {
                        close(sockfd);
                        sockfd = -1;
                        pr_debug("default:%d\n", slavesockfd);
                        return -1;
                    }
                    else
                    {
                        new_fd = sockfd;
                        sockserver_run(recv_num, revbuf);
                        memset(revbuf, 0, sizeof(revbuf));
                    }
                }
        }
    }
    return 0;
}

void *slavesockclient_run(void *a)
{
    int reg = -1;
    struct sockaddr_in server_addr;
    while(1)
    {
        //LED检查
        slaveledinitialization(1);
        memset(&server_addr, 0, sizeof(server_addr));
        if((reg = slabroadcast(&server_addr)) < 0)
        {
            pr_debug("从机-侦测主机...\n");
            sleep(5);
        }
        else
        {
            pr_debug("从机-侦测主机成功\n");
            break;
        }
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6001);
    
    printf("Server IP is:%s\n", inet_ntoa(server_addr.sin_addr));
    printf("Server Send Port:%d\n", ntohs(server_addr.sin_port));
    
    while(1)
    {
        if ((slavesockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        {
            printf("create socket failed: %s\n", strerror(errno));
            slaveledinitialization(1);
            sleep(30);
            continue;
        }
        printf("create socket ok\n");
        if (slaveconnect_retry(slavesockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
        {
            printf("can't connect to %s: %s\n", inet_ntoa(server_addr.sin_addr), strerror(errno));
            slaveledinitialization(1);
            sleep(30);
            continue;
        } 
        else 
        {
            printf("connect ok, slavesockfd:%d\n", slavesockfd);
            if (slaveprint_uptime(slavesockfd) < 0) 
            {
                printf("slaveprint_uptime error: %s\n", strerror(errno));
                slaveledinitialization(1);
                sleep(30);
                continue;
            }
        }
        close(slavesockfd);
        pr_debug("close(slavesockfd):%d\n", slavesockfd);
        sleep(30);
    }
    return 0;
}
