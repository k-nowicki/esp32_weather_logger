cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "connect.c" "stdin_out.c" "addr_from_stdin.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_netif
                    )

project(protocol_common)