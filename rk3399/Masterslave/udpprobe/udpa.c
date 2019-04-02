#include<stdio.h>
#include<sys/socket.h>
#include<unistd.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
int main()
{
    setvbuf(stdout,NULL,_IONBF,0);
    fflush(stdout);
    int sock=-1;
    if((sock=socket(AF_INET,SOCK_DGRAM,0))==-1)
    {
        perror("socket");
        return -1;
    }
    const int opt=-1;
    int nb=0;
    nb=setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt));//设置套接字类型
    if(nb==-1)
    {
        //cout<<"set socket error...\n"<<endl;
        return -1;
    }
    struct sockaddr_in addrto;
    bzero(&addrto,sizeof(struct sockaddr_in));
    addrto.sin_family=AF_INET;
    addrto.sin_addr.s_addr=htonl(INADDR_BROADCAST);//套接字地址为广播地址
    addrto.sin_port=htons(9999);//套接字广播端口号为9999
    int nlen=sizeof(addrto);
    unsigned char msg[64]={0x60};
    /*
    while(1)
    {
        sleep(5);
        int ret=sendto(sock, msg, 12, 0, (struct sockaddr*)&addrto, nlen);//向广播地址发布消息
        if(ret < 0)
        {
            //cout<<"send error...\n"<<endl;
            return -1;
        }
        else 
        {
            printf("ok\n");
        }
    }
    */
    int ret=sendto(sock, msg, 12, 0, (struct sockaddr*)&addrto, nlen);//向广播地址发布消息
    if(ret < 0)
    {
        //cout<<"send error...\n"<<endl;
        return -1;
    }
    else 
    {
        printf("sendto ok\n");
    }
    int count = 0;
    unsigned char buffer[128] = {0};
    count = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&addrto, &nlen);

    printf("Server IP is%s\n",inet_ntoa(addrto.sin_addr));
    printf("Server Send Port:%d\n",ntohs(addrto.sin_port));
    printf("count:%d\n", count);
    printf("buffer:%s\n", buffer);
    sleep(5);
    return 0;
}













