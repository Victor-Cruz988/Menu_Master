#ifndef UI_GUEST_H
#define UI_GUEST_H

// 菜品结构体
typedef struct
{
    char name[64];
    char price[32];
    int quantity; // 购买数量
} dish_info_t;

// 购物车项目结构体
typedef struct
{
    char name[64];
    char price[32];
    int quantity;
    double total_price;
} cart_item_t;

// 购物车结构体
typedef struct
{
    cart_item_t items[1024];
    int item_count;
    double total_amount;
} cart_t;

void ui_guest_create(void);
void ui_cart_create(void);
void ui_cart_add_item(const char *name, const char *price, int quantity);
void ui_cart_save_order(void);

#endif
