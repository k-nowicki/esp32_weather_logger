/* KK Weather Station
 *
 * Platform: ESP32 (Tested on ESP32-CAM Development Board)
 * See project documentation for more detailed description.
 *
 *  Copyright (c) <2022> <Karol Nowicki>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
*/

//System
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
//Peripherals and libs
#include "driver/gpio.h"
#include <Wire.h>
#include <k_math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Fonts/FreeSans9pt7b.h>
#include <dht11.h>
#include <BH1750.h>
//App
#include "setup.h"
#include "app.h"


extern "C" {
	void app_main(void);
}



/*******************************************************************************
 * Variable definitions
 */



//Sensor global objects
BH1750 lightMeter(BH1750_ADDR);
Adafruit_BMP280 pressureMeter(&Wire); // I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



/*******************************************************************************
 *  App Main
 */
void app_main(void){
	//create semaphores
	current_measuers_mutex = xSemaphoreCreateMutex();
	uart_mutex = xSemaphoreCreateMutex();

	initArduino();
    //Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(10));

    //Set I2C interface
	Wire.begin(I2C_SDA, I2C_SCL);
	vTaskDelay(pdMS_TO_TICKS(100));
	printf("search_i2c...");

    search_i2c(); // search for I2C devices (helpful if any uncertainty about sensor addresses raised)
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("GPIO's configuration...\n");
	//Set GPIOS for DHT11 and DS18B20 as GPIOs
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DHT11], PIN_FUNC_GPIO);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_DS18B20], PIN_FUNC_GPIO);
    //Set up DHT11 GPIO
    DHT11_init(GPIO_DHT11);
    printf("GPIO's configured!\n");

    //Setup OLED display
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
	  display.display();
      printf("SSD1306 allocation failed");
      for(;;); // Don't proceed, loop forever
    }
    //BH1750 Initialization
    if(!lightMeter.begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR, &Wire)){
    	printf("BH1750 initialization failed!");
    	for(;;); // Don't proceed, loop forever
    }
    //BMP280 Initialization
    if(!pressureMeter.begin(BMP280_ADDR)){
    	printf("BMP280 initialization failed!");
    	for(;;); // Don't proceed, loop forever
    }

    //Create business tasks
    xTaskCreatePinnedToCore( vDHT11Task, "DHT11", 1024, NULL, SENSORS_TASK_PRIO, NULL, tskNO_AFFINITY );
    xTaskCreatePinnedToCore( vSensorsTask, "SENS", 2048, NULL, SENSORS_TASK_PRIO, NULL, tskNO_AFFINITY );
	xTaskCreatePinnedToCore( vDisplayTask, "OLED", 2048, NULL, DISPLAY_TASK_PRIO, NULL, tskNO_AFFINITY );

    //Create and start stats task
    xTaskCreatePinnedToCore(stats_task, "STATS", 2048, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);
}


/*******************************************************************************
 *	Application Tasks
 *
 */


/**
 * @brief Task responsible for reading all sensors
 *
 * @param arg
 */
