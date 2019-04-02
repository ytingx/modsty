#include "psockclient.h"
#include "struct.h"
#include "md5c.h"

extern int new_fd;
extern int client_fd;
extern int single_fd;
extern int g_heartbeat;
extern const byte softwareversion[10];

static int tempcount = 0;
static int coldcount = 0;

//发送心跳
int sendheartbeat()
{
    byte heartbeat[10] = {0xf2,0x00,0x06};
    time_t timep;
    time_t nowtime;
    heartbeat[3] = hostinfo.dataversion;
   
    byte chipsend[10] = {0x30,0x00,0x06,0x01};
    byte restchipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte route[128] = {"route add default gw "};
    byte gw[20] = {0};
    byte interface[10] = {" br-lan"};
    
    time(&nowtime);
    struct tm *ptr;
    time_t lt;
    lt = time(NULL);
    ptr = localtime(&lt);
    if(4320 <= ((nowtime - hostinfo.runtime) / 60))
    {
        if((ptr->tm_hour > 2) && (ptr->tm_hour < 5))
        {
            if(5 <= ((nowtime - hostinfo.freetime) / 60))
            {
                saveautoscestatus();
                IRUART0_Send(single_fd, restchipsend, restchipsend[2]);
                pr_debug("定时重启-心跳\n");
                byte logsend[128] = {0xf3,0x00};
                byte logbuf[64] = {"Host regularly restart"};
                logsend[2] = (3 + strlen(logbuf));
                memcpy(logsend + 3, logbuf, strlen(logbuf));
                sendsingle(client_fd, logsend, logsend[2]);
                sleep(2);
                system("reboot");
            }
        }
    }
    else
    {
        pr_debug("host runing time:%ld min, hostinfo.freetime:%ld\n", ((nowtime - hostinfo.runtime) / 60), ((nowtime - hostinfo.freetime) / 60));
    }

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
    IRUART0_Send(single_fd, chipsend, 6);

    if(client_fd > 0)
    {
        if(heartbeat[2] != send(client_fd, heartbeat, heartbeat[2], MSG_NOSIGNAL))
        {
            perror("send");
            pr_debug("心跳发送失败\n");
            close(client_fd);
            client_fd = -1;
            return -1;
        }
        else
        {
            time(&timep);
            pr_debug("[%s]第%d次发送云心跳\n", ctime(&timep), coldcount);
            coldcount++;
            g_heartbeat = 1;
        }
    }
    else
    {
        pr_debug("云服务器停止服务\n");
        close(client_fd);
        client_fd = -1;
        return -1;
    }
    return 0;
}

//LED初始化 0:云连接成功 1:云连接失败
void ledinitialization(int mark)
{
    pr_debug("Led initialization...\n");
    byte irsend[10] = {0x30,0x00,0x06,0x08,0x00};
    byte chipsend[10] = {0x30,0x00,0x06,0x01};
    byte apsta[20] = {"apsta0"};
    byte hoststr[20] = {0};
    byte route[128] = {"route add default gw "};
    byte gw[20] = {0};
    byte interface[10] = {" br-lan"};

    time_t nowtime;
    struct tm *ptr;
    time_t lt;
    lt = time(NULL);
    ptr = localtime(&lt);

    byte restchipsend[10] = {0x30,0x00,0x06,0x08,0x00};
    time(&nowtime);
    if(4320 <= ((nowtime - hostinfo.runtime) / 60))
    {
        if((ptr->tm_hour > 2) && (ptr->tm_hour < 5))
        {
            if(5 <= ((nowtime - hostinfo.freetime) / 60))
            {
                saveautoscestatus();
                IRUART0_Send(single_fd, restchipsend, restchipsend[2]);
                pr_debug("定时重启-led\n");
                byte logsend[128] = {0xf3,0x00};
                byte logbuf[64] = {"Host regularly restart"};
                logsend[2] = (3 + strlen(logbuf));
                memcpy(logsend + 3, logbuf, strlen(logbuf));
                sendsingle(client_fd, logsend, logsend[2]);
                sleep(2);
                system("reboot");
            }
        }
    }
    else
    {
        pr_debug("host runing time:%ld min, hostinfo.freetime:%ld\n", ((nowtime - hostinfo.runtime) / 60), ((nowtime - hostinfo.freetime) / 60));
    }
    
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
        IRUART0_Send(single_fd, chipsend, 6);
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
                system("cp -f /mnt/mtd/sta/etc/config/* /etc/config/");
                replace(HOSTSTATUS, apsta, 5);
                IRUART0_Send(single_fd, irsend, irsend[2]);
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
    IRUART0_Send(single_fd, chipsend, 6);

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
    IRUART0_Send(single_fd, chipsend, 6);
}

//程序升级
void upgrade(int sockfd)
{
    pr_debug("发送升级验证\n");
    byte update[10] = {0x04,0x00,0x07};
    memcpy(update + 3, softwareversion, 4);
    send(sockfd, update, update[2], MSG_NOSIGNAL);
}

//版本验证
void versionvalidation(byte *buf)
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
            //system("rm -f /mnt/mtd/protocol");
            system("cp /tmp/protocol /mnt/mtd/");
            sleep(1);
            system("chmod 777 /mnt/mtd/protocol");
            saveautoscestatus();
            byte logsend[128] = {0xf3,0x00};
            byte logbuf[64] = {"Host upgraded successfully"};
            logsend[2] = (3 + strlen(logbuf));
            memcpy(logsend + 3, logbuf, strlen(logbuf));
            sendsingle(client_fd, logsend, logsend[2]);
            sleep(2);
            pr_debug("升级重启程序\n");
            exit(0);
        }
    }
}

