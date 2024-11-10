# multicore ESP32S3 thermal camera using MLX90640 
trying a thing here with FreeRTOS multi-core processing and the MLX90640
thermal camera sensor module thinger to see if i can eke more than 8 frames per
second out of it. 

## thermal camera code largely adapted from: 

- https://github.com/embedded-kiddie/Arduino-XIAO-ESP32/tree/main/MLX90640
- https://github.com/Samox1/ESP_Thermal_Camera_WebServer/blob/master/ESP_ThermalCamera_WebServer/ESP_ThermalCamera_WebServer.ino
- https://github.com/weinand/thermal-imaging-camera/blob/main/main.cpp#L277


## this tutorial is was an incredible help for learning the FreeRTOS multitasking
stuff: 
- https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
## and this one was absolutely instrumental in learning about the queue and stuff. 
- https://github.com/eduautomatiza/esp32-taskNotify
