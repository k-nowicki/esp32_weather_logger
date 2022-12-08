/* KK Weather Station
 * SDLOG_TASK_PRIO Task
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
#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#undef _POSIX_SOURCE
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

//App headers
#include "tasks.h"



/*******************************************************************************/

static const char *TAG = "SDLOG";

/**
 * @brief Task responsible for logging measurements to file on SD card.
 *
 * Handles log files
 * Once every second:
 *    - append new measurements to current log file (open file, append, close file)
 *
 * @param arg
 *
 */
void vSDLOGTask(void*){

  DIR *dir;
  struct dirent *entry;

  ESP_LOGI(TAG, "Listing SD card / dir content...");

  if ((dir = opendir(SD_MOUNT_POINT)) == NULL)
    perror("opendir() error");
  else {
    puts("contents of "SD_MOUNT_POINT);
    while ((entry = readdir(dir)) != NULL)
      printf("  %s\n", entry->d_name);
    closedir(dir);
  }

  while (1) {
     const char *file_hello = SD_MOUNT_POINT"/hello.txt";

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

     const char *file_foo = SD_MOUNT_POINT"/foo.txt";

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

