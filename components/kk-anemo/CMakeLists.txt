cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/kk-anemo.cpp" 
                       INCLUDE_DIRS "." "src/"
                       REQUIRES arduino)

project(kk-anemo)
