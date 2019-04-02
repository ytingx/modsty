#ifndef __UARTFUNCTION_H__
#define __UARTFUNCTION_H__
#include "modstyhead.h"

//获取mac地址
int get_mac(char *mac);

//字符串转16进制
void StrToHex(byte *pbDest, byte *pbSrc, int nLen);

//16进制转字符串
void HexToStr(byte *pbDest, byte *pbSrc, int nLen);

#endif //__UARTFUNCTION_H__
