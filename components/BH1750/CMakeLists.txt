cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/BH1750.cpp" 
                       INCLUDE_DIRS "." "src/"
                       REQUIRES arduino)

project(BH1750)
