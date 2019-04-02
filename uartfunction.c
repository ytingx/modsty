#include "uartfunction.h"

//获取mac地址
int get_mac(char *mac)
{
    struct ifreq ifreq;
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, "eth0");    //Currently, only get eth0

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        close(sock);
        return -1;
    }
    
    memcpy(mac, ifreq.ifr_hwaddr.sa_data, 6);

    close(sock);
    return 0;
}

//字符串转16进制
void StrToHex(byte *pbDest, byte *pbSrc, int nLen)
{
    char h1,h2;
    byte s1,s2;
    int i;

    for (i=0; i<nLen; i++)
    {
        h1 = pbSrc[2*i];
        h2 = pbSrc[2*i+1];

        s1 = toupper(h1) - 0x30;
        if (s1 > 9) 
            s1 -= 7;

        s2 = toupper(h2) - 0x30;
        if (s2 > 9) 
            s2 -= 7;

        pbDest[i] = s1*16 + s2;
    }
}

//16进制转字符串
void HexToStr(byte *pbDest, byte *pbSrc, int nLen)
{
    char ddl,ddh;
    int i;

    for (i=0; i<nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        pbDest[i*2] = ddh;
        pbDest[i*2+1] = ddl;
    }
    pbDest[nLen*2] = '\0';
}
