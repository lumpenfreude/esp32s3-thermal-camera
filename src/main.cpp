#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_I2CDevice.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>             // Arduino SPI library
#include "semphr.h"

#define TFT_CS   17  // Chip select control pin
#define TFT_DC   10  // Data Command control pin
#define TFT_RST   3  // Reset pin (could connect to RST pin)
 
// Initialize Adafruit ST7789 TFT library
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Address of MLX90640 
const byte MLX90640_address = 0x33;
// TA Shift of MLX90640 at room temp or whatever
#define TA_SHIFT 8

paramsMLX90640 mlx90640;

// info we want to grab from mlx90640
float mlx90640To[768];
float MaxTemp;
float MinTemp;
float CenterTemp;

int R_colour, G_colour, B_colour;              

//task and semaphore create
TaskHandle_t Task0;
TaskHandle_t Task1;
SemaphoreHandle_t xMutex;

// create colour gradients (i ganked this from the instructables thing i link
// in the readme, i'm not normally the type to spell color with a u
void getColour(int j){
  if (j >= 0 && j < 30){
    R_colour = 0;
    G_colour = 0;
    B_colour = 20 + (120.0/30.0) * j;
  } 
  if (j >= 30 && j < 60){
    R_colour = (120.0 / 30) * (j - 30.0);
    G_colour = 0;
    B_colour = 140 - (60.0/30.0) * (j - 30.0);
  } 
  if (j >= 60 && j < 90){
    R_colour = 120 + (135.0/30.0) * (j - 60.0);
    G_colour = 0;
    B_colour = 80 - (70.0/30.0) * (j - 60.0);
  }
  if (j >= 90 && j < 120){
    R_colour = 255;
    G_colour = 0 + (60.0/30.0) * (j - 90.0);
    B_colour = 10 - (10.0/30.0) * (j - 90.0);
  }
  if (j >= 120 && j < 150){
    R_colour = 255;
    G_colour = 60 + (175.0/30.0) * (j - 120.0);
    B_colour = 0;     
  }
  if (j >= 150 && j <= 180){
    R_colour = 255;
    G_colour = 235 + (20.0/30.0) * (j - 150.0);
    B_colour = 0 + 255.0/30.0 * (j - 150.0);
  }
}

boolean isConnected(){
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); 
  return (true);
}

// code of our task to run on core0 (hence the name task0)
void Task0code(void* pvParameters){
  for(;;){
    if(xSemaphoreTake(xMutex, (TickType_t) 0xFFFFFFFF) == 1){
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
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(10);
  }
}

// code of our task to run on core 1 (hence the name task1)
void Task1code(void* pvParameters) {
  for (;;) {
    float localTo[768];
    if(xSemaphoreTake(xMutex, (TickType_t) 0xFFFFFFFF) == 1){
      memcpy(localTo, mlx90640To, sizeof(mlx90640To));
      xSemaphoreGive(xMutex);
    }
    uint8_t w,h;
    uint8_t box = 10;
    tft.setAddrWindow(0, 0, 0, 0);
    for (h = 0; h < 24; h++) {
      for (w = 0; w < 32; w++) {
        localTo[h*32 + w] = 180.0 * (localTo[h*32 + w] - MinTemp) / (MaxTemp - MinTemp);
        getColour(localTo[h*32 + w]);
        tft.fillRect(box * w, box * h, box, box, tft.color565(R_colour, G_colour, B_colour));    
      } 
    }
    tft.endWrite();
  }
}


void setup() {
  Wire.begin();
  Wire.setClock(400000); // was having trouble with wire set high freq this early
  tft.init(240, 320, SPI_MODE2);    // Init ST7789 display 240x320 pixel
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
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
  Wire.setClock(1000000);
  //create our mutex
  xMutex = xSemaphoreCreateMutex();
  //create a task that executes the Task0code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task0code, "Task0", 10000, NULL, 1, &Task0, 0);
  //create a task that executes the Task0code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 1);
}

void loop() {
  // nothing to do here, everything happens in the Task1Code and Task2Code functions
}


