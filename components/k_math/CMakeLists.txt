# Adafruit Bus IO Library
# https://github.com/adafruit/Adafruit_BusIO
# MIT License

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "k_math.cpp" 
                       INCLUDE_DIRS ".")

project(k_math)
