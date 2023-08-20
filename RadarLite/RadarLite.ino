#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "bitmap.h"
#include <HardwareSerial.h>

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_CLK 18
#define TFT_CS 14
#define TFT_DC 27
#define TFT_RST 33

//idk why the colours are flipped
#define RED 0x001F

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

int localPlayerX = 0;
int localPlayerY = 0;
int enemyPlayers[32][2];
int numEnemyPlayers = 0;
int previousEnemyNumber = -1;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(2);

  tft.fillScreen(ILI9341_BLACK);

  displayMap(map_data, map_width, map_height, 0, 0);
}

void loop() {
  if (!Serial.available()) {
    //makes a call to the pc program that is listening for a # 
    Serial.write('#');
  }else {
    // "/" is the end of the 
    String data = Serial.readStringUntil('/');
    int startIndex = 0;
    while (startIndex < data.length()) {
      int endIndex = data.indexOf(';', startIndex);
      if (endIndex == -1) {
        break;
      }

      String segment = data.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
      // Parse and process the received data
      if (segment.startsWith("L")) {//L for local player 
        // Parse local player data (data format: "L,x,y")
        data.remove(0, 2);  // Remove "L," prefix
        int commaIndex = data.indexOf(',');

        clearPlayer(localPlayerX, localPlayerY);

        localPlayerX = data.substring(0, commaIndex).toInt();
        localPlayerY = data.substring(commaIndex + 1).toInt();

        // Display local player position
        displayPlayer(localPlayerX, localPlayerY, ILI9341_GREEN);
      }
      if (segment.startsWith("E")) {
        segment.remove(0, 1);
        int commaIndex1 = segment.indexOf(',');
        int enemyIndex = segment.substring(0, commaIndex1).toInt();
        segment.remove(0, commaIndex1 + 1);
        int commaIndex2 = segment.indexOf(',');

        if (enemyIndex < previousEnemyNumber) {
          // Clear all stored enemy player positions only way i could get all to be displayed at one time and cleared 
          for (int i = 0; i < numEnemyPlayers; i++) {
            clearPlayer(enemyPlayers[i][0], enemyPlayers[i][1]);
          }
          numEnemyPlayers = 1;  // Reset the number of enemy players Enemy starts at one dummy the map is 0 
        }

        previousEnemyNumber = enemyIndex;

        // Store enemy player position
        enemyPlayers[numEnemyPlayers][0] = segment.substring(0, commaIndex2).toInt();
        enemyPlayers[numEnemyPlayers][1] = segment.substring(commaIndex2 + 1).toInt();

        displayPlayer(enemyPlayers[numEnemyPlayers][0], enemyPlayers[numEnemyPlayers][1], RED);
        numEnemyPlayers++;
      }
    }
  }
}

void displayMap(const uint16_t *image_data, int width, int height, int x, int y) {
  for (int row = height - 1; row >= 0; row--) {
    for (int col = 0; col < width; col++) {
      uint16_t color565 = image_data[row * width + col];
      tft.drawPixel(x + col, y + row, color565);
    }
  }
}

void displayPlayer(int x, int y, uint16_t color) {
  //vertical flipping cause I wired the display in upsidedown 
  int flippedY = map_height - y;

  if ((x < 0 || x > map_width) || (flippedY < 0 || flippedY > map_height)) {
    return;
  }

  tft.fillRect(x, flippedY, 4, 4, color);
}

void clearPlayer(int x, int y) {
  int flippedY = map_height - y;

  tft.fillRect(x, flippedY, 4, 4, ILI9341_BLACK);
}
