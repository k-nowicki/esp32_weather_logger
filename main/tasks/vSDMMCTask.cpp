/* KK Weather Station
 * SDMMC Task
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

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

//App headers
#include "tasks.h"

void unmount_sd();
uint8_t init_sd();
//void unmount_sd(sdmmc_card_t *);
//uint8_t init_sd(sdmmc_card_t &);
sdmmc_card_t * card;
/*******************************************************************************/


static const char *TAG = "SDMMC";
#define MOUNT_POINT "/sdcard"


/**
 * @brief Task responsible for handling SD Card storage
 *
 * @param arg
 *
 */
void vSDMMCTask(void*){

  if(init_sd() != ESP_OK){
      ESP_LOGE(TAG, "Cannot initialize SD Card!");
      ESP_LOGE(TAG, "Task will end...");
      vTaskDelete( NULL );
  }
  while (1) {
     const char *file_hello = MOUNT_POINT"/hello.txt";

     ESP_LOGI(TAG, "Opening file %s", file_hello);
     FILE *f = fopen(file_hello, "w");
     if (f == NULL) {
         ESP_LOGE(TAG, "Failed to open file for writing");
         ESP_LOGE(TAG, "Task will end...");
         vTaskDelete( NULL );
     }
     ESP_LOGI(TAG, "File opened, lets try to write something...", file_hello);
     fprintf(f, "Hello %s!\n This is kk_weather_station project writing to file on SD Card!\n", card->cid.name);
     fclose(f);
     ESP_LOGI(TAG, "File written");

     const char *file_foo = MOUNT_POINT"/foo.txt";

     // Check if destination file exists before renaming
     struct stat st;
     if (stat(file_foo, &st) == 0) {
         // Delete it if it exists
         unlink(file_foo);
     }

     // Rename original file
     ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
     if (rename(file_hello, file_foo) != 0) {
         ESP_LOGE(TAG, "Rename failed");
         ESP_LOGE(TAG, "Task will end...");
         unmount_sd();
         vTaskDelete( NULL );
     }

     // Open renamed file for reading
     ESP_LOGI(TAG, "Reading file %s", file_foo);
     f = fopen(file_foo, "r");
     if (f == NULL) {
         ESP_LOGE(TAG, "Failed to open file for reading");
         ESP_LOGE(TAG, "Task will end...");
         unmount_sd();
         vTaskDelete( NULL );
     }

     // Read a line from file
     char line[64];
     fgets(line, sizeof(line), f);
     fclose(f);

     // Strip newline
     char *pos = strchr(line, '\n');
     if (pos) {
         *pos = '\0';
     }
     ESP_LOGI(TAG, "Read from file: '%s'", line);

     ESP_LOGI(TAG, "End of SDMMC Task, all done! Task will be deleted!");
     unmount_sd();
     vTaskDelete( NULL );
  }
}

//void unmount_sd(sdmmc_card_t *card){
void unmount_sd(){
  // All done, unmount partition and disable SDMMC peripheral
  const char mount_point[] = MOUNT_POINT;
  esp_vfs_fat_sdcard_unmount(mount_point, card);
  ESP_LOGI(TAG, "Card unmounted");
}

//uint8_t init_sd(sdmmc_card_t & _card){
uint8_t init_sd(){
  esp_err_t ret;

//  sdmmc_card_t * card = &_card;

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .format_if_mount_failed = true,
#else
    .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
  // Please check its source code and implement error recovery when developing
  // production applications.

  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // Set bus width to use:
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
  slot_config.width = 4;
#else
  slot_config.width = 1;
#endif

  // On chips where the GPIOs used for SD card can be configured, set them in
  // the slot_config structure:
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

  // Enable internal pullups on enabled pins. The internal pullups
  // are insufficient however, please make sure 10k external pullups are
  // connected on the bus. This is for debug / example purpose only.
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
