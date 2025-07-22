cmake_minimum_required(VERSION 3.10)

# 设置交叉编译
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
# 这里指定的路径必须是绝对路径
set(CMAKE_C_COMPILER /usr/local/arm-linux/bin//arm-linux-gcc)
set(CMAKE_CXX_COMPILER /usr/local/arm-linux/bin//arm-linux-g++)

project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

#add_executable(main main.c mouse_cursor_icon.c ./mysrc/ui_login.c ./mysrc/ui_admin.c ./mysrc/ui_guest.c ./mysrc/guest_my.c ./font/lv_mysongti_font_20.c ./font/lv_mysongti_font_30.c ./font/lv_mysongti_font_40.c ./font/gouwuche.c)
# 收集 mysrc 目录下所有 .c 文件
file(GLOB SRC_FILES "code/src/*.c")
# 收集 font 目录下所有 .c 文件
file(GLOB FONT_FILES "font/*.c")

add_executable(main
    main.c
    mouse_cursor_icon.c
    ${SRC_FILES}
    ${FONT_FILES}
)
include_directories(${PROJECT_SOURCE_DIR}/code/include)
include_directories(${PROJECT_SOURCE_DIR}/font)

# 注释掉这行，如果要使用DRM驱动，则需要放开这行
# include(${CMAKE_CURRENT_LIST_DIR}/lvgl/tests/FindLibDRM.cmake)
# include_directories(${Libdrm_INCLUDE_DIRS})

# 注释掉下面三个，这个是在PC端使用的
# find_package(SDL2)
# find_package(SDL2_image)
# include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

target_link_libraries(main lvgl lvgl::examples lvgl::demos lvgl::thorvg ${SDL2_LIBRARIES} m pthread)
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/main DEPENDS main)
