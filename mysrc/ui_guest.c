#include "ui_guest.h"
#include "guest_my.h"
#include "lvgl/lvgl.h"
#include "lv_mysongti_font_20.h"
#include "lv_mysongti_font_30.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define DISHES_FILE "data/dishes.txt"
#define MAX_DISHES 100
#define DISH_WIN_W 320
#define DISH_WIN_H 150

static lv_obj_t *dish_cont = NULL;
static cart_t g_cart = {0}; // 全局购物车

// 购物车添加菜品
void ui_cart_add_item(const char *name, const char *price, int quantity)
{
    if (quantity <= 0)
        return;

    // 检查是否已存在该菜品
    for (int i = 0; i < g_cart.item_count; i++)
    {
        if (strcmp(g_cart.items[i].name, name) == 0)
        {
            g_cart.items[i].quantity += quantity;
            g_cart.items[i].total_price = g_cart.items[i].quantity * atof(g_cart.items[i].price);
            return;
        }
    }

    // 添加新菜品
    if (g_cart.item_count < 100)
    {
        strncpy(g_cart.items[g_cart.item_count].name, name, sizeof(g_cart.items[g_cart.item_count].name) - 1);
        strncpy(g_cart.items[g_cart.item_count].price, price, sizeof(g_cart.items[g_cart.item_count].price) - 1);
        g_cart.items[g_cart.item_count].quantity = quantity;
        g_cart.items[g_cart.item_count].total_price = quantity * atof(price);
        g_cart.item_count++;
    }
}

// 保存订单到文件
void ui_cart_save_order(void)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char filename[128];
    snprintf(filename, sizeof(filename), "data/order/order_%04d%02d%02d_%02d%02d%02d.txt",
             tm_info->tm_year + 1910, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    FILE *fp = fopen(filename, "w");
    if (!fp)
        return;

    fprintf(fp, "订单时间: %4d-%d-%d %d:%d:%d\n",
            tm_info->tm_year + 1910,
            tm_info->tm_mon + 1,
            tm_info->tm_mday,
            tm_info->tm_hour,
            tm_info->tm_min,
            tm_info->tm_sec);
    fprintf(fp, "\n");
    for (int i = 0; i < g_cart.item_count; i++)
    {
        fprintf(fp, "%-14s %-8s %-4d %-8.2f\n",
                g_cart.items[i].name,
                g_cart.items[i].price,
                g_cart.items[i].quantity,
                g_cart.items[i].total_price);
    }
    fprintf(fp, "\n");
    fprintf(fp, "总计: ￥%.2f\n", g_cart.total_amount);
    fclose(fp);
}

// 更新购物车中指定菜品的数量
static void update_cart_item_quantity(const char *name, const char *price, int new_quantity)
{
    // 查找菜品是否已在购物车中
    for (int i = 0; i < g_cart.item_count; i++)
    {
        if (strcmp(g_cart.items[i].name, name) == 0)
        {
            if (new_quantity <= 0)
            {
                // 数量为0，从购物车中移除
                for (int j = i; j < g_cart.item_count - 1; j++)
                {
                    g_cart.items[j] = g_cart.items[j + 1];
                }
                g_cart.item_count--;
            }
            else
            {
                // 更新数量和小计
                g_cart.items[i].quantity = new_quantity;
                g_cart.items[i].total_price = new_quantity * atof(g_cart.items[i].price);
            }
            return;
        }
    }

    // 菜品不在购物车中，且数量大于0
    if (new_quantity > 0 && g_cart.item_count < 100)
    {
        strncpy(g_cart.items[g_cart.item_count].name, name, sizeof(g_cart.items[g_cart.item_count].name) - 1);
        strncpy(g_cart.items[g_cart.item_count].price, price, sizeof(g_cart.items[g_cart.item_count].price) - 1);
        g_cart.items[g_cart.item_count].quantity = new_quantity;
        g_cart.items[g_cart.item_count].total_price = new_quantity * atof(price);
        g_cart.item_count++;
    }
}

