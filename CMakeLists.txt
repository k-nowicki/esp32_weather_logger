# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(EXCLUDE_COMPONENTS freemodbus)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

set(EXTRA_COMPONENT_DIRS </components/arduino/cores/esp32>)
set(EXTRA_COMPONENT_DIRS </components/arduino/libraries/SPI>)
set(EXTRA_COMPONENT_DIRS </components/arduino/libraries/Wire>)
set(EXTRA_COMPONENT_DIRS </components/dht11>)
set(EXTRA_COMPONENT_DIRS </components/k_math>)
set(EXTRA_COMPONENT_DIRS </components/BH1750/src>)
set(EXTRA_COMPONENT_DIRS </components/kk-anemo/src>)
set(EXTRA_COMPONENT_DIRS </components/sntp_sync>)
set(EXTRA_COMPONENT_DIRS </components/protocol_common)
#set(EXTRA_COMPONENT_DIRS </components/kk_hhtp_app)

project(${ProjectId})

