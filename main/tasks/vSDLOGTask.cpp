/* KK Weather Station
 * SDLOG_TASK_PRIO Task
 *
 * Platform: ESP32 (Tested on ESP32-CAM Development Board)
 * See project documentation for more detailed description.
 *
 *  Copyright (c) <2022> <Karol Nowicki>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
*/

//System
#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#undef _POSIX_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include <time.h>
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

//App headers
#include "tasks.h"

bool is_date_changed();
void replace_or_continue_current_log_file(void);
void rename_log_file(tm *);
uint8_t begin_log_file(const char *);
uint8_t end_log_file(const char *);
uint8_t reinitialize_sdcard(void);

static const char *TAG = "SDLOG";
#define CURR_LOG_FNAME static_cast<const char *>(SD_MOUNT_POINT"/CURRENT.LOG")



/*******************************************************************************
 * @brief Task responsible for logging measurements to file on SD card.
 *
 * Handles log files
 * Once every second:
 *    - append new measurements to current log file (open file, append, close file)
 * Once every 24 hours (at 00:00:00)
 *    - end and rename current log file to yesterdays date
 *    - begin new log file
 *
 * @param arg
 *
 */
void vSDLOGTask(void*){
  TickType_t xLastWakeTime;
  time_t now, file_time_t;
  struct tm file_tm;
  measurement measurements;
  FILE *f;

  //Wait until RTC sends notify that is synchronized with external RTC
  ulTaskNotifyTakeIndexed( LOGGER_NOTIFY_ARRAY_INDEX, pdTRUE, portMAX_DELAY );
  //for desynchronize RTCTask logs with this task logs to not interfere each other
  vTaskDelay(pdMS_TO_TICKS(100));

  xLastWakeTime = xTaskGetTickCount();   //https://www.freertos.org/xtaskdelayuntiltask-control.html
  //Log file must be todays log file
  //if older log file exists and hasn't been renamed should be ended and renamed now
  replace_or_continue_current_log_file();
  ESP_LOGI(TAG, "Start logging measurements to SD card.");
  while (1) {
   /**
    * If date has changed than current log file is ended, renamed to yesterdays
    * date and new current log file is opened. Needs to be done before new log entry.
    */
    if( is_date_changed() ){
      ESP_LOGI(TAG, "New day, new log file. Renaming current log to yesterdays date.");
      end_log_file(CURR_LOG_FNAME);
      //get time of yesterday:
      file_time_t = time(NULL) - (24 * 60 * 60);
      localtime_r(&file_time_t, &file_tm);
      //Rename original file
      rename_log_file(&file_tm);
      begin_log_file(CURR_LOG_FNAME);
    }
    /**
     * Store new data entry to log file
     * Done once per period specified by LOGGING_INTERVAL
     */
    f = fopen(CURR_LOG_FNAME, "a+");
    if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open log file!");
      if(sdmmc_get_status(card) != ESP_OK)  //There is probably some kind of problem with SD card
        reinitialize_sdcard();    //If thats true, try to reinitialize
    }else{
      //Prepare and store log entry
      now = time(NULL);
      measurements = get_latest_measurements();
      fprintf(f, "{\"time\":\"%lld\",\"int_t\":%3.2F, \"ext_t\":%3.2F, \"humi\":%d, \"sun\":%5.2F, \"press\":%4.2f},\n",
                    static_cast<long long>(now),
                    measurements.iTemp,
                    measurements.eTemp,
                    static_cast<int>(measurements.humi),
                    measurements.lux,
                    measurements.pres);
      fclose(f);
    }

    // Wait for the next cycle exactly 1 second- it is critical to .
    xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(LOGGING_INTERVAL_MS) );
  }
}

uint8_t reinitialize_sdcard(void){
  uint8_t err;
  ESP_LOGW(TAG, "Trying to reinitialize SD Card...");
  xSemaphoreTake(card_mutex, portMAX_DELAY);
  err = reinit_sd();
  xSemaphoreGive(card_mutex);
  if(err != ESP_OK){
    ESP_LOGE(TAG, "Cannot initialize SD Card!");
  }else{
    ESP_LOGI(TAG, "SD Card reinitialized successfully.");
  }
  return err;
}

