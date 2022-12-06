/* KK Weather Station
 * RTC Task
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
#include <stdio.h>
#include <stdlib.h>
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
#include <protocol_common.h>
#include <k_math.h>

//App headers
#include "tasks.h"


/*******************************************************************************/

/**
 * @brief Task responsible for Real Time Clocks
 * @details
 *        There are 3 available time sources:
 *          - Internal esp32 RTC
 *          - External DS3231 RTC
 *          - NTP time sources
 *        The purpose of this task is to ensure accurate time source at (almost) any moment.
 *        Local RTC is used by the system and App and it is instantly available, but
 *        at every power loss or even hard reset, the internal RTC is reseting too.
 *        External RTC is more reliable, but it also needs to be controlled and set if
 *        needed (i.e. when RTC battery is dead).
 *        NTP is used to synchronize both internal and external RTCs every 1 hour*.
 *        NTP update is triggered also when RTCs inconsistency is found.
 *
 *        Task loop executes every 10seconds
 *
 *        *) SNTP sync period is configured by menuconfig. The defalut sync period is set to 1 hour (3600000ms)
 */
void vRTCTask(void*){
  const char* TAG = "rtc";
  const char* TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";  //TODO: make it configurable by menuconfig
  int retry = 0;
  uint8_t hour, min, sec, mday, mon, wday;
  uint16_t year;
  time_t now;
  struct tm timeinfo;

  // Set timezone for system RTC
  setenv("TZ", TIMEZONE, 1);
  tzset();

  //Sync local RTC with external RTC
  update_int_rtc_from_ext_rtc();

  //Get external RTC time
  rtc.getDateTime(&hour, &min, &sec, &mday, &mon, &year, &wday);

  //Initialize sNTP synchronization events
  initialize_sntp();

  //Wait for NTP Update
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && ++retry < 30) {
    ESP_LOGI(TAG, "Waiting for NTP... (%d/%d)", retry, 30);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  ESP_LOGI(TAG, "RTC Clocks updated with NTP.");
  while (1) {
    //Get time from internal RTC:
    time(&now);
    localtime_r(&now, &timeinfo);

    //Compare both, update the one that is out or both
    if((year < 2022)&&((timeinfo.tm_year+1900) >= 2022)){   //Bad RTC Time, good local time
        ESP_LOGW(TAG, "External RTC out! Updating from internal RTC.");
        update_ext_rtc_from_int_rtc();
    }else if(((timeinfo.tm_year+1900) < 2022)&&(year >= 2022)){ //Bad local, good RTC time
        ESP_LOGW(TAG, "Internal RTC out! Updating from external RTC.");
        update_int_rtc_from_ext_rtc();
    }else if((year < 2022&&((timeinfo.tm_year+1900) < 2022))){  //both out- trigger immediate NTP Update
      ESP_LOGW(TAG, "Both RTCs out! Calling NTP Update!");
      sntp_stop();
      sntp_init();
    }

    //Print out Time
    if (!rtc.getDateTime(&hour, &min, &sec, &mday, &mon, &year, &wday)) {
      ESP_LOGE(TAG, "Get ext RTC time failed");
    }else {
      ESP_LOGI(TAG, "External RTC time: %d-%02d-%04d  %02d:%02d:%02d", mday, mon, year, hour, min, sec);
    }

    ESP_LOGI(TAG, "System RTC time: %d-%02d-%04d  %02d:%02d:%02d", timeinfo.tm_mday, timeinfo.tm_mon+1,
                   timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}


/**
 * Callback called on SNTP synchronization event.
 * Used to synchronize external RTC with NTP time
 *
 * @param tv
 */
void time_sync_notification_cb(struct timeval *tv){
  const char* TAG = "NTP time sync";
  time_t now;
  struct tm timeinfo;
  //update RTC
  time(&now);
  localtime_r(&now, &timeinfo);
  if(rtc.write(&timeinfo)){
    ESP_LOGI(TAG, "Local and external RTC updated.");
  }else{
    ESP_LOGE(TAG, "Local RTC sync done, external RTC reported error!");
  }
}

uint8_t update_ext_rtc_from_int_rtc(void){
  time_t now;
  struct tm timeinfo;
  //update RTC
  time(&now);
  localtime_r(&now, &timeinfo);
  if(rtc.write(&timeinfo))
    return 0;
  else
    return 1;
}

void update_int_rtc_from_ext_rtc(void){
  timeval tv_now = {0,0};
  tv_now.tv_sec = rtc.getEpoch();
  settimeofday(&tv_now, NULL);
}

/**
 * Initialize SNTP RTC updates
 *
 */
void initialize_sntp(void){
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
  sntp_init();
}
