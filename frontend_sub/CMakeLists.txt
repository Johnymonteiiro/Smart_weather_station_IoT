# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(monitor_app)

# Cria imagem SPIFFS a partir da pasta 'web' e a grava na partição 'storage'
spiffs_create_partition_image(storage web FLASH_IN_PROJECT)
