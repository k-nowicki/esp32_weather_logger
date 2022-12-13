/*
 * kk_http.h
 *
 *  Created on: 12 12 2022
 *      Author: Karol Nowicki
 */

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <nvs_flash.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_common.h"
#include <esp_tls_crypto.h>
#include <esp_http_server.h>
#include <esp_https_server.h>

#ifndef COMPONENTS_HH_HTTP_SERVER_
#define COMPONENTS_HH_HTTP_SERVER_


void setup_httpd(void);

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t get_handler(httpd_req_t *req);
esp_err_t post_handler(httpd_req_t *req);

#if CONFIG__KK__ENABLE_HTTPS_USER_CALLBACK
void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb);
#endif
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


#endif /* COMPONENTS_HH_HTTP_SERVER_ */
