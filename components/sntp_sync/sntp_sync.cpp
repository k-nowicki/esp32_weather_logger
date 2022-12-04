/* LwIP SNTP

   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_common.h"
#include "esp_sntp.h"
#include "sntp_sync.h"

static const char *TAG = "sntp_sync";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;
static void obtain_time(void);
static void initialize_sntp(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv){
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv){
  ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void sync_time_with_ntp(void){
  ++boot_count;
  ESP_LOGI(TAG, "Boot count: %d", boot_count);

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2016 - 1900)) {
    ESP_LOGI(TAG, "Time is not set yet. Getting time over NTP...");
    obtain_time();
    // update 'now' variable with current time
    time(&now);
  }
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  else {
    // add 500 ms error to the current system time.
    // Only to demonstrate a work of adjusting method!
    {
        ESP_LOGI(TAG, "Add a error for test adjtime");
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        int64_t cpu_time = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        int64_t error_time = cpu_time + 500 * 1000L;
        struct timeval tv_error = { .tv_sec = error_time / 1000000L, .tv_usec = error_time % 1000000L };
        settimeofday(&tv_error, NULL);
    }

    ESP_LOGI(TAG, "Time was set, now just adjusting it. Use SMOOTH SYNC method.");
    obtain_time();
    // update 'now' variable with current time
    time(&now);
  }
#endif

  char strftime_buf[64];

  // Set timezone to Polish time and print local time
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time in Warsaw is: %s", strftime_buf);

  if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
    struct timeval outdelta;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
      adjtime(NULL, &outdelta);
      ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %li sec: %li ms: %li us",
               (long)outdelta.tv_sec,
               outdelta.tv_usec/1000,
               outdelta.tv_usec%1000);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }

//  const int deep_sleep_sec = 10;
//  ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
//  esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static void obtain_time(void){


  /**
   * NTP server address could be aquired via DHCP,
   * see LWIP_DHCP_GET_NTP_SRV menuconfig option
   */
#ifdef LWIP_DHCP_GET_NTP_SRV
  sntp_servermode_dhcp(1);
#endif

  /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
   * Read "Establishing Wi-Fi or Ethernet Connection" section in
   * examples/protocols/README.md for more information about this function.
   */

  initialize_sntp();

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 10;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  time(&now);
  localtime_r(&now, &timeinfo);

//  ESP_ERROR_CHECK( network_disconnect() );
}

static void initialize_sntp(void){
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
  sntp_init();
}
