#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

#include "ui_login.h"
#include "ui_admin.h"
#include "guest_my.h"

int main(void)
{
    lv_init();
    
    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER,"/dev/input/event0");


    ui_login_create();


    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }
    
    return 0;
}
