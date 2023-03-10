/* KK Weather Station
 * Camera Task
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
#include "camera_helper.h"
#include "ff.h"

//App headers
#include "tasks.h"

#define PICTURE_INTERVAL_S (PICTURE_INTERVAL_M * 60)

static bool is_time_to_get_picture(void);
char *get_next_file_full_path(char *path);
static void ensure_todays_path_exist(char *path);
static const char *TAG = "CAMERA";

/*******************************************************************************/


/**
 * @brief Task responsible for reading all fast sensors
 *
 * @param arg
 */
void vCameraTask(void*){
  TickType_t xLastWakeTime;
  size_t pictureSize;
  char *filename = NULL;
  char pic_filename[FILEPATH_LEN_MAX];

  //Wait until RTC sends notify that is synchronized with external RTC
  ulTaskNotifyTakeIndexed( CAMERA_TASK_NOTIFY_ARRAY_INDEX, pdTRUE, portMAX_DELAY );
  //for desynchronize RTCTask logs with this task logs to not interfere each other
  vTaskDelay(pdMS_TO_TICKS(250));

  xLastWakeTime = xTaskGetTickCount();   //https://www.freertos.org/xtaskdelayuntiltask-control.html
  while (1) {
    /**
     * The task sequence is:
     *   - set filename to current.jpg
     *   - if it is time to permanently save picture - determine the filename and change pic_filename
     *   - take picture and save as pic_filename
     *   - the loop ends
     */

    //set filename to current.jpg
    sprintf(pic_filename, "%s/%s", CAM_FILE_PATH, "/0/current.jpg" );

    //if it is time to permanently save picture - determine the filename
    if(is_time_to_get_picture() == true){
      //Do not save pictures if it is completely dark
      measurement measurements;
      measurements = get_latest_measurements();
      if(measurements.lux < 0.5){
        ESP_LOGI(TAG, "Too dark for saving picture!");
      }else{
        ESP_LOGI(TAG, "Time to take picture!!");
        filename = get_next_file_full_path((char *)CAM_FILE_PATH);  //warning- this allocates memory that needs to be freed
        if(filename != NULL){
          strcpy(pic_filename, filename );
          free(filename);
          ESP_LOGI(TAG, "Successfully created Filename: %s", pic_filename);
        }else{
          ESP_LOGE(TAG, "Can not get new filename!");
        }
      }
    }


    ESP_LOGI(TAG, "Taking picture!");
    if(camera_capture(pic_filename, &pictureSize) != ESP_OK){
      ESP_LOGE(TAG, "Can not take picture!");
    }else{
      ESP_LOGI(TAG, "Picture stored on SD Card!");
    }
    // Wait for the next cycle exactly 1 second.
    xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(5000) );
  }
}

/**
 * @param path Path to root dcim directory
 * @return next filename with date specific path i.e. next file in <path>/2023/02/28/
 */
char *get_next_file_full_path(char *path) {
  char today_path[FILENAME_LEN];
  char full_dir_path[FILEPATH_LEN_MAX];

  ensure_todays_path_exist(path);

  //prepare full path to target dir
  get_today_path(today_path);
  sprintf(full_dir_path, "%s%s", path, today_path);

  char *newest_file = get_newest_file(full_dir_path);
  if (newest_file == NULL) { return NULL; }

  char *next_file = (char *)malloc(60);
  if (next_file == NULL) {
      ESP_LOGE(TAG, "Can not allocate memory!");
      free(newest_file);
      return NULL;
  }

  uint32_t nnn;
  sscanf(newest_file, "%03d.jpg", &nnn);
  nnn++;
  sprintf(next_file, "%s/%03d.jpg", full_dir_path, nnn);
  free(newest_file);
  return next_file;
}

/**
 * @return true if PICTURE_INTERVAL_S has passed since last true returned
 *         false otherwise
 */
static bool is_time_to_get_picture(void) {
    static time_t last_time = 0;

    time_t current_time = time(NULL);
    time_t interval_time = PICTURE_INTERVAL_S;

    if (current_time - last_time >= interval_time) {
        last_time = current_time;
        return true;
    } else {
        return false;
    }
}

/**
 * @param path Path to root dcim directory for pictures
 * Function create directories needed to store pictures for current date
 * according to the pattern: path/YYYY/MM/DD/
 * where YYYY is always 4 digit year
 *  MM is always 2 digit month
 *  DD is always 2 digit day of the month
 *
 */
static void ensure_todays_path_exist(char *path){
  esp_err_t res;
  time_t now = 0;
  struct tm timeinfo;
  char pic_filename[FILEPATH_LEN_MAX];
  char * path_end_ptr = NULL;
  char * path_start_ptr = NULL;

  //fetch localtime
  now = time(NULL);
  localtime_r(&now, &timeinfo);
  //set path_start_ptr to the
  path_start_ptr = pic_filename;

  //create YYYY directory
  sprintf(pic_filename, "%s/%04d", path, (timeinfo.tm_year+1900) );
  res = mkdir(pic_filename, 0777);
  if(res != FR_OK && res != FR_EXIST){
    ESP_LOGW(TAG, "Can not create directory '%s'! Error: %d", pic_filename, res);
  }else{
    ESP_LOGI(TAG, "Directory '%s' created successfully!", pic_filename);
  }
  path_end_ptr = path_start_ptr + strlen(pic_filename); //set end pointer to end of path

  //create MM directory
  sprintf(path_end_ptr, "/%02d", timeinfo.tm_mon+1 );
  res = mkdir(pic_filename, 0777);
  if(res != FR_OK && res != FR_EXIST){
    ESP_LOGW(TAG, "Can not create directory '%s'! Error: %d", pic_filename, res);
  }else{
    ESP_LOGI(TAG, "Directory '%s' created successfully!", pic_filename);
  }
  path_end_ptr = path_start_ptr + strlen(pic_filename); //set pointer to end of path

  //create DD directory
  sprintf(path_end_ptr, "/%02d", timeinfo.tm_mday );
  res = mkdir(pic_filename, 0777);
  if(res != FR_OK && res != FR_EXIST){
    ESP_LOGW(TAG, "Can not create directory '%s'! Error: %d", pic_filename, res);
  }else{
    ESP_LOGI(TAG, "Directory '%s' created successfully!", pic_filename);
  }
}
