/*
 * kk_http.h
 *
 *  Created on: 12 12 2022
 *      Author: Karol Nowicki
 */

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

#ifndef COMPONENTS_KK_HTTP_APP_
#define COMPONENTS_KK_HTTP_APP_
//#ifdef __cplusplus
//extern "C" {
//#endif


/// Helper macro for checking file extension
#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/*#****************************************************************************/


/**
 * Handler to download a file kept on the server
 * @param req Request pointer
 * @return
 *      ESP_OK if success
 *      ESP_FAIL otherwise
 */
esp_err_t file_get_handler(httpd_req_t *req);

/**
 * \brief Handler to execute HTTP GET /pic/ requests
 *
 * Respond with picture OL list for date given in /pic/?DDMMYYY
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t pic_get_handler(httpd_req_t *req);

/**
 * Handler to redirect incoming GET request for / to /index.html
 * @param req Request data
 * @return ESP_OK
 */
esp_err_t index_html_get_handler(httpd_req_t *req);

/**
 * \brief Handler to execute HTTP GET /set/ requests
 *
 * Recognize command from request uri and tries to execute if command valid.
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t set_get_handler(httpd_req_t *req);

/**
 * \brief Handler to execute HTTP POST /set/ requests
 *
 * Recognize command from request uri and tries to execute if command valid.
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t set_post_handler(httpd_req_t *req);

/**
 * Handler to respond with dynamic data
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t data_get_handler(httpd_req_t *req);

/**
 * Sends json formatted fresh measurements as a http response
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t send_current_measurements(httpd_req_t *req);

/**
 * Sends json formatted up time as a http response
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t send_current_ms(httpd_req_t *req);

/**
 * Sends confirmation and execute software reset
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t reset_send_confirmation(httpd_req_t *req);


/**
 *  Set HTTP response content type according to file extension
 * @param req Request data
 * @param filename Pointer to filename
 * @return Error codes from @see httpd_resp_set_type()
 *
 */
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

/**
 * Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path)
 */
/**
 * Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path)
 *
 * @param dest  Destination buffer pointer
 * @param base_path web server base path (i.e. /sd/www)
 * @param uri   requested uri
 * @param destsize  size of destination buffer
 * @return
 *      NULL when fail
 *      Pointer to path, skipping the base
 */
const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);

/**
 *
 * @param path Path to directory
 * @param date Date of interest
 * @param list_buf pointer to buffer for generated list. Buffer must be initialized and huge (recommended is 32KB)
 */
void generate_html_list(char* path, char* date, char *list_buf);


//#ifdef __cplusplus
//}
//#endif

#endif /* COMPONENTS_KK_HTTP_APP_ */
