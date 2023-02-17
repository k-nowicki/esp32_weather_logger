cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/ErriezDS1307.cpp" 
                       INCLUDE_DIRS "." "src/"
                       REQUIRES arduino)

project(ErriezDS1307)
