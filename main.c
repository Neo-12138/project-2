#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
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

#include "libxml/xmlmemory.h" //需要放在\86下
#include "libxml/parser.h"

#define DISP_BUF_SIZE (800 * 480) // 定义显示缓冲区的大小，800x480 像素

int soc_fd; // 客户端套接字

int init_sock(void)
{
    // 创建套接字
    soc_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == soc_fd) {
        perror("socket failed!\n");
        exit(-1);
    }
    // 设置端口复用
    int on = 1;
    setsockopt(soc_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    // 发起连接请求
    printf("正在连接\n");

    socklen_t len = sizeof(struct sockaddr);
    // 配置结构体
    struct sockaddr_in jack;
    jack.sin_family      = AF_INET;
    jack.sin_port        = htons(3000);
    jack.sin_addr.s_addr = inet_addr("192.168.10.148");

    int con_fd = connect(soc_fd, (struct sockaddr *)&jack, len);
    if(-1 == con_fd) {
        perror("connect failed!\n");
        exit(-1);
    }
    printf("已连接！\n");
    return soc_fd;
}

// 发送录音文件
int send_file(int soc_fd)
{
    int wav_fd = open("./example.wav", O_RDWR);
    if(-1 == wav_fd) {
        perror("open wav failed!\n");
        return -1;
    }
    char buf[1024];
    int sum = 0;
    int ret;
    while(1) {
        memset(buf, 0, sizeof(buf));
        ret = read(wav_fd, buf, 1024);
        if(-1 == ret) {
            perror("read wav failed!\n");
            return -1;
        } else if(0 == ret)
            break;
        else {
            sum += ret;
            write(soc_fd, buf, ret);
        }
    }
    printf("写入了%d个字节", sum);
    close(wav_fd);
    printf("传输成功！\n");
}

static void recording_btn_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
        system("arecord -d3 -c1 -r16000 -twav -fS16_LE example.wav"); // 录音
        printf("录音结束\n");
        send_file(soc_fd);
        rec_file(soc_fd);
        ///////////////////////
        xmlChar * id = parse_xml("result.xml");

        if(atoi((char *)id) == 2) printf("2222222\n");
        /////////////////////////
        printf("id=%s\n", id);
    } else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
}

