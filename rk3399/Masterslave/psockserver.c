#include "psockserver.h"
#include "struct.h"

extern int new_fd;
extern int fd_A[BACKLOG + 1];
extern int client_fd;
extern int g_heartbeat;

void print_time(char * ch,time_t *now)
{
    struct tm*stm;
    stm = localtime(now);//立即获取当前时间
    sprintf(ch, "%02d:%02d:%02d\n", stm->tm_hour, stm->tm_min, stm->tm_sec);
}

int MAX(int a,int b)
{
    if(a > b)
        return a;
    return b;
}

//主机发送数据至从机
//0:所有从机  非0:单台从机
int hosttoslave(int slavemark, byte *buffer, int len)
{
    int sendlen = (len + 4);
    byte hosttoslavesend[512] = {0xf6};
    //长度
    hosttoslavesend[1] = ((sendlen >> 8) & 0xff);
    hosttoslavesend[2] = (sendlen & 0xff);
    //从机标识
    hosttoslavesend[3] = slavemark;
    //拼接原始指令
    memcpy(hosttoslavesend + 4, buffer, len);

    int i = 0;
    
    for(i = 0; i < sendlen; i++)
    {
        pr_debug("hosttoslave[%d]:%0x\n", i, hosttoslavesend[i]);
    }

    if((slavemark == 0) || (slavemark == 0xff))
    {
        for(i = 2; i < 5; i++)
        {
            if(slaveident[i].sockfd != 0)
            {
                pr_debug("hosttoslave :%d,",slaveident[i].sockfd);
                if(sendlen != send(slaveident[i].sockfd, hosttoslavesend, sendlen, MSG_NOSIGNAL))
                {
                    pr_debug("发送指令至从机失败\n");
                    perror("send");
                    close(slaveident[i].sockfd);
                    slaveident[i].sockfd = 0;
                }
                else
                {
                    pr_debug("Success\n");
                }
            }
        }
    }
    else
    {
        if(slaveident[slavemark].sockfd != 0)
        {
            pr_debug("hosttoslave :%d,",slaveident[slavemark].sockfd);
            if(sendlen != send(slaveident[slavemark].sockfd, hosttoslavesend, sendlen, MSG_NOSIGNAL))
            {
                pr_debug("发送指令至从机失败\n");
                perror("send");
                close(slaveident[slavemark].sockfd);
                slaveident[slavemark].sockfd = 0;
            }
            else
            {
                pr_debug("Success\n");
            }
        }
    }
}

//send单一回复
int sendsingle(int fd, byte *buffer, int len)
{
    int i = 0;
    
    if(len != send(fd, buffer, len, MSG_NOSIGNAL))
    {
        perror("send");
        pr_debug("sendsingle 数据发送失败\n");
    }
    else
    {
        for(i = 0; i < len; i++)
        {
            pr_debug("sendsingle[%d]:%0x\n", i, buffer[i]);
        }
        return len;
    }
}

//串口上传信息至app
int UartToApp_Send(byte *buffer, int len)
{
    int j = 0;
    /*
    int g = 0;
    for(g = 0; g < len; g++)
    {
        pr_debug("App_send[%d]:%0x\n", g, buffer[g]);
    }
    */
    if(client_fd > 0)
    {
        if(len != send(client_fd, buffer, len, MSG_NOSIGNAL))
        {
            perror("send");
            close(client_fd);
            client_fd = -1;
        }
    }
    for(j = 0; j < MAX_CON_NO; j++)
    {
        //pr_debug("fd_A[%d]:%d\n", j, fd_A[j]);
        if(fd_A[j] != 0)
        {
            pr_debug("data send:%d,",fd_A[j]);
            //排除从机
            if(slaveident[2].sockfd == fd_A[j])
            {
                pr_debug("排除从机2:%d,",fd_A[j]);
            }
            else if(slaveident[3].sockfd == fd_A[j])
            {
                pr_debug("排除从机3:%d,",fd_A[j]);
            }
            else if(slaveident[4].sockfd == fd_A[j])
            {
                pr_debug("排除从机4:%d,",fd_A[j]);
            }
            else
            {
                if(len != send(fd_A[j], buffer, len, MSG_NOSIGNAL))
                {
                    pr_debug("串口上传信息至app失败\n");
                    perror("send");
                    close(fd_A[j]);
                    fd_A[j] = 0;
                    //return -1;
                }
                else
                {
                    pr_debug("Success\n");
                }
            }
        }
    }
    return 0;
}

