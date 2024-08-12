#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include "request_gy39.h" //获取gy_39

#include "arecording.h"

#define DISP_BUF_SIZE (800 * 480) // 定义显示缓冲区的大小，800x480 像素

static void recording_btn_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
        system("arecord -d3 -c1 -r16000 -twav -fS16_LE example.wav"); // 录音
        printf("录音结束\n");

        if(send_file(soc_fd) == 0) {
            printf("发送成功\n");
            if(rec_file(soc_fd) == 0) {
                printf("接受成功\n");
                xmlChar * id = parse_xml("result.xml");
                if(id) {
                    printf("id=%s\n", id);
                    if(atoi((char *)id) == 1) {
                        system("mplayer 21.mp3 &");
                    }
                    if(atoi((char *)id) == 2) {
                        system("killall 9 mplayer");
                    }
                    if(atoi((char *)id) == 100) {
                        printf("请求gy39\n");
                        gy_info * p = request_gy39();
                        printf("[%d]\n", __LINE__);
                    }
                    xmlFree(id);
                }
            }
        }
    } else if(code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
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