int rec_file(int soc_fd)
{
    char xml_buf[1024];
    int xml_fd;
    xml_fd = open("./result.xml", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(-1 == xml_fd) {
        perror("open result failed!\n");
        return -1;
    }
    int ret = read(soc_fd, xml_buf, 1024);
    write(xml_fd, xml_buf, ret);
    close(xml_fd);
}

// 传入的cur == object作为根节点
xmlChar * __get_cmd_id(xmlDocPtr doc, xmlNodePtr cur)
{
    xmlChar *key, *id;

    cur = cur->xmlChildrenNode;

    while(cur != NULL) {
        // 查找到cmd子节点
        if((!xmlStrcmp(cur->name, (const xmlChar *)"cmd"))) {
            // 取出内容
            key = xmlNodeGetContent(cur);

            printf("cmd: %s\n", key);
            xmlFree(key);

            // 读取节点属性
            id = xmlGetProp(cur, (const xmlChar *)"id");
            printf("id: %s\n", id);

            xmlFree(doc);
            return id;
        }
        cur = cur->next;
    }
    // 释放文档指针
    xmlFree(doc);
    return NULL;
}

xmlChar * parse_xml(char * xmlfile)
{
    xmlDocPtr doc;
    xmlNodePtr cur1, cur2;

    // 分析一个xml文件，并返回一个xml文档的对象指针： 也就是指向树
    doc = xmlParseFile(xmlfile);
    if(doc == NULL) {
        fprintf(stderr, "Document not parsed successfully. \n");
        return NULL;
    }

    // 获得文档的根节点
    cur1 = xmlDocGetRootElement(doc);
    if(cur1 == NULL) {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    // 检查根节点的名称是否为nlp
    if(xmlStrcmp(cur1->name, (const xmlChar *)"nlp")) {
        fprintf(stderr, "document of the wrong type, root node != nlp");
        xmlFreeDoc(doc);
        return NULL;
    }

    // 获取子元素节点
    cur1 = cur1->xmlChildrenNode;

    while(cur1 != NULL) {
        // 检查子元素是否为result
        if((!xmlStrcmp(cur1->name, (const xmlChar *)"result"))) {
            // 得到result的子节点
            cur2 = cur1->xmlChildrenNode;
            while(cur2 != NULL) {
                // 查找到准确率
                if((!xmlStrcmp(cur2->name, (const xmlChar *)"confidence"))) {
                    xmlChar * key = xmlNodeGetContent(cur2);
                    printf("confidence: %s\n", key);

                    // 若准确率低于30，则放弃当前识别
                    if(atoi((char *)key) < 30) {
                        xmlFree(doc);
                        fprintf(stderr, "sorry, I'm NOT sure what you say.\n");
                        return NULL;
                    }
                }

                // 查找到object，则执行提取字符串及属性
                if((!xmlStrcmp(cur2->name, (const xmlChar *)"object"))) {
                    return __get_cmd_id(doc, cur2);
                }
                cur2 = cur2->next;
            }
        }
        cur1 = cur1->next;
    }

    // 释放文档指针
    xmlFreeDoc(doc);
    return NULL;
}

void recording_btn(void)
{
    lv_obj_t * label; // 声明一个指向标签对象的指针变量

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act()); // 创建第一个按钮对象
    // 为按钮1添加一个事件回调函数，当任何事件发生时调用 event_handler
    lv_obj_add_event_cb(btn1, recording_btn_event, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(btn1, 100, 240); // 设置按钮坐标
    lv_obj_set_size(btn1, 80, 40);  // 设置按钮大小

    // 在按钮1上创建一个标签对象
    label = lv_label_create(btn1);
    // 设置标签的文本为 "录音"
    lv_label_set_text(label, "luyin"); // 没装字库 ，暂时只能用拼音
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

int main(void)
{
    lv_init(); // 初始化 LittlevGL 库

    fbdev_init(); // 初始化 Linux 的帧缓冲设备，获取屏幕信息并进行内存映射

    static lv_color_t buf[DISP_BUF_SIZE]; // 定义一个缓冲区用于 LittlevGL 绘制屏幕内容

    static lv_disp_draw_buf_t disp_buf;                         // 定义显示缓冲区描述符
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE); // 初始化显示缓冲区

    static lv_disp_drv_t disp_drv;   // 定义显示驱动描述符
    lv_disp_drv_init(&disp_drv);     // 初始化显示驱动描述符
    disp_drv.draw_buf = &disp_buf;   // 设置显示驱动的缓冲区
    disp_drv.flush_cb = fbdev_flush; // 设置刷新回调函数为帧缓冲设备的刷新函数
    disp_drv.hor_res  = 800;         // 设置显示的水平分辨率
    disp_drv.ver_res  = 480;         // 设置显示的垂直分辨率
    lv_disp_drv_register(&disp_drv); // 向 LittlevGL 注册显示驱动

    evdev_init();                                     // 初始化输入设备，主要用于触摸屏或鼠标
    static lv_indev_drv_t indev_drv_1;                // 定义输入设备驱动描述符
    lv_indev_drv_init(&indev_drv_1);                  // 初始化输入设备驱动
    indev_drv_1.type         = LV_INDEV_TYPE_POINTER; // 设置输入设备类型为指针（触摸屏或鼠标）
    indev_drv_1.read_cb      = evdev_read;            // 设置读取输入数据的回调函数
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv_1); // 注册输入设备到 LittlevGL

    /* *****************************代  码  区********************************** */
    soc_fd = init_sock();
    recording_btn();
    /* ************************************************************************ */

    while(1) {
        lv_timer_handler(); // 处理 LittlevGL 的任务，包括刷新屏幕和处理输入事件
        usleep(5000);       // 休眠 5 毫秒，降低 CPU 占用率
    }

    return 0;
}

uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0; // 用于记录程序开始的时间戳
    if(start_ms == 0) {
        struct timeval tv_start;                                          // 定义时间结构体
        gettimeofday(&tv_start, NULL);                                    // 获取当前时间
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000; // 将时间转换为毫秒
    }

    struct timeval tv_now;                                      // 定义当前时间的结构体
    gettimeofday(&tv_now, NULL);                                // 获取当前时间
    uint64_t now_ms;                                            // 当前时间的毫秒表示
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000; // 转换为毫秒

    uint32_t time_ms = now_ms - start_ms; // 计算从程序开始到现在经过的时间
    return time_ms;                       // 返回经过的时间
}

// // 创建第二个按钮对象
// lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
// // 为按钮2添加一个事件回调函数，当任何事件发生时调用 event_handler
// lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
// // 将按钮2对齐到屏幕中心，偏移为 (0, 40)
// lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
// // 将按钮2设置为可切换状态（Checkable）
// lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
// // 设置按钮2的高度为内容高度
// lv_obj_set_height(btn2, LV_SIZE_CONTENT);

// // 在按钮2上创建一个标签对象
// label = lv_label_create(btn2);
// // 设置标签的文本为 "Toggle"
// lv_label_set_text(label, "Toggle");
// // 将标签居中对齐到按钮2上
// lv_obj_center(label);