// 数量加减事件
// 数量加事件
static void dish_add_event_cb(lv_event_t *e)
{
    lv_obj_t *qty = (lv_obj_t *)lv_event_get_user_data(e);
    int n = atoi(lv_label_get_text(qty)) + 1;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", n);
    lv_label_set_text(qty, buf);

    // 获取菜品信息
    lv_obj_t *panel = lv_obj_get_parent(qty);
    lv_obj_t *name_win = lv_obj_get_child(panel, 1);
    lv_obj_t *name_lbl = lv_obj_get_child(name_win, 0);
    lv_obj_t *price_lbl = lv_obj_get_child(name_win, 1);

    const char *name = lv_label_get_text(name_lbl);
    const char *price_text = lv_label_get_text(price_lbl);

    // 从 price_text 中找到第一个数字或小数点的位置
    const char *p = price_text;
    while (*p && !(isdigit((unsigned char)*p) || *p == '.'))
    {
        p++;
    }
    update_cart_item_quantity(name, p, n);
}

// 数量减事件
static void dish_sub_event_cb(lv_event_t *e)
{
    lv_obj_t *qty = (lv_obj_t *)lv_event_get_user_data(e);
    int n = atoi(lv_label_get_text(qty));
    if (n > 0)
        n--;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", n);
    lv_label_set_text(qty, buf);

    // 获取菜品信息
    lv_obj_t *panel = lv_obj_get_parent(qty);
    lv_obj_t *name_win = lv_obj_get_child(panel, 1);
    lv_obj_t *name_lbl = lv_obj_get_child(name_win, 0);
    lv_obj_t *price_lbl = lv_obj_get_child(name_win, 1);

    const char *name = lv_label_get_text(name_lbl);
    const char *price_text = lv_label_get_text(price_lbl);

    // 同样扫描到第一个数字或小数点
    const char *p = price_text;
    while (*p && !(isdigit((unsigned char)*p) || *p == '.'))
    {
        p++;
    }

    update_cart_item_quantity(name, p, n);
}

// 购物车按钮事件
static void cart_btn_event_cb(lv_event_t *e)
{
    ui_cart_create();
}

// 返回主界面事件
static void back_to_main_event_cb(lv_event_t *e)
{
    ui_guest_create();
}

// 付款成功界面
static void ui_payment_success_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // 设置scr背景为白色
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xffffff), 0);

    // 创建小票背景容器，颜色为深灰色
    lv_obj_t *ticket_bg = lv_obj_create(scr);
    lv_obj_set_size(ticket_bg, 280, 350);
    lv_obj_set_style_bg_color(ticket_bg, lv_color_hex(0xcccccc), 0); // 浅灰色
    lv_obj_set_style_radius(ticket_bg, 20, 0);
    lv_obj_clear_flag(ticket_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(ticket_bg, LV_ALIGN_CENTER, 0, -20);

    // 小票内容标签
    lv_obj_t *lbl_ticket = lv_label_create(ticket_bg);
    lv_label_set_text(lbl_ticket, "感谢您的光临!\n\n");
    lv_obj_set_style_text_font(lbl_ticket, &lv_mysongti_font_20, 0);
    lv_obj_set_style_text_align(lbl_ticket, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_ticket, LV_ALIGN_TOP_MID, 0, 10);

    // 随机显示一个订单编号
    char order_id[32];
    snprintf(order_id, sizeof(order_id), "# %04d", rand() / 1000);
    lv_obj_t *order_label = lv_label_create(ticket_bg);
    lv_label_set_text(order_label, order_id);
    lv_obj_set_style_text_font(order_label, &lv_font_montserrat_38, 0);
    lv_obj_align(order_label, LV_ALIGN_TOP_MID, 0, 80);

    lv_obj_t *lbl_order = lv_label_create(ticket_bg);
    lv_label_set_text(lbl_order, "订单已提交\n请耐心等待");
    lv_obj_set_style_text_font(lbl_order, &lv_mysongti_font_20, 0);
    lv_obj_align(lbl_order, LV_ALIGN_BOTTOM_MID, 0, -25);

    // 返回主界面按钮
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "返回主界面");
    lv_obj_center(btn_lbl);
    lv_obj_set_style_text_font(btn_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(btn, back_to_main_event_cb, LV_EVENT_CLICKED, NULL);
}

