#include "fileoperate.h"

extern int client_fd;

//文件内容替换
int replace(char* FileName, byte* str, int len)
{
    //pr_debug("Replace\n");
    FILE *fin = NULL, *ftp = NULL;
    byte rep[2500] = {0};
    fin = fopen(FileName, "a+");
    if(fin == NULL)
    {
        pr_debug("%s: %s\n", FileName, strerror(errno));
        byte logsend[256] = {0xf3,0x00};
        byte logbuf[256] = {0};
        sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, FileName, strerror(errno));
        logsend[2] = (3 + strlen(logbuf));
        memcpy(logsend + 3, logbuf, strlen(logbuf));
        send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
        exit(-1);
    }

    ftp = fopen("/data/modsty/tmp", "w");
    if(ftp == NULL)
    {
        pr_debug("%s: %s\n", FileName, strerror(errno));
        byte logsend[256] = {0xf3,0x00};
        byte logbuf[256] = {0};
        sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, FileName, strerror(errno));
        logsend[2] = (3 + strlen(logbuf));
        memcpy(logsend + 3, logbuf, strlen(logbuf));
        send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
        exit(-1);
    }

    //pr_debug("replace_The openedfile's descriptor is %s, %d, %d\n", FileName, fileno(fin), fileno(ftp));
    while(fgets(rep, 2500, fin))
    {
        if(0 == strncmp(str, rep, len))
        {
            //pr_debug("replace_重复信息\n");
            continue;
        }
        fputs(rep, ftp);
    }
    strcat(str, "\n");
    fputs(str, ftp);
    fclose(fin);
    fclose(ftp);
    fin = NULL;
    ftp = NULL;
    remove(FileName);
    rename("/data/modsty/tmp", FileName);
    return 1;
}

//文件内容删除
int filecondel(char* FileName, char str[2500], int len)
{
    FILE *fin = NULL, *ftp = NULL;
    byte rep[2500] = {0};
    fin = fopen(FileName, "r");
    if(fin == NULL)
    {
        pr_debug("filecondel:file open failed\n");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        return -1;
    }
    ftp = fopen("/data/modsty/tmp", "w");
    if(ftp == NULL)
    {
        pr_debug("filecondel:file open failed\n");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        byte logsend[256] = {0xf3,0x00};
        byte logbuf[256] = {0};
        sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, FileName, strerror(errno));
        logsend[2] = (3 + strlen(logbuf));
        memcpy(logsend + 3, logbuf, strlen(logbuf));
        send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
        exit(-1);
    }

    //pr_debug("filecondel_The openedfile's descriptor is %s, %d, %d\n", FileName, fileno(fin), fileno(ftp));
    while(fgets(rep, 2500, fin))
    {
        if(0 == strncmp(str, rep, len))
        {
            //pr_debug("filecondel_重复信息\n");
            continue;
        }
        fputs(rep, ftp);
    }
    fclose(fin);
    fclose(ftp);
    fin = NULL;
    ftp = NULL;
    remove(FileName);
    rename("/data/modsty/tmp", FileName);
    return 1;
}

void InsertLine(char* FileName, char str[2500])
{
    //pr_debug("Insert \n");
    int i = 0;
    FILE *fp = NULL;
    if((fp = fopen(FileName, "a"))==NULL)
    {
        pr_debug( "Insert Can't open file!\n ");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        byte logsend[256] = {0xf3,0x00};
        byte logbuf[256] = {0};
        sprintf(logbuf, "File:%s, Line:%05d %s: %s", __FILE__, __LINE__, FileName, strerror(errno));
        logsend[2] = (3 + strlen(logbuf));
        memcpy(logsend + 3, logbuf, strlen(logbuf));
        send(client_fd, logsend, logsend[2], MSG_NOSIGNAL);
        exit(-1);
    }
    //pr_debug("InsertLine_The openedfile's descriptor is %s, %d\n", FileName, fileno(fp));
    i = fputs(str,fp);
    /*
    if (i >= 0)
    {
        pr_debug("InsertLine_信息存储成功\n");
    }
    */
    fclose(fp);
    fp = NULL;
    //system("chmod 777 /data/modsty/nodeid");
}

//查找文件包含内容并返回
int filefindstr(byte *str, byte *buf, char *FileName)
{
    FILE *fd = NULL;
    byte strbuf[2500] = {0};
    if((fd = fopen(FileName, "r")) == NULL)
    {
        pr_debug("filefindstr_file open failed\n");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        //exit(-1);
        return -1;
    }
    //pr_debug("filefindstr_The openedfile's descriptor is %s, %d\n", FileName, fileno(fd));
    while(fgets(strbuf, 2500, fd))
    {
        if(NULL != strstr(strbuf, str))
        {
            //pr_debug("filefind:%s\n",strbuf);
            strcpy(buf, strbuf);
            fclose(fd);
            fd = NULL;
            return 0;
        }
        else
        {
            continue;
        }
    }
    fclose(fd);
    fd = NULL;
    return -1;
}

//查找文件内容并返回
int filefind(byte *str, byte *buf, char *FileName, int len)
{
    FILE *fd = NULL;
    byte strbuf[2500] = {0};
    if((fd = fopen(FileName, "r")) == NULL)
    {
        pr_debug("filefind_file open failed\n");
        pr_debug("%s: %s\n", FileName, strerror(errno));
        //exit(-1);
        return -1;
    }
    //pr_debug("filefind_The openedfile's descriptor is %s, %d\n", FileName, fileno(fd));
    while(fgets(strbuf, 2500, fd))
    {
        if(0 == strncmp(str, strbuf, len))
        {
            //pr_debug("filefind:%s\n",strbuf);
            strcpy(buf, strbuf);
            fclose(fd);
            fd = NULL;
            return 0;
        }
        else
        {
            continue;
        }
    }
    fclose(fd);
    fd = NULL;
    return -1;
}

//查找文件中相同节点信息
int contains(byte *string, int len, char* FileName)
{
    int i = 0;
    FILE * fd;
    if((fd = fopen(FileName, "r")) == NULL)
    {
        pr_debug("Can't open %s, program will to exit.", FileName);
        pr_debug("%s: %s\n", FileName, strerror(errno));
        //exit(-1);
        return -1;
    }
    //pr_debug("contains_The openedfile's descriptor is %s, %d\n", FileName, fileno(fd));
    byte buf[2500] = {0};
    //byte buf_hex[BUFFER_SIZE];
    while (fgets(buf, BUFFER_SIZE, fd))
    {
        if(0 == strncmp(string, buf, len))
        {
            //StrToHex(buf_hex, buf, stringSize);
            fclose(fd);
            fd = NULL;
            return -1;
        }
        else
        {
            continue;
        }
    }
    //pr_debug("非重复节点信息，可存储!\n");
    fclose(fd);
    fd = NULL;
    return 1;
}
