/* KK Weather Station
 * Stats task
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

static esp_err_t print_real_time_stats(TickType_t);

/**
 * @brief Task responsible for communicating at UART Debug port:
 *      - System Real Time Statistics
 *      - Current Measurements
 *      - Current system Time [not yet implemented]
 *      - Current system state  [not yet implemented]
 *      - Web Server requests?  [not yet implemented]
 *
 * @param arg
 */
void vStatsTask(void *arg){
  measurement tmp_measurements;
  int stats_error;
  //Print real time stats and measurements periodically
  while (1) {
    stats_error = print_real_time_stats(STATS_TICKS); //this takes STATS_TICKS ms when it is counting
    tmp_measurements = get_latest_measurements();
    xSemaphoreTake(g_uart_mutex, portMAX_DELAY);      //take UART port
    if (stats_error == ESP_OK) {
      printf("Real time stats obtained\n");
    } else {
      printf("Error getting real time stats\n");
    }
    printf("-----------------------------------------\n");
    printf("Current measurements:\n");
    printf("DHT Temperature:   %d °C\n", (int)tmp_measurements.eTemp);
    printf("DHT Humidity:      %d %%\n", (int)tmp_measurements.humi);
    printf("DHT Status code:   %d\n", tmp_measurements.dht_status);
    printf("BH1750 Light exp:  %6.2F Lux\n", tmp_measurements.lux);
    printf("BMP Temperature:   %3.2F °C\n", tmp_measurements.iTemp);
    printf("BMP Atm. pressure: %4.2f hPa\n", tmp_measurements.pres);
    printf("BMP Altitude:      %5.2F m\n", tmp_measurements.alti);
    printf("Wind Speed:        %2.3F m/s\n", tmp_measurements.wind);
    printf("=========================================\n\n");
    xSemaphoreGive(g_uart_mutex);     //give back UART port
  }
}


/*******************************************************************************
 *  Application helper functions
 *
 */


/**
 * @brief   Function to print the CPU usage of tasks over a given duration.
 *
 * This function will measure and print the CPU usage of tasks over a specified
 * number of ticks (i.e. real time stats). This is implemented by simply calling
 * uxTaskGetSystemState() twice separated by a delay, then calculating the
 * differences of task run times before and after the delay.
 *
 * @note    If any tasks are added or removed during the delay, the stats of
 *          those tasks will not be printed.
 * @note    This function should be called from a high priority task to minimize
 *          inaccuracies with delays.
 * @note    When running in dual core mode, each core will correspond to 50% of
 *          the run time.
 *
 * @param   xTicksToWait    Period of stats measurement
 *
 * @return
 *  - ESP_OK                Success
 *  - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
 *  - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET
 *  - ESP_ERR_INVALID_STATE Delay duration too short
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait){
  TaskStatus_t *start_array = NULL, *end_array = NULL;
  UBaseType_t start_array_size, end_array_size;
  uint32_t start_run_time, end_run_time, total_elapsed_time;
  esp_err_t ret;

  //Allocate array to store current task states
  start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
  start_array = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * start_array_size);
  if (start_array == NULL) {
    ret = ESP_ERR_NO_MEM;
    goto exit;
  }
  //Get current task states
  start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
  if (start_array_size == 0) {
    ret = ESP_ERR_INVALID_SIZE;
    goto exit;
  }

  vTaskDelay(xTicksToWait);

  //Allocate array to store tasks states post delay
  end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
  end_array = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * end_array_size);
  if (end_array == NULL) {
    ret = ESP_ERR_NO_MEM;
    goto exit;
  }
  //Get post delay task states
  end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
  if (end_array_size == 0) {
    ret = ESP_ERR_INVALID_SIZE;
    goto exit;
  }

  //Calculate total_elapsed_time in units of run time stats clock period.
  total_elapsed_time = (end_run_time - start_run_time);
  if (total_elapsed_time == 0) {
    ret = ESP_ERR_INVALID_STATE;
    goto exit;
  }
  //lock the UART to not interrupt printing
  xSemaphoreTake(g_uart_mutex, portMAX_DELAY);
  printf("Real time stats over %d ticks\n", xTicksToWait);
  printf("-----------------------------------------\n");
  printf("| Task | Run Time | Percentage\n");
  xSemaphoreGive(g_uart_mutex);
  //Match each task in start_array to those in the end_array
  for (int i = 0; i < start_array_size; i++) {
    int k = -1;
    for (int j = 0; j < end_array_size; j++) {
      if (start_array[i].xHandle == end_array[j].xHandle) {
        k = j;
        //Mark that task have been matched by overwriting their handles
        start_array[i].xHandle = NULL;
        end_array[j].xHandle = NULL;
        break;
      }
    }
    //Check if matching task found
    if (k >= 0) {
      uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
      uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
      xSemaphoreTake(g_uart_mutex, portMAX_DELAY);
      printf("| %s | %d | %d%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
      xSemaphoreGive(g_uart_mutex);
    }
  }
  xSemaphoreTake(g_uart_mutex, portMAX_DELAY);
  //Print unmatched tasks
  for (int i = 0; i < start_array_size; i++) {
    if (start_array[i].xHandle != NULL) {
        printf("| %s | Deleted\n", start_array[i].pcTaskName);
    }
  }
  for (int i = 0; i < end_array_size; i++) {
    if (end_array[i].xHandle != NULL) {
      printf("| %s | Created\n", end_array[i].pcTaskName);
    }
  }
  xSemaphoreGive(g_uart_mutex);
  ret = ESP_OK;

exit:    //Common return path
  free(start_array);
  free(end_array);
  return ret;
}