// 提交订单并清空购物车事件
static void submit_order_event_cb(lv_event_t *e)
{
    ui_cart_save_order();
    // 清空购物车
    g_cart.item_count = 0;
    g_cart.total_amount = 0;
    ui_payment_success_create();
}

// 关闭确认弹窗事件
static void close_confirm_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *popup = lv_obj_get_parent(btn);
    lv_obj_t *mask = lv_obj_get_parent(popup);
    lv_obj_del(mask);
}

// 确认付款弹窗事件
static void confirm_payment_event_cb(lv_event_t *e)
{
    // 创建确认付款弹窗前，先创建遮罩层
    lv_obj_t *scr = lv_screen_active();
    // 1. 创建遮罩层
    lv_obj_t *mask = lv_obj_create(scr);
    lv_obj_set_size(mask, lv_obj_get_width(scr), lv_obj_get_height(scr));
    lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(mask, LV_OPA_50, 0); // 半透明
    lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(mask, LV_OBJ_FLAG_CLICKABLE); // 拦截事件

    // 2. 创建弹窗，父对象设为mask
    lv_obj_t *confirm_popup = lv_obj_create(mask);
    lv_obj_set_size(confirm_popup, 600, 400);
    lv_obj_align(confirm_popup, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(confirm_popup, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(confirm_popup, 1, 0);
    lv_obj_set_style_border_color(confirm_popup, lv_color_hex(0xcccccc), 0);
    lv_obj_clear_flag(confirm_popup, LV_OBJ_FLAG_SCROLLABLE);

    const char payment_pic_path[128] = {"A:/home/menu_master/data/image/weixin.bmp"};
    // 在按钮上显示照片
    lv_obj_t *photo = lv_img_create(confirm_popup);
    lv_img_set_src(photo, payment_pic_path);
    lv_obj_align(photo, LV_ALIGN_CENTER, -10, 20);
    lv_obj_set_size(photo, 400, 200);
    lv_obj_set_style_radius(photo, 20, 0);

    // 标题
    lv_obj_t *title = lv_label_create(confirm_popup);
    lv_label_set_text(title, "确认付款");
    lv_obj_align(title, LV_ALIGN_TOP_MID, -5, 0);
    lv_obj_set_style_text_font(title, &lv_mysongti_font_20, 0);

    // 显示当前金额
    double total = g_cart.total_amount;
    int total_int = (int)total;
    int total_frac = (int)((total - total_int) * 100 + 0.5);
    lv_obj_t *amount_lbl = lv_label_create(confirm_popup);
    // 设置标签大小
    lv_obj_set_size(amount_lbl, 200, 40);
    lv_label_set_text_fmt(amount_lbl, "%d.%02d", total_int, total_frac);
    lv_obj_align(amount_lbl, LV_ALIGN_TOP_MID, 10, 60);
    lv_obj_set_style_text_font(amount_lbl, &lv_font_montserrat_48, 0);

    // 确认按钮
    lv_obj_t *confirm_btn = lv_btn_create(confirm_popup);
    lv_obj_set_size(confirm_btn, 120, 40);
    lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t *confirm_lbl = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_lbl, "确认付款");
    lv_obj_center(confirm_lbl);
    lv_obj_set_style_text_font(confirm_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(confirm_btn, submit_order_event_cb, LV_EVENT_CLICKED, NULL);

    // 取消按钮
    lv_obj_t *cancel_btn = lv_btn_create(confirm_popup);
    lv_obj_set_size(cancel_btn, 40, 40);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xff0000), 0);
    lv_obj_set_style_radius(cancel_btn, 40, 0);
    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "x");
    lv_obj_center(cancel_lbl);
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_28, 0);

    // 添加关闭事件
    lv_obj_add_event_cb(cancel_btn, close_confirm_event_cb, LV_EVENT_CLICKED, NULL);
}

