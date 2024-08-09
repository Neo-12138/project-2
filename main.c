#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#define DISP_BUF_SIZE (800 * 480)

static lv_obj_t * user_input;
static lv_obj_t * pass_input;
static lv_obj_t * kb; // 虚拟键盘对象

// 处理登录按钮的事件函数
static void login_event_handler(lv_event_t * e)
{
    const char * user = lv_textarea_get_text(user_input);
    const char * pass = lv_textarea_get_text(pass_input);

    if(strcmp(user, "admin") == 0 && strcmp(pass, "password") == 0) {
        lv_label_set_text(lv_obj_get_child(lv_event_get_current_target(e), NULL), "Login Successful");
    } else {
        lv_label_set_text(lv_obj_get_child(lv_event_get_current_target(e), NULL), "Login Failed");
    }
}

// 处理输入框的点击事件，弹出虚拟键盘
static void text_input_event_handler(lv_event_t * e)
{
    lv_keyboard_set_textarea(kb, lv_event_get_target(e)); // 将输入框与虚拟键盘绑定
}

// 创建登录界面函数
void create_login_screen(void)
{
    lv_obj_t * login_screen = lv_obj_create(NULL);
    lv_scr_load(login_screen);

    lv_obj_t * user_label = lv_label_create(login_screen);
    lv_label_set_text(user_label, "Username:");
    lv_obj_align(user_label, LV_ALIGN_CENTER, -50, -50);

    user_input = lv_textarea_create(login_screen);
    lv_obj_align(user_input, LV_ALIGN_CENTER, 50, -50);
    lv_textarea_set_one_line(user_input, true);
    lv_obj_add_event_cb(user_input, text_input_event_handler, LV_EVENT_FOCUSED, NULL); // 添加事件处理器

    lv_obj_t * pass_label = lv_label_create(login_screen);
    lv_label_set_text(pass_label, "Password:");
    lv_obj_align(pass_label, LV_ALIGN_CENTER, -50, 0);

    pass_input = lv_textarea_create(login_screen);
    lv_obj_align(pass_input, LV_ALIGN_CENTER, 50, 0);
    lv_textarea_set_one_line(pass_input, true);
    lv_textarea_set_password_mode(pass_input, true);
    lv_obj_add_event_cb(pass_input, text_input_event_handler, LV_EVENT_FOCUSED, NULL); // 添加事件处理器

    lv_obj_t * login_btn = lv_btn_create(login_screen);
    lv_obj_align(login_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_size(login_btn, 120, 50);
    lv_obj_add_event_cb(login_btn, login_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_label = lv_label_create(login_btn);
    lv_label_set_text(btn_label, "Login");
    lv_obj_center(btn_label);

    // 创建虚拟键盘
    kb = lv_keyboard_create(login_screen);
    lv_obj_set_size(kb, LV_HOR_RES, LV_VER_RES / 2); // 设置键盘尺寸
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);     // 将键盘对齐到屏幕底部
}

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

    /* ****************************************************************** */
    create_login_screen();
    /* ************************************************************************ */

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