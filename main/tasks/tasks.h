/*
 * tasks.h
 *
 *  Created on: 5 gru 2022
 *      Author: Karol Nowicki
 */

#ifndef MAIN_TASKS_TASKS_H_
#define MAIN_TASKS_TASKS_H_

//App libs
#include "../setup.h"
#include "../app.h"

//task handlers
extern TaskHandle_t g_vRTCTaskHandle;
extern TaskHandle_t g_vDHT11TaskHandle;
extern TaskHandle_t g_vSensorsTaskHandle;
extern TaskHandle_t g_vDisplayTaskHandle;
extern TaskHandle_t g_vCameraTaskHandle;
extern TaskHandle_t g_vSDCSVLGTaskHandle;
extern TaskHandle_t g_vSDAVGLGTaskHandle;
extern TaskHandle_t g_vSDJSLGTaskHandle;
extern TaskHandle_t g_vStatsTaskHandle;

//Tasks declarations
void vSensorsTask(void*);
void vDHT11Task(void*);
void vRTCTask(void*);
void vDisplayTask(void*);
void vStatsTask(void*);
void vSDJSLGTask(void*);
void vSDCSVLGTask(void*);
void vSDAVGLGTask(void*);
void vCameraTask(void*);



#endif /* MAIN_TASKS_TASKS_H_ */
