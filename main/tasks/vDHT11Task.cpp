/* KK Weather Station
 * DHT11 Task
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

//App headers
#include "tasks.h"



/*******************************************************************************/


/**
 * @brief Task responsible for reading DHT11 extremely slow sensor and reading
 *        also slow wind speed from anemometer
 *
 * @param arg
 *
 * DHT11 needs usually around 2 seconds to take measurement. As tests showed,
 * it also should not be read too often (most reads ends up with error then).
 * All other sensors are extremely fast in compare and can be read on the fly.
 * This is why reading of DHT11 is moved to separate task with different approach.
 *
 * The anemometer also needs around 1 second to make measurement, so it is
 * placed here. This task probably should change name to vSlowSensTask or
 * something like that.
 *
 */
void vDHT11Task(void*){
  measurement tmp_measurements;
  dht11_reading dht_read;
  while (1) {
    dht_read = DHT11_read();
    //update status of last read
    tmp_measurements.dht_status = dht_read.status;
    //store DHT11 values only if status OK and temperature or humidity != 0
    if(dht_read.status==DHT11_OK && (dht_read.temperature != 0 || dht_read.humidity != 0)){
      //update measurements only when reads OK
      tmp_measurements.eTemp = dht_read.temperature;
      tmp_measurements.humi = dht_read.humidity;
    }
    //read also wind speed
    tmp_measurements.wind = g_windMeter.readWind();
    //store measurements in curr_measures
    xSemaphoreTake(g_current_measuers_mutex, portMAX_DELAY);
    g_curr_measures.eTemp = tmp_measurements.eTemp;
    g_curr_measures.humi = tmp_measurements.humi;
    g_curr_measures.dht_status = tmp_measurements.dht_status;
    g_curr_measures.wind = tmp_measurements.wind;
    xSemaphoreGive(g_current_measuers_mutex);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
