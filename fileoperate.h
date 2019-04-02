#ifndef __FILEOPERATE_H__
#define __FILEOPERATE_H__
#include "modstyhead.h"

//文件内容替换
int replace(char* FileName, byte* str, int len);

//文件内容删除
int filecondel(char* FileName, char str[2500], int len);

//存储
void InsertLine(char* FileName, char str[2500]);

//查找文件包含内容并返回
int filefindstr(byte *str, byte *buf, char *FileName);

//查找文件内容并返回
int filefind(byte *str, byte *buf, char *FileName, int len);

//查找文件中相同节点信息
int contains(byte *string, int len, char* FileName);

#endif //__FILEOPERATE_H__
