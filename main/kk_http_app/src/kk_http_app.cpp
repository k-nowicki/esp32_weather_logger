/**
 *  kk_http_app.cpp
 *
 *  This file is part of ESP32 Weather Logger https://github.com/k-nowicki/esp32_weather_logger
 *
 *  Created on: 12 12 2022
 *      Author: Karol Nowicki
 *
 *  Web App Handles for ESP32 Weather Logger
 *
 *  Functions responsible for
 *   - File serving from SD card (for html, css, js, img, ico etc)
 *   - Responding to asynchronous requests with dynamic data
 *   - Redirecting / to /index.htm
 *   -
 *
 */

#include <time.h>
#include <app.h>
//#ifdef B1000000   //arduino libs loaded in app.h defines those marcos in different way than esp-idf does
//#undef B1000000
//#undef B110
//#endif
#include "kk_http_app.h"
#include "kk_http_server_setup.h"


static const char* TAG = "HTTP";


/**
 *  Handler to download a file kept on the server
 */
esp_err_t file_get_handler(httpd_req_t *req){
  char filepath[FILE_PATH_MAX];
  FILE *fd = NULL;
  struct stat file_stat;

  const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                           req->uri, sizeof(filepath));
  if (!filename) {
      ESP_LOGE(TAG, "Filename is too long");
      // Respond with 500 Internal Server Error
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
      return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Resolved filename: %s", filename);
  // If name has trailing '/', respond with index
  if (filename[strlen(filename) - 1] == '/') {
      return index_html_get_handler(req);
  }

  if (stat(filepath, &file_stat) == -1) {
    // If file not present on filesystem check if URI corresponds to one of the hardcoded paths
//    if (strcmp(filename, "/index.html") == 0) {
//      return index_html_get_handler(req);
//    } else if (strcmp(filename, "/favicon.ico") == 0) {
//      return favicon_get_handler(req);
//    }
    ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
    /* Respond with 404 Not Found */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
    return ESP_FAIL;
  }

  fd = fopen(filepath, "r");
  if (!fd) {
    ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
  set_content_type_from_file(req, filename);

  // Retrieve the pointer to scratch buffer for temporary storage
  char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
  size_t chunksize;
  do {
    // Read file in chunks into the scratch buffer
    chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);
    if (chunksize > 0) {
      // Send the buffer contents as HTTP response chunk
      if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
        fclose(fd);
        ESP_LOGE(TAG, "File sending failed!");
        // Abort sending file
        httpd_resp_sendstr_chunk(req, NULL);
        // Respond with 500 Internal Server Error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }
  // Keep looping till the whole file is sent
  } while (chunksize != 0);

  // Close file after sending complete
  fclose(fd);
  ESP_LOGI(TAG, "File sending complete");
  // Respond with an empty chunk to signal HTTP response completion
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/**
 * Handler to respond with dynamic data
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t data_get_handler(httpd_req_t *req){
//  req->uri
  if (strncmp(req->uri + strlen((char*)req->user_ctx), "current_measurements.json", 25) == 0) {
      return send_current_measurements(req);
//  } else if (strncmp(req->uri, "current_measurements", 3) == 0) {
//    handle_post();
//  } else if (strncmp(req->uri, "PUT", 3) == 0) {
//    handle_put();
//  } else if (strncmp(req->uri, "DELETE", 6) == 0) {
//    handle_delete();
  } else {
      ESP_LOGE(TAG, "Failed to recognize path: %s", req->uri);
      /* Respond with 404 Not Found */
      httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset does not exist");
      return ESP_FAIL;
  }
}

/**
 * Sends json formatted fresh measurements as a http response
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t send_current_measurements(httpd_req_t *req){
  measurement measurements;
  time_t now;
  httpd_resp_set_type(req, "application/x-javascript");
  char * respond_buf = (char*)malloc(512);
  now = time(NULL);
  measurements = get_latest_measurements();
  sprintf(respond_buf, "{\"time\":\"%lld\",\"int_t\":%3.2F, \"ext_t\":%3.2F, \"humi\":%d, \"sun\":%5.2F, \"press\":%4.2f}\n",
                (long long)(now),
                measurements.iTemp,
                measurements.eTemp,
                (int)(measurements.humi),
                measurements.lux,
                measurements.pres);
  httpd_resp_send(req, respond_buf, -1);  // Response body can be empty
  return ESP_OK;
}

/**
 * Handler to redirect incoming GET request for / to /index.html
 */
esp_err_t index_html_get_handler(httpd_req_t *req){
  httpd_resp_set_status(req, "301 Moved Permanently");
  httpd_resp_set_hdr(req, "Location", "/index.htm");
  httpd_resp_send(req, NULL, 0);  // Response body can be empty
  return ESP_OK;
}


/**
 *  Set HTTP response content type according to file extension
 */
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename){
  if(IS_FILE_EXT(filename, ".pdf")){
    return httpd_resp_set_type(req, "application/pdf");
  }else if (IS_FILE_EXT(filename, ".html")){
    return httpd_resp_set_type(req, "text/html");
  }else if (IS_FILE_EXT(filename, ".htm")){
    return httpd_resp_set_type(req, "text/html");
  }else if (IS_FILE_EXT(filename, ".jpeg")){
    return httpd_resp_set_type(req, "image/jpeg");
  }else if (IS_FILE_EXT(filename, ".ico")){
    return httpd_resp_set_type(req, "image/x-icon");
  }else if (IS_FILE_EXT(filename, ".css")){
    return httpd_resp_set_type(req, "text/css");
  }else if (IS_FILE_EXT(filename, ".js")){
    return httpd_resp_set_type(req, "application/x-javascript");
  }else if (IS_FILE_EXT(filename, ".xml")){
    return httpd_resp_set_type(req, "application/xml");
  }else if (IS_FILE_EXT(filename, ".gif")){
    return httpd_resp_set_type(req, "image/gif");
  }else if (IS_FILE_EXT(filename, ".png")){
    return httpd_resp_set_type(req, "image/png");
  }else if (IS_FILE_EXT(filename, ".3gp")){
    return httpd_resp_set_type(req, "video/mpeg");
  }else if (IS_FILE_EXT(filename, ".json")){
    return httpd_resp_set_type(req, "text/json");
  }// This is a limited set only. For any other type always set as plain text
  return httpd_resp_set_type(req, "text/plain");
}

/**
 * Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path)
 */
const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize){
  const size_t base_pathlen = strlen(base_path);
  size_t pathlen = strlen(uri);

  const char *quest = strchr(uri, '?');
  if (quest) {
    pathlen = MIN(pathlen, quest - uri);
  }
  const char *hash = strchr(uri, '#');
  if (hash) {
    pathlen = MIN(pathlen, hash - uri);
  }
  if (base_pathlen + pathlen + 1 > destsize) {
    return NULL;  // Full path string won't fit into destination buffer
  }
  // Construct full path (base + path)
  strcpy(dest, base_path);
  strlcpy(dest + base_pathlen, uri, pathlen + 1);
  // Return pointer to path, skipping the base
  return dest + base_pathlen;
}


