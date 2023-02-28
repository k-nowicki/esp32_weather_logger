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
#include <dirent.h>
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
  xSemaphoreTake(g_current_measuers_mutex, portMAX_DELAY);
  last_measures = g_curr_measures;  //safely read current values
  xSemaphoreGive(g_current_measuers_mutex);
  return last_measures;
}
/**
 * Save given measurements (except DHT11) to global curr_measures variable
 * @param measures Measurements to store
 */
void store_measurements(measurement measures){
  xSemaphoreTake(g_current_measuers_mutex, portMAX_DELAY);
  g_curr_measures.lux = measures.lux;
  g_curr_measures.iTemp = measures.iTemp;
  g_curr_measures.pres = measures.pres;
  g_curr_measures.alti = measures.alti;
  g_curr_measures.time = time(NULL);
  xSemaphoreGive(g_current_measuers_mutex);
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

/*******************************************************************************
 * SD Card and filesystem helpers
 *
 */

/**
 * Unmount SD Card
 *
 */
//void unmount_sd(sdmmc_card_t *card){
void unmount_sd(){
  // All done, unmount partition and disable SDMMC peripheral
  const char mount_point[] = SD_MOUNT_POINT;
  esp_vfs_fat_sdcard_unmount(mount_point, g_card);
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
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &g_card);

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
  sdmmc_card_print_info(stdout, g_card);
  return ESP_OK;
}

/**
 * Reinitialize SD Card
 *
 */
uint8_t reinit_sd(void){
  const char *TAG = "";
  uint8_t err;
  ESP_LOGW(TAG, "Trying to reinitialize SD Card...");
  xSemaphoreTake(g_card_mutex, portMAX_DELAY);
  unmount_sd();
  err = init_sd();
  xSemaphoreGive(g_card_mutex);
  if(err != ESP_OK){
    ESP_LOGE(TAG, "Cannot initialize SD Card!");
  }else{
    ESP_LOGI(TAG, "SD Card reinitialized successfully.");
  }
  return err;
}

/**
 * thread-safely checks sd card status, if something is wrong, tries to reinitialize it
 */
void ensure_card_works(void){
  xSemaphoreTake(g_card_mutex, portMAX_DELAY);
  uint8_t status = sdmmc_get_status(g_card);
  xSemaphoreGive(g_card_mutex);
  if(status != ESP_OK)
    reinit_sd();
}

/*******************************************************************************
 * File system helpers
 */

/**
 * @param path Path to directory to search
 * @return  pointer to string containing newest filename.
 *          If dir is empty, return 000.jpg filename.
 */
char *get_newest_file(char *path) {
  const char *TAG = "GET_NEWFILE";
  DIR *dir;
  struct dirent *ent;
  struct stat st;
  time_t newest = 0;
  char *newest_file = NULL;
  char newest_yet[MAXNAMLEN+1];
  char filename[PATH_MAX];

  strcpy(newest_yet, "000.jpg");  //if dir is empty assume that 000.jpg file is found

  dir = opendir(path);
  if (dir == NULL) {
    ESP_LOGE(TAG, "Cannot open DIR!");
    return NULL;
  }
  while ((ent = readdir(dir)) != NULL) {

    snprintf(filename, PATH_MAX, "%s/%s", path, ent->d_name);

    if (stat(filename, &st) == -1) {
      ESP_LOGE(TAG, "Cannot stat!");
      continue;
    }

    if (S_ISREG(st.st_mode) && st.st_mtime > newest) {
      newest = st.st_mtime;
      strcpy(newest_yet, ent->d_name);  //  strdup is too slow
    }
  }
  closedir(dir);
  newest_file = strdup(newest_yet);
  ESP_LOGE(TAG, "Newest file found: %s", newest_file);
  return newest_file;
}

void get_today_path(char *path_buf){
  time_t now = 0;
  struct tm timeinfo;

  //fetch localtime
  now = time(NULL);
  localtime_r(&now, &timeinfo);

  //create path
  sprintf(path_buf, "/%04d/%02d/%02d", (timeinfo.tm_year+1900), timeinfo.tm_mon+1, timeinfo.tm_mday );
}

/*******************************************************************************
 * Other
 *
 */

