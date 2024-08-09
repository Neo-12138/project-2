#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define DISP_BUF_SIZE (128 * 800)

int main(void)
{
    /*LittlevGL init*/
    lv_init(); // lvgl初始化

    /*Linux frame buffer device init*/
    fbdev_init(); // 初始化linux的帧缓冲设备，获取屏幕信息，并做内存映射

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE]; // 画屏幕缓冲区

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE); //

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res  = 800;
    disp_drv.ver_res  = 480;
    lv_disp_drv_register(&disp_drv); // 向lvgl注册一个显示结构体

    evdev_init();                                             // 触摸屏初始化  名字改成0
    static lv_indev_drv_t indev_drv_1;                        // 输入设备结构体对象
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/ // 初始化一个结构体对象
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;                 // 设置输入类型为触摸板，鼠标，按钮

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb      = evdev_read; // 用户输入设备读取输入数据的回调函数
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv_1); // 注册输入结构体对象到lvgl

    /*Set a cursor for the mouse*/ // 设置一个鼠标
    // LV_IMG_DECLARE(mouse_cursor_icon)
    // lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    // lv_img_set_src(cursor_obj, &mouse_cursor_icon);      /*Set the image source*/
    // lv_indev_set_cursor(mouse_indev, cursor_obj);        /*Connect the image  object to the driver*/

    /*Create a Demo*/
    lv_demo_widgets(); // 官方写好的一个案例

    /*Handle LitlevGL tasks (tickless mode)*/ // 循环检测用户对设备是否有操作
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
