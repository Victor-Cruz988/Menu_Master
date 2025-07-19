#include "ui_admin.h"
#include "ui_login.h"
#include "lvgl/lvgl.h"
#include "lv_mysongti_font_20.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void btn_event_cb(lv_event_t *e);
static void modify_price_event_cb(lv_event_t *e);

// 校验价格字符串是否合法（只允许数字和最多一个小数点，且不能为负数）
static int is_valid_price(const char *s)
{
    if (!s || !*s)
        return 0;
    int dot = 0;
    for (const char *p = s; *p; ++p)
    {
        if (*p == '.')
        {
            if (dot)
                return 0; // 只能有一个小数点
            dot = 1;
        }
        else if (!isdigit((unsigned char)*p))
        {
            return 0;
        }
    }
    // 不能只有小数点
    if (strcmp(s, ".") == 0)
        return 0;
    return 1;
}

// 全局指针保存当前弹窗键盘和遮罩层
static lv_obj_t *g_popup_keyboard = NULL;
static lv_obj_t *g_popup_mask = NULL;
// 全局缓冲区保存当前选中菜品名
static char g_selected_dish[128] = {0};
// 全局指针保存当前弹窗输入框和消息标签
static lv_obj_t *g_popup_price_ta = NULL;
static lv_obj_t *g_popup_msg_label = NULL;

// 输入框失焦事件回调，自动隐藏键盘
static void price_ta_unfocus_event_cb(lv_event_t *e)
{
    if (g_popup_keyboard)
    {
        lv_obj_del(g_popup_keyboard);
        g_popup_keyboard = NULL;
    }
}

// 输入框事件回调，点击时弹出键盘
static void price_ta_event_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *scr = lv_scr_act();
    if (g_popup_keyboard)
    {
        lv_obj_del(g_popup_keyboard);
        g_popup_keyboard = NULL;
    }
    g_popup_keyboard = lv_keyboard_create(scr);
    lv_obj_set_size(g_popup_keyboard, 800, 240);
    lv_obj_align(g_popup_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(g_popup_keyboard, ta);
}

// 返回管理后台的事件回调
static void back_to_admin_event_cb(lv_event_t *e)
{
    ui_admin_create();
}

// 关闭弹窗的事件回调
static void close_popup_event_cb(lv_event_t *e)
{
    // 删除键盘
    if (g_popup_keyboard)
    {
        lv_obj_del(g_popup_keyboard);
        g_popup_keyboard = NULL;
    }
    // 删除弹窗和遮罩层
    if (g_popup_mask)
    {
        lv_obj_del(g_popup_mask);
        g_popup_mask = NULL;
    }
}

// 刷新菜品价格列表
static void refresh_price_list(void)
{
    lv_obj_clean(lv_scr_act());
    modify_price_event_cb(NULL);
}

