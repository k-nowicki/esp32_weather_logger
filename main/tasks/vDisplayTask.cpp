/* KK Weather Station
 * Display Task
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
 * @brief Task responsible for OLED display UI
 *
 * @param arg
 */
void vDisplayTask(void *arg){
  measurement tmp_measurements;
  time_t now;
  struct tm timeinfo;
  tcpip_adapter_ip_info_t ip;

  g_display.display();
  vTaskDelay(pdMS_TO_TICKS(200));
  g_display.clearDisplay();
  g_display.setTextSize(1);      // Normal 1:1 pixel scale
  g_display.setTextColor(SSD1306_WHITE); // Draw white text
  g_display.setCursor(0, 10);     // Start at top-left corner
  g_display.cp437(true);         // Use full 256 char 'Code Page 437' font
  g_display.setFont(&FreeSans9pt7b);

  g_display.println("Weather Station V 1.0");
  g_display.setFont();
  g_display.println("by KNowicki @ 2022");
  g_display.display();
  vTaskDelay(pdMS_TO_TICKS(1000));
  while(1){
    vTaskDelay(pdMS_TO_TICKS(100));
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip); //get IP address.
    tmp_measurements = get_latest_measurements();         //safely read current values
    time(&now);                                           //get time
    localtime_r(&now, &timeinfo);
    g_display.clearDisplay();
    g_display.setCursor(0, 0);     // Start at top-left corner
    g_display.printf("%0d-%02d-%04d  %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon+1,
                   timeinfo.tm_year+1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    g_display.printf("Intern T: %3.2F %cC\n", tmp_measurements.iTemp,'\xF8');
    g_display.printf("Extern T: %3.2F %cC\n", tmp_measurements.eTemp,'\xF8');
    g_display.printf("Humidity: %d%%\n", (int)tmp_measurements.humi);
    g_display.printf("Sun expo: %5.2F Lux\n", tmp_measurements.lux);
    g_display.printf("Pressure: %4.2f hPa\n", tmp_measurements.pres);
    g_display.printf("Altitude: %5.2Fm\n", tmp_measurements.alti);
    g_display.printf("IP: " IPSTR, IP2STR(&ip.ip));
    g_display.display();
  }
}


/**
 * @brief Sets up display for the app and displays splash screen
 *
 */
void init_app_screen(void){
  g_display.display();
  vTaskDelay(pdMS_TO_TICKS(200));
  g_display.clearDisplay();
  g_display.setTextSize(1);      // Normal 1:1 pixel scale
  g_display.setTextColor(SSD1306_WHITE); // Draw white text
  g_display.setCursor(0, 10);     // Start at top-left corner
  g_display.cp437(true);         // Use full 256 char 'Code Page 437' font
  g_display.setFont(&FreeSans9pt7b);

  g_display.println("Weather Station V 1.0");
  g_display.setFont();
  g_display.println("by KNowicki @ 2022");
  g_display.display();
}
