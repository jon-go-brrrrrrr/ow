#include <Adafruit_MLX90640.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Wire.h>
#include <SPI.h>

// MLX90640 thermal camera
Adafruit_MLX90640 mlx;
float frame[32*24];
float prevFrame[32*24];

// Display setup (ST7789 172x320 for ESP32-C6)
#define SCREEN_WIDTH 172
#define SCREEN_HEIGHT 320
#define DISPLAY_X 2
#define DISPLAY_Y 35
#define THERMAL_SCALE 5  // Each thermal pixel = 5x5 display pixels (32*5=160 width, 24*5=120 height)

// I2C pins for ESP32-C6 (MLX90640)
#define SDA_PIN 1
#define SCL_PIN 4

// ST7789 SPI pins for ESP32-C6
#define TFT_CS 14      // LCD_CS
#define TFT_DC 15      // LCD_DC
#define TFT_RST 21     // LCD_RST
#define TFT_MOSI 6     // GPIO6
#define TFT_SCLK 7     // GPIO7
#define TFT_BL 22      // LCD_BL (backlight)

// Create display instance
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

bool hasPrevFrame = false;
uint32_t frameCount = 0;
uint32_t lastHeaderUpdate = 0;

// Backlight control
void setupBacklight() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // Turn on backlight
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  delay(100);
  
  // Initialize backlight
  setupBacklight();

  // Initialize I2C for MLX90640 (pins 1, 4)
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize ST7789 display
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  display.init(172, 320);
  display.setRotation(1);  // Landscape
  display.fillScreen(ST77XX_BLACK);
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10, 10);
  display.println("Init MLX90640...");
  
  // Initialize MLX90640
  Serial.println("Adafruit MLX90640 ST7789 Display (ESP32-C6)");
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("MLX90640 not found!");
    display.fillScreen(ST77XX_BLACK);
    display.setTextColor(ST77XX_RED);
    display.setCursor(10, 10);
    display.println("MLX90640 ERROR");
    while (1) delay(10);
  }
  
  Serial.println("Found Adafruit MLX90640");
  display.fillScreen(ST77XX_BLACK);
  display.setTextColor(ST77XX_GREEN);
  display.setCursor(10, 10);
  display.println("MLX90640 Found!");
  delay(1000);

  Serial.print("Serial number: ");
  Serial.print(mlx.serialNumber[0], HEX);
  Serial.print(mlx.serialNumber[1], HEX);
  Serial.println(mlx.serialNumber[2], HEX);

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_4_HZ);
  
  Serial.println("MLX90640 initialized successfully");
  display.fillScreen(ST77XX_BLACK);
  delay(1000);
}

void loop() {
  delay(500);
  
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Failed to get frame");
    return;
  }

  frameCount++;

  // Get ambient temperature
  float ta = mlx.getTa(false);
  
  // Single pass: smooth frame + find min/max in one loop
  float minTemp = frame[0];
  float maxTemp = frame[0];
  
  if (hasPrevFrame) {
    for (int i = 0; i < 32*24; i++) {
      frame[i] = (frame[i] + prevFrame[i]) * 0.5f;
      prevFrame[i] = frame[i];
      
      if (frame[i] < minTemp) minTemp = frame[i];
      if (frame[i] > maxTemp) maxTemp = frame[i];
    }
  } else {
    for (int i = 0; i < 32*24; i++) {
      prevFrame[i] = frame[i];
      if (frame[i] > maxTemp) maxTemp = frame[i];
    }
    hasPrevFrame = true;
  }

  // Clamp range to avoid division by zero
  if (minTemp >= maxTemp) {
    minTemp = maxTemp - 1.0f;
  }

  // Clear only the thermal image area (reduces flicker vs full-screen erase)
  display.fillRect(DISPLAY_X - 1, DISPLAY_Y - 1, 32 * THERMAL_SCALE + 2, 24 * THERMAL_SCALE + 2, ST77XX_BLACK);
  
  // Draw header info (update only once per second to reduce flicker)
  if (frameCount == 1 || (millis() - lastHeaderUpdate > 1000)) {
    display.fillRect(0, 0, SCREEN_WIDTH, 32, ST77XX_BLACK);
    char header[80];
    snprintf(header, sizeof(header), "Ta:%.1fC %d-%dC F#%u", ta, (int)minTemp, (int)maxTemp, frameCount);
    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(2, 10);
    display.println(header);
    lastHeaderUpdate = millis();
  }
  
  // Draw thermal image with bounds checking
  float tempRange = maxTemp - minTemp;
  
  for (uint8_t h = 0; h < 24; h++) {
    for (uint8_t w = 0; w < 32; w++) {
      float t = frame[(23 - h) * 32 + w];
      uint16_t color = getTempColor(t, minTemp, tempRange);
      
      uint16_t x = DISPLAY_X + (w * THERMAL_SCALE);
      uint16_t y = DISPLAY_Y + (h * THERMAL_SCALE);
      
      // Bounds check before drawing
      if (x + THERMAL_SCALE <= SCREEN_WIDTH && y + THERMAL_SCALE <= SCREEN_HEIGHT) {
        display.fillRect(x, y, THERMAL_SCALE, THERMAL_SCALE, color);
      }
    }
  }
  
  // Draw frame border
  display.drawRect(DISPLAY_X - 1, DISPLAY_Y - 1, 32 * THERMAL_SCALE + 1, 24 * THERMAL_SCALE + 1, ST77XX_WHITE);
  
  // Print to Serial for debugging
  Serial.println("===================================");
  Serial.print("Frame #");
  Serial.print(frameCount);
  Serial.print(" | Ambient: ");
  Serial.print(ta, 1);
  Serial.print("C | Range: ");
  Serial.print(minTemp, 1);
  Serial.print(" - ");
  Serial.print(maxTemp, 1);
  Serial.println("C");
}

// Optimized: Convert temperature to 16-bit RGB565 color
// Precomputes normalized value to avoid redundant division
uint16_t getTempColor(float t, float minTemp, float tempRange) {
  if (tempRange <= 0) return ST77XX_WHITE;
  
  float normalized = (t - minTemp) / tempRange;
  if (normalized < 0) normalized = 0;
  if (normalized > 1) normalized = 1;
  
  // Color gradient: Blue (cold) → Cyan → Green → Yellow → Red (hot)
  uint8_t r, g, b;
  
  if (normalized < 0.25f) {
    // Blue to Cyan: (0,0,255) to (0,255,255)
    float t_local = normalized / 0.25f;
    r = 0;
    g = (uint8_t)(t_local * 255);
    b = 255;
  } else if (normalized < 0.5f) {
    // Cyan to Green: (0,255,255) to (0,255,0)
    float t_local = (normalized - 0.25f) / 0.25f;
    r = 0;
    g = 255;
    b = (uint8_t)((1.0f - t_local) * 255);
  } else if (normalized < 0.75f) {
    // Green to Yellow: (0,255,0) to (255,255,0)
    float t_local = (normalized - 0.5f) / 0.25f;
    r = (uint8_t)(t_local * 255);
    g = 255;
    b = 0;
  } else {
    // Yellow to Red: (255,255,0) to (255,0,0)
    float t_local = (normalized - 0.75f) / 0.25f;
    r = 255;
    g = (uint8_t)((1.0f - t_local) * 255);
    b = 0;
  }
  
  return RGB565(r, g, b);
}

// Convert RGB to 16-bit RGB565 format
// R: 5 bits (bits 15-11), G: 6 bits (bits 10-5), B: 5 bits (bits 4-0)
inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
