/*
 * setup.h
 *
 *	Configuration for Weather Station
 *	This file contains:
 *		- PINs to peripherals connection settings
 *		- BUS devices addresses
 *		- Devices configuration (i.e. Display resolution)
 *		- Network setup
 *		- APP threads setup and RTOS config that is not configured by menuconfig
 *
 *  Created on: 30 lis 2022
 *      Author: Karol
 */

#ifndef MAIN_SETUP_H_
#define MAIN_SETUP_H_

/*******************************************************************************
 *  External Hardware setup
 *  (Sensors, Display, RTC, Buttons, etc)
 *
 */
//DHT11 sensor line
#define GPIO_DHT11    GPIO_NUM_13
//DS18B20 sensor line
#define GPIO_DS18B20  GPIO_NUM_16
//I2C Bus lines (OLED, RTC, BH1750, BMP280)
#define I2C_SDA 14
#define I2C_SCL 15
//I2C Device addrs
#define OLED_ADDR   0x3c
#define BH1750_ADDR 0x23
#define DS3231_ADDR 0x57
#define BMP280_ADDR 0x76
//OLED SCREEN
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)

/*******************************************************************************
 *  Network Setup
 */
#ifndef WIFISSID
#define WIFISSID "WiFi_Net_Name"
#define WIFIPSK  "WiFi_Passwd"
#endif

/*******************************************************************************
 *  System Setup
 */
#define SENSORS_TASK_PRIO   4
#define DISPLAY_TASK_PRIO   5
#define STATS_TASK_PRIO     6

#define STATS_TICKS         pdMS_TO_TICKS(1000)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE



#endif /* MAIN_SETUP_H_ */
