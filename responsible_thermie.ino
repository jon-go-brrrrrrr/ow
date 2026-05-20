#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <TFT_eSPI.h>

#define TFT_WIDTH 320
#define TFT_HEIGHT 172

#define MLX_WIDTH 32
#define MLX_HEIGHT 24

// ESP32-C6 I2C pins
#define I2C_SDA 6
#define I2C_SCL 7

Adafruit_MLX90640 mlx;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

float frame[MLX_WIDTH * MLX_HEIGHT];

// Temperature range from current frame
float frameMinTemp = 20.0;
float frameMaxTemp = 32.0;

#define PALETTE_COUNT 10
uint16_t colorPalettes[PALETTE_COUNT][6] = {
  { TFT_BLUE, TFT_CYAN, TFT_GREEN, TFT_YELLOW, TFT_RED, TFT_MAGENTA },
  { TFT_BLACK, TFT_DARKGREY, TFT_LIGHTGREY, TFT_WHITE, TFT_ORANGE, TFT_PINK },
  { TFT_NAVY, TFT_OLIVE, TFT_DARKGREEN, TFT_DARKCYAN, TFT_MAROON, TFT_PURPLE },
  { TFT_BLUE, TFT_GREEN, TFT_DARKGREEN, TFT_ORANGE, TFT_MAROON, TFT_RED },
  { TFT_NAVY, TFT_DARKGREEN, TFT_GREEN, TFT_YELLOW, TFT_ORANGE, TFT_RED },
  { TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_YELLOW, TFT_GREEN, TFT_RED },
  { TFT_WHITE, TFT_ORANGE, TFT_RED, TFT_BLUE, TFT_GREEN, TFT_BLACK },
  { TFT_PURPLE, TFT_MAGENTA, TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN },
  { TFT_YELLOW, TFT_PINK, TFT_WHITE, TFT_BLUE, TFT_DARKCYAN, TFT_DARKGREEN },
  { TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA }
};
int paletteIndex = 0;

// ============= MLX90640 Functions =============
void MLXInit() {
  Wire.begin(I2C_SDA, I2C_SCL);  // SDA on GPIO 6, SCL on GPIO 7
  Wire.setClock(1000000);
  if (!mlx.begin()) {
    Serial.println("ERROR: Failed to initialize MLX90640!");
    while(1);
  }
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_16_HZ);
  Serial.println("MLX90640 initialized successfully");
}

uint16_t getColorForTemp(float val) {
  int colorIndex;
  
  // Avoid divide by zero if min==max
  float tempRange = frameMaxTemp - frameMinTemp;
  if (tempRange < 0.1) tempRange = 0.1;
  
  // Map temperature to color index
  if (val <= frameMinTemp) {
    colorIndex = 0;
  } else if (val >= frameMaxTemp) {
    colorIndex = 5;
  } else {
    colorIndex = (int)((val - frameMinTemp) / tempRange * 5);
    if (colorIndex > 5) colorIndex = 5;
  }
  
  return colorPalettes[paletteIndex][colorIndex];
}

// Safe bilinear interpolation with bounds checking
float interpolateTemp(int displayX, int displayY) {
  // Map display coordinates to sensor coordinates
  float sensorX = (displayX / 320.0) * (MLX_WIDTH - 1);
  float sensorY = (displayY / 150.0) * (MLX_HEIGHT - 1);

  int x0 = (int)sensorX;
  int y0 = (int)sensorY;
  int x1 = x0 + 1;
  int y1 = y0 + 1;
  
  // Clamp to array bounds
  x0 = constrain(x0, 0, MLX_WIDTH - 1);
  x1 = constrain(x1, 0, MLX_WIDTH - 1);
  y0 = constrain(y0, 0, MLX_HEIGHT - 1);
  y1 = constrain(y1, 0, MLX_HEIGHT - 1);
  
  // Fractional parts
  float fx = sensorX - (int)sensorX;
  float fy = sensorY - (int)sensorY;
  
  // Bilinear interpolation
  float f00 = frame[y0 * MLX_WIDTH + x0];
  float f10 = frame[y0 * MLX_WIDTH + x1];
  float f01 = frame[y1 * MLX_WIDTH + x0];
  float f11 = frame[y1 * MLX_WIDTH + x1];
  
  float f0 = f00 * (1 - fx) + f10 * fx;
  float f1 = f01 * (1 - fx) + f11 * fx;
  
  return f0 * (1 - fy) + f1 * fy;
}

void displayThermalImage() {
  if (!mlx.getFrame(frame)) {
    Serial.println("ERROR: Failed to read frame");
    MLXInit();
    return;
  }

  // Find actual min/max in frame
  frameMinTemp = 1000;
  frameMaxTemp = -1000;
  float tempCenter = 0.0;

  for (int i = 0; i < MLX_WIDTH * MLX_HEIGHT; i++) {
    if (frame[i] < frameMinTemp) {
      frameMinTemp = frame[i];
    }
    if (frame[i] > frameMaxTemp) {
      frameMaxTemp = frame[i];
    }
  }
  
  // Center pixel
  tempCenter = frame[(MLX_HEIGHT / 2) * MLX_WIDTH + (MLX_WIDTH / 2)];

  // Clear sprite
  sprite.fillSprite(TFT_BLACK);

  // Render thermal image with interpolation (320x150 for display area)
  for (int y = 0; y < 150; y++) {
    for (int x = 0; x < 320; x++) {
      float val = interpolateTemp(x, y);
      uint16_t color = getColorForTemp(val);
      sprite.drawPixel(x, y, color);
    }
  }

  // Display temperature overlay
  sprite.setTextFont(1);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);

  // Top info bar
  sprite.drawString("Range: " + String((int)frameMinTemp) + "-" + String((int)frameMaxTemp) + "C", 5, 152);

  // Bottom stats
  sprite.setTextSize(0);
  sprite.drawString("Min: " + String(frameMinTemp, 1) + "C", 10, 162);
  sprite.drawString("Ctr: " + String(tempCenter, 1) + "C", 130, 162);
  sprite.drawString("Max: " + String(frameMaxTemp, 1) + "C", 240, 162);

  // Crosshair at center
  sprite.drawLine(155, 70, 165, 70, TFT_WHITE);
  sprite.drawLine(160, 65, 160, 75, TFT_WHITE);

  sprite.pushSprite(0, 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Thermal Camera (ESP32-C6 + MLX90640) ===");
  Serial.println("Display: 320x172");
  Serial.println("I2C: SDA=GPIO6, SCL=GPIO7");
  Serial.println("Mode: Auto Temperature Range");
  Serial.println("============================================\n");
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  sprite.createSprite(320, 172);

  MLXInit();
  
  Serial.println("Setup complete - Starting thermal imaging...\n");
}

void loop() {
  displayThermalImage();
  delay(100);  // ~10 FPS refresh rate
}