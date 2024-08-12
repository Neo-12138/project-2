#ifndef _REQUEST_GY39_H
#define _REQUEST_GY39_H

typedef struct gy39_info
{
    double lux; // 光照强度
    double tem; // 温度
    double atm; // 气压
    double hum; // 湿度
    double alt; // 海拔
} gy_info;

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

int init_tty(int fd);
gy_info * request_gy39(void);

#endif