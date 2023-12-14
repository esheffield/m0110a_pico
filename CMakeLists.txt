cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(m0110a_pico_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(m0110a_pico
    m0110a_pico.c
)

pico_enable_stdio_usb(m0110a_pico 1)
pico_enable_stdio_uart(m0110a_pico 0)

pico_add_extra_outputs(m0110a_pico)

pico_generate_pio_header(m0110a_pico ${CMAKE_CURRENT_LIST_DIR}/m0110a_pico.pio)

target_link_libraries(m0110a_pico
    pico_stdlib
    hardware_pio
)
