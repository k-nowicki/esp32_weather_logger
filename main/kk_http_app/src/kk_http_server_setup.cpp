/**
 * kk_http_server_setup.c
 *
 *  Created on: 14 12 2022
 *      Author: Karol Nowicki
 *
 *  \brief  Https Server setup for esp32 weather station
 *
 *  This file adopts esp-idf https server to be used with weather station.
 *  contains app layer functions for:
 *   - HTTP(s) server initalization
 *   - configuring handles for server callbacks
 *
 */

#include "kk_http_server_setup.h"
#include "kk_http_app.h"

static const char* TAG = "HTTP";


/**
 * \brief Registers appropriate handlers for starting and stopping server depending on
 *        network interface.
 *
 * This is the initializer that should be called before network connection is established
 * It registers handlers for starting and stopping web server on network events (connect & disconnect)
 *
 */
void setup_httpd(void){
  static httpd_handle_t server = NULL;
  /* Register event handlers to start server when Wi-Fi or Ethernet is connected,
   * and stop server when disconnection happens.
   */
#ifdef CONFIG_KK_CONNECT_WIFI
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_KK_CONNECT_WIFI
#ifdef CONFIG_KK_CONNECT_ETHERNET
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_KK_CONNECT_ETHERNET
}

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
httpd_handle_t start_webserver(const char *base_path){
  httpd_handle_t server = NULL;
  httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

  static struct file_server_data *server_data = NULL;

  if (server_data) {
    ESP_LOGE(TAG, "File server already started");
    return (httpd_handle_t)ESP_ERR_INVALID_STATE;
  }

  // Allocate memory for server data
  server_data = (file_server_data *)calloc(1, sizeof(struct file_server_data));
  if (!server_data) {
    ESP_LOGE(TAG, "Failed to allocate memory for server data");
    return (httpd_handle_t)ESP_ERR_NO_MEM;
  }
  //set www
  strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server");
  extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
  extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
  conf.cacert_pem = cacert_pem_start;
  conf.cacert_len = cacert_pem_end - cacert_pem_start;

  extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
  extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
  conf.prvtkey_pem = prvtkey_pem_start;
  conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

#if CONFIG_KK_ENABLE_HTTPS_USER_CALLBACK
  conf.user_cb = https_server_user_callback;
#endif

  /* Use the URI wildcard matching function in order to
   * allow the same handler to respond to multiple different
   * target URIs which match the wildcard scheme */
  conf.httpd.uri_match_fn = httpd_uri_match_wildcard;

  esp_err_t ret = httpd_ssl_start(&server, &conf);
  if (ESP_OK != ret) {
    ESP_LOGI(TAG, "Error starting server!");
    return (httpd_handle_t)ESP_FAIL;
  }

  // Set URI handlers
  ESP_LOGI(TAG, "Registering URI handlers");

  const httpd_uri_t uri_post = {
    .uri      = "/data/*",
    .method   = HTTP_GET,
    .handler  = data_get_handler,
    .user_ctx = (void*)"/data/"
  };
  httpd_register_uri_handler(server, &uri_post);  //for POST /uri

  const httpd_uri_t file_get = {
    .uri      = "/*",
    .method   = HTTP_GET,
    .handler  = file_get_handler,
    .user_ctx = server_data
  };
  httpd_register_uri_handler(server, &file_get);   //for GET  /*

  return server;
}



/**
 * Stops webserver
 * @param server Server pointer
 */
void stop_webserver(httpd_handle_t server){
  if (server) {
    httpd_stop(server);
  }
}

/**
 * Handler function for network disconnected event
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/event-handling.html
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server) {
    stop_webserver(*server);
    *server = NULL;
  }
}

/**
 * Handler function for network connected event
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/event-handling.html
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server == NULL) {
    *server = start_webserver(WWW_BASE_PATH);
  }
}

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
void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb){
    ESP_LOGI(TAG, "Session Created!");
    ESP_LOGI(TAG, "Socket FD: %d", user_cb->tls->sockfd);

    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = (char*)calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", &user_cb->tls->servercert);
    ESP_LOGI(TAG, "Server certificate info:\n%s", buf);
    memset(buf, 0x00, buf_size);

    cert = mbedtls_ssl_get_peer_cert(&user_cb->tls->ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
#endif
