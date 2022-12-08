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

//GPIO_16 - PSRAM
//GPIO_02 - FLASH
//GPIO_12 - bootstrap
//GPIO_04 -

//DHT11 sensor line
#define GPIO_DHT11    GPIO_NUM_33 //33
//DS18B20 sensor line
#define GPIO_DS18B20  GPIO_NUM_4
//I2C Bus lines (OLED, RTC, BH1750, BMP280)
#define I2C_SDA GPIO_NUM_13
#define I2C_SCL GPIO_NUM_0
//Those are only informative statements, SD pins can not be changed:
#define SD_CMD_PIN  GPIO_NUM_14
#define SD_CLK_PIN  GPIO_NUM_15
#define SD_D0_PIN   GPIO_NUM_2
//#define SD_D1_PIN   NULL
//#define SD_D2_PIN   NULL
//#define SD_D3_PIN   NULL

//I2C Device addrs
#define OLED_ADDR   0x3c
#define BH1750_ADDR 0x23
//#define DS3231_ADDR 0x57  // this address is defined inside DS3231 library
#define BMP280_ADDR 0x76
//OLED SCREEN
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//DS18B20 sensor
#define MAX_DEVICES          (8)
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD        (1000)   // milliseconds


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

#define SDMMC_TASK_PRIO     17
#define STATS_TASK_PRIO     11
#define DISPLAY_TASK_PRIO   13
#define SENSORS_TASK_PRIO   14
#define DHT11_TASK_PRIO     15
#define RTC_TASK_PRIO       16

#define STATS_TICKS         pdMS_TO_TICKS(1000)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE



#endif /* MAIN_SETUP_H_ */
