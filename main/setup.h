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
#define GPIO_DHT11    GPIO_NUM_33
//DS18B20 sensor line
#define GPIO_DS18B20  GPIO_NUM_4
//I2C Bus lines (OLED, RTC, BH1750, BMP280)
#define I2C_SDA GPIO_NUM_13
#define I2C_SCL GPIO_NUM_4
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
 *  SD Card Setup
 */
#define SD_MOUNT_POINT "/sd"
#define SD_MAX_FILES 5
#define SD_ALLOCATION_UNIT_SIZE 16 * 512   //tradeoff between heap demand and speed

/*******************************************************************************
 *  Camera Setup
 */

//#define CONFIG_FRAMESIZE_UXGA 1  //configured by menuconfig in KK Camera Configuration menu

/*******************************************************************************
 *  System Setup
 */

#define CAM_TASK_PRIO       20
#define SDAVGLG_TASK_PRIO   18
#define SDCSVLG_TASK_PRIO   18
#define SDJSLG_TASK_PRIO    18
#define DISPLAY_TASK_PRIO   15
#define HTTP_TASK_PRIO      DISPLAY_TASK_PRIO
#define SENSORS_TASK_PRIO   13
#define DHT11_TASK_PRIO     12
#define RTC_TASK_PRIO       10
#define STATS_TASK_PRIO     9

#define STATS_TICKS         pdMS_TO_TICKS(10*1000)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

/*******************************************************************************
 *  App Setup
 */
//Loggging settings
#define LOGGING_INTERVAL_MS 1000  //Interval between measurements logged to SD card in milliseconds
#define LOG_FILE_DIR "/www/logs"  //directory holding logs without mount point (ex: "/www/logs" puts logs in SD_MOUNT_POINT/www/logs/logname.log)
#define AVG_LOG_FILE_DIR "/www/logs/avg"  //directory holding avg logs without mount point (ex: "/www/avg/logs")
#define AVG_MESUREMENTS_NO 60    //how many measurements takes to calculate average

//Picture settings
#define PIC_FILE_DIR "/www/dcim"  //directory holding pictures without mount point (ex: "/www/logs" puts logs in SD_MOUNT_POINT/www/dcim/picture.jpg)
#define CAM_FILE_PATH static_cast<const char *>(SD_MOUNT_POINT PIC_FILE_DIR)
#define PICTURE_INTERVAL_M 5          //number of minutes between pictures
#define FILENAME_LEN 25           //Length of camera picture filename NNN_DDMMYYY.jpg
#define FILEPATH_LEN_MAX 40       //Maximum length of full path to picture (for buffer allocation- keep it short, but not shorter than necessary)
#define PIC_LIST_BUFFER_SIZE  (1500/PICTURE_INTERVAL_M)*(2*FILENAME_LEN+28) +55 //(MINUTES_IN_DAY/PICTURE_INTERVAL_M)*(2*FILENAME_LEN + IL_TEXT_LEN) + HTML_WRAP_TEXT_LEN
#endif /* MAIN_SETUP_H_ */
