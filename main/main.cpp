/* KK Weather Station
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
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <Wire.h>
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
#include <protocol_common.h>
#include <k_math.h>

//App
#include "setup.h"
#include "tasks/tasks.h"
#include "app.h"
//#ifdef B1000000   //arduino libs loaded in app.h defines those marcos in different way than esp-idf does
//#undef B1000000
//#undef B110
//#endif
#include "kk_http_app/src/kk_http_app.h"
#include "kk_http_app/src/kk_http_server_setup.h"

extern "C" {
  void app_main(void);
}



/*******************************************************************************
 * Variable definitions
 */

//Business logic global variables
measurement curr_measures; // Current measurements
ErriezDS3231 rtc;          // RTC object

//Sensor global objects
BH1750 lightMeter(BH1750_ADDR);
Adafruit_BMP280 pressureMeter(&Wire); // I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//SD Card global object
sdmmc_card_t * card;

//semaphores
SemaphoreHandle_t current_measuers_mutex;
SemaphoreHandle_t uart_mutex;
SemaphoreHandle_t card_mutex;

//task handlers
TaskHandle_t vSDCSVLGTaskHandle = NULL;
TaskHandle_t vSDAVGLGTaskHandle = NULL;
TaskHandle_t vSDJSLGTaskHandle = NULL;

/*******************************************************************************
 *  App Main
 */
void app_main(void){
  const char* TAG = "app_setup";
  //create semaphores
  current_measuers_mutex = xSemaphoreCreateMutex();
  uart_mutex = xSemaphoreCreateMutex();
  card_mutex = xSemaphoreCreateMutex();

//  initArduino();
  //Allow other core to finish initialization
  vTaskDelay(pdMS_TO_TICKS(10));

  //Set I2C interface
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  //Setup OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    display.display();
    ESP_LOGE(TAG, "SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "SSD1306 OLED Display initialized.");
    init_app_screen();
  }

//  search_i2c(); // search for I2C devices (helpful if any uncertainty about sensor addresses raised)

  //Set GPIOS for DHT11 and DS18B20 as GPIOs
  ESP_LOGI(TAG, "GPIO's configuration...");
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DHT11], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DS18B20], PIN_FUNC_GPIO);
  //Set up DHT11 GPIO
  DHT11_init(GPIO_DHT11);
  ESP_LOGI(TAG, "GPIO's initialized!");


  //BH1750 Initialization
  if(!lightMeter.begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR, &Wire)){
    ESP_LOGE(TAG, "BH1750 initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "BH1750 Light meter initialized.");
  }
  //BMP280 Initialization
  if(!pressureMeter.begin(BMP280_ADDR)){
    ESP_LOGE(TAG, "BMP280 initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "BMP280 Pressure meter initialized.");
  }
  if(!rtc.begin(&Wire)){
    ESP_LOGE(TAG, "RTC DS3231 initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "DS3231 Real Time Clock initialized.");
  }
  if(init_sd() != ESP_OK){
    ESP_LOGE(TAG, "Cannot initialize SD Card!");
    for(;;); // Don't proceed, loop forever
  }

  ESP_LOGI(TAG, "Initializing NVS flash...");
  ESP_ERROR_CHECK( nvs_flash_init() );
  ESP_LOGI(TAG, "Initializing network interface...");
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK( esp_event_loop_create_default() );
  ESP_LOGI(TAG, "Initializing http(s) server...");
  setup_httpd();
  ESP_LOGI(TAG, "Connecting to WiFi network...");
  ESP_ERROR_CHECK(network_connect());


  //DS18B20 Initialization
//  initialize_ds18b20();

  //Create business tasks:
  xTaskCreatePinnedToCore( vRTCTask, "RTC", 3096, NULL, RTC_TASK_PRIO, NULL, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vDHT11Task, "DHT11", 1024, NULL, SENSORS_TASK_PRIO, NULL, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vSensorsTask, "SENS", 2048, NULL, SENSORS_TASK_PRIO, NULL, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vDisplayTask, "OLED", 2048, NULL, DISPLAY_TASK_PRIO, NULL, tskNO_AFFINITY );
  //Loggers
  xTaskCreatePinnedToCore( vSDCSVLGTask, "SDCSVLG", 6*1024, NULL, SDCSVLG_TASK_PRIO, &vSDCSVLGTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vSDJSLGTask, "SDJSLG", 6*1024, NULL, SDJSLG_TASK_PRIO, &vSDJSLGTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vSDAVGLGTask, "SDAVGLG", 6*1024, NULL, SDAVGLG_TASK_PRIO, &vSDAVGLGTaskHandle, tskNO_AFFINITY );
  //Create and start stats task
  xTaskCreatePinnedToCore(stats_task, "STATS", 2048, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);
}