int pwverification(int sockfd,  byte *account)
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

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
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

int print_uptime(int sockfd)
{
    int n = 0;
    int i = 0;
    int heartlogo = 0;
    int pwerrcount = 0;
    int recvSize = 0;
    FILE *fp = NULL;
    byte recvBuf[BUFLEN] = {0};
    char buf[BUFLEN] = {0};
    byte password[50] = {0x01,0x00,0x24};
    byte revbuf[2048] = {0};

    byte verifypw[20] = {0x05,0x61,0x64,0x6d,0x69,0x6e,0x06,0x31,0x32,0x33,0x34,0x35,0x36};
    //获取本机序列号
    memcpy(password + 3, hostinfo.hostsn, 6);
    //pw
    memcpy(password + 9, verifypw, 13);
    //许可
    memcpy(password + 22, hostinfo.license, 8);
    
    //验证帐号密码
    if(send(sockfd, password, password[2], MSG_NOSIGNAL) == -1)
    {
        perror("fail to send datas.\n");
        exit(-1);
    }
    /*
    for(i = 0; i < password[2]; i++)
    {
        pr_debug("Server.password[%d]:%0x\n", i, password[i]);
    }
    while(1)
    {
        if((recvSize = recv(sockfd, recvBuf, BUFLEN, 0) == -1))
        {
            perror("fail to receive datas.\n");
            exit(-1);
        }
        for(i = 0; i < recvSize; i++)
        {
            pr_debug("Server.modsty[%d]:%0x\n", i, recvBuf[i]);
        }
        if(memcmp(recvBuf, verifypw, 9) != 0)
        {
            pwerrcount++;
            perror("密码或者用户名错误\n");
            if(pwerrcount == 2)
            {
                return -1;
            }
            sleep(1);
            continue;
        }
        else
        {
            break;
        }
    }

    pr_debug("密码验证成功\n");
    */
    int recv_num = 0;

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    //LED检查
    ledinitialization(0);
    
    //程序升级
    upgrade(sockfd);
    
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
                    close(client_fd);
                    client_fd = -1;
                    g_heartbeat = 0;
                    return -1;
                }
                //心跳
                heartlogo = sendheartbeat();
                if(heartlogo == -1)
                {
                    close(client_fd);
                    client_fd = -1;
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
                        pr_debug("default:%d\n", client_fd);
                        return -1;
                    }
                    else
                    {
                        revbuf[recv_num] = '\0';
                        pr_debug("recv_num:%d\n", recv_num);
                        if(revbuf[0] == 0x05)
                        {
                            //程序升级
                            pr_debug("程序升级\n");
                            versionvalidation(revbuf);

                            //从机升级
                            hosttoslave(0xff, revbuf, revbuf[2]);
                            sleep(3);
                        }
                        else if(revbuf[0] == 0x10)
                        {
                            //密码验证
                            pwverification(sockfd, revbuf);
                        }
                        else
                        {
                            new_fd = sockfd;
                            sockserver_run(recv_num, revbuf);
                        }
                        memset(revbuf, 0, sizeof(revbuf));
                    }
                }
        }
    }
    return 0;
}

void *sockclient_run(void *a)
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    struct sockaddr_in *sinp;
    int err;
    char seraddr[INET_ADDRSTRLEN] = {0};
    short serport = 0;

    char domainname[20] = "server.modsty.com";
    //char domainname[20] = "192.168.0.13";

    //记录时间
    time(&hostinfo.runtime);
    time(&hostinfo.freetime);

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    /*
       hint.ai_flags = AI_CANONNAME;
       hint.ai_protocol = 0;
       hint.ai_addrlen = 0;
       hint.ai_addr = NULL;
       hint.ai_canonname = NULL;
       hint.ai_next = NULL;
       if ((err = getaddrinfo(argv[1], "ruptime", &hint, &ailist)) != 0) 
       */
    while(1)
    {
        if ((err = getaddrinfo(domainname, "6001", &hint, &ailist)) != 0) 
        {
            pr_debug("getaddrinfo error: %s\n", gai_strerror(err));
            //exit(-1);
            //LED检查
            ledinitialization(1);
            sleep(30);
            continue;
        }
        else
        {
            pr_debug("getaddrinfo ok\n");
            break;
        }
    }
    
    aip = ailist;

    sinp = (struct sockaddr_in *)aip->ai_addr;
    if (inet_ntop(sinp->sin_family, &sinp->sin_addr, seraddr, INET_ADDRSTRLEN) != NULL)
    {
        pr_debug("server address is %s\n", seraddr);
    }
    serport = ntohs(sinp->sin_port);
    pr_debug("server port is %d\n", serport);

    while(1)
    {
        if ((client_fd = socket(aip->ai_family, SOCK_STREAM, 0)) < 0) 
        {
            pr_debug("create socket failed: %s\n", strerror(errno));
            ledinitialization(1);
            sleep(30);
            continue;
        }
        pr_debug("create socket ok\n");
        if (connect_retry(client_fd, aip->ai_addr, aip->ai_addrlen) < 0) 
        {
            pr_debug("can't connect to %s: %s\n", domainname, strerror(errno));
            ledinitialization(1);
            sleep(30);
            continue;
        } 
        else 
        {
            pr_debug("connect ok, client_fd:%d\n", client_fd);
            if (print_uptime(client_fd) < 0) 
            {
                pr_debug("print_uptime error: %s\n", strerror(errno));
                ledinitialization(1);
                sleep(30);
                continue;
            }
        }

        close(client_fd);
        pr_debug("close(client_fd):%d\n", client_fd);
        sleep(30);
    }
    return 0;
}
