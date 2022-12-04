# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# (Not part of the boilerplate)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

idf_component_register(SRCS "sntp_sync.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES nvs_flash protocol_common ErriezDS3231)

project(sntp_sync)
