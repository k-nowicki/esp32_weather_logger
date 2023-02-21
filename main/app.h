/*
 * app.h
 *
 *	This file contains Application level:
 *	 - task declarations
 *	 - global variable declarations
 *	 - function declarations
 *	 - struct, enum etc definitions
 *
 *	This is not right place for configuration definitions- check setup.h
 *
 *  Created on: 30 lis 2022
 *      Author: Karol
 */

#ifndef MAIN_APP_H_
#define MAIN_APP_H_

//Peripheral drivers
#include <sdmmc_cmd.h>
#include "driver/gpio.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Fonts/FreeSans9pt7b.h>
#include <dht11.h>
#include <BH1750.h>
#include "esp_camera.h"


//#define DS3231    //Uncomment if using DS3231 instead of DS1307

#ifdef DS3231
#include <ErriezDs3231.h>
#else
#include <ErriezDS1307.h>
#endif
//#include <owb.h>
//#include <owb_rmt.h>
//#include <ds18b20.h>

//Struct to hold instance of all env measurements
struct measurement{
  float lux = 0.0;	//light exposure (BH1750)
  float iTemp = 0.0;	//internal temperature (BMP280)
  float eTemp = 0.0;	//external temperature (DHT11)
  float dTemp = 0.0;	//dedicated temperature (DS18B20)
  float humi = 0.0;	//humidity (DHT11)
  float pres = 0.0;	//atm. pressure (BPM280)
  float alti = 0.0;	//altitude (BMP280, calculated)
  int dht_status = 0; //dht last_measurement status
  time_t time = 0;  //time of measurement
};

//Business logic global variables
extern measurement g_curr_measures;	//Current measurements
// Create RTC object


#ifdef DS3231           // RTC object depending on used physical device
extern ErriezDS3231 g_rtc;
#else
extern ErriezDS1307 g_rtc;
#endif

//Sensor global objects
extern BH1750 g_lightMeter;
extern Adafruit_BMP280 g_pressureMeter; // I2C
extern Adafruit_SSD1306 g_display;

//SD Card global object
extern sdmmc_card_t * g_card;

//semaphores
extern SemaphoreHandle_t g_current_measuers_mutex;
extern SemaphoreHandle_t g_uart_mutex;
extern SemaphoreHandle_t g_card_mutex;

//Tasks handlers
extern TaskHandle_t g_vSDCSVLGTaskHandle;
extern TaskHandle_t g_vSDAVGLGTaskHandle;
extern TaskHandle_t g_vSDJSLGTaskHandle;

//setup helper functions
//void initialize_ds18b20(void);

//task helper functions
void init_app_screen(void);
measurement get_latest_measurements(void);
void store_measurements(measurement);
void search_i2c(void);
void time_sync_notification_cb(struct timeval *);
void initialize_sntp(void);
uint8_t update_ext_rtc_from_int_rtc(void);
void update_int_rtc_from_ext_rtc(void);
void unmount_sd(void);
uint8_t init_sd(void);
uint8_t reinit_sd(void);
void ensure_card_works(void);

/*******************************************************************************
 * App Definitions
 * WARNING
 * These are not settings!
 * Do not change them unless you know what you are doing!
 */
#define LOGGER_RTC_WAIT_FOR_NOTIFY_MS (LOGGING_INTERVAL_MS-20)
#define CSV_LOGGER_NOTIFY_ARRAY_INDEX 0
#define AVG_LOGGER_NOTIFY_ARRAY_INDEX 0
#define JS_LOGGER_NOTIFY_ARRAY_INDEX 0
#define LOGGER_NOTIFY_VALUE 1

#endif /* MAIN_APP_H_ */