int sockserver_run(int recvlen, byte *sockbuf)
{
    int i;
    int offset = 0;
    byte revbuf[1024] = {0};

    pr_debug("recvlen:%d\n", recvlen);
    while(offset < recvlen)
    {			
        memset(revbuf, 0, sizeof(revbuf));
        memcpy(revbuf, sockbuf + offset, ((sockbuf[offset + 1] << 8) + sockbuf[offset + 2]));
        offset += ((sockbuf[offset + 1] << 8) + sockbuf[offset + 2]);
        
        pr_debug("offset:%d\n", offset);
        
        for(i = 0; i < ((revbuf[1] << 8) + revbuf[2]); i++)
        {
            pr_debug( "Receives app data[%d]:%0x\n" , i, revbuf[i]);
        }
        
        switch(revbuf[0])
        {
            case 0x08:
                pr_debug("云mesh升级应答\n");
                meshupmd5check(revbuf);
                break;
            case 0x11:
                pr_debug("修改密码\n");
                changepassword(revbuf);
                break;
            case 0x12:
                pr_debug("绑定主机\n");
                bindhost(revbuf);
                break;
            case 0x13:
                pr_debug("强制绑定主机\n");
                forcedbindhost(revbuf);
                break;
            case 0x14:
                pr_debug("解绑主机\n");
                unbindhost(revbuf);
                break;
            case 0x15:
                pr_debug("添加子帐号\n");
                addaubaccount(revbuf);
                break;
            case 0x16:
                pr_debug("删除子帐号\n");
                deletesubaccount(revbuf);
                break;
            case 0x17:
                pr_debug("获取子帐号信息\n");
                getsubaccount(revbuf);
                break;
            case 0x20:
                pr_debug("获取主机版本号\n");
                obtainshvers(revbuf);
                break;
            case 0x21:
                pr_debug("配置IP地址\n");
                networkconfig(revbuf);
                break;
            case 0x22:
                pr_debug("同步时间\n");
                appsettime(revbuf);
                break;
            case 0x23:
                pr_debug("组网\n");
                networking(revbuf);
                break;
            case 0x25:
                pr_debug("wifi config\n");
                wificonfig(revbuf);
                break;
            case 0x26:
                pr_debug("从机功能\n");
                slavefunction(revbuf);
                break;
            case 0x27:
                pr_debug("获取主从机网络-时间信息");
                getnetworkinfo(revbuf);
                break;
            case 0x29:
                pr_debug("获取主机macsn\n");
                appgetmacsn(revbuf);
                break;
            case 0x2a:
                pr_debug("设置主机macsn\n");
                modifymacsn(revbuf);
                break;
            case 0x30:
                pr_debug("重命名\n");
                namingcollection(revbuf);
                break;
            case 0x31:
                pr_debug("数据同步\n");
                node_up(revbuf);
                break;
            case 0x35:
                pr_debug("mesh升级\n");
                appmeshupgrade(revbuf);
                break;
            case 0x40:
                pr_debug("节点注册信息查询\n");
                singlenodeinfo(revbuf);
                break;
            case 0x42:
                pr_debug("删除节点\n");
                delnodedev(revbuf);
                break;
            case 0x43:
                pr_debug("本地关联\n");
                localassociation(revbuf);
                break;
            case 0x44:
                pr_debug("获取自动场景状态\n");
                getautoscestatus(revbuf);
                break;
            case 0x45:
                pr_debug("智能锁功能\n");
                smartlockmanage(revbuf);
                break;
            case 0x46:
                pr_debug("关联管理\n");
                relatedmanage(revbuf);
                break;
            case 0x47:
                pr_debug("智能控制面板配置\n");
                smartcontrolpanel(revbuf);
                break;
            case 0x50://手动场景配置
                manualscenario(revbuf);
                break;
            case 0x52:
                pr_debug("删除手动场景\n");
                delmanualscenario(revbuf);
                break;
            case 0x53:
                pr_debug("自动场景配置\n");
                autoscene(revbuf);
                break;
            case 0x54:
                pr_debug("删除自动场景\n");
                delautoscene(revbuf);
                break;
            case 0x55:
                pr_debug("区域配置\n");
                regassociation(revbuf);
                break;
            case 0x57:
                pr_debug("删除区域\n");
                appdelarea(revbuf);
                break;
            case 0x58:
                pr_debug("完整红外码发送\n");
                completeirsend(revbuf);
                break;
            case 0x59:
                pr_debug("配置红外设备\n");
                configirdev(revbuf);
                break;
            case 0x5a:
                pr_debug("删除红外设备\n");
                delirdev(revbuf);
                break;
            case 0x5b:
                pr_debug("IR-一键匹配\n");
                irmatch(revbuf);
                break;
            case 0x5c:
                pr_debug("IR-智能学习\n");
                irlntelearn(revbuf);
                break;
            case 0x5d:
                pr_debug("添加红外快捷操作\n");
                irquickopera(revbuf);
                break;
            case 0x5e:
                pr_debug("删除红外快捷\n");
                delirquick(revbuf);
                break;
            /*
            case 0x5f:
                pr_debug("自动场景使能\n");
                autoscene_enable(revbuf);
                break;
                */
            case 0x60:
                pr_debug("单指令控制\n");
                appsw(revbuf);
                break;
            case 0x61:
                pr_debug("自动场景使能\n");
                autoscene_enable(revbuf);
                break;
            case 0x62:
                pr_debug("手动场景执行\n");
                manualsceope(revbuf);
                break;
            case 0x63:
                pr_debug("区域执行\n");
                regoperation(revbuf);
                break;
            case 0x65:
                pr_debug("执行红外快捷操作\n");
                runirquick(revbuf);
                break;
            case 0xf1:
                pr_debug("app心跳\n");
                appheartbeat();
                break;
            case 0x70:
                pr_debug("配置安防\n");
                securityconfig(revbuf);
                break;
            case 0x71:
                pr_debug("执行安防动作\n");
                performsecactions(revbuf);
                break;
            case 0xf2:
                pr_debug("云心跳应答\n");
                g_heartbeat = 0;
                break;
            case 0xf6:
                pr_debug("主机下发至从机指令\n");
                slavereceinstruct(revbuf);
                break;
            case 0xf7:
                pr_debug("从机上传至主机指令\n");
                slaveinproc(revbuf);
                break;
            case 0xf8:
                pr_debug("从机上传至主机透传指令\n");
                slavepasscomm(revbuf);
                break;
            default:
                pr_debug("psockserver no type...\n");
                return -1;
        }
    }
    return 0;
}

