/* KK Weather Station
 * Sensors Task
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
 * @brief Task responsible for reading all fast sensors
 *
 * @param arg
 */
void vSensorsTask(void*){
  measurement tmp_measurements;
  while (1) {
    float itemp, etemp, humi, lux, pres, alti;

    // Read all fast sensors
    lux = g_lightMeter.readLightLevel();
    itemp = g_pressureMeter.readTemperature();
    pres = g_pressureMeter.readPressure();
    alti = g_pressureMeter.readAltitude(1013.25);
    #ifdef EXTERNAL_SENSOR_HTU21
    etemp = g_htu21.readTemperature();
    humi = g_htu21.readHumidity();
    #endif

    tmp_measurements.eTemp = isnan(etemp) ? 0.0 : etemp;
    tmp_measurements.humi =  isnan(humi) ? 0.0 : humi;
    tmp_measurements.dht_status = (isnan(humi) || isnan(etemp)) ? -2 : 0;
    tmp_measurements.lux = (lux == -1 || lux > 65000) ? 0.0 : lux;
    tmp_measurements.iTemp = isnan(itemp) ? 0.0 : itemp;
    tmp_measurements.pres = isnan(pres) ? 0.0 : pres/100;
    tmp_measurements.alti = isnan(alti) ? 0.0 : alti;
    store_measurements(tmp_measurements);   //Store in global curr_measures

    vTaskDelay(pdMS_TO_TICKS(150)); //BH1750 has 120ms avg measurement time, bmp280 about 6ms
  }
}
