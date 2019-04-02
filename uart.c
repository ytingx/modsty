#include "uart.h"
extern struct cycle_buffer *fifo;
extern struct cycle_buffer *chip;
extern int uart_fd;
extern int single_fd;

/*******************************************************************
 * 名称：                  UART0_Open
 * 功能：                打开串口并返回串口设备文件描述
 * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)
 * 出口参数：        正确返回为1，错误返回为0
 *******************************************************************/
int UART0_Open(int fd,char* port)
{

    fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);
    if (FALSE == fd)
    {
        perror("Can't Open Serial Port");
        return(FALSE);
    }
    /*
    //恢复串口为阻塞状态                               
    if(fcntl(fd, F_SETFL, 0) < 0)
    {
        pr_debug("fcntl failed!\n");
        return(FALSE);
    }     
    else
    {
        pr_debug("fcntl=%d\n",fcntl(fd, F_SETFL,0));
    }
    //测试是否为终端设备    
    if(0 == isatty(STDIN_FILENO))
    {
        pr_debug("standard input is not a terminal device\n");
        return(FALSE);
    }
    else
    {
        pr_debug("isatty success!\n");
    }
    */
    pr_debug("fd->open=%d\n",fd);
    return fd;
}
/*******************************************************************
 * 名称：                UART0_Close
 * 功能：                关闭串口并返回串口设备文件描述
 * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)
 * 出口参数：        void
 *******************************************************************/

void UART0_Close(int fd)
{
    close(fd);
}

/*******************************************************************
 * 名称：                UART0_Set
 * 功能：                设置串口数据位，停止位和效验位
 * 入口参数：        fd        串口文件描述符
 *                              speed     串口速度
 *                              flow_ctrl   数据流控制
 *                           databits   数据位   取值为 7 或者8
 *                           stopbits   停止位   取值为 1 或者2
 *                           parity     效验类型 取值为N,E,O,,S
 *出口参数：          正确返回为1，错误返回为0
 *******************************************************************/
int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{

    int   i;
    int   status;
    int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};

    struct termios options;

    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.
    */
    if(tcgetattr(fd, &options) != 0)
    {
        perror("SetupSerial 1");    
        //exit(-1);
        return(FALSE); 
    }

    //设置串口输入波特率和输出波特率
    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
    {
        if  (speed == name_arr[i])
        {             
            cfsetispeed(&options, speed_arr[i]); 
            cfsetospeed(&options, speed_arr[i]);  
        }
    }     

    //修改控制模式，保证程序不会占用串口
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;

    //设置数据流控制
    switch(flow_ctrl)
    {

        case 0 ://不使用流控制
            options.c_cflag &= ~CRTSCTS;
            break;   

        case 1 ://使用硬件流控制
            options.c_cflag |= CRTSCTS;
            break;
        case 2 ://使用软件流控制
            options.c_cflag |= IXON | IXOFF | IXANY;
            break;
    }
    //设置数据位
    //屏蔽其他标志位
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {  
        case 5    :
            options.c_cflag |= CS5;
            break;
        case 6    :
            options.c_cflag |= CS6;
            break;
        case 7    :    
            options.c_cflag |= CS7;
            break;
        case 8:    
            options.c_cflag |= CS8;
            break;  
        default:   
            fprintf(stderr,"Unsupported data size\n");
            return (FALSE); 
    }
    //设置校验位
    switch (parity)
    {  
        case 'n':
        case 'N': //无奇偶校验位。
            options.c_cflag &= ~PARENB; 
            options.c_iflag &= ~INPCK;    
            break; 
        case 'o':  
        case 'O'://设置为奇校验    
            options.c_cflag |= (PARODD | PARENB); 
            options.c_iflag |= INPCK;             
            break; 
        case 'e': 
        case 'E'://设置为偶校验  
            options.c_cflag |= PARENB;       
            options.c_cflag &= ~PARODD;       
            options.c_iflag |= INPCK;      
            break;
        case 's':
        case 'S': //设置为空格 
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break; 
        default:  
            fprintf(stderr,"Unsupported parity\n");    
            return (FALSE); 
    } 
    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // 设置停止位 
    switch (stopbits)
    {  
        case 1:   
            options.c_cflag &= ~CSTOPB; break; 
        case 2:   
            options.c_cflag |= CSTOPB; break;
        default:   
            fprintf(stderr,"Unsupported stop bits\n"); 
            return (FALSE);
    }

    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//我加的
    //options.c_lflag &= ~(ISIG | ICANON);

    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 0; /* 读取一个字符等待1*(1/10)s */  
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
    //tcflush(fd,TCIFLUSH);

    //激活配置 (将修改后的termios数据设置到串口中）
    if (tcsetattr(fd,TCSANOW,&options) != 0)  
    {
        perror("com set error!\n");  
        return (FALSE); 
    }
    return (TRUE); 
}
/*******************************************************************
 * 名称：                UART0_Init()
 * 功能：                串口初始化
 * 入口参数：        fd       :  文件描述符   
 *               speed  :  串口速度
 *                              flow_ctrl  数据流控制
 *               databits   数据位   取值为 7 或者8
 *                           stopbits   停止位   取值为 1 或者2
 *                           parity     效验类型 取值为N,E,O,,S
 *                      
 * 出口参数：        正确返回为1，错误返回为0
 *******************************************************************/
