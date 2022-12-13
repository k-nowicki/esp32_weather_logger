cmake_minimum_required(VERSION 3.5)


idf_component_register(SRCS "src/kk_http_app_server.cpp" 
					   INCLUDE_DIRS "." "src/"
					   EMBED_TXTFILES "certs/cacert.pem" "certs/prvtkey.pem"
                       REQUIRES nvs_flash protocol_common esp_https_server)

project(kk_http_app_server)
