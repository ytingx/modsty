#include "pchipsend.h"
#include "struct.h"

extern int master;
extern int single_fd;
extern int answer;
extern int new_fd;
extern int g_led;
extern byte ackswer[15];
extern struct chip_buffer *chip;

int slavechipuartsend(byte *sendbuf, int datalen)
{
    int slavemark = 0;    
    //从机标识
    //设置时钟-主机重启
    if((sendbuf[3] == 0x02) || (sendbuf[3] == 0x08))
    {
        slavemark = 0xff;
    }
    //红外控制设备
    //学习红外码
    else if((sendbuf[3] >= 0x04) && (sendbuf[3] <= 0x06))
    {
        slavemark = sendbuf[4];
    }
    else if(sendbuf[3] == 0x09)
    {
        slavemark = sendbuf[4];
    }
    else
    {
        slavemark = 0x01;
    }
    if(slavemark != 1)
    {
        /*
        int i = 0;
        for(i = 0; i < (datalen + 4); i++)
        {
            pr_debug("slavemark[%d]:%0x\n", i, slavemark[i]);
        }
        */
        //发送至从机
        hosttoslave(slavemark, sendbuf, datalen);
    }
    return slavemark;
}

int slavechipSend(int fd, byte *send_buf,int data_len)
{
    int len = 0;
    /*
    int i = 0;
    for(i = 0; i < data_len; i++)
    {
        pr_debug("UART0_chipSend_buf[%d]:%0x\n", i, send_buf[i]);
    }
    */
    len = write(fd, send_buf, data_len);

    //指令发送检查
    /*
    if(0 == memcmp(rele_check, send_buf, 7))
    {
        a_config = 0;
    }
    */
    /*
    time_t timep;
    byte time_buf[128] = {0};
    byte buf[256] = {0};
    time(&timep);
    strcpy(buf, ctime(&timep));
    HexToStr(time_buf, send_buf, data_len);
    strncat(buf, time_buf, data_len * 2);
    buf[strlen(buf)] = '\n';
    //pr_debug("uartlog:%s\n", buf);
    InsertLine("/data/modsty/uartlog", buf);
    memset(buf, 0, sizeof(buf));
    */
    if (len == data_len)
    {
        return len;
    }     
    else   
    {
        tcflush(fd,TCOFLUSH);
        return FALSE;
    }
}

int Chip_Send(int fd, byte *send_buf, int data_len)
{
    int mark = 0;
    int len = 0;
    int i = 0;
    for(i = 0; i < data_len; i++)
    {
        pr_debug("Chip_Send_buf[%d]:%0x\n", i, send_buf[i]);
    }
    mark = slavechipuartsend(send_buf, data_len);
    pr_debug("mark:%0x\n", mark);
    if((mark <= 1) || (mark == 0xff))
    {
        len = write(fd, send_buf, data_len);

        pr_debug("IRUART0_Sendlen:%d\n", len);
        if (len == data_len)
        {
            return len;
        }     
        else   
        {
            tcflush(fd,TCOFLUSH);
            return FALSE;
        }
    }
    /*
    time_t timep;
    byte buf[1200] = {0};
    byte time_buf[1200] = {0};
    time(&timep);
    strcpy(buf, ctime(&timep));
    HexToStr(time_buf, send_buf, data_len);
    strncat(buf, time_buf, data_len * 2);
    buf[strlen(buf)] = '\n';
    InsertLine("/data/modsty/iruartlog", buf);
    memset(buf, 0, sizeof(buf));
    */
}

void Chip_Split()
{
    byte buf[2500] = {0};
    int uart_logo = 0;
    int data_len = 0;
    int irlogo = 0;
    int chip_len = 0;

    int i = 0;
    int j = 0;
    int scenelen = 0;
    //取出缓冲区内所有数据
    pthread_mutex_lock(&chip->lock);
    chip_len = chip_get(buf, sizeof(buf));
    pthread_mutex_unlock(&chip->lock);
    //int struct_conut = 0;
    /*
    for(i = 0; i < chip_len; i++)
    {
        pr_debug("buf[%d]:%0x\n", i, buf[i]);
    }
    */
    if(chip_len > 0)
    {
        pr_debug("UartSplit_chip_len:%d\n", chip_len);
        int offset = 0;
        byte uartsend[512] = {0};
        //pr_debug("buf[10]:%0x\n", buf[10]);
        while(1)
        {
            if(0 == (buf[offset + 3]))
            {
                pr_debug("ChipSend信息处理完毕\n");
                break;
            }
            if(1 == uart_logo)
            {
                pr_debug("ChipSend信息处理完毕\n");
                break;
            }
            
            data_len = ((buf[offset + 1] << 8) + buf[offset + 2]);

            //pr_debug("UartSplit_chip_data_len:%d\n", data_len);
            //pr_debug("buf[%d]:%0x\n", offset, buf[offset]);
            //pr_debug("buf[%d]:%0x\n", offset + 1, buf[offset + 1]);
            switch(buf[offset + 3])
            {
                //主机Led/获取时钟/获取温湿度-6
                case 0x01:
                case 0x03:
                case 0x07:
                case 0x08:
                case 0x09:
                    memcpy(uartsend, buf + offset, data_len);
                    Chip_Send(single_fd, uartsend, data_len);
                    offset += data_len;
                    break;
                    //设置时钟-12
                case 0x02:
                    memcpy(uartsend, buf + offset, data_len);
                    Chip_Send(single_fd, uartsend, data_len);
                    offset += data_len;
                    break;
                    //控制红外设备-变长
                case 0x04:
                    memcpy(uartsend, buf + offset, data_len);
                    Chip_Send(single_fd, uartsend, data_len);
                    offset += data_len;
                    break;
                //进入红外学习模式-7
                case 0x05:
                case 0x06:
                    memcpy(uartsend, buf + offset, data_len);
                    Chip_Send(single_fd, uartsend, data_len);
                    offset += data_len;
                    break;
                default:
                    pr_debug("default:%0x\n", buf[offset + 3]);
                    for(i = 0; i < chip_len; i++)
                    {
                        pr_debug("default%0x\n", buf[i]);
                    }
                    uart_logo = 1;
                    pr_debug("UartSend不支持的类型\n");
                    break;

            }
            if(uartsend[3] == 0x04)
            {
                pr_debug("sleep(1)\n");
                sleep(1);
            }
            else
            {
                usleep(10000);
            }
        }
    }
}

void *chip_uart_run(void *arg)
{
    while(1)
    {
        //uart指令检查
        Chip_Split();
        usleep(600000);
        autosceresetregularly();
    }
}
