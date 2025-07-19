#include "ui_login.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lv_mysongti_font_20.h"
#include "lv_mysongti_font_30.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "ui_admin.h"
#include "ui_guest.h"
#include "guest_my.h"

#define USER_FILE "data/users.txt"

static lv_obj_t *username_ta;
static lv_obj_t *password_ta;
static lv_obj_t *login_msg;
static lv_obj_t *kb = NULL;

/* 创建初始管理员账户 */
static void create_admin_if_not_exist(void)
{
    // 确保data目录存在
    struct stat st = {0};
    if (stat("data", &st) == -1)
    {
        if (mkdir("data", 0700) == -1 && errno != EEXIST)
        {
            // printf("[ERROR] 创建data目录失败: %s\n", strerror(errno));
        }
    }
    // 检查admin是否存在
    int fd = open(USER_FILE, O_RDONLY);
    int admin_exist = 0;
    if (fd >= 0)
    {
        char buf[256];
        ssize_t n;
        char *p;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
        {
            buf[n] = '\0';
            p = strstr(buf, "admin:123456");
            if (p)
            {
                admin_exist = 1;
                break;
            }
        }
        close(fd);
    }
    if (!admin_exist)
    {
        fd = open(USER_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0)
        {
            write(fd, "admin:123456\n", 14);
            close(fd);
            // printf("[INFO] 已写入初始管理员账号到 %s\n", USER_FILE);
        }
        else
        {
            // printf("[ERROR] 写入管理员账号失败: %s\n", strerror(errno));
        }
    }
}

