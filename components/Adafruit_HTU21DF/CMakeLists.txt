# Adafruit Bus IO Library
# https://github.com/adafruit/Adafruit_BusIO
# MIT License

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "Adafruit_HTU21DF.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES arduino Adafruit-GFX-Library)

project(Adafruit_HTU21DF)
