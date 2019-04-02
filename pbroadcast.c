#include "pbroadcast.h"
#include "pautosce.h"
#include "struct.h"

extern int client_fd;
extern byte  dhcp_s[10];

void modifynetwork(void)
{
    system("sh /mnt/mtd/ipaddr.sh");
    byte dhcp_str[10] = {"dhcp0"};
    memcpy(dhcp_s, dhcp_str, 5);
    replace(HOSTSTATUS, dhcp_str, 4);
}

int broadmachine(byte *buf, byte *message)
{
    pr_debug("Broadcast from the machine\n");
    int i = 0;
    byte slavestr[64] = {0};
    byte slavehex[32] = {0};

    for(i = 0; i < 18; i++)
    {
        pr_debug("broadmachine[%d]:%0x\n", i, buf[i]);
    }

    //已被添加从机
    if(buf[10] > 1)
    {
        if(0 == memcmp(hostinfo.hostsn, buf + 11, 6))
        {
            if(slaveident[buf[10]].assignid == buf[10])
            {
                pr_debug("广播-从机验证通过\n");
                memcpy(message, buf, buf[2]);
                return 0;
            }
        }
    }
    //新从机
    else if(buf[10] == 0)
    {
        //hostinfo.receslave = 1;
        //允许添加
        if(hostinfo.receslave == 1)
        {
            for(i = 2; i < 5; i++)
            {
                if(slaveident[i].assignid == 0)
                {
                    slaveident[i].assignid = i;
                    memcpy(slaveident[i].slavesn, buf + 4, 6);
                    memcpy(slaveident[i].mastersn, buf + 11, 7);
                    slaveident[i].slavesn[6] = i;

                    memcpy(slavehex, buf + 4, 6);
                    slavehex[6] = i;
                    memcpy(slavehex + 7, hostinfo.hostsn, 7);
                    HexToStr(slavestr, slavehex, 14);
                    replace(MASSLAINFO, slavestr, 12);

                    int g = 1;
                    for(g = 0; g < 7; g++)
                    {
                        pr_debug("hostinfo.hostsn[%d]:%0x\n", g, hostinfo.hostsn[g]);
                    }
                    for(g = 0; g < 14; g++)
                    {
                        pr_debug("slavehex[%d]:%0x\n", g, slavehex[g]);
                    }
                    
                    memcpy(message, buf, 4);
                    memcpy(message + 4, slavehex, 14);
                    for(g = 0; g < 18; g++)
                    {
                        pr_debug("message[%d]:%0x\n", g, message[g]);
                    }
                    return 0;
                }
            }
        }
    }
    return -1;
}

