cmake_minimum_required(VERSION 3.10)
project(YUV_BMP_Overlay)

set(CMAKE_CXX_STANDARD 17)

# Добавляем флаги компиляции
add_compile_options(-Wall -Wextra -O2)

# Добавляем исполняемый файл
add_executable(YUV_BMP_Overlay main.cpp)
