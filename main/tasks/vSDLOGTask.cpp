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

void replace_or_continue_current_log_file(void);
void rename_log_file(tm *);
void begin_log_file(const char *);
void end_log_file(const char *);

/*******************************************************************************/

static const char *TAG = "SDLOG";
#define CURR_LOG_FNAME static_cast<const char *>(SD_MOUNT_POINT"/CURRENT.LOG")

/**
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

  /**
   * Dummy wait until RTC is synchronized with external RTC
   * TODO: Should wait for signal from RTC task that time is set
   */
  vTaskDelay(pdMS_TO_TICKS(5000));

  xLastWakeTime = xTaskGetTickCount();   //https://www.freertos.org/xtaskdelayuntiltask-control.html

  //Log file must be todays log file
  //if older log file exists and hasn't been renamed should be ended and renamed now
  replace_or_continue_current_log_file();
  ESP_LOGI(TAG, "Start logging measurements to SD card.");
  while (1) {
    /**
     *  Store new data entry to log file
     *  Done once per period specified by LOGGING_INTERVAL
     */
    if(true){
      f = fopen(CURR_LOG_FNAME, "a+");
      if (f == NULL) {
        /* TODO: Ensure the file is opened. And if it really can't open- register that fact to be reported to end user later */
        ESP_LOGE(TAG, "Failed to open log file!");
      }else{           //Prepare and store log message
        now = time(NULL);
        measurements = get_latest_measurements(); //safely read current values
        fprintf(f, "{\"time\":\"%lld\",\"int_t\":%3.2F, \"ext_t\":%3.2F, \"humi\":%d, \"sun\":%5.2F, \"press\":%4.2f},\n",
                      static_cast<long long>(now),
                      measurements.iTemp,
                      measurements.eTemp,
                      static_cast<int>(measurements.humi),
                      measurements.lux,
                      measurements.pres);
        fclose(f);
      }
    }

    /*
     * TODO: RTC task must send a signal by queue that date has changed
     * than current log file is ended, renamed to yesterdays date
     * and new current log file is opened
     */
    if(false){  //Check queue
      end_log_file(CURR_LOG_FNAME);
      //get time of yesterday:
      file_time_t = time(NULL) - (24 * 60 * 60);
      localtime_r(&file_time_t, &file_tm);
      //Rename original file
      rename_log_file(&file_tm);
      begin_log_file(CURR_LOG_FNAME);
    }
    // Wait for the next cycle.
    xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(LOGGING_INTERVAL_MS) );
  }
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
  time_t now, file_time_t;
  struct tm timeinfo, file_tm;
  struct stat fileStat;

  if(stat(CURR_LOG_FNAME, &fileStat) > 0){   //if file exists
    file_time_t = fileStat.st_atime;      //get last modification time of the file
    time(&now);                           //get now time
    localtime_r(&now, &timeinfo);         //modify both to localtime
    localtime_r(&file_time_t, &file_tm);
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
 * Change name of current log file to YYMMDD.LOG ex: 221209.LOG
 * @param time Pointer to tm struct with time that will be stored in filename
 *
 */
void rename_log_file(tm * time){
  char arch_log_filename[25];
  sprintf(arch_log_filename, "%02d%02d%02d.LOG", static_cast<uint8_t>(time->tm_year-100), time->tm_mon+1, time->tm_mday );
  ESP_LOGI(TAG, "Renaming file %s to %s", CURR_LOG_FNAME, arch_log_filename);
  if (rename(CURR_LOG_FNAME, arch_log_filename) != 0) {
    /*
     * TODO: Resolve problem with renaming
     * If reason is that file already exist, it should open that file, append data from current log, and close
     */
    ESP_LOGE(TAG, "Log file rename failed");
  }
}

/**
 * Creates or recreates new json log file starting with '[' sign
 * @param filename  Name of file
 *
 */
void begin_log_file(const char * filename){
  FILE *f = fopen(filename, "w");
  fprintf(f, "[");
  fclose(f);
}

/**
 * Ends existing json log file with ']' sign
 * @param filename  Name of file to be ended
 *
 */
void end_log_file(const char * filename){
  FILE *f = fopen(filename, "a+");
  fseek (f , -2 , SEEK_CUR );   //set position at the last but one byte of file (this will be a comma sign before \n)
  fputs ( "]" , f);             //rewrite it to the end of json format
  fclose(f);
}
