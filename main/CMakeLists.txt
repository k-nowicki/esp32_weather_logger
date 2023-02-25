idf_component_register(SRCS "main.cpp" 
							"tasks/vRTCTask.cpp" 
							"tasks/vDHT11Task.cpp" 
							"tasks/vSensorsTask.cpp" 
							"tasks/vDisplayTask.cpp" 
							"tasks/vStatsTask.cpp"
							"tasks/vSDJSLGTask.cpp"
							"tasks/vSDCSVLGTask.cpp"
							"tasks/vSDAVGLGTask.cpp"
							"tasks/vCameraTask.cpp"
							"app_global_helper.cpp"
							"camera_helper.cpp"
							"kk_http_app/src/kk_http_app.cpp"
							"kk_http_app/src/kk_http_server_setup.cpp"
                       INCLUDE_DIRS "." 
                       EMBED_TXTFILES "kk_http_app/certs/cacert.pem" "kk_http_app/certs/prvtkey.pem")

                       