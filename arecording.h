#ifndef __ARECORDING_H
#define __ARECORDING_H

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h> // 在文件头部包含此行代码

#include "libxml/xmlmemory.h" //需要放在\86下
#include "libxml/parser.h"

#include "request_gy39.h"

int soc_fd; // 客户端套接字

int init_sock(void);                                   // 函数原型
int send_file(int sock_fd);                            // 函数原型
int rec_file(int sock_fd);                             // 函数原型
xmlChar * __get_cmd_id(xmlDocPtr doc, xmlNodePtr cur); // 函数原型
xmlChar * parse_xml(char * xmlfile);                   // 函数原型
void recording_btn(void);                              // 函数原型
uint32_t custom_tick_get(void);                        // 函数原型

#endif