/**
 * Check if date is different than in previous call
 * @return true if date has changed, false otherwise.
 */
bool is_date_changed(){
  time_t now;
  struct tm timeinfo;
  static int yday = -1;

  time(&now);  localtime_r(&now, &timeinfo);
  //at first call initialize yday as today
  if(yday == -1)
    yday = timeinfo.tm_yday;
  //check if date has changed
  if(yday != timeinfo.tm_yday){
    yday = timeinfo.tm_yday;
    return true;
  }
  else
    return false;
}

/**
 * Check if CURR_LOG_FNAME exists, begin file if not
 * If exists:
 *  - check last modification date,
 *  - if it is not today:
 *    - end that file with date from last modification
 *    - begin new log file
 *  - otherwise continue with that file
 */
void replace_or_continue_current_log_file(){
  time_t now =0, file_time_t =0;
  struct tm timeinfo, file_tm;
  struct stat fileStat;

  if(stat(CURR_LOG_FNAME, &fileStat) == ESP_OK){   //if file exists
    file_time_t = fileStat.st_mtime;      //get last modification time of the file
    now= time(NULL);                      //get now time
    localtime_r(&now, &timeinfo);         //modify both to localtime
    localtime_r(&file_time_t, &file_tm);
    ESP_LOGI(TAG, "CURRENT.LOG file last modification date: %4d-%2d-%2d", (file_tm.tm_year+1900), file_tm.tm_mon+1, file_tm.tm_mday);
    //if file mod yday older than now yday or file mod year older than now year
    if((file_tm.tm_year < timeinfo.tm_year) || (file_tm.tm_yday < timeinfo.tm_yday)){
      end_log_file(CURR_LOG_FNAME);   //end that file
      rename_log_file(&file_tm);       //rename it with date of last modification.
      begin_log_file(CURR_LOG_FNAME); //and begin new log file
    }
    return;
  }else{  //if file don't exist
    begin_log_file(CURR_LOG_FNAME);
    return;
  }
}

/**
 * Change name of current log file to YYMMDD.LOG ex: 091222.LOG
 * @param time Pointer to tm struct with time that will be stored in filename
 *
 */
void rename_log_file(tm * time){
  char arch_log_filename[30];
  uint8_t status = 0;
  sprintf(arch_log_filename, "%s/%02d%02d%02d.LOG", SD_MOUNT_POINT, time->tm_mday, time->tm_mon+1, static_cast<uint8_t>(time->tm_year-100)  );
  ESP_LOGI(TAG, "Renaming file %s to %s", CURR_LOG_FNAME, arch_log_filename);
  status = rename(CURR_LOG_FNAME, arch_log_filename);
  if (status != 0) {
    /*
     * TODO: Resolve problem with renaming
     * If reason is that file already exist, it should open that file, append data from current log, than close and rename
     * If reason is different than it should be reported to end user
     */
    ESP_LOGE(TAG, "Log file rename failed with error: %d", status);

  }
}

/**
 * Creates or recreates new json log file starting with '[' sign
 * @param filename  Name of file
 * @return ESP_OK when successful, ESP_FAIL otherwise
 *
 */
uint8_t begin_log_file(const char * filename){
  FILE *f = fopen(filename, "w");
  if(f != NULL){
    fprintf(f, "[");
    fclose(f);
    return ESP_OK;
  }else{
    return ESP_FAIL;
  }
}

/**
 * Ends existing json log file with ']' sign
 * @param filename  Name of file to be ended
 * @return ESP_OK when successful, ESP_FAIL otherwise
 *
 */
uint8_t end_log_file(const char * filename){
  FILE *f = fopen(filename, "r+b");
  if(f != NULL){
    fseek(f , -2 , SEEK_END );   //set position at the last but one byte of file (this will be a comma sign before \n)
    fputs( "]" , f);             //rewrite it to the end of json format
    fclose(f);
    return ESP_OK;
  }else{
    return ESP_FAIL;
  }
}
