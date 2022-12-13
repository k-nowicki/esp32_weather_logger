/*
 * kk_http_app.cpp
 *
 *  Created on: 12 12 2022
 *      Author: Karol Nowicki
 *
 *  Http App for ESP-IDF
 *
 */

#include "kk_http_app_server.h"


static const char* TAG = "HTTP";


/* An HTTP GET handler */
esp_err_t root_get_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, "<h1>Hello Secure World! from GET /</h1><p>This is message from root_get_handler</p>", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}



/**
 *  URI handler function to be called during GET /uri request
 *
 */
esp_err_t get_handler(httpd_req_t *req){
  /* Send a simple response */
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, "<h1>Hello Secure World! from GET /uri</h1><p>This is message from ordinary get_handler</p>", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

/**
 * URI handler function to be called during POST /uri request
 *
 */
esp_err_t post_handler(httpd_req_t *req){
  /* Destination buffer for content of HTTP request.
   * httpd_req_recv() accepts char* only, but content could
   * as well be any binary data (needs type casting).
   * In case of string data, null termination will be absent, and
   * content length would give length of string */
  char content[100];

  /* Truncate if content length larger than the buffer */
  size_t recv_size = MIN(req->content_len, sizeof(content));

  int ret = httpd_req_recv(req, content, recv_size);
  if (ret <= 0) {  /* 0 return value indicates connection closed */
    /* Check if timeout occurred */
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
      httpd_resp_send_408(req);
    /* In case of error, returning ESP_FAIL will
     * ensure that the underlying socket is closed */
    return ESP_FAIL;
  }

  /* Send a simple response */
  const char resp[] = "<h1>Hello Secure World!</h1><p>This message is from POST /uri post_handler</p>";
  httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
  .uri      = "/uri",
  .method   = HTTP_GET,
  .handler  = get_handler,
  .user_ctx = NULL
};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
  .uri      = "/uri",
  .method   = HTTP_POST,
  .handler  = post_handler,
  .user_ctx = NULL
};

static const httpd_uri_t root = {
  .uri       = "/",
  .method    = HTTP_GET,
  .handler   = root_get_handler,
  .user_ctx  = NULL
};


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

void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server) {
    stop_webserver(*server);
    *server = NULL;
  }
}

void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  httpd_handle_t* server = (httpd_handle_t*) arg;
  if (*server == NULL) {
    *server = start_webserver();
  }
}

httpd_handle_t start_webserver(void){
  httpd_handle_t server = NULL;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server");

  httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

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
  esp_err_t ret = httpd_ssl_start(&server, &conf);
  if (ESP_OK != ret) {
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
  }

  // Set URI handlers
  ESP_LOGI(TAG, "Registering URI handlers");
  httpd_register_uri_handler(server, &root);
  httpd_register_uri_handler(server, &uri_get);
  httpd_register_uri_handler(server, &uri_post);

  return server;
}


/**
 * Function for stopping the webserver
 *
 */
void stop_webserver(httpd_handle_t server){
  if (server) {
    httpd_stop(server);
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
    char *buf = calloc(buf_size, sizeof(char));
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
