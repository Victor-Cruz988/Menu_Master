#include "ui_admin.h"
#include "ui_login.h"
#include "lvgl/lvgl.h"
#include "lv_mysongti_font_20.h"

static void btn_event_cb(lv_event_t *e);// 按钮事件回调

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