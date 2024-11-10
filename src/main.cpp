#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

const byte MLX90640_address = 0x33;
#define TA_SHIFT 8

paramsMLX90640 mlx90640;
float mlx90640To[768];

float MaxTemp;
float MinTemp;
float CenterTemp;

TaskHandle_t getFrameArray;
TaskHandle_t Task1;

boolean isConnected(){
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); 
  return (true);
}

void getFrameArraycode(void* pvParameters){
	for(;;){
    for (byte x = 0; x < 2; x++){
      uint16_t mlx90640Frame[834];
      int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
      float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
      float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
      float tr = Ta - TA_SHIFT;
      float emissivity = 0.95;
      MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
      
      CenterTemp = (mlx90640To[367]+mlx90640To[368]+mlx90640To[399]+mlx90640To[400]) / 4.0;
      MaxTemp = mlx90640To[0];
      MinTemp = mlx90640To[0];
  		
      for (int x = 0; x < 768; x++){
        if (mlx90640To[x] > MaxTemp){
          MaxTemp = mlx90640To[x];
        }
        if (mlx90640To[x] < MinTemp){
          MinTemp = mlx90640To[x];
        }
  		
      }
  	}
    Serial.print("Min: ");
    Serial.print(MinTemp);
    Serial.print(" C, ");
    Serial.print("Max: ");
    Serial.print(MaxTemp);
    Serial.print(" C, ");
    Serial.print("Center: ");
    Serial.print(CenterTemp);
    Serial.println(" C");
  }
}


	/*
  Serial.print("getFrameArray running on core ");
	Serial.println(xPortGetCoreID());
	
  for (;;) {
    Serial.println("Core 0 processing");
    delay((int)random(100, 1000));
  }
	*/


void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    Serial.println("Core 1 processing");
    delay((int)random(100, 1000));
  }
}

void setup() {
	Wire.begin();
	Wire.setClock(400000);

	Serial.begin(115200);

	if (isConnected() == false){
		Serial.println("MLX90640 not at 0x33");
		while (1);
	}

	int status;
	uint16_t eeMLX90640[832];
	status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
	if (status != 0)
		Serial.println("Failed to load system params.");
	status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
	if (status != 0)
		Serial.println("Parameter extract failed");
	MLX90640_SetRefreshRate(MLX90640_address, 0x06);
	Wire.setClock(8000000);
  //create a task that executes the getTask0code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(getFrameArraycode, "getFrameArray", 10000, NULL, 1, &getFrameArray, 0);
  //create a task that executes the getTask1code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 1);
}

void loop() {
  // nothing to do here, everything happens in the Task1Code and Task2Code functions
}