// 键盘事件回调
static void ta_event_cb(lv_event_t *e)
{
    if (!username_ta || !password_ta)
        return; // 防止控件已被清空
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_FOCUSED)
    {
        if (!kb)
        {
            kb = lv_keyboard_create(lv_scr_act());
            lv_obj_set_size(kb, 800, 240);
        }
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    // 失焦时隐藏键盘
    else if (code == LV_EVENT_DEFOCUSED)
    {
        if (kb)
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

/* 验证登录信息 */
static int check_login(const char *user, const char *pass)
{
    // printf("[DEBUG] check_login: user='%s', pass='%s'\n", user, pass);
    int fd = open(USER_FILE, O_RDONLY);
    if (fd < 0)
    {
        // printf("[ERROR] 无法打开用户文件: %s\n", strerror(errno));
        return 0;
    }
    char buf[256];
    ssize_t n;
    char line[128];
    int i = 0;
    char target[128];
    snprintf(target, sizeof(target), "%s:%s", user, pass);
    // printf("[DEBUG] check_login: target='%s'\n", target);
    int found = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        for (ssize_t j = 0; j < n; ++j)
        {
            if (buf[j] == '\n' || i >= (int)sizeof(line) - 1)
            {
                line[i] = '\0';
                if (strcmp(line, target) == 0)
                {
                    found = 1;
                    break;
                }
                i = 0;
            }
            else
            {
                line[i++] = buf[j];
            }
        }
        if (found)
            break;
    }
    close(fd);
    // printf("[DEBUG] check_login: found=%d\n", found);
    return found;
}

/* 登录按钮点击 */
static void login_event_cb(lv_event_t *e)
{
    if (!username_ta || !password_ta)
        return; // 防止控件已被清空
    const char *user = lv_textarea_get_text(username_ta);
    const char *pass = lv_textarea_get_text(password_ta);

    if (check_login(user, pass))
    {
        lv_label_set_text(login_msg, "登录成功!");
        if (strcmp(user, "admin") == 0)
            ui_admin_create();
        else
        {
            strncpy(current_user, user, sizeof(current_user) - 1); // 这里写入账号
            current_user[sizeof(current_user) - 1] = '\0';
            ui_guest_create();
        }
    }
    else
    {
        if (user[0] == '\0')
        {
            lv_label_set_text(login_msg, "用户名不能为空!");
            return;
        }
        else if (pass[0] == '\0')
        {
            lv_label_set_text(login_msg, "密码不能为空!");
            return;
        }
        else
        {
            lv_label_set_text(login_msg, "用户名或密码错误!");
        }
    }
}

/* 注册按钮点击 */
static void register_event_cb(lv_event_t *e)
{
    if (!username_ta || !password_ta)
        return; // 防止控件已被清空
    const char *user = lv_textarea_get_text(username_ta);
    const char *pass = lv_textarea_get_text(password_ta);

    if (user[0] == '\0')
    {
        lv_label_set_text(login_msg, "用户名不能为空!");
        return;
    }

    if (strcmp(user, "admin") == 0)
    {
        lv_label_set_text(login_msg, "不能注册admin账户!");
        return;
    }

    // 检查用户名是否已存在
    int fd = open(USER_FILE, O_RDONLY);
    int exists = 0;
    if (fd >= 0)
    {
        char buf[256];
        ssize_t n;
        char line[128];
        int i = 0;
        while ((n = read(fd, buf, sizeof(buf))) > 0)
        {
            for (ssize_t j = 0; j < n; ++j)
            {
                if (buf[j] == '\n' || i >= (int)sizeof(line) - 1)
                {
                    line[i] = '\0';
                    // 检查冒号前的用户名
                    char *sep = strchr(line, ':');
                    if (sep)
                    {
                        *sep = '\0';
                        if (strcmp(line, user) == 0)
                        {
                            exists = 1;
                            break;
                        }
                    }
                    i = 0;
                }
                else
                {
                    line[i++] = buf[j];
                }
            }
            if (exists)
                break;
        }
        close(fd);
    }

    if (exists)
    {
        lv_label_set_text(login_msg, "注册失败,用户已存在!");
        return;
    }

    // 写入新用户
    fd = open(USER_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0)
    {
        char entry[256];
        int len = snprintf(entry, sizeof(entry), "%s:%s\n", user, pass);
        write(fd, entry, len);
        fsync(fd);
        close(fd);
        lv_label_set_text(login_msg, "注册成功,请点击登录!");
    }
    else
    {
        lv_label_set_text(login_msg, "注册失败,无法写入文件!");
    }
}

/* 创建登录界面 */
void ui_login_create(void)
{
    // 清空控件指针，防止野指针
    username_ta = NULL;
    password_ta = NULL;
    login_msg = NULL;
    kb = NULL;

    create_admin_if_not_exist();

    lv_obj_t *scr = lv_scr_act();

    // 主标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "智能点餐系统");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_mysongti_font_30, 0);

    // 用户名输入框
    username_ta = lv_textarea_create(scr);
    lv_obj_set_size(username_ta, 400, 50);
    lv_obj_align(username_ta, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_text_font(username_ta, &lv_mysongti_font_20, 0);
    lv_textarea_set_placeholder_text(username_ta, "用户名");
    lv_textarea_set_text(username_ta, "admin"); // 默认输入guest
    lv_obj_add_event_cb(username_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 密码输入框
    password_ta = lv_textarea_create(scr);
    lv_obj_set_size(password_ta, 400, 50);
    lv_obj_align_to(password_ta, username_ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_text_font(password_ta, &lv_mysongti_font_20, 0);
    lv_textarea_set_placeholder_text(password_ta, "密码");
    lv_textarea_set_text(password_ta, "123456"); // 默认输入123456
    lv_textarea_set_password_mode(password_ta, true); // 自动隐藏
    lv_obj_add_event_cb(password_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 登录按钮
    lv_obj_t *btn_login = lv_btn_create(scr);
    lv_obj_set_size(btn_login, 120, 40);
    lv_obj_align_to(btn_login, password_ta, LV_ALIGN_OUT_BOTTOM_MID, -100, 30);
    lv_obj_t *lbl_login = lv_label_create(btn_login);
    lv_label_set_text(lbl_login, "登录");
    lv_obj_center(lbl_login);
    lv_obj_set_style_text_font(lbl_login, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_login, login_event_cb, LV_EVENT_CLICKED, NULL);

    // 注册按钮
    lv_obj_t *btn_register = lv_btn_create(scr);
    lv_obj_set_size(btn_register, 120, 40);
    lv_obj_align_to(btn_register, password_ta, LV_ALIGN_OUT_BOTTOM_MID, 100, 30);
    lv_obj_t *lbl_register = lv_label_create(btn_register);
    lv_label_set_text(lbl_register, "注册");
    lv_obj_center(lbl_register);
    lv_obj_set_style_text_font(lbl_register, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_register, register_event_cb, LV_EVENT_CLICKED, NULL);

    // 提示信息
    login_msg = lv_label_create(scr);
    lv_obj_align_to(login_msg, btn_login, LV_ALIGN_OUT_BOTTOM_MID, 30, 20);
    lv_label_set_text(login_msg, "");
    lv_obj_set_style_text_font(login_msg, &lv_mysongti_font_20, 0);
}