struct user
{
    byte name[32];
    byte pwd[32];
};

/* ################### network seting value ############### */
void *sock_run(void *a)
{
    int sendSize = 0;
    int recvSize = 0;
    int login = -1;
    int sockfd = 0;
    int clientfd = 0;
    
    //byte sendBuf[MAX_DATA_SIZE] = {0};
    byte revbuf[2048] = {0};
    //账户操作
    byte account[128] = {0};
    //存放帐号密码
    byte account_pw[64] = {0};
    byte account_pw_str[128] = {0};
    byte account_pw_temp[128] = {0};
    //本机mac
    byte account_mac[10] = {0};
    //应答帐号登陆指令
    byte account_answer[64] = {0x80};
    
    struct sockaddr_in serverSockaddr;
    struct sockaddr_in clientSockaddr;
    
    fd_set servfd, recvfd;//用于select处理用的
    //int fd_client[BACKLOG + 1] = {0};
    byte fd_C[BACKLOG + 1][32];//用于保存客户端的用户名
    //用于计算客户端个数
    int conn_amount = 0;
    
    int max_servfd = 0;
    int max_recvfd = 0;
    int s_on = 1;
    
    socklen_t sinSize = 0;
    byte ch[64] = {0};
    int g_i = 0;
    int j = 0;
    struct timeval timeout;
    struct timeval recvtimeout = {3,0};
    struct user use;
    time_t now; struct tm *timenow;

    //establish a socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("fail to establish a socket");
        exit(1);
    }
    pr_debug("Success to establish a socket...\n");

    //init sockaddr_in
    serverSockaddr.sin_family = AF_INET;
    serverSockaddr.sin_port = htons(SERVER_PORT);
    serverSockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverSockaddr.sin_zero, 0, sizeof(serverSockaddr.sin_zero));

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &s_on, sizeof(s_on));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&recvtimeout, sizeof(struct timeval));

    //bind socket
    if(bind(sockfd, (struct sockaddr *)&serverSockaddr, sizeof(struct sockaddr)) == -1)
    {
        perror("fail to bind");
        exit(1);
    }
    pr_debug("Success to bind the socket...\n");

    //listen on the socket
    if(listen(sockfd, BACKLOG) == -1)
    {
        perror("fail to listen");
        exit(1);
    }

    time(&now);
    timeout.tv_sec = 0;//1秒遍历一遍
    timeout.tv_usec = 1000;
    sinSize = sizeof(clientSockaddr);//注意要写上，否则获取不了IP和端口

    FD_ZERO(&servfd);//清空所有server的fd
    FD_ZERO(&recvfd);//清空所有client的fd
    FD_SET(sockfd, &servfd);
    max_servfd = sockfd;
    max_recvfd = 0;
    //memset(fd_A, 0, sizeof(fd_A));
    memset(fd_C, 0, sizeof(fd_C));

    while(1)
    {
        FD_ZERO(&servfd);//清空所有server的fd
        FD_ZERO(&recvfd);//清空所有client的fd
        FD_SET(sockfd, &servfd);
        //timeout.tv_sec = 10;//可以减少判断的次数
        //timeout.tv_usec = 6000;
        timeout.tv_usec = 100000;
        switch(select(max_servfd + 1, &servfd, NULL, NULL, &timeout))
        {
            case -1:
                perror("max_servfd_select error");
                break;
            case 0:
                //pr_debug("time out!\n");
                break;
            default:
                pr_debug("has datas to offer accept\n");
                if(FD_ISSET(sockfd, &servfd))//sockfd 有数据表示可以进行accept
                {
                    //accept a client's request
                    if((clientfd = accept(sockfd, (struct sockaddr *)&clientSockaddr, &sinSize)) == -1)
                    {
                        perror("fail to accept");
                        exit(1);
                    }
                    pr_debug("Success to accpet a connection request...\n");
                    pr_debug(">>>>>> %s:%d join in! ID(fd):%d \n", inet_ntoa(clientSockaddr.sin_addr), ntohs(clientSockaddr.sin_port), clientfd);
                    print_time(ch, &now);
                    pr_debug("The time of joining is:%s\n", ch);
                    
                    //if((recvSize = recv(clientfd, account, sizeof(account), MSG_WAITALL)) == -1 || recvSize == 0)
                    if(((recvSize = recv(clientfd, account, sizeof(account), 0)) == -1) || (recvSize == 0))
                    {
                        perror("fail to receive datas");
                        close(clientfd);
                    }
                    else
                    {
                        int ig = 0;
                        for(ig = 0; ig < ((account[1] << 4 ) + account[2]); ig++)
                        {
                            pr_debug("帐号操作[%d]:%0x\n", ig, account[ig]);
                        }
                        //检查是否登陆
                        if(account[0] == 0x10)
                        {
                            login = pwverification(clientfd, account);
                        }
                        else if((account[0] == 0xf5) && (account[3] == 0x01))
                        {
                            login = slavelogincheck(clientfd, account);
                            pr_debug("从机登录:%d\n", login);
                        }
                        else
                        {
                            pr_debug("连接后-未登录:%d\n", clientfd);
                            close(clientfd);
                        }
                        memset(revbuf, 0, sizeof(revbuf));
                        if(login == 0)
                        {
                            //每加入一个客户端都向fd_set写入
                            int fd_alen = 0;
                            for(fd_alen = 0; fd_alen < MAX_CON_NO; fd_alen++)
                            {
                                if(fd_A[fd_alen] == 0)
                                {
                                    fd_A[fd_alen] = clientfd;
                                    break;
                                }
                            }
                            max_recvfd = MAX(max_recvfd, clientfd);
                        }
                        else
                        {
                            close(clientfd);
                        }
                    }
                    pr_debug("阻塞完成:%d\n", recvSize);
                }
                break;
        }
        //FD_COPY(recvfd,servfd);
        //app
        for(g_i = 0; g_i < MAX_CON_NO; g_i++)
        {
            if(fd_A[g_i] != 0)
            {
                FD_SET(fd_A[g_i], &recvfd);
            }
        }
        switch(select(max_recvfd + 1, &recvfd, NULL, NULL, &timeout))
        {
            case -1:
                //select error
                perror("max_recvfd_select error");
                break;
            case 0:
                //timeout
                //pr_debug("time out!\n");
                break;
            default:
                for(g_i = 0; g_i < MAX_CON_NO; g_i++)
                {
                    if(FD_ISSET(fd_A[g_i], &recvfd))
                    {
                        //receive datas from client
                        if(((recvSize = recv(fd_A[g_i], revbuf, 2048, 0)) == -1) || (recvSize == 0))
                        {
                            perror("fail to receive datas");
                            close(fd_A[g_i]);
                            //表示该client是关闭的
                            pr_debug("fd %d close\n", fd_A[g_i]);
                            FD_CLR(fd_A[g_i], &recvfd);
                            fd_A[g_i] = 0;
                        }
                        else//客户端发送数据过来
                        {
                            memset(&revbuf[recvSize], '\0', 1);
                            pr_debug("recvSize:%d\n", recvSize);
                            /*
                            int igv = 0;
                            for(igv = 0; igv < recvSize; igv++)
                            {
                                pr_debug("revbuf[%d]:%0x\n", igv, revbuf[igv]);
                            }
                            */
                            //获取当前连接的描述符
                            new_fd = fd_A[g_i];
                            sockserver_run(recvSize, revbuf);
                            //可以判断recvBuf是否为bye来判断是否可以close
                            memset(revbuf, 0, sizeof(revbuf));
                        }
                    }
                }
                break;
        }
    }
    return 0;
}