static void vSensorsTask(void*){
	measurement tmp_measurements;
	int dht_status = 0;
    while (1) {
    	// Read all sensors
    	tmp_measurements.lux = lightMeter.readLightLevel();
    	tmp_measurements.iTemp = pressureMeter.readTemperature();
    	tmp_measurements.pres = pressureMeter.readPressure()/100;
    	tmp_measurements.alti = pressureMeter.readAltitude(1013.25);
		dht_status = DHT11_read().status;
		//Store them in global curr_measures (but without dht11 measures)
		store_measurements(tmp_measurements);
    	//print on UART
		xSemaphoreTake(uart_mutex, portMAX_DELAY);

		printf("DHT Temperature is %d °C\n", (int)tmp_measurements.eTemp);
		printf("DHT Humidity is %d %%\n", (int)tmp_measurements.humi);
		printf("DHT Status code is %d\n", dht_status);
		printf("BH1 Light exposure is %6.2F Lux\n", tmp_measurements.lux);
		printf("BMP Barometer Temperature is %3.2F °C\n", tmp_measurements.iTemp);
		printf("BMP Atmospheric pressure is %4.2f hPa\n", tmp_measurements.pres);
		printf("BMP Altitude is %5.2F m\n", tmp_measurements.alti);
		printf("=========================================\n\n");
		xSemaphoreGive(uart_mutex);
		vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Task responsible for reading all sensors
 *
 * @param arg
 *
 * Task needed, because DHT11 needs some times more than 2seconds to take measurements
 * When this was
 */
static void vDHT11Task(void*){
	measurement tmp_measurements;
	int dht_status = DHT11_TIMEOUT_ERROR;
    while (1) {
    	// Read
    	dht_status = DHT11_read().status;
    	if(dht_status==DHT11_OK){
    		//make the reads from DHT11
    		tmp_measurements.eTemp = DHT11_read().temperature;
    		tmp_measurements.humi = DHT11_read().humidity;
    		//store them in curr)measures
    		xSemaphoreTake(current_measuers_mutex, portMAX_DELAY);
    		curr_measures.eTemp = tmp_measurements.eTemp;
			curr_measures.humi = tmp_measurements.humi;
    		xSemaphoreGive(current_measuers_mutex);
    	}
		vTaskDelay(pdMS_TO_TICKS(400));
    }
}

/**
 * @brief Task responsible for OLED display UI
 *
 * @param arg
 */
static void vDisplayTask(void *arg){
	measurement tmp_measurements;
	display.display();
	vTaskDelay(pdMS_TO_TICKS(200));
	display.clearDisplay();
	display.setTextSize(1);      // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 10);     // Start at top-left corner
	display.cp437(true);         // Use full 256 char 'Code Page 437' font
	display.setFont(&FreeSans9pt7b);

	display.println("Weather Station V 1.0");
	display.setFont();
	display.println("by KNowicki @ 2022");
	display.display();
	vTaskDelay(pdMS_TO_TICKS(1000));
	while(1){
		vTaskDelay(pdMS_TO_TICKS(200));
		display.clearDisplay();
		display.setCursor(0, 0);     // Start at top-left corner
		tmp_measurements = get_latest_measurements();	//safely read current values
		display.printf("Intern T: %3.2F %cC\n", tmp_measurements.iTemp,'\xF8');
		display.printf("Extern T: %3.2F %cC\n", tmp_measurements.eTemp,'\xF8');
		display.printf("Humidity: %d%%\n", (int)tmp_measurements.humi);
		display.printf("Sun expo: %6.2FLux\n", tmp_measurements.lux);
		display.printf("Pressure: %4.2f hPa\n", tmp_measurements.pres);
		display.printf("Altitude: %5.2Fm\n", tmp_measurements.alti);

		display.display();
	}
}


/**
 * @brief Task responsible for communicating System Real Time Statistics at UART Debug port
 *
 * @param arg
 */
static void stats_task(void *arg){
    //Print real time stats periodically
    while (1) {
    	xSemaphoreTake(uart_mutex, portMAX_DELAY);
    	printf("\n\n=========================================\n");
    	printf("Getting real time stats over %d ticks\n", STATS_TICKS);
        if (print_real_time_stats(STATS_TICKS) == ESP_OK) {
            printf("Real time stats obtained\n");
        } else {
            printf("Error getting real time stats\n");
        }
        printf("-----------------------------------------\n\n");
        xSemaphoreGive(uart_mutex);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/*******************************************************************************
 *	Application helper functions
 *
 */

/**
 * curr_measures are global variable used in many tasks. That is why it needs to
 * be protected against changing value in the middle of writing/reading.
 * @return Copy of curr_measures done under mutex control.
 */
measurement get_latest_measurements(void){
	measurement last_measures;
	xSemaphoreTake(current_measuers_mutex, portMAX_DELAY);
	last_measures = curr_measures;	//safely read current values
	xSemaphoreGive(current_measuers_mutex);
	return last_measures;
}
/**
 * Save given measurements to global curr_measures variable
 * @param measures Measurements to store
 */
void store_measurements(measurement measures){
	xSemaphoreTake(current_measuers_mutex, portMAX_DELAY);
	curr_measures.lux = measures.lux;
	curr_measures.iTemp = measures.iTemp;
	curr_measures.pres = measures.pres;
	curr_measures.alti = measures.alti;
	xSemaphoreGive(current_measuers_mutex);
}

/**
 * @brief   Function to print the CPU usage of tasks over a given duration.
 *
 * This function will measure and print the CPU usage of tasks over a specified
 * number of ticks (i.e. real time stats). This is implemented by simply calling
 * uxTaskGetSystemState() twice separated by a delay, then calculating the
 * differences of task run times before and after the delay.
 *
 * @note    If any tasks are added or removed during the delay, the stats of
 *          those tasks will not be printed.
 * @note    This function should be called from a high priority task to minimize
 *          inaccuracies with delays.
 * @note    When running in dual core mode, each core will correspond to 50% of
 *          the run time.
 *
 * @param   xTicksToWait    Period of stats measurement
 *
 * @return
 *  - ESP_OK                Success
 *  - ESP_ERR_NO_MEM        Insufficient memory to allocated internal arrays
 *  - ESP_ERR_INVALID_SIZE  Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET
 *  - ESP_ERR_INVALID_STATE Delay duration too short
 */
static esp_err_t print_real_time_stats(TickType_t xTicksToWait){
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time, total_elapsed_time;
    esp_err_t ret;

    //Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    vTaskDelay(xTicksToWait);

    //Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = (TaskStatus_t*) malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto exit;
    }
    //Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto exit;
    }

    //Calculate total_elapsed_time in units of run time stats clock period.
    total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ret = ESP_ERR_INVALID_STATE;
        goto exit;
    }

    printf("| Task | Run Time | Percentage\n");
    //Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        int k = -1;
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {
                k = j;
                //Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
        //Check if matching task found
        if (k >= 0) {
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            printf("| %s | %d | %d%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    //Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            printf("| %s | Deleted\n", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            printf("| %s | Created\n", end_array[i].pcTaskName);
        }
    }
    ret = ESP_OK;

exit:    //Common return path
    free(start_array);
    free(end_array);
    return ret;
}



/***********************************************
 * I2C search
 *
 */
void search_i2c(void){
	uint8_t error, address;
	int nDevices;
	printf("Scanning...\n");
	nDevices = 0;
	for (address = 1; address < 127; address++) {
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0) {
			printf(" -I2C device found at address 0x");
			if (address < 16)
				printf("0");
//			printf(address, HEX);
			printf("%02x\n", address);
			nDevices++;
		}
		else if (error == 4) {
			printf(" -Unknown error at address 0x");
			if (address < 16)
				printf("0");
			printf((const char *)&address, HEX);
		}
	}
	if (nDevices == 0)
		printf(" -No I2C devices found\n");
	else
		printf("Done I2C scanning!\n");
	vTaskDelay(pdMS_TO_TICKS(2000)); // wait 2 seconds
}



