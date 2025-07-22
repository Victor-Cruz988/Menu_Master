#include "include.h"

#define ORDER_DIR "data/order/"
#define MAX_ORDERS 100
#define MAX_FILENAME 128

static void order_list_event_cb(lv_event_t *e);
static char order_files[MAX_ORDERS][MAX_FILENAME];
static int order_count = 0;

char current_user[64] = {0};

// 读取订单文件列表
static void load_order_files(void)
{
    order_count = 0;
    DIR *dir = opendir(ORDER_DIR);
    if (!dir)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && order_count < MAX_ORDERS)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        strncpy(order_files[order_count], entry->d_name, MAX_FILENAME - 1);
        order_files[order_count][MAX_FILENAME - 1] = '\0';
        order_count++;
    }
    closedir(dir);
}

// 返回按钮事件
static void my_back_event_cb(lv_event_t *e)
{
    guest_my_create();
}

// 弹窗关闭按钮事件
static void close_popup_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *popup = lv_obj_get_parent(btn);
    // 删除遮罩层
    lv_obj_t *mask = lv_obj_get_parent(popup);
    lv_obj_del(mask);
}

// 删除订单按钮事件
static void delete_order_event_cb(lv_event_t *e)
{
    const char *filepath = (const char *)lv_event_get_user_data(e);
    if (filepath)
        remove(filepath);
    // 关闭弹窗并刷新订单列表
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *popup = lv_obj_get_parent(btn);
    lv_obj_t *mask = lv_obj_get_parent(popup);
    lv_obj_del(mask);
    order_list_event_cb(NULL);
}

// 订单项点击事件（弹窗显示内容）
static void order_item_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *list = lv_obj_get_parent(btn);
    const char *filename = lv_list_get_btn_text(list, btn);
    static char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", ORDER_DIR, filename);
    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return;
    char content[2048] = {0};
    fread(content, 1, sizeof(content) - 1, fp);
    fclose(fp);

    // 创建弹窗
    lv_obj_t *scr = lv_screen_active();

    // 1. 创建遮罩层
    lv_obj_t *mask = lv_obj_create(scr);
    lv_obj_set_size(mask, lv_obj_get_width(scr), lv_obj_get_height(scr));
    lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(mask, LV_OPA_50, 0); // 半透明
    lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(mask, LV_OBJ_FLAG_CLICKABLE); // 拦截事件

    // 2. 创建弹窗，父对象设为mask
    lv_obj_t *popup = lv_obj_create(mask);
    lv_obj_set_size(popup, 600, 400);
    lv_obj_align(popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(popup, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(popup, 16, 0);
    lv_obj_set_style_border_width(popup, 2, 0);
    lv_obj_set_style_border_color(popup, lv_color_hex(0xcccccc), 0);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    // 关闭按钮（右上角）
    lv_obj_t *close_btn = lv_btn_create(popup);
    lv_obj_set_size(close_btn, 40, 40);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff0000), 0);
    lv_obj_set_style_radius(close_btn, 40, 0);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "x");
    lv_obj_center(close_lbl);
    lv_obj_set_style_text_font(close_lbl, &lv_font_montserrat_28, 0);
    lv_obj_add_event_cb(close_btn, close_popup_event_cb, LV_EVENT_CLICKED, NULL);

    // 内容标签
    lv_obj_t *label = lv_label_create(popup);
    lv_label_set_text(label, content);
    lv_obj_set_width(label, 580);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(label, &lv_mysongti_font_20, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    // 删除按钮（底部中间）
    lv_obj_t *delete_btn = lv_btn_create(popup);
    lv_obj_set_size(delete_btn, 100, 40);
    lv_obj_align(delete_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *delete_lbl = lv_label_create(delete_btn);
    lv_label_set_text(delete_lbl, "删除");
    lv_obj_center(delete_lbl);
    lv_obj_set_style_text_font(delete_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(delete_btn, delete_order_event_cb, LV_EVENT_CLICKED, filepath);
}

// 创建订单列表页面
static void order_list_event_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "历史订单");
    lv_obj_set_style_text_font(title, &lv_mysongti_font_30, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_set_style_text_font(back_lbl, &lv_mysongti_font_20, 0);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, my_back_event_cb, LV_EVENT_CLICKED, NULL);

    // 订单列表容器
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 700, 360);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 80);
    // 禁止左右滑动，只允许上下滑动
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    // 加载订单文件
    load_order_files();
    for (int i = 0; i < order_count; i++)
    {
        lv_obj_t *btn = lv_list_add_btn(list, NULL, order_files[i]);
        // 设置列表按钮大小
        lv_obj_set_size(btn, 680, 40);
        lv_obj_add_event_cb(btn, order_item_event_cb, LV_EVENT_CLICKED, NULL);
    }
}

