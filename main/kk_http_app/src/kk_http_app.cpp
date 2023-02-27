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
 *   - File serving from SD card
 *   - Responding to Web API calls
 *   - Redirecting / to /index.htm
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


/*******************************************************************************
 *    Handlers for defined http methods/paths
 *******************************************************************************/

/**
 * Handler to download a file kept on the server
 * @param req Request pointer
 * @return
 *      ESP_OK if success
 *      ESP_FAIL otherwise
 *
 */
esp_err_t file_get_handler(httpd_req_t *req){
  FILE *fd = NULL;
  char filepath[FILE_PATH_MAX];
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
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
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
#ifdef CONFIG_KK_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/**
 * \brief Handler to execute HTTP GET /pic_list/ requests
 *
 * Respond with picture list for date given in /pic_list/?DDMMYYY
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t pic_get_handler(httpd_req_t *req){
  char* list_buf = NULL;

  char* date = strrchr(req->uri, '?'); //get date from uri which is right after ? sign
  if(date != NULL) {  //if uri contains parameteters
    date += 1;
  }else{              //if not - Respond with 404 Not Found
    ESP_LOGE(TAG, "Failed to recognize path: %s", req->uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset does not exist");
    return ESP_FAIL;
  }

  list_buf = (char*)malloc(PIC_LIST_BUFFER_SIZE);    //allocate memory
  if(list_buf == NULL){
    ESP_LOGE(TAG, "Cannot allocate memory for picture list! Requested size: %i Bytes", PIC_LIST_BUFFER_SIZE);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate memory for file list!");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Generating list of pictures from %s", date);
  ESP_LOGI(TAG, "List from path %s", CAM_FILE_PATH);
  generate_html_list((char*)CAM_FILE_PATH, date, list_buf);
  ESP_LOGE(TAG, "Zawartosc listy:\n %s", list_buf);

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "text/html");
#ifdef CONFIG_KK_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send(req, list_buf, -1);  // Response body can be empty
  free(list_buf);
  return ESP_OK;
}


/**
 * \brief Handler to execute HTTP GET /set/ requests
 *
 * Recognize command from request uri and tries to execute if command valid.
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t set_get_handler(httpd_req_t *req){
  //perform as if it were a POST
  return set_post_handler(req);
}

/**
 * \brief Handler to execute HTTP POST /set/ requests
 *
 * Recognize command from request uri and tries to execute if command valid.
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t set_post_handler(httpd_req_t *req){
//  req->uri
  if(strncmp(req->uri, "/set/reset", 10) == 0){
    return reset_send_confirmation(req);
//  }else if(strncmp(req->uri + strlen((char*)req->user_ctx), "current_ms.json", 25) == 0){
//    return send_current_ms(req);
  }else{
    ESP_LOGE(TAG, "Failed to recognize path: %s", req->uri);
    /* Respond with 404 Not Found */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset does not exist");
    return ESP_FAIL;
  }
}

/**
 * Handler to respond with dynamic data
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t data_get_handler(httpd_req_t *req){
//  req->uri
  if(strncmp(req->uri + strlen((char*)req->user_ctx), "current_measurements.json", 25) == 0){
    return send_current_measurements(req);
  }else if(strncmp(req->uri + strlen((char*)req->user_ctx), "current_ms.json", 25) == 0){
    return send_current_ms(req);
//  }else if(strncmp(req->uri + strlen((char*)req->user_ctx), "history", 7) == 0){
//    return send_history(req);
  }else{
    ESP_LOGE(TAG, "Failed to recognize path: %s", req->uri);
    /* Respond with 404 Not Found */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset does not exist");
    return ESP_FAIL;
  }
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

/*******************************************************************************
 *    Executive methods for specific request uris
 *******************************************************************************/

