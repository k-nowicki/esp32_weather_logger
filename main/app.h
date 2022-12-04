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

#include <ErriezDs3231.h>

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
};

//Business logic global variables
measurement curr_measures;	//Current measurements
// Create RTC object
ErriezDS3231 rtc;


//Tasks declarations
static void vSensorsTask(void*);
static void vDHT11Task(void*);
static void vRTCTask(void*);
static void vDisplayTask(void*);
static void stats_task(void*);

//setup helper functions
//void initialize_ds18b20(void);

//task helper functions
void init_app_screen(void);
measurement get_latest_measurements(void);
void store_measurements(measurement);
static esp_err_t print_real_time_stats(TickType_t);
void search_i2c(void);
void time_sync_notification_cb(struct timeval *);
void initialize_sntp(void);
uint8_t update_ext_rtc_from_int_rtc(void);
void update_int_rtc_from_ext_rtc(void);

//semaphores
SemaphoreHandle_t current_measuers_mutex;
SemaphoreHandle_t uart_mutex;



#endif /* MAIN_APP_H_ */
