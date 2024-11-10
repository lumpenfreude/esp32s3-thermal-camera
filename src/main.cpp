#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "driver/gpio.h"

const byte MLX90640_address = 0x33;
#define TA_SHIFT 8

paramsMLX90640 mlx90640;
float mlx90640To[768];

float MaxTemp;
float MinTemp;
float CenterTemp;

TaskHandle_t getFrameTask;
TaskHandle_t serverTask;
TaskHandle_t updateImageTask;
QueueHandle_t queue;

boolean isConnected(){
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); 
  return (true);
}

void getFrameTaskcode(void* pvParameters){
  float mlxTx[768];
  queue = xQueueCreate(5, sizeof(mlxTx));
  for(;;){
    for (byte x = 0; x < 2; x++){
      uint16_t mlx90640Frame[834];
      int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
      float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
      float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
      float tr = Ta - TA_SHIFT;
      float emissivity = 0.95;
      MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
    }
    if (queue == 0){
      Serial.print("Failed to create queue:");
    }
    memcpy(mlxTx, mlx90640To, sizeof(mlx90640To));
    xQueueSend(queue, (void*)mlxTx, (TickType_t)0);
  }
}

void updateImageTaskcode(void* pvParameters) {
  float mlxRx[768];
  for (;;) {
    if(xQueueReceive(queue, &(mlxRx), (TickType_t)5)){
      CenterTemp = (mlxRx[367]+mlxRx[368]+mlxRx[399]+mlxRx[400]) / 4.0;
      MaxTemp = mlxRx[0];
      MinTemp = mlxRx[0];
      for (int x = 0; x < 768; x++){
        if (mlxRx[x] > MaxTemp){
          MaxTemp = mlxRx[x];
        }
        if (mlxRx[x] < MinTemp){
          MinTemp = mlxRx[x];
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
    }else{
      Serial.print(".");
    }
  }
}

void serverTaskcode(void* pvParameters) {
  for (;;) {
    Serial.println("the server is 'running'");
    delay((int)random(100, 1000));
  }
}

void setup() {
  Serial.begin(115200);
  
  Wire.begin();
	Wire.setClock(400000);
  
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
  
  xTaskCreatePinnedToCore(getFrameTaskcode, "get frames from MLX90640", 10000, NULL, 1, &getFrameTask, 0);
  
  xTaskCreatePinnedToCore(serverTaskcode, "run the server", 10000, NULL, 1, &serverTask, 1);
  
  xTaskCreatePinnedToCore(updateImageTaskcode, "update image", 10000, NULL, 1, &updateImageTask, 1);
}

void loop() {
  // nothing to do here, everything happens in the serverTaskCode and Task2Code functions
}


