#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_I2CDevice.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>             // Arduino SPI library

#define TFT_CS   17  // Chip select control pin
#define TFT_DC   10  // Data Command control pin
#define TFT_RST   3  // Reset pin (could connect to RST pin)

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define PART_BOUNDARY "123456789000000000000987654321"

//the colors we will be using
const uint16_t camColors[] = {0x480F,
                              0x400F, 0x400F, 0x400F, 0x4010, 0x3810, 0x3810, 0x3810, 0x3810, 0x3010, 0x3010,
                              0x3010, 0x2810, 0x2810, 0x2810, 0x2810, 0x2010, 0x2010, 0x2010, 0x1810, 0x1810,
                              0x1811, 0x1811, 0x1011, 0x1011, 0x1011, 0x0811, 0x0811, 0x0811, 0x0011, 0x0011,
                              0x0011, 0x0011, 0x0011, 0x0031, 0x0031, 0x0051, 0x0072, 0x0072, 0x0092, 0x00B2,
                              0x00B2, 0x00D2, 0x00F2, 0x00F2, 0x0112, 0x0132, 0x0152, 0x0152, 0x0172, 0x0192,
                              0x0192, 0x01B2, 0x01D2, 0x01F3, 0x01F3, 0x0213, 0x0233, 0x0253, 0x0253, 0x0273,
                              0x0293, 0x02B3, 0x02D3, 0x02D3, 0x02F3, 0x0313, 0x0333, 0x0333, 0x0353, 0x0373,
                              0x0394, 0x03B4, 0x03D4, 0x03D4, 0x03F4, 0x0414, 0x0434, 0x0454, 0x0474, 0x0474,
                              0x0494, 0x04B4, 0x04D4, 0x04F4, 0x0514, 0x0534, 0x0534, 0x0554, 0x0554, 0x0574,
                              0x0574, 0x0573, 0x0573, 0x0573, 0x0572, 0x0572, 0x0572, 0x0571, 0x0591, 0x0591,
                              0x0590, 0x0590, 0x058F, 0x058F, 0x058F, 0x058E, 0x05AE, 0x05AE, 0x05AD, 0x05AD,
                              0x05AD, 0x05AC, 0x05AC, 0x05AB, 0x05CB, 0x05CB, 0x05CA, 0x05CA, 0x05CA, 0x05C9,
                              0x05C9, 0x05C8, 0x05E8, 0x05E8, 0x05E7, 0x05E7, 0x05E6, 0x05E6, 0x05E6, 0x05E5,
                              0x05E5, 0x0604, 0x0604, 0x0604, 0x0603, 0x0603, 0x0602, 0x0602, 0x0601, 0x0621,
                              0x0621, 0x0620, 0x0620, 0x0620, 0x0620, 0x0E20, 0x0E20, 0x0E40, 0x1640, 0x1640,
                              0x1E40, 0x1E40, 0x2640, 0x2640, 0x2E40, 0x2E60, 0x3660, 0x3660, 0x3E60, 0x3E60,
                              0x3E60, 0x4660, 0x4660, 0x4E60, 0x4E80, 0x5680, 0x5680, 0x5E80, 0x5E80, 0x6680,
                              0x6680, 0x6E80, 0x6EA0, 0x76A0, 0x76A0, 0x7EA0, 0x7EA0, 0x86A0, 0x86A0, 0x8EA0,
                              0x8EC0, 0x96C0, 0x96C0, 0x9EC0, 0x9EC0, 0xA6C0, 0xAEC0, 0xAEC0, 0xB6E0, 0xB6E0,
                              0xBEE0, 0xBEE0, 0xC6E0, 0xC6E0, 0xCEE0, 0xCEE0, 0xD6E0, 0xD700, 0xDF00, 0xDEE0,
                              0xDEC0, 0xDEA0, 0xDE80, 0xDE80, 0xE660, 0xE640, 0xE620, 0xE600, 0xE5E0, 0xE5C0,
                              0xE5A0, 0xE580, 0xE560, 0xE540, 0xE520, 0xE500, 0xE4E0, 0xE4C0, 0xE4A0, 0xE480,
                              0xE460, 0xEC40, 0xEC20, 0xEC00, 0xEBE0, 0xEBC0, 0xEBA0, 0xEB80, 0xEB60, 0xEB40,
                              0xEB20, 0xEB00, 0xEAE0, 0xEAC0, 0xEAA0, 0xEA80, 0xEA60, 0xEA40, 0xF220, 0xF200,
                              0xF1E0, 0xF1C0, 0xF1A0, 0xF180, 0xF160, 0xF140, 0xF100, 0xF0E0, 0xF0C0, 0xF0A0,
                              0xF080, 0xF060, 0xF040, 0xF020, 0xF800,
                             };
float p = 3.1415926;
// Initialize Adafruit ST7789 TFT library
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
 
const byte MLX90640_address = 0x33;
#define TA_SHIFT 8

paramsMLX90640 mlx90640;
float mlx90640To[768];
float MaxTemp;
float MinTemp;
float CenterTemp;

const byte calcStart = 20;

TaskHandle_t Task0;
TaskHandle_t Task1;
SemaphoreHandle_t mlx_mutex = NULL;


boolean isConnected(){
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); 
  return (true);
}

void Task0code(void* pvParameters){
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
	}
}

void Task1code(void* pvParameters) {
  for (;;) {
    uint8_t w,h;
    uint8_t box = 10;
    tft.setAddrWindow(0, 0, 0, 0);

    for (h = 0; h < 24; h++) {
      for (w = 0; w < 32; w++) {
        uint8_t colorIndex = map(mlx90640To[w+(32*h)], MinTemp-5.0, MaxTemp+5.0, 0, 255);
        colorIndex = constrain(colorIndex, 0, 255);

        tft.fillRect(box * w, box * h, box, box, camColors[colorIndex]);
        //display.writePixel(w, h, camColors[colorIndex]);
      } 
    }
    tft.endWrite();
  }
}


void setup() {
	Wire.begin();
	Wire.setClock(400000);
  tft.init(240, 320, SPI_MODE2);    // Init ST7789 display 135x240 pixel
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
  //create a task that executes the Task0code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task0code, "Task0", 10000, NULL, 1, &Task0, 0);
  //create a task that executes the Task0code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 1);
}

void loop() {
  // nothing to do here, everything happens in the Task1Code and Task2Code functions
}