void *bcastserver_run(void *a)
{
    int ret = -1; 
    int sock = -1;

    struct sockaddr_in server_addr;
    struct sockaddr_in from_addr;
    struct sockaddr_in BastAddr;
    int from_len = sizeof(struct sockaddr_in);
    int bOptVal = 1;

    memset(&from_addr, 0, from_len);
    memset(&BastAddr, 0, from_len);
    memset(&server_addr, 0, from_len);

    BastAddr.sin_family = AF_INET;
    BastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    int count = -1;
    fd_set readfd;
    byte buffer[60] = {0};
    byte bufs[10] = {0x42};
    byte local_mac[10] = {0};
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("sock error");
        return;
    }

    memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bOptVal, sizeof(bOptVal));

    ret = bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0)
    {
        perror("bind error");
        return;
    }

    get_mac(local_mac);
    while(1)
    {
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        FD_ZERO(&readfd);
        FD_SET(sock, &readfd);

        ret = select(sock + 1, &readfd, NULL, NULL, &timeout);
        //pr_debug("ret=%d\n",ret);
        switch(ret)
        {
            case -1:
                break;
            case 0:
                //pr_debug("broadcast timeout\n");
                break;
            default:
                if(FD_ISSET(sock, &readfd))
                {
                    count = recvfrom(sock, buffer, sizeof(buffer), 0, \
                            (struct sockaddr*)&from_addr, &from_len);

                    if(buffer[0] == IP_FOUND)
                    {
                        //pr_debug("Client IP is%s\n",inet_ntoa(from_addr.sin_addr));

                        //pr_debug("Client Send Port:%d\n",ntohs(from_addr.sin_port));
                        //memcpy(&buffer,IP_FOUND_ACK,sizeof(IP_FOUND_ACK)+1);
                        BastAddr.sin_port = from_addr.sin_port;
                        //BastAddr.sin_addr = from_addr.sin_addr;
                        //pr_debug("UdpSend IP is%s\n",inet_ntoa(BastAddr.sin_addr));
                        //pr_debug("UdpSend Port:%d\n",ntohs(BastAddr.sin_port));
                        memcpy(bufs + 1, hostinfo.hostsn, 6);
                        count = sendto(sock, &bufs, 7, 0, \
                                (struct sockaddr*)&BastAddr, from_len);
                        if(-1 == count)
                        {
                            pr_debug("sendto: %s\n", strerror(errno));
                        }
                        //pr_debug("count:%d, bufs:%c\n", count, bufs);
                    }
                    else if(buffer[0] == 0x60)
                    {
                        int result = 0;
                        byte bromessage[24] = {0};
                        result = broadmachine(buffer, bromessage);

                        if(result == 0)
                        {
                            BastAddr.sin_port = from_addr.sin_port;
                            count = sendto(sock, &bromessage, bromessage[2], 0, \
                                    (struct sockaddr*)&BastAddr, from_len);
                            if(-1 == count)
                            {
                                pr_debug("sendto: %s\n", strerror(errno));
                            }
                        }
                        pr_debug("count:%d, result:%d\n", count, result);
                    }
                    else if(buffer[0] == 0x34)
                    {	
                        byte ip_addr[256] = {0};
                        byte recv_pw[20] = {0};
                        byte recv_mac[10] = {0};
                        byte recv_ip[20] = {0};
                        byte recv_pw_str[40] = {0};

                        int org = 0;
                        for(org = 0; org < 6; org++)
                        {
                            pr_debug("local_mac[%d]:%0x\n", org, local_mac[org]);
                        }
                        memcpy(recv_mac, buffer + (buffer[1] - 22), 6);

                        if(0 != (memcmp(local_mac, recv_mac, 6)))
                        {
                            break;
                        }

                        memcpy(recv_pw, buffer + 2, buffer[2] + buffer[buffer[2] + 3] + 2);
                        HexToStr(recv_pw_str, recv_pw, buffer[2] + buffer[buffer[2] + 3] + 2);
                        /*
                        if(contains(recv_pw_str, strlen(recv_pw_str), strlen(recv_pw_str), INITIALPW))
                        {
                            //byte ipsend[15] = {0x35, 0x0d};
                            break;
                        }
                        */
                        pr_debug("recv_pw_str:%s\n", recv_pw_str);
                        FILE *fd = NULL;
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
                        memcpy(recv_ip, buffer + (buffer[1] - 16), 16);
                        snprintf(ip_addr, sizeof(ip_addr), "ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d\nroute add default gw %d.%d.%d.%d\necho \"nameserver %d.%d.%d.%d \">> /etc/resolv.conf\n", recv_ip[0],recv_ip[1],recv_ip[2],recv_ip[3], recv_ip[4], recv_ip[5], recv_ip[6], recv_ip[7], recv_ip[8], recv_ip[9], recv_ip[10], recv_ip[11], recv_ip[12], recv_ip[13], recv_ip[14], recv_ip[15]);
                        int fplen = 0;
                        fplen = fprintf(fd, "%s", ip_addr);
                        pr_debug("fplen:%d\n", fplen);
                        fclose(fd);
                        fd = NULL;

                        modifynetwork();
                        //pr_debug("%s\n", ip_addr);
                        //pr_debug("strlen(ip_addr):%d\n", strlen(ip_addr));
                    }
                }
        } //end switch 
    } //end while
}