// 读取菜品数据（标准IO实现，去除空格，分离名字和价格）
int ui_guest_load_dishes(dish_info_t *dishes, int max_count)
{
    FILE *fp = fopen(DISHES_FILE, "r");
    if (!fp)
        return 0;
    char line[512];
    int n = 0;
    while (n < max_count && fgets(line, sizeof(line), fp))
    {
        // 去除行首尾空格和换行
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            ++p;
        char *q = p + strlen(p) - 1;
        while (q > p && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n'))
            *q-- = '\0';
        if (*p == '\0')
            continue;
        // 拆分名字和价格
        char *space1 = strchr(p, ' ');
        if (space1 && *(space1 + 1))
        {
            *space1 = '\0';
            strncpy(dishes[n].name, p, sizeof(dishes[n].name) - 1);
            dishes[n].name[sizeof(dishes[n].name) - 1] = 0;
            strncpy(dishes[n].price, space1 + 1, sizeof(dishes[n].price) - 1);
            dishes[n].price[sizeof(dishes[n].price) - 1] = 0;
        }
        else
        {
            strncpy(dishes[n].name, p, sizeof(dishes[n].name) - 1);
            dishes[n].name[sizeof(dishes[n].name) - 1] = 0;
            dishes[n].price[0] = 0;
        }
        n++;
    }
    fclose(fp);
    return n;
}

// 按钮“我的” 回调函数
static void my_btn_event_cb(lv_event_t *e)
{
    // 创建“我的”界面
    guest_my_create();
}

// 主界面
void ui_guest_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    dish_cont = NULL;

    // 创建菜品主窗口
    lv_obj_t *dish_win = lv_obj_create(scr);
    lv_obj_set_size(dish_win, 800, 480);
    lv_obj_align(dish_win, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(dish_win, lv_color_hex(0xffffff), 0);
    lv_obj_set_scroll_dir(dish_win, LV_DIR_VER);
    lv_obj_set_style_pad_row(dish_win, 20, 0);
    lv_obj_set_style_pad_column(dish_win, 20, 0);
    lv_obj_clear_flag(dish_win, LV_OBJ_FLAG_SCROLLABLE);

    // 菜品区域：放在窗口内
    dish_cont = lv_obj_create(dish_win);
    lv_obj_set_size(dish_cont, 700, 480); // 设置为实际可视高度，超出可滑动
    lv_obj_align(dish_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_scroll_dir(dish_cont, LV_DIR_VER);
    lv_obj_set_style_pad_row(dish_cont, 20, 0);
    lv_obj_set_style_pad_column(dish_cont, 20, 0);
    lv_obj_set_style_bg_opa(dish_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(dish_cont, 0, 0);

    // 读取菜品数据
    dish_info_t dishes[MAX_DISHES];
    int dish_count = ui_guest_load_dishes(dishes, MAX_DISHES);
    int col = 0, row = 0;
    for (int i = 0; i < dish_count; ++i)
    {
        int x = (col % 2) * (DISH_WIN_W + 20) + 10;
        int y = (row) * (DISH_WIN_H + 20);
        lv_obj_t *panel = lv_obj_create(dish_cont);
        lv_obj_set_size(panel, DISH_WIN_W, DISH_WIN_H);
        lv_obj_set_style_radius(panel, 12, 0);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0xf8f8f8), 0);
        lv_obj_set_style_border_width(panel, 1, 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0xcccccc), 0);
        lv_obj_set_pos(panel, x, y);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        // 左侧图片（占满左半部分）
        lv_obj_t *img = lv_image_create(panel);
        // 读取菜品图片 图片路径为 "data/image/1.bmp"
        char img_path[128];
        snprintf(img_path, sizeof(img_path), "A:/home/menu_master/data/image/%d.bmp", i + 1);

        lv_image_set_src(img, img_path);
        lv_obj_set_size(img, DISH_WIN_W / 2, DISH_WIN_H);
        lv_obj_align(img, LV_ALIGN_TOP_LEFT, -20, -20);

        // 创建一个新的窗口，在这个上面放名称和价格的信息
        lv_obj_t *name_win = lv_obj_create(panel);
        lv_obj_set_size(name_win, DISH_WIN_W / 2 - 4, DISH_WIN_H / 2);
        lv_obj_align(name_win, LV_ALIGN_TOP_RIGHT, 20, -20);
        // 设置名称窗口的颜色
        lv_style_t name_win_style;
        lv_style_init(&name_win_style);
        lv_style_set_bg_color(&name_win_style, lv_color_hex(0xf8f8f8));
        lv_style_set_radius(&name_win_style, 12);
        lv_obj_add_style(name_win, &name_win_style, 0);
        lv_obj_clear_flag(name_win, LV_OBJ_FLAG_SCROLLABLE); // 禁止信息窗口滑动

        // 右侧上方显示名字
        lv_obj_t *name_lbl = lv_label_create(name_win);
        lv_label_set_text(name_lbl, dishes[i].name);
        lv_obj_align(name_lbl, LV_ALIGN_CENTER, 0, -15);
        lv_obj_set_style_text_font(name_lbl, &lv_mysongti_font_20, 0);

        // 右侧显示价格
        lv_obj_t *price_lbl = lv_label_create(name_win);
        lv_label_set_text_fmt(price_lbl, "￥%s", dishes[i].price);
        lv_obj_align(price_lbl, LV_ALIGN_CENTER, -0, 15);
        lv_obj_set_style_text_font(price_lbl, &lv_mysongti_font_20, 0);

        // 数量与加减（右侧下方，竖直分布）
        lv_obj_t *qty = lv_label_create(panel);
        // 从g_cart中获取该菜品的初始数量
        int initial_qty = 0;
        for (int j = 0; j < g_cart.item_count; j++)
        {
            if (strcmp(g_cart.items[j].name, dishes[i].name) == 0)
            {
                initial_qty = g_cart.items[j].quantity;
                break;
            }
        }
        char qty_buf[8];
        snprintf(qty_buf, sizeof(qty_buf), "%d", initial_qty);
        lv_label_set_text(qty, qty_buf);
        lv_obj_align(qty, LV_ALIGN_BOTTOM_RIGHT, -50, 0);
        lv_obj_set_style_text_font(qty, &lv_mysongti_font_20, 0);

        // 加减按钮（右下角）
        lv_obj_t *btn_sub = lv_btn_create(panel);
        lv_obj_set_size(btn_sub, 32, 32);
        lv_obj_align(btn_sub, LV_ALIGN_BOTTOM_RIGHT, -80, 0);
        lv_obj_t *lbl_sub = lv_label_create(btn_sub);
        lv_label_set_text(lbl_sub, "-");
        lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_32, 0);
        lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, -1);

        lv_obj_t *btn_add = lv_btn_create(panel);
        lv_obj_set_size(btn_add, 32, 32);
        lv_obj_align(btn_add, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_t *lbl_add = lv_label_create(btn_add);
        lv_label_set_text(lbl_add, "+");
        lv_obj_set_style_text_font(lbl_add, &lv_font_montserrat_32, 0);
        lv_obj_center(lbl_add);

        lv_obj_add_event_cb(btn_add, dish_add_event_cb, LV_EVENT_CLICKED, qty);
        lv_obj_add_event_cb(btn_sub, dish_sub_event_cb, LV_EVENT_CLICKED, qty);

        col++;
        if (col == 2)
        {
            col = 0;
            row++;
        }
    }
    // 购物车按钮（页面最右下角）
    // lv_obj_t *cart_btn = lv_btn_create(scr);
    // lv_obj_set_size(cart_btn, 50, 50);
    // lv_obj_align(cart_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -20);
    // lv_obj_t *cart_lbl = lv_image_create(cart_btn);
    // lv_image_set_src(cart_lbl, "A:/home/menu_master/data/image/gouwuche.bmp");
    // lv_obj_center(cart_lbl);
    // lv_obj_add_event_cb(cart_btn, cart_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cart_lbl = lv_image_create(scr);
    lv_image_set_src(cart_lbl, "A:/home/menu_master/data/image/gouwuche.bmp");
    lv_obj_set_size(cart_lbl, 50, 50);
    lv_obj_align(cart_lbl, LV_ALIGN_BOTTOM_RIGHT, -5, -20);
    lv_obj_add_flag(cart_lbl, LV_OBJ_FLAG_CLICKABLE); // 使图片可点击
    lv_obj_add_event_cb(cart_lbl, cart_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // "我的"按钮（页面最左下角）
    lv_obj_t *me_bnt = lv_btn_create(scr);
    lv_obj_set_size(me_bnt, 60, 60);
    lv_obj_align(me_bnt, LV_ALIGN_BOTTOM_LEFT, 15, -20);
    lv_obj_t *me_lbl = lv_label_create(me_bnt);
    lv_label_set_text(me_lbl, "我的");
    lv_obj_center(me_lbl);
    lv_obj_set_style_text_font(me_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(me_bnt, my_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// 创建购物车页面
void ui_cart_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    // 购物车为空时弹窗提示
    if (g_cart.item_count == 0)
    {
        lv_obj_t *mask = lv_obj_create(scr);
        lv_obj_set_size(mask, lv_obj_get_width(scr), lv_obj_get_height(scr));
        lv_obj_set_style_bg_color(mask, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(mask, LV_OPA_50, 0); // 半透明
        lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(mask, LV_OBJ_FLAG_CLICKABLE); // 拦截事件
        
        lv_obj_t *popup = lv_obj_create(mask);
        lv_obj_set_size(popup, 320, 180);
        lv_obj_align(popup, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(popup, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_border_width(popup, 1, 0);
        lv_obj_set_style_border_color(popup, lv_color_hex(0xcccccc), 0);

        lv_obj_t *label = lv_label_create(popup);
        lv_label_set_text(label, "购物车为空!");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
        lv_obj_set_style_text_font(label, &lv_mysongti_font_20, 0);

        lv_obj_t *btn = lv_btn_create(popup);
        lv_obj_set_size(btn, 100, 40);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
        lv_obj_t *btn_lbl = lv_label_create(btn);
        lv_label_set_text(btn_lbl, "返回");
        lv_obj_center(btn_lbl);
        lv_obj_set_style_text_font(btn_lbl, &lv_mysongti_font_20, 0);
        lv_obj_add_event_cb(btn, back_to_main_event_cb, LV_EVENT_CLICKED, NULL);
        return;
    }

    lv_obj_clean(scr);
    // 标题
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "购物车");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_mysongti_font_30, 0);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_set_style_text_font(back_lbl, &lv_mysongti_font_20, 0);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, back_to_main_event_cb, LV_EVENT_CLICKED, NULL);

    // 表头
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 740, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x878787), 0);
    lv_obj_set_style_radius(header, 8, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name_header = lv_label_create(header);
    lv_label_set_text(name_header, "菜品名称");
    lv_obj_align(name_header, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_text_font(name_header, &lv_mysongti_font_20, 0);

    lv_obj_t *price_header = lv_label_create(header);
    lv_label_set_text(price_header, "单价");
    lv_obj_align(price_header, LV_ALIGN_LEFT_MID, 235, 0);
    lv_obj_set_style_text_font(price_header, &lv_mysongti_font_20, 0);

    lv_obj_t *qty_header = lv_label_create(header);
    lv_label_set_text(qty_header, "数量");
    lv_obj_align(qty_header, LV_ALIGN_LEFT_MID, 380, 0);
    lv_obj_set_style_text_font(qty_header, &lv_mysongti_font_20, 0);

    lv_obj_t *total_header = lv_label_create(header);
    lv_label_set_text(total_header, "小计");
    lv_obj_align(total_header, LV_ALIGN_LEFT_MID, 500, 0);
    lv_obj_set_style_text_font(total_header, &lv_mysongti_font_20, 0);

    // 购物车内容区域
    lv_obj_t *cart_cont = lv_obj_create(scr);
    lv_obj_set_size(cart_cont, 760, 220); // 高度可根据实际界面调整
    lv_obj_align(cart_cont, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_scroll_dir(cart_cont, LV_DIR_VER);
    lv_obj_set_style_pad_row(cart_cont, 10, 0);
    lv_obj_set_style_bg_opa(cart_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(cart_cont, 0, 0);

    // 显示购物车项目
    g_cart.total_amount = 0;
    int y_offset = 0;

    for (int i = 0; i < g_cart.item_count; i++)
    {
        lv_obj_t *item = lv_obj_create(cart_cont);
        lv_obj_set_size(item, 700, 50);
        lv_obj_align(item, LV_ALIGN_TOP_MID, 0, y_offset);
        lv_obj_set_style_bg_color(item, lv_color_hex(0xf8f8f8), 0);
        lv_obj_set_style_radius(item, 8, 0);
        lv_obj_set_style_border_width(item, 1, 0);
        lv_obj_set_style_border_color(item, lv_color_hex(0xe0e0e0), 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        // 菜品名称
        lv_obj_t *name_lbl = lv_label_create(item);
        lv_label_set_text(name_lbl, g_cart.items[i].name);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 20, 0);
        lv_obj_set_style_text_font(name_lbl, &lv_mysongti_font_20, 0);

        // 单价
        lv_obj_t *price_lbl = lv_label_create(item);
        lv_label_set_text_fmt(price_lbl, "￥%s", g_cart.items[i].price);
        lv_obj_align(price_lbl, LV_ALIGN_LEFT_MID, 200, 0);
        lv_obj_set_style_text_font(price_lbl, &lv_mysongti_font_20, 0);

        // 数量
        lv_obj_t *qty_lbl = lv_label_create(item);
        lv_label_set_text_fmt(qty_lbl, "%d", g_cart.items[i].quantity);
        lv_obj_align(qty_lbl, LV_ALIGN_LEFT_MID, 380, 0);
        lv_obj_set_style_text_font(qty_lbl, &lv_font_montserrat_20, 0);

        // 小计
        double price = g_cart.items[i].total_price;
        int int_part = (int)price;
        int frac_part = (int)((price - int_part) * 100 + 0.5);
        lv_obj_t *total_lbl = lv_label_create(item);
        lv_label_set_text_fmt(total_lbl, "￥%d.%02d", int_part, frac_part);
        lv_obj_align(total_lbl, LV_ALIGN_LEFT_MID, 480, 0);
        lv_obj_set_style_text_font(total_lbl, &lv_mysongti_font_20, 0);

        g_cart.total_amount += g_cart.items[i].total_price;
        y_offset += 60;
    }

    // 总计
    double total = g_cart.total_amount;
    int total_int = (int)total;
    int total_frac = (int)((total - total_int) * 100 + 0.5);
    lv_obj_t *total_item = lv_obj_create(scr);
    lv_obj_set_size(total_item, 740, 50);
    lv_obj_align(total_item, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(total_item, lv_color_hex(0xe8e8e8), 0);
    lv_obj_set_style_radius(total_item, 8, 0);
    lv_obj_set_style_border_width(total_item, 2, 0);
    lv_obj_set_style_border_color(total_item, lv_color_hex(0x4caf50), 0);
    lv_obj_clear_flag(total_item, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *total_label = lv_label_create(total_item);
    lv_label_set_text(total_label, "总计:");
    lv_obj_align(total_label, LV_ALIGN_LEFT_MID, 100, 0);
    lv_obj_set_style_text_font(total_label, &lv_mysongti_font_20, 0);

    lv_obj_t *total_amount = lv_label_create(total_item);
    lv_label_set_text_fmt(total_amount, "￥%d.%02d", total_int, total_frac);
    lv_obj_align(total_amount, LV_ALIGN_LEFT_MID, 450, 0);
    lv_obj_set_style_text_font(total_amount, &lv_mysongti_font_20, 0);

    // 提交订单按钮
    lv_obj_t *submit_btn = lv_btn_create(scr);
    lv_obj_set_size(submit_btn, 120, 50);
    lv_obj_align(submit_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(submit_btn, lv_color_hex(0x4caf50), 0);
    lv_obj_t *submit_lbl = lv_label_create(submit_btn);
    lv_label_set_text(submit_lbl, "确认订单");
    lv_obj_center(submit_lbl);
    lv_obj_set_style_text_font(submit_lbl, &lv_mysongti_font_20, 0);
    lv_obj_add_event_cb(submit_btn, confirm_payment_event_cb, LV_EVENT_CLICKED, NULL);
}
