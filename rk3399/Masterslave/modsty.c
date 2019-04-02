#include "modstyhead.h"
#include "pautosce.h"
#include "pbroadcast.h"
#include "psockclient.h"
#include "pslavesockclient.h"
#include "psockserver.h"
#include "puartread.h"
#include "pslaveuartread.h"
#include "puartsend.h"
#include "pchipsend.h"
#include "buffer.h"
#include "pledcheck.h"
#include "securitymonitor.h"
#include "struct.h"

const byte softwareversion[10] = {0x04,0x09,0x03,0xe8};

//主机标识
int master = 0x01;
int new_fd = -1;
int uart_fd = -1;
int single_fd = -1;
//int g_bindhost = 0;
//心跳
int g_heartbeat = 0;
//mesh组网标识
int g_mesh = 0;
int slavesockfd = -1;
int client_fd = -1;
int gpio_fd = -1;
int gpio_ledfd = -1;

struct cycle_buffer *fifo = NULL;
struct chip_buffer *chip = NULL;

int readlogo = 0;

//自动场景调用
byte  g_autotime[BUF_SIZE] = {0};
byte  relevance_temp[10] = {0};
byte  dhcp_s[10] = {0};

int fd_A[BACKLOG + 1] = {0};//保存客户端的socket描述符

//led校验
int g_led_on = 0;
int g_led = 0;
int g_meshdata = 0;

//场景配置应答
int answer = 0;
byte ackswer[15] = {0};

//初始化
void programinit()
{
    byte readmode[10] = {0};
    byte localsn[12] = {0x69,0x69,0x69,0x69,0x69,0x69};
    byte localmac[12] = {0x69,0x69,0x69,0x69,0x69,0x69};
    
    memcpy(hostinfo.hostsn, localsn, 6);
    memcpy(hostinfo.wanmac, localmac, 6);
    memcpy(hostinfo.wifimac, localmac, 6);

    if(0 == filefind("mode", readmode, HOSTSTATUS, 4))
    {
        hostinfo.mode = readmode[4] - '0';
        //读取主从机信息
        readmasterslave();
        //读取主从机配置信息
        readconfiginfo();
        
        if(hostinfo.mode == 1)
        {
            hostinfo.hostsn [6] = 0x01;
            //获取帐号信息
            readaccount();
            //读取节点信息
            readnode();
            //获取区域
            readareainfo();
            //获取本地关联
            readlocalass();
            //读取手势关联
            readgestureinfo();
            //读取烟雾关联
            readsmokeinfo();
            //读取震动关联
            readshakeinfo();
            //读取门磁关联
            readdoormagnetinfo();
            //读取水浸关联
            readfloodinginfo();
            //读取红外活动侦测关联
            readiractivityinfo();
            //读取雨雪关联
            readrainsnowinfo();
            //读取燃气关联
            readgasinfo();
            //读取风速关联
            readwindspeed();
            //读取一氧化碳关联
            readcarmoxde();
            //读取智能控制面板关联
            readsmartctlpanel();
            //读取智能锁用户信息
            readsmartlockinfo();
            //读取智能锁关联
            readlockinfo();
            //获取手动场景
            readmansce();
            //读取安防信息
            readsecurityconfig(1, "1");
            //读取自动场景信息
            readautosce();
            //读取保存状态
            readautoscestatus();
            //读取触发条件
            readtriggercond();
        }
        else
        {
            pr_debug("从机\n");
        }
    }
    else
    {
        pr_debug("新出厂设备\n");
        //获取帐号信息
        readaccount();
    }
}

int main(void)
{
    int ret = 0;
    //创建线程
    pthread_t serial, sock, timer, key, led, broadcast, uart_send, sockclient, chip_send, security;
    pthread_t slaveclient, slaveserial;

    pthread_attr_t thread_attr;
    size_t stack_size = 2097152;
    //size_t stack_size = 8388608;
    int status = 0;

    status = pthread_attr_init(&thread_attr);
    if(status != 0)
    {
        pr_debug("初始化线程属性失败\n");
    }

    //status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    /*
       获取当前线程栈大小
       status = pthread_attr_getstacksize(&thread_attr, &stack_size);
       if(status != 0)
       {
       pr_debug("获取当前线程栈大小失败\n");
       }
       pr_debug("当前线程栈大小:%u; 栈最小:%u\n", stack_size, PTHREAD_STACK_MIN);
    */

    //设置当前线程栈大小
    status = pthread_attr_setstacksize(&thread_attr, stack_size);
    if(status != 0)
    {
        pr_debug("设置线程栈大小失败\n");
    }
    //获取当前线程栈大小
    status = pthread_attr_getstacksize(&thread_attr, &stack_size);
    if(status != 0)
    {
        pr_debug("获取当前线程栈大小失败\n");
    }
    //pr_debug("当前线程栈大小:%u; 栈最小:%u\n", stack_size, PTHREAD_STACK_MIN);

    ret = init_cycle_buffer();
    if(ret == -1)
    {
        pr_debug("init_cycle_buffer fail...\n");
        return ret;
    }
    
    ret = init_chip_buffer();
    if(ret == -1)
    {
        pr_debug("init_chip_buffer fail...\n");
        return ret;
    }

    //初始化
    programinit();
    
    if(hostinfo.mode == 1)
    {
        //线程开
        pthread_create(&serial, NULL, serial_run, 0);
        //8266 uart send
        pthread_create(&uart_send, &thread_attr, uart_run, 0);
        //chip uart send
        pthread_create(&chip_send, &thread_attr, chip_uart_run, 0);
        sleep(1);
        pthread_create(&led, &thread_attr, ledcheck, NULL);
        sleep(10);
        pthread_create(&sock, NULL, sock_run, 0);

        //pthread_create(&security, NULL, security_run, NULL);

        pthread_create(&timer, NULL, timer_run, 0);

        //pthread_create(&key, &thread_attr, button_run, 0);

        pthread_create(&broadcast, &thread_attr, bcastserver_run, 0);

        pthread_create(&sockclient, NULL, sockclient_run, 0);

        //线程结束时，存放回收数据
        pthread_join(serial, NULL);
        
        pthread_join(uart_send, NULL);
        
        pthread_join(chip_send, NULL);
        
        pthread_join(led, NULL);

        pthread_join(sock, NULL);
    
        //pthread_join(security, NULL);
        
        pthread_join(timer, NULL);

        //pthread_join(key, NULL);

        pthread_join(broadcast, NULL);

        pthread_join(sockclient, NULL);
    }
    else
    {
        pthread_create(&slaveserial, NULL, slaveserial_run, 0);
        
        pthread_create(&slaveclient, NULL, slavesockclient_run, 0);
       

        pthread_join(slaveserial, NULL);
        
        pthread_join(slaveclient, NULL);
    }

    status = pthread_attr_destroy(&thread_attr);
    if(status != 0)
    {
        pr_debug("销毁线程属性失败!\n");
    }

    pthread_mutex_destroy(&fifo->lock);
    
    return 0;
}