/**
 * Sends json formatted fresh measurements as a http response
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t send_current_measurements(httpd_req_t *req){
  measurement measurements;
  time_t now;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
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
#ifdef CONFIG_KK_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  httpd_resp_send(req, respond_buf, -1);  // Response body can be empty
  free(respond_buf);
  return ESP_OK;
}

/**
 * Sends json formatted up time as a http response
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t send_current_ms(httpd_req_t *req){
  int64_t us = 0;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "application/x-javascript");
#ifdef CONFIG_KK_HTTPD_CONN_CLOSE_HEADER
  httpd_resp_set_hdr(req, "Connection", "close");
#endif
  char * respond_buf = (char*)malloc(512);
  us = esp_timer_get_time();
  sprintf(respond_buf, "{\"value\":\"%lld\"}\n", us);
  httpd_resp_send(req, respond_buf, -1);  // Response body can be empty
  free(respond_buf);
  return ESP_OK;
}


/**
 * Sends confirmation and execute software reset
 *
 * @param req Request pointer
 * @return ESP_OK
 */
esp_err_t reset_send_confirmation(httpd_req_t *req){
  httpd_resp_set_status(req, "203 Reset Content");
  strcpy((char*)req->uri, "/reset.htm");    //change uri to match real file
  file_get_handler(req);            //send reset.htm page
  vTaskDelay(pdMS_TO_TICKS(3000));  //wait for response to be physically sent
  ESP_LOGW(TAG, "Performing system restart...");
  ///TODO: something sometimes blocks esp_reset function and http server stops here forever
  esp_restart();                    //perform software restart
  ESP_LOGW(TAG, "This message should never print.");
//  for(;;) vTaskDelay(pdMS_TO_TICKS(1000));  //wait forever
  return ESP_OK;
}

/*******************************************************************************
 *    Helper methods for http app
 *******************************************************************************/

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
  }else if (IS_FILE_EXT(filename, ".csv")){
    return httpd_resp_set_type(req, "application/CSV");
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


/**
 * Generating html numbered list of links to pictures for given path and date
 * @param path Path to directory for which list is created
 * @param date  Date that files must match
 * @param list_buf pointer to preallocated buffer for generated list
 */
void generate_html_list(char* path, char* date, char *list_buf) {
  DIR* dir = opendir(path);
  struct tm tm_file;
  struct tm tm_filter = {0,0,0,0,0,0,0,0,0};
  char time_str[32];
  int cx = 0;
  struct dirent* entry;
  struct stat file_stat;
  char file_path[FILEPATH_LEN_MAX];

  if (!dir) {
    ESP_LOGE(TAG, "Failed to open directory %s", path);
    return;
  }

  strptime(date, "%d/%m/%Y", &tm_filter);
  cx = sprintf(list_buf, "<ol id=\"pic_list\">Lista zdjęć z dnia %s:\n", date);
  if(cx > 0) { list_buf += cx; }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      char* file_ext = strrchr(entry->d_name, '.');
      if (file_ext && strcmp(file_ext, ".jpg") == 0) {

        #pragma GCC diagnostic push // use compiler specific pragmas to disable the "directive output may be truncated writing" error
        #pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(file_path, FILEPATH_LEN_MAX, "%s/%s", path, entry->d_name);
        #pragma GCC diagnostic pop

        if (stat(file_path, &file_stat) == 0) {
          localtime_r(&file_stat.st_mtime, &tm_file);
          if (tm_file.tm_year == tm_filter.tm_year && tm_file.tm_mon == tm_filter.tm_mon && tm_file.tm_mday == tm_filter.tm_mday) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_file);
            cx = sprintf(list_buf, "<li><a href=\"dcim/%s\">%s</a> - %s</li>\n", entry->d_name, entry->d_name, time_str);
            if(cx > 0) { list_buf += cx; }
          }
        } else {
          ESP_LOGE(TAG, "Failed to stat file %s", file_path);
        }
      }
    }
  }
  sprintf(list_buf, "</ol>\n");
  closedir(dir);
}
