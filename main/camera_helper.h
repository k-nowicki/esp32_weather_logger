/*
 * camera_helper.h
 *
 *  Created on: 22 lut 2023
 *      Author: Karol
 */

#ifndef MAIN_CAMERA_HELPER_H_
#define MAIN_CAMERA_HELPER_H_

#include "setup.h"
#include "app.h"
#include "sensor.h"


#if CONFIG_FRAMESIZE_VGA
  #define FRAMESIZE FRAMESIZE_VGA
  #define FRAMESIZE_STRING "640x480"
#elif CONFIG_FRAMESIZE_SVGA
  #define FRAMESIZE FRAMESIZE_SVGA
  #define FRAMESIZE_STRING "800x600"
#elif CONFIG_FRAMESIZE_XGA
  #define FRAMESIZE FRAMESIZE_XGA
  #define FRAMESIZE_STRING "1024x768"
#elif CONFIG_FRAMESIZE_HD
  #define FRAMESIZE FRAMESIZE_HD
  #define FRAMESIZE_STRING "1280x720"
#elif CONFIG_FRAMESIZE_SXGA
  #define FRAMESIZE FRAMESIZE_SXGA
  #define FRAMESIZE_STRING "1280x1024"
#elif CONFIG_FRAMESIZE_UXGA
  #define FRAMESIZE FRAMESIZE_UXGA
  #define FRAMESIZE_STRING "1600x1200"
#endif

//Camera configuration structure
extern camera_config_t camera_config;

/**
 * @param framesize
 * @return
 *
 */
esp_err_t init_camera(int framesize);

/**
 * @param FileName
 * @param pictureSize
 * @return
 */
esp_err_t camera_capture(char * FileName, size_t *pictureSize);


#endif /* MAIN_CAMERA_HELPER_H_ */