int UART0_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
    int err;
    //设置串口数据帧格式
    if (UART0_Set(fd,115200,0,8,1,'N') == FALSE)
    {                                                         
        return FALSE;
    }
    else
    {
        return  TRUE;
    }
}

/*******************************************************************
 * 名称：                  UART0_Recv
 * 功能：                接收串口数据
 * 入口参数：        fd                  :文件描述符    
 *                              rcv_buf     :接收串口中数据存入rcv_buf缓冲区中
 *                              data_len    :一帧数据的长度
 * 出口参数：        正确返回为1，错误返回为0
 *******************************************************************/
int UART0_Recv(int fd, byte *rc_buf, int data_len, int *dat_len)
{
    int i = 2;
    int len = 0;
    unsigned loop = 1;
    int maxufd = 0;
    int temp_len = 0;
    int fs_sel = 0;

    fd_set fs_read;
    fd_set tmp_inset;
    
    int fds[2] = {0};
    byte r_buf[1024] = {0};
    byte temp_buf[1024] = {0};

    struct timeval time;
    
    FD_ZERO(&fs_read);
    //FD_SET(fd, &fs_read);
    FD_SET(uart_fd, &fs_read);
    FD_SET(single_fd, &fs_read);
    fds[0] = uart_fd;
    fds[1] = single_fd;

    maxufd = (fds[0] > fds[1]) ? fds[0] : fds[1];

    time.tv_sec = 10;
    time.tv_usec = 0;

    if(loop && (FD_ISSET(fds[0], &fs_read) || FD_ISSET(fds[1], &fs_read)))
    {
        tmp_inset = fs_read;

        //使用select实现串口的多路通信
        fs_sel = select(maxufd + 1, &tmp_inset, NULL, NULL, &time);
        //fs_sel = select(fd + 1, &fs_read, NULL, NULL, &time);

        //int read_count = 0;
        if(fs_sel)
        {
            pr_debug("fs_sel:%d\n", fs_sel);
            for(i = 0; i < 2; i++)
            {
                if(FD_ISSET(fds[i], &tmp_inset))
                {
                    while((temp_len = read(fds[i], temp_buf, data_len)) > 0)
                    {
                        memcpy(r_buf + len, temp_buf, temp_len);
                        memset(temp_buf, 0, sizeof(temp_buf));
                        len += temp_len;
                        temp_len = 0;
                        //read_count++;
                        //usleep(850);//GM8135s
                        usleep(3000);
                    }
                    //pr_debug("uart_recv:%d\n", len);
                    *dat_len = len;
                    memcpy(rc_buf, r_buf, len);
                    return len;
                }
            }
        }
        else
        {
            pr_debug("No data received!\n");
            return FALSE;
        }
    }
}
/********************************************************************
 * 名称：                  UART0_Send
 * 功能：                发送数据
 * 入口参数：        fd                  :文件描述符    
 *                              send_buf    :存放串口发送数据
 *                              data_len    :一帧数据的个数
 * 出口参数：        正确返回为1，错误返回为0
 *******************************************************************/
int UART0_Send(int fd, byte *send_buf, int data_len)
{
    int i = 0;
    int len = 0;
    //crc
    byte add = 0x00;
    //数据头
    byte head_buf[100] = {0};
    //数据长度
    head_buf[1] = data_len;
    for(i = 0; i < data_len - 1; i++)
    {
        add += send_buf[i];
    }
    memcpy(head_buf, send_buf, data_len);
    head_buf[data_len - 1] = add;
    //pr_debug("add:%0x, head_buf[data_len]:%d\n", add, head_buf[data_len]);
    /*
    for(i = 0; i < data_len; i++)
    {
        pr_debug("fifo_put[%d]:%0x\n", i, head_buf[i]);
    }
    */
    pthread_mutex_lock(&fifo->lock);
    fifo_put(head_buf, data_len);
    pthread_mutex_unlock(&fifo->lock);
    /*
    pr_debug("(1)fifo->in:%d\n", fifo->in);
    for(i = 0; i < (data_len + 3); i++)
    {
        pr_debug("fifo_put[%d]:%0x\n", i, head_buf[i]);
    }
    */
    
    return data_len;
}

int IRUART0_Send(int fd, byte *send_buf, int data_len)
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

    /*
    for(i = 0; i < data_len; i++)
    {
        pr_debug("chip_put[%d]:%0x\n", i, head_buf[i]);
    }
    */
    pthread_mutex_lock(&chip->lock);
    chip_put(head_buf, data_len);
    pthread_mutex_unlock(&chip->lock);
    pr_debug("(1)chip->in:%d\n", chip->in);
    
    return data_len;
}
