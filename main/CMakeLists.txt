idf_component_register(SRCS "main.cpp" 
							"tasks/vRTCTask.cpp" 
							"tasks/vDHT11Task.cpp" 
							"tasks/vSensorsTask.cpp" 
							"tasks/vDisplayTask.cpp" 
							"tasks/stats_task.cpp"
							"tasks/vSDLOGTask.cpp"
							"app_global_helper.cpp"
                       INCLUDE_DIRS "." )