// 账号信息页面
static lv_obj_t *old_pass_ta, *new_pass_ta, *info_msg;
static lv_obj_t *kb = NULL;
// 文本区域事件回调
// 处理焦点事件，显示或隐藏键盘
static void ta_event_cb(lv_event_t *e)
{
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
    else if (code == LV_EVENT_DEFOCUSED)
    {
        if (kb)
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

// 保存账号信息回调
static void save_account_info_event_cb(lv_event_t *e)
{
    if (!old_pass_ta || !new_pass_ta || !info_msg)
        return;
    const char *old_pass = lv_textarea_get_text(old_pass_ta);
    const char *new_pass = lv_textarea_get_text(new_pass_ta);

    if (old_pass[0] == '\0' || new_pass[0] == '\0')
    {
        lv_label_set_text(info_msg, "原密码和新密码不能为空!");
        return;
    }

    // 校验原密码
    FILE *fp = fopen("data/users.txt", "r+");
    if (!fp)
    {
        lv_label_set_text(info_msg, "无法打开用户文件!");
        return;
    }
    char lines[100][128];
    int line_count = 0, found = 0;
    char user[64], pass[64];
    /*  fgets(lines[line_count], sizeof(lines[0]), fp)
        这行代码每次从文件 fp 读取一行，存入 lines[line_count]。
        sizeof(lines[0]) 表示每行的最大长度，防止缓冲区溢出。

        sscanf(lines[line_count], "%63[^:]:%63[^\n]", user, pass);
        这行代码用来解析每一行，假设每行格式为 用户名:密码。
        它把冒号前的内容读到 user，冒号后的内容读到 pass。

        if (strcmp(user, current_user) == 0 && strcmp(pass, old_pass) == 0)
        检查当前行的用户名和密码是否与要查找的用户和旧密码匹配。

        snprintf(lines[line_count], sizeof(lines[0]), "%s:%s\n", current_user, new_pass);
        如果匹配成功，就用新密码覆盖这一行。

        line_count++
        处理下一行。
    */
    while (fgets(lines[line_count], sizeof(lines[0]), fp))
    {
        /*
        %63[^:]
            这是 scanf 的格式控制符，意思是最多读取63个不是冒号(:)的字符，并存入 user。
            [^:] 表示“不是冒号的字符”。63 是最大宽度，防止缓冲区溢出（假设 user 数组大小为64，留一个结尾的 \0）。
        :%63[^\n]
            读取冒号后最多63个不是换行符(\n)的字符，存入 pass
        */
        sscanf(lines[line_count], "%63[^:]:%63[^\n]", user, pass);
        if (strcmp(user, current_user) == 0 && strcmp(pass, old_pass) == 0)
        {
            found = 1;
            snprintf(lines[line_count], sizeof(lines[0]), "%s:%s\n", current_user, new_pass);
        }
        line_count++;
    }
    fclose(fp);

    if (!found)
    {
        lv_label_set_text(info_msg, "原密码错误!");
        return;
    }

    // 写回所有用户
    fp = fopen("data/users.txt", "w");
    if (!fp)
    {
        lv_label_set_text(info_msg, "无法写入用户文件!");
        return;
    }
    for (int i = 0; i < line_count; i++)
    {
        fputs(lines[i], fp);
    }
    fclose(fp);

    lv_label_set_text(info_msg, "密码修改成功!");
    lv_textarea_set_text(old_pass_ta, "");
    lv_textarea_set_text(new_pass_ta, "");
}

// 账号信息页面
static void account_info_event_cb(lv_event_t *e)
{
    old_pass_ta = NULL;
    new_pass_ta = NULL;
    info_msg = NULL;
    kb = NULL;
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "账号信息");
    lv_obj_set_style_text_font(title, &lv_mysongti_font_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 用户名（只读）
    lv_obj_t *user_label = lv_label_create(scr);
    lv_label_set_text_fmt(user_label, "用户名:%s", current_user);
    lv_obj_set_style_text_font(user_label, &lv_mysongti_font_20, 0);
    lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, 60);

    // 原密码输入框
    old_pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(old_pass_ta, 300, 40);
    lv_obj_align(old_pass_ta, LV_ALIGN_TOP_MID, 0, 110);
    lv_textarea_set_placeholder_text(old_pass_ta, "原密码");
    lv_textarea_set_password_mode(old_pass_ta, true);
    lv_obj_set_style_text_font(old_pass_ta, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(old_pass_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 新密码输入框
    new_pass_ta = lv_textarea_create(scr);
    lv_obj_set_size(new_pass_ta, 300, 40);
    lv_obj_align(new_pass_ta, LV_ALIGN_TOP_MID, 0, 170);
    lv_textarea_set_placeholder_text(new_pass_ta, "新密码");
    lv_textarea_set_password_mode(new_pass_ta, true);
    lv_obj_set_style_text_font(new_pass_ta, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(new_pass_ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // 信息提示
    info_msg = lv_label_create(scr);
    lv_obj_align(info_msg, LV_ALIGN_TOP_MID, 0, 220);
    lv_label_set_text(info_msg, "");
    lv_obj_set_style_text_font(info_msg, &lv_mysongti_font_20, 0);

    // 保存按钮
    lv_obj_t *save_btn = lv_btn_create(scr);
    lv_obj_set_size(save_btn, 120, 40);
    lv_obj_align(save_btn, LV_ALIGN_TOP_MID, 0, 270);
    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "保存");
    lv_obj_center(save_lbl);
    lv_obj_set_style_text_font(save_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(save_btn, save_account_info_event_cb, LV_EVENT_CLICKED, NULL);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_center(back_lbl);
    lv_obj_set_style_text_font(back_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(back_btn, my_back_event_cb, LV_EVENT_CLICKED, NULL);
}

// 退出登录
static void logout_event_cb(lv_event_t *e)
{
    // 清空当前屏幕
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    ui_login_create();
}

// 返回主界面按钮事件回调
static void my_back_guest_event_cb(lv_event_t *e)
{
    // 返回到主界面
    ui_guest_create();
}

// 创建我的页面
void guest_my_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 10);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_center(back_lbl);
    lv_obj_set_style_text_font(back_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(back_btn, my_back_guest_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_order = lv_btn_create(scr);
    lv_obj_set_size(btn_order, 400, 60);
    lv_obj_align(btn_order, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_t *lbl_order = lv_label_create(btn_order);
    lv_label_set_text(lbl_order, "历史订单");
    lv_obj_center(lbl_order);
    lv_obj_set_style_text_font(lbl_order, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_order, order_list_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_info = lv_btn_create(scr);
    lv_obj_set_size(btn_info, 400, 60);
    lv_obj_align_to(btn_info, btn_order, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    lv_obj_t *lbl_info = lv_label_create(btn_info);
    lv_label_set_text(lbl_info, "账号信息");
    lv_obj_center(lbl_info);
    lv_obj_set_style_text_font(lbl_info, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_info, account_info_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_logout = lv_btn_create(scr);
    lv_obj_set_size(btn_logout, 400, 60);
    lv_obj_align_to(btn_logout, btn_info, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    lv_obj_t *lbl_logout = lv_label_create(btn_logout);
    lv_label_set_text(lbl_logout, "退出登录");
    lv_obj_center(lbl_logout);
    lv_obj_set_style_text_font(lbl_logout, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_logout, logout_event_cb, LV_EVENT_CLICKED, NULL);
}
