/* KK Weather Station
 * Global helpers
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
#include "app.h"



/*******************************************************************************
 *  Application helper functions
 *
 */

/**
 * curr_measures are global variable used in many tasks. That is why it needs to
 * be protected against changing value in the middle of writing/reading.
 * @return Copy of curr_measures done under mutex control.
 */
measurement get_latest_measurements(void){
  measurement last_measures;
  xSemaphoreTake(current_measuers_mutex, portMAX_DELAY);
  last_measures = curr_measures;  //safely read current values
  xSemaphoreGive(current_measuers_mutex);
  return last_measures;
}
/**
 * Save given measurements (except DHT11) to global curr_measures variable
 * @param measures Measurements to store
 */
void store_measurements(measurement measures){
  xSemaphoreTake(current_measuers_mutex, portMAX_DELAY);
  curr_measures.lux = measures.lux;
  curr_measures.iTemp = measures.iTemp;
  curr_measures.pres = measures.pres;
  curr_measures.alti = measures.alti;
  xSemaphoreGive(current_measuers_mutex);
}


/***********************************************
 * I2C search
 *
 */
void search_i2c(void){
  const char* TAG = "app_setup";
  uint8_t error, address;
  int nDevices;
  ESP_LOGI(TAG, "Searching I2C devices...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      ESP_LOGI(TAG, " -I2C device found at address 0x%02x", address);
      nDevices++;
    }
    else if(error == 4) {
      printf(" -Unknown error at address 0x");
      ESP_LOGI(TAG, " -Unknown error at address 0x");
      if(address < 16)
        printf("0");
      printf((const char *)&address, HEX);
    }
  }
  if(nDevices == 0)
    ESP_LOGW(TAG, " -No I2C devices found");
  else
    ESP_LOGI(TAG, "Done I2C scanning!");
//  vTaskDelay(pdMS_TO_TICKS(2000)); // wait 2 seconds
}

/**
 * Unmount SD Card
 *
 */
//void unmount_sd(sdmmc_card_t *card){
void unmount_sd(){
  // All done, unmount partition and disable SDMMC peripheral
  const char mount_point[] = SD_MOUNT_POINT;
  esp_vfs_fat_sdcard_unmount(mount_point, card);
  ESP_LOGI("", "Card unmounted");
}

/**
 * Initialize and mount SD Card
 *
 */
uint8_t init_sd(){
  const char *TAG = "";
  esp_err_t ret;
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .format_if_mount_failed = true,
#else
    .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .max_files = SD_MAX_FILES,
    .allocation_unit_size = SD_ALLOCATION_UNIT_SIZE
  };

  const char mount_point[] = SD_MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card | Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp target board has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // Set bus width to use:
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  slot_config.width = 4;
#else
  slot_config.width = 1;
#endif

  // On chips where the GPIOs used for SD card can be configured, set them in the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
  slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
  slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
  slot_config.d0 = CONFIG_EXAMPLE_PIN_D0;
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  slot_config.d1 = CONFIG_EXAMPLE_PIN_D1;
  slot_config.d2 = CONFIG_EXAMPLE_PIN_D2;
  slot_config.d3 = CONFIG_EXAMPLE_PIN_D3;
#endif  // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
#endif  // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

  // Enable internal pullups on enabled pins. The internal pullups are insufficient
  // however, please make sure 10k external pullups are connected on the bus.
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
              "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }else{
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Filesystem mounted");

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);
  return ESP_OK;
}

/**
 * Reinitialize SD Card
 *
 */
uint8_t reinit_sd(void){
  unmount_sd();
  return init_sd();
}