// 保存价格修改的事件回调
static void save_price_event_cb(lv_event_t *e)
{
    lv_obj_t *price_ta = g_popup_price_ta;
    lv_obj_t *msg_label = g_popup_msg_label;
    const char *selected_dish = g_selected_dish;
    const char *new_price = lv_textarea_get_text(price_ta);

    // 判断输入是否为数字
    if (!is_valid_price(new_price))
    {
        lv_label_set_text(msg_label, "请输入有效价格");
        lv_obj_set_style_text_font(msg_label, &lv_mysongti_font_20, 0);
        return;
    }
    double price_val = atof(new_price);

    // 格式化为两位小数
    char new_price_fmt[32];
    snprintf(new_price_fmt, sizeof(new_price_fmt), "%.2f", price_val);

    // 读取原菜品文件
    FILE *fp = fopen("data/dishes.txt", "r");
    if (!fp)
    {
        lv_label_set_text(msg_label, "无法打开菜品文件!");
        lv_obj_set_style_text_font(msg_label, &lv_mysongti_font_20, 0);
        return;
    }

    char lines[100][256];
    int line_count = 0;
    int found = 0;
    char line[256];

    // 读取所有菜品
    while (fgets(line, sizeof(line), fp) && line_count < 99)
    {
        // 去除换行符
        char *newline = strchr(line, '\n');
        if (newline)
            *newline = '\0';

        // 检查是否是目标菜品
        char current_name[128];
        char current_price[128];
        if (sscanf(line, "%127s %127s", current_name, current_price) == 2)
        {
            if (strcmp(current_name, selected_dish) == 0)
            {
                // 找到目标菜品，更新价格（保留两位小数）
                snprintf(lines[line_count], sizeof(lines[0]), "%s %s", selected_dish, new_price_fmt);
                found = 1;
                printf("DEBUG: 找到菜品 %s,价格从 %s 修改为 %s\n", selected_dish, current_price, new_price_fmt);
            }
            else
            {
                // 其他菜品保持不变
                strncpy(lines[line_count], line, sizeof(lines[0]) - 1);
                lines[line_count][sizeof(lines[0]) - 1] = '\0';
            }
        }
        else
        {
            // 格式不正确的行保持不变
            strncpy(lines[line_count], line, sizeof(lines[0]) - 1);
            lines[line_count][sizeof(lines[0]) - 1] = '\0';
        }
        line_count++;
    }
    fclose(fp);

    if (!found)
    {
        lv_label_set_text(msg_label, "未找到该菜品!");
        lv_obj_set_style_text_font(msg_label, &lv_mysongti_font_20, 0);
        return;
    }

    // 写回文件
    fp = fopen("data/dishes.txt", "w");
    if (!fp)
    {
        lv_label_set_text(msg_label, "无法写入菜品文件!");
        lv_obj_set_style_text_font(msg_label, &lv_mysongti_font_20, 0);
        return;
    }

    for (int i = 0; i < line_count; i++)
    {
        fprintf(fp, "%s\n", lines[i]);
    }
    fclose(fp);

    // 修改成功，关闭弹窗和键盘，刷新列表
    if (g_popup_keyboard)
    {
        lv_obj_del(g_popup_keyboard);
        g_popup_keyboard = NULL;
    }
    if (g_popup_mask)
    {
        lv_obj_del(g_popup_mask);
        g_popup_mask = NULL;
    }
    g_popup_price_ta = NULL;
    g_popup_msg_label = NULL;
    refresh_price_list();
}

