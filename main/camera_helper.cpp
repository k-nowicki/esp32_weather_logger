/*
 * camera_helper.cpp
 *
 *  Created on: 22 lut 2023
 *      Author: Karol
 */

#include "setup.h"
#include "camera_helper.h"

static const char *TAG = "CAMHLP";

//Board config for camera
camera_config_t camera_config = {
  .pin_pwdn = CAM_PIN_PWDN,
  .pin_reset = CAM_PIN_RESET,
  .pin_xclk = CAM_PIN_XCLK,
  .pin_sscb_sda = CAM_PIN_SIOD,
  .pin_sscb_scl = CAM_PIN_SIOC,

  .pin_d7 = CAM_PIN_D7,
  .pin_d6 = CAM_PIN_D6,
  .pin_d5 = CAM_PIN_D5,
  .pin_d4 = CAM_PIN_D4,
  .pin_d3 = CAM_PIN_D3,
  .pin_d2 = CAM_PIN_D2,
  .pin_d1 = CAM_PIN_D1,
  .pin_d0 = CAM_PIN_D0,
  .pin_vsync = CAM_PIN_VSYNC,
  .pin_href = CAM_PIN_HREF,
  .pin_pclk = CAM_PIN_PCLK,

  //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_SXGA, //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

  .jpeg_quality = 8, //0-63 lower number means higher quality
  .fb_count = 3, //if more than one, i2s runs in continuous mode. Use only with JPEG

  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_LATEST //CAMERA_GRAB_WHEN_EMPTY //
};

esp_err_t init_camera(int framesize){
  ESP_LOGI(TAG, "Start init_camera");
  ESP_LOGI(TAG, "grab_mode=%d", camera_config.grab_mode);
  ESP_LOGI(TAG, "fb_location=%d", camera_config.fb_location);
  //initialize the camera
  camera_config.frame_size = (framesize_t)framesize;
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK){
    ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }

  ESP_LOGI(TAG, "Finish init_camera");
  return ESP_OK;
}

esp_err_t camera_capture(char * FileName, size_t *pictureSize){
  FILE* f;

  //clear internal queue  - Not sure what purpose it has, but it basically takes one more photo for nothing
//  camera_fb_t * fb = esp_camera_fb_get();
//  if(fb){
//      //ESP_LOGI(TAG, "fb->len=%d", fb->len);
//      esp_camera_fb_return(fb);
//  }

  //acquire a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera Capture Failed");
    return ESP_FAIL;
  }
  //replace this with your own function
  //process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
  f = fopen(FileName, "wb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    ESP_LOGE(TAG, "PATH: %s", FileName);
    return ESP_FAIL;
  }
  fwrite(fb->buf, fb->len, 1, f);
  ESP_LOGI(TAG, "fb->len=%d", fb->len);
  *pictureSize = (size_t)fb->len;
  fclose(f);

  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  return ESP_OK;
}
