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


//App headers
#include "tasks.h"

#define PICTURE_INTERVAL_S (PICTURE_INTERVAL_M * 60)

char *get_next_file(char *path);
static bool is_time_to_get_picture(void);
static const char *TAG = "CAMERA";
#define CAM_FILE_PATH static_cast<const char *>(SD_MOUNT_POINT PIC_FILE_DIR)

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
  char pic_filename[60];
  vTaskDelay(pdMS_TO_TICKS(10000));  //wait 10 secs (rtc update, logs initialization etc)

  xLastWakeTime = xTaskGetTickCount();   //https://www.freertos.org/xtaskdelayuntiltask-control.html
  while (1) {
    /**
     * The task sequence is:
     *   - set filename to current.jpg
     *   - if it is time to permanently save picture - determine the filename and change pic_filename
     *   - take picture and save as pic_filename
     *   - the loop ends
     */

    //first set filename to current.jpg
    sprintf(pic_filename, "%s/%s", CAM_FILE_PATH, "/0/current.jpg" );

    //if it is time to permanently save picture - determine the filename
    if(is_time_to_get_picture() == true){
      ESP_LOGI(TAG, "Time to take picture!!");
      filename = get_next_file((char *)CAM_FILE_PATH);
      if(filename != NULL){
        sprintf(pic_filename, "%s/%s", CAM_FILE_PATH, filename );
        free(filename);
        ESP_LOGI(TAG, "Successfully created Filename: %s", pic_filename);
      }else{
        ESP_LOGE(TAG, "Can not get new filename!");
      }
    }

    ESP_LOGI(TAG, "Taking picture!");
    if(camera_capture(pic_filename, &pictureSize) != ESP_OK){
        ESP_LOGE(TAG, "Can not take picture!");
    }
    ESP_LOGI(TAG, "Picture stored on SD Card!");
    // Wait for the next cycle exactly 1 second.
    xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(5000) );
  }
}



/**
 * @param path Path to directory
 * @return next filename
 */
char *get_next_file(char *path) {
  char *newest_file = get_newest_file(path);
  if (newest_file == NULL) {
      return NULL;
  }

  char *next_file = (char *)malloc(FILENAME_LEN + 1);
  if (next_file == NULL) {
      ESP_LOGE(TAG, "Can not allocate memory!");
      free(newest_file);
      return NULL;
  }

  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);

  uint32_t nnn, dd, mm, yyyy;
  sscanf(newest_file, "%03d_%02d%02d%04d.jpg", &nnn, &dd, &mm, &yyyy);

  if (dd == tm_now->tm_mday && mm == tm_now->tm_mon + 1 && yyyy == tm_now->tm_year + 1900) {
      nnn++;
  } else {
      nnn = 1;
  }

  snprintf(next_file, FILENAME_LEN + 1, "%03d_%02d%02d%04d.jpg", nnn, tm_now->tm_mday, tm_now->tm_mon + 1, (tm_now->tm_year + 1900)%10000u);

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
