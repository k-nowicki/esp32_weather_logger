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
