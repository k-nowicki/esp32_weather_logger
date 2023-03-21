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
#include "kk_http_app/src/kk_http_app.h"
#include "kk_http_app/src/kk_http_server_setup.h"
#include "camera_helper.h"

extern "C" {
  void app_main(void);
}



/*******************************************************************************
 * Variable definitions
 */

//Business logic global variables
measurement g_curr_measures; // Current measurements

// RTC object depending on used physical device
#ifdef CONFIG_RTC_DS1307
ErriezDS1307 g_rtc;
#elif CONFIG_RTC_DS3231
ErriezDS3231 g_rtc;
#endif

//Sensor global objects
BH1750 g_lightMeter(BH1750_ADDR);
ANEMO g_windMeter(ANEMO_ADDR);
Adafruit_BMP280 g_pressureMeter(&Wire1); // I2C
Adafruit_SSD1306 g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);
#ifdef EXTERNAL_SENSOR_HTU21
Adafruit_HTU21DF g_htu21 = Adafruit_HTU21DF();
#endif

//SD Card global object
sdmmc_card_t * g_card;

//semaphores
SemaphoreHandle_t g_current_measuers_mutex;
SemaphoreHandle_t g_uart_mutex;
SemaphoreHandle_t g_card_mutex;

//task handlers
TaskHandle_t g_vRTCTaskHandle = NULL;
TaskHandle_t g_vDHT11TaskHandle = NULL;
TaskHandle_t g_vSensorsTaskHandle = NULL;
TaskHandle_t g_vDisplayTaskHandle = NULL;
TaskHandle_t g_vCameraTaskHandle = NULL;
TaskHandle_t g_vSDCSVLGTaskHandle = NULL;
TaskHandle_t g_vSDAVGLGTaskHandle = NULL;
TaskHandle_t g_vSDJSLGTaskHandle = NULL;
TaskHandle_t g_vStatsTaskHandle = NULL;

/*******************************************************************************
 *  App Main
 */
void app_main(void){
  const char* TAG = "app_setup";
  //create semaphores
  g_current_measuers_mutex = xSemaphoreCreateMutex();
  g_uart_mutex = xSemaphoreCreateMutex();
  g_card_mutex = xSemaphoreCreateMutex();
  if(g_uart_mutex == NULL){
      ESP_LOGE(TAG, "UART Mutex creation failed! Hold till reset!");
      for(;;);
  }

  //initArduino();
  //Allow other core to finish initialization
  vTaskDelay(pdMS_TO_TICKS(10));

  //Setup Camera
  if(init_camera(FRAMESIZE) != ESP_OK){
    ESP_LOGE(TAG, "Camera initialization failed!");
//    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "Camera initialized.");
  }

  //Set I2C interface
  Wire1.begin(I2C_SDA, I2C_SCL);
  Wire1.setClock(400000);

  //Setup OLED display
  if(!g_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    g_display.display();
    ESP_LOGE(TAG, "SSD1306 allocation failed");
  }else{
    ESP_LOGI(TAG, "SSD1306 OLED Display initialized.");
    init_app_screen();
  }

  //search_i2c(); // search for I2C devices (helpful if any uncertainty about sensor addresses raised)

  //Set GPIOS for DHT11 and DS18B20 as GPIOs
  ESP_LOGI(TAG, "GPIO's configuration...");
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DHT11], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DS18B20], PIN_FUNC_GPIO);
  //Set up DHT11 GPIO
  DHT11_init(GPIO_DHT11);
  ESP_LOGI(TAG, "GPIO's initialized!");


  //BH1750 Initialization
  if(!g_lightMeter.begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR, &Wire1)){
    ESP_LOGE(TAG, "BH1750 initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "BH1750 Light meter initialized.");
  }

  //KK-ANEMO Initialization
  if(!g_windMeter.begin(ANEMO_ADDR, &Wire1)){
    ESP_LOGE(TAG, "ANEMOMETER initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "ANEMOMETER initialized.");
  }

  //BMP280 Initialization
  if(!g_pressureMeter.begin(BMP280_ADDR)){
    ESP_LOGE(TAG, "BMP280 initialization failed!");
    //for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "BMP280 Pressure meter initialized.");
  }

  #ifdef EXTERNAL_SENSOR_HTU21
  //HTU21 Initialization
  if(!g_htu21.begin(&Wire1)){
    ESP_LOGE(TAG, "HTU21 initialization failed!");
    //for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "HTU21 Pressure meter initialized.");
  }
  #endif

  //RTC Initialization
  if(!g_rtc.begin(&Wire1)){
    ESP_LOGE(TAG, "RTC DS3231 initialization failed!");
    for(;;); // Don't proceed, loop forever
  }else{
    ESP_LOGI(TAG, "DS3231 Real Time Clock initialized.");
  }

  //SD Card initialization
  if(init_sd() != ESP_OK){
    ESP_LOGE(TAG, "Cannot initialize SD Card!");
    for(;;); // Don't proceed, loop forever
  }

  //WiFi Initialization
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
  //initialize_ds18b20();

  //Create business tasks:
  xTaskCreatePinnedToCore( vRTCTask, "RTC", 3096, NULL, RTC_TASK_PRIO, &g_vRTCTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vDHT11Task, "DHT11", 2048, NULL, SENSORS_TASK_PRIO, &g_vDHT11TaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vSensorsTask, "SENS", 2048, NULL, SENSORS_TASK_PRIO, &g_vSensorsTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vDisplayTask, "OLED", 2048, NULL, DISPLAY_TASK_PRIO, &g_vDisplayTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vCameraTask, "CAM", 48*1024, NULL, CAM_TASK_PRIO, &g_vCameraTaskHandle, tskNO_AFFINITY );
  //Loggers
  xTaskCreatePinnedToCore( vSDCSVLGTask, "SDCSVLG", 6*1024, NULL, SDCSVLG_TASK_PRIO, &g_vSDCSVLGTaskHandle, tskNO_AFFINITY );
//  xTaskCreatePinnedToCore( vSDJSLGTask, "SDJSLG", 6*1024, NULL, SDJSLG_TASK_PRIO, &g_vSDJSLGTaskHandle, tskNO_AFFINITY );
  xTaskCreatePinnedToCore( vSDAVGLGTask, "SDAVGLG", 6*1024, NULL, SDAVGLG_TASK_PRIO, &g_vSDAVGLGTaskHandle, tskNO_AFFINITY );
  //Create and start stats task
  xTaskCreatePinnedToCore(vStatsTask, "STATS", 3072, NULL, STATS_TASK_PRIO, &g_vStatsTaskHandle, tskNO_AFFINITY);

}


