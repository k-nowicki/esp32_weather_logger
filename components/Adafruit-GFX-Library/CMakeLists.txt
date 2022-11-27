set(srcs 
    "Adafruit_GFX.cpp"
    )

# "glcdfont.c" defined but not used font
idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES arduino)
    
project(Adafruit-GFX-Library)