/**
 *  kk_http_server_setup
 *
 *  This file is part of ESP32 Weather Logger https://github.com/k-nowicki/esp32_weather_logger
 *
 *
 *  Created on: 12 12 2022
 *      Author: Karol Nowicki
 *
 *  HTTPS server setup
 *  Server starting and stoping callbacks
 *
 */

#include <WiFiUdp.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <nvs_flash.h>
#include "esp_netif.h"
#include "esp_eth.h"
#ifdef B1000000   //arduino libs loaded in app.h defines those marcos in different way than esp-idf does
#undef B1000000
#undef B110
#endif
#include "esp_vfs.h"
#include "protocol_common.h"
#include <esp_tls_crypto.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <setup.h>


#ifndef COMPONENTS_KK_HTTP_SERVER_SETUP_
#define COMPONENTS_KK_HTTP_SERVER_SETUP_

//#ifdef __cplusplus
//extern "C" {
//#endif

/**
 * WWW_BASE_PATH defines starting path of http server directory including mount point
 * all files requested as /filename.xyz will be searched as WWW_BASE_PATH/filename.xyz
 * i.e. /sd/www/filename.xyz
 */
#define WWW_BASE_PATH "/sd/www"

/// Max length a file path can have on storage
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/// Scratch buffer size for temporary storage during file transfer
#define SCRATCH_BUFSIZE  8192

/// Struct for serving file contents
struct file_server_data {
  /// Base path of file storage
  char base_path[ESP_VFS_PATH_MAX + 1];
  /// Scratch buffer for temporary storage during file transfer
  char scratch[SCRATCH_BUFSIZE];
};


/**
 * \brief Registers appropriate handlers for starting and stopping server depending on
 *        network interface.
 *
 * This is the initializer that should be called before network connection is established
 * It registers handlers for starting and stopping web server on network events (connect & disconnect)
 *
 */
void setup_httpd(void);

/**
 *
 * @param base_path
 *        Root directory of http server (file system path including mount point)
 *        example: "/sd/www"
 *
 * @return
 *      HTTP Server Instance Handle
 *      ESP_ERR_INVALID_STATE if server already started
 *      ESP_ERR_NO_MEM if can not initialize memory
 *      ESP_FAIL if any other problem with starting server
 */
httpd_handle_t start_webserver(const char *base_path);

/**
 * Stops webserver
 * @param server Server pointer
 */
void stop_webserver(httpd_handle_t server);

/**
 * Handler function for network disconnected event
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/event-handling.html
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * Handler function for network connected event
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/event-handling.html
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#if CONFIG_KK_ENABLE_HTTPS_USER_CALLBACK
/**
 * Example callback function to get the certificate of connected clients,
 * whenever a new SSL connection is created
 *
 * Can also be used to other information like Socket FD, Connection state, etc.
 *
 * NOTE: This callback will not be able to obtain the client certificate if the
 * following config `Set minimum Certificate Verification mode to Optional` is
 * not enabled (enabled by default).
 *
 * The config option is found here - Component config → ESP-TLS
 *
 */
void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb);
#endif



//
//#ifdef __cplusplus
//}
//#endif

#endif /* COMPONENTS_KK_HTTP_SERVER_SETUP_ */
