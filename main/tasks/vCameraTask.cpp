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
  time_t now =0;
  struct tm timeinfo;
  char pic_filename[60];
  vTaskDelay(pdMS_TO_TICKS(10000));  //wait 10 secs (rtc update, logs initialization etc)

  xLastWakeTime = xTaskGetTickCount();   //https://www.freertos.org/xtaskdelayuntiltask-control.html
  while (1) {
    /**
     * The task sequence is:
     *   - make filename from current datetime
     *   - take picture and store it under the filename on SD Card
     *   - find out is there enough room left on the SD
     *      - if not - delete 10 oldest pictures
     *   - the loop ends
     */

    //get current time
    now= time(NULL);                      //get now time
    localtime_r(&now, &timeinfo);         //modify to localtime
//    sprintf(pic_filename, "%s/%02d%02d%02d.jpg", CAM_FILE_PATH, timeinfo->tm_mday, timeinfo->tm_mon+1, static_cast<uint8_t>(timeinfo->tm_year-100)  );
    sprintf(pic_filename, "%s/%s", CAM_FILE_PATH, "current.jpg" );
    ESP_LOGI(TAG, "Taking picture!");
    if(camera_capture(pic_filename, &pictureSize) != ESP_OK){
        ESP_LOGE(TAG, "Can not take picture!");
    }
    ESP_LOGI(TAG, "Picture stored on SD Card!");
    // Wait for the next cycle exactly 1 second- it is critical to .
    xTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(PIC_INTERVAL_S * 1000) );
  }
}