// 菜品选择事件回调 - 创建遮罩层和弹窗
static void dish_select_event_cb(lv_event_t *e)
{
    const char *dish_name = (const char *)lv_event_get_user_data(e);
    strncpy(g_selected_dish, dish_name, sizeof(g_selected_dish) - 1);
    g_selected_dish[sizeof(g_selected_dish) - 1] = '\0';
    lv_obj_t *scr = lv_scr_act();
    // 查找原价
    char orig_price[128] = "";
    FILE *fp = fopen("data/dishes.txt", "r");
    if (fp)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp))
        {
            char *newline = strchr(line, '\n');
            if (newline)
                *newline = '\0';
            char name[128], price[128];
            if (sscanf(line, "%127s %127s", name, price) == 2)
            {
                if (strcmp(name, dish_name) == 0)
                {
                    strncpy(orig_price, price, sizeof(orig_price) - 1);
                    orig_price[sizeof(orig_price) - 1] = '\0';
                    break;
                }
            }
        }
        fclose(fp);
    }
    // 创建遮罩层
    g_popup_mask = lv_obj_create(scr);
    lv_obj_set_size(g_popup_mask, lv_obj_get_width(scr), lv_obj_get_height(scr));
    lv_obj_align(g_popup_mask, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(g_popup_mask, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(g_popup_mask, LV_OPA_50, 0);
    lv_obj_set_style_border_width(g_popup_mask, 0, 0);
    lv_obj_add_flag(g_popup_mask, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(g_popup_mask, LV_OBJ_FLAG_SCROLLABLE);

    // 弹窗作为遮罩层的子对象
    lv_obj_t *popup = lv_obj_create(g_popup_mask);
    lv_obj_set_size(popup, 400, 300);
    lv_obj_align(popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(popup, lv_color_white(), 0);
    lv_obj_set_style_border_width(popup, 2, 0);
    lv_obj_set_style_border_color(popup, lv_color_black(), 0);
    lv_obj_set_style_radius(popup, 10, 0);
    // 设置弹窗不可滚动
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    // 关闭按钮 (X)
    lv_obj_t *close_btn = lv_btn_create(popup);
    lv_obj_set_size(close_btn, 30, 30);
    // 设置按钮颜色
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xff0000), 0);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "X");
    lv_obj_center(close_lbl);
    lv_obj_set_style_text_font(close_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(close_btn, close_popup_event_cb, LV_EVENT_CLICKED, NULL);

    // 显示选中的菜品名称
    lv_obj_t *dish_label = lv_label_create(popup);
    char dish_text[256];
    snprintf(dish_text, sizeof(dish_text), "当前菜品:%s", dish_name);
    lv_label_set_text(dish_label, dish_text);
    lv_obj_set_style_text_font(dish_label, &lv_mysongti_font_20, 0);
    lv_obj_align(dish_label, LV_ALIGN_TOP_MID, 0, 10);

    // 显示原价
    lv_obj_t *orig_label = lv_label_create(popup);
    char orig_text[128];
    snprintf(orig_text, sizeof(orig_text), "原价:%s", orig_price);
    lv_label_set_text(orig_label, orig_text);
    lv_obj_set_style_text_font(orig_label, &lv_mysongti_font_20, 0);
    lv_obj_align_to(orig_label, dish_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // 新价格输入框
    g_popup_price_ta = lv_textarea_create(popup);
    lv_obj_set_size(g_popup_price_ta, 300, 50);
    lv_obj_align_to(g_popup_price_ta, orig_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_text_font(g_popup_price_ta, &lv_mysongti_font_20, 0);
    lv_textarea_set_placeholder_text(g_popup_price_ta, "请输入新价格");
    lv_obj_add_event_cb(g_popup_price_ta, price_ta_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(g_popup_price_ta, price_ta_unfocus_event_cb, LV_EVENT_DEFOCUSED, NULL);

    // 保存按钮
    lv_obj_t *save_btn = lv_btn_create(popup);
    lv_obj_set_size(save_btn, 120, 50);
    lv_obj_align_to(save_btn, g_popup_price_ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "保存修改");
    lv_obj_center(save_lbl);
    lv_obj_set_style_text_font(save_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(save_btn, save_price_event_cb, LV_EVENT_CLICKED, NULL);

    // 消息提示标签
    g_popup_msg_label = lv_label_create(popup);
    lv_obj_align_to(g_popup_msg_label, save_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_label_set_text(g_popup_msg_label, "");
    lv_obj_set_style_text_font(g_popup_msg_label, &lv_mysongti_font_20, 0);
}

static void modify_price_event_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);

    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "选择要修改价格的菜品");
    lv_obj_set_style_text_font(title, &lv_mysongti_font_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_center(back_lbl);
    lv_obj_set_style_text_font(back_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(back_btn, back_to_admin_event_cb, LV_EVENT_CLICKED, NULL);

    // 创建列表容器
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 400, 400);
    lv_obj_align_to(list, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_text_font(list, &lv_mysongti_font_20, 0);

    // 读取菜品文件并创建列表项
    FILE *fp = fopen("data/dishes.txt", "r");
    if (!fp)
    {
        lv_obj_t *error_label = lv_label_create(scr);
        lv_label_set_text(error_label, "无法打开菜品文件!");
        lv_obj_set_style_text_font(error_label, &lv_mysongti_font_20, 0);
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    char line[256];
    int item_count = 0;

    while (fgets(line, sizeof(line), fp) && item_count < 50) // 最多显示50个菜品
    {
        // 去除换行符
        char *newline = strchr(line, '\n');
        if (newline)
            *newline = '\0';

        // 解析菜品名称和价格
        char dish_name[128];
        char price[128];
        if (sscanf(line, "%127s %127s", dish_name, price) == 2)
        {
            // 创建列表项文本
            char item_text[256];
            snprintf(item_text, sizeof(item_text), "%s  ￥%s", dish_name, price);

            // 创建列表项（无图标，防止乱码）
            lv_obj_t *list_item = lv_list_add_btn(list, NULL, item_text);

            // 为列表项分配菜品名称的内存并设置事件
            char *dish_name_copy = malloc(strlen(dish_name) + 1);
            strcpy(dish_name_copy, dish_name);
            lv_obj_add_event_cb(list_item, dish_select_event_cb, LV_EVENT_CLICKED, dish_name_copy);

            item_count++;
        }
    }
    fclose(fp);

    if (item_count == 0)
    {
        lv_obj_t *no_dish_label = lv_label_create(scr);
        lv_label_set_text(no_dish_label, "没有找到菜品!");
        lv_obj_set_style_text_font(no_dish_label, &lv_mysongti_font_20, 0);
        lv_obj_align(no_dish_label, LV_ALIGN_CENTER, 0, 0);
    }
}

// 创建管理后台界面
void ui_admin_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);

    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "管理后台");
    lv_obj_set_style_text_font(title, &lv_mysongti_font_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    int col_w = 200;  // 每列宽度
    int col_gap = 40; // 列间距
    int btn_h = 40;
    int btn_gap = 20;
    int base_y = 60;

    // 第一列：菜品管理
    lv_obj_t *cat_mgmt = lv_label_create(scr);
    lv_label_set_text(cat_mgmt, "菜品管理");
    lv_obj_set_style_text_font(cat_mgmt, &lv_mysongti_font_20, 0);
    lv_obj_align(cat_mgmt, LV_ALIGN_TOP_LEFT, 20, base_y);

    const char *btn_names1[] = {"添加新菜品", "删除菜品", "修改价格", "更改菜品图片"};
    for (int i = 0; i < 4; ++i)
    {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, col_w, btn_h);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 20, base_y + 30 + i * (btn_h + btn_gap));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names1[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_mysongti_font_20, 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    // 第二列：统计分析
    int col2_x = 20 + col_w + col_gap;
    lv_obj_t *stat = lv_label_create(scr);
    lv_label_set_text(stat, "统计分析");
    lv_obj_set_style_text_font(stat, &lv_mysongti_font_20, 0);
    lv_obj_align(stat, LV_ALIGN_TOP_LEFT, col2_x, base_y);

    const char *btn_names2[] = {"营业额总计", "各菜品销量统计"};
    for (int i = 0; i < 2; ++i)
    {
        lv_obj_t *btn = lv_btn_create(scr);
        lv_obj_set_size(btn, col_w, btn_h);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, col2_x, base_y + 30 + i * (btn_h + btn_gap));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names2[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_mysongti_font_20, 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(10 + i));
    }

    // 第三列：管理退出
    int col3_x = col2_x + col_w + col_gap;
    lv_obj_t *exit_label = lv_label_create(scr);
    lv_label_set_text(exit_label, "管理退出");
    lv_obj_set_style_text_font(exit_label, &lv_mysongti_font_20, 0);
    lv_obj_align(exit_label, LV_ALIGN_TOP_LEFT, col3_x, base_y);

    lv_obj_t *btn_admin_acount = lv_btn_create(scr);
    lv_obj_set_size(btn_admin_acount, col_w, btn_h);
    lv_obj_align(btn_admin_acount, LV_ALIGN_TOP_LEFT, col3_x, base_y + 30);
    lv_obj_t *lbl_admin_acount = lv_label_create(btn_admin_acount);
    lv_label_set_text(lbl_admin_acount, "管理员账号信息");
    lv_obj_center(lbl_admin_acount);
    lv_obj_set_style_text_font(lbl_admin_acount, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_admin_acount, btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)12);

    lv_obj_t *btn_exit = lv_btn_create(scr);
    lv_obj_set_size(btn_exit, col_w, btn_h);
    lv_obj_align(btn_exit, LV_ALIGN_TOP_LEFT, col3_x, base_y + 30 + btn_h + btn_gap);
    lv_obj_t *lbl_exit = lv_label_create(btn_exit);
    lv_label_set_text(lbl_exit, "返回登录页");
    lv_obj_center(lbl_exit);
    lv_obj_set_style_text_font(lbl_exit, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn_exit, btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)100);
}

// 按钮事件回调（后续可根据参数区分功能）
static void btn_event_cb(lv_event_t *e)
{
    int btn_id = (int)(intptr_t)lv_event_get_user_data(e);
    switch (btn_id)
    {
    case 0: /* 添加新菜品 */
        break;
    case 1: /* 删除菜品 */
        break;
    case 2: /* 修改价格 */
        modify_price_event_cb(e);
        break;
    case 3: /* 更改图片与分类 */
        break;
    case 10: /* 营业额总计 */
        break;
    case 11: /* 各菜品销量统计 */
        break;
    case 12: /* 管理员账号信息 */
        break;
    case 100: /* 返回登录页 */
        lv_obj_clean(lv_scr_act());
        ui_login_create();
        break;
    default:
        break;
    }
}