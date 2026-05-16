#include <Adafruit_MLX90640.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// MLX90640 thermal camera
Adafruit_MLX90640 mlx;
float frame[32*24]; // buffer for full frame of temperatures

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Visible panel is centered horizontally in the 128x64 buffer
#define VISIBLE_X 28
#define VISIBLE_Y 24
#define VISIBLE_WIDTH 72
#define VISIBLE_HEIGHT 40
#define OLED_I2C_ADDRESS 0x3C

// Custom I2C pins for ESP32-C
#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int16_t headerScroll = 0;
bool hasPrevFrame = false;
float prevFrame[32*24];

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  delay(100);

  // Initialize custom I2C on pins 5 (SDA) and 6 (SCL)
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(VISIBLE_X, VISIBLE_Y);
  display.println("Init MLX90640...");
  display.display();
  
  // Initialize MLX90640
  Serial.println("Adafruit MLX90640 OLED Display");
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("MLX90640 not found!");
    display.clearDisplay();
    display.setCursor(VISIBLE_X, VISIBLE_Y + 12);
    display.println("MLX90640 ERROR");
    display.display();
    while (1) delay(10);
  }
  
  Serial.println("Found Adafruit MLX90640");

  display.clearDisplay();
  display.setCursor(VISIBLE_X, VISIBLE_Y + 12);
  display.println("MLX90640 Found!");
  display.display();
  delay(1000);

  Serial.print("Serial number: ");
  Serial.print(mlx.serialNumber[0], HEX);
  Serial.print(mlx.serialNumber[1], HEX);
  Serial.println(mlx.serialNumber[2], HEX);

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_4_HZ);
  
  Serial.println("MLX90640 initialized successfully");
}

void loop() {
  delay(500);
  
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Failed to get frame");
    return;
  }

  // Get ambient temperature
  float ta = mlx.getTa(false);
  
  // Smooth the frame with the previous reading to reduce jitter
  if (hasPrevFrame) {
    for (int i = 0; i < 32*24; i++) {
      frame[i] = (frame[i] + prevFrame[i]) * 0.5f;
      prevFrame[i] = frame[i];
    }
  } else {
    for (int i = 0; i < 32*24; i++) {
      prevFrame[i] = frame[i];
    }
    hasPrevFrame = true;
  }

  // Find min and max temperatures in frame
  float minTemp = frame[0];
  float maxTemp = frame[0];
  
  for (int i = 0; i < 32*24; i++) {
    if (frame[i] < minTemp) minTemp = frame[i];
    if (frame[i] > maxTemp) maxTemp = frame[i];
  }

  // Clear display and draw frame
  display.clearDisplay();
  
  // Draw thermal data as ASCII art (scaled to fit the visible OLED area)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header text scrolls horizontally when it is too long for the visible width
  char header[32];
  snprintf(header, sizeof(header), "Ta:%.1fC %d-%dC", ta, (int)minTemp, (int)maxTemp);
  drawScrollingText(header, VISIBLE_Y, VISIBLE_WIDTH);
  
  // Display thermal image as dot sizes based on temperature
  // Each thermal pixel is approximately 2.25 x 1.67 display pixels
  uint8_t dotX = VISIBLE_X;
  uint8_t dotY = VISIBLE_Y + 8;
  for (uint8_t h = 0; h < 24; h++) {
    for (uint8_t w = 0; w < 32; w++) {
      // Flip the frame vertically so the OLED image is not upside down
      float t = frame[(23 - h) * 32 + w];
      uint8_t dotSize = getDotSizeForTemp(t, minTemp, maxTemp);
      drawDot(dotX + (w * 2), dotY + (h * 1), dotSize);
    }
  }
  
  display.display();
  
  // Also print to Serial for debugging
  Serial.println("===================================");
  Serial.print("Ambient temperature = ");
  Serial.print(ta, 1);
  Serial.println(" degC");
  Serial.print("Temperature range: ");
  Serial.print(minTemp, 1);
  Serial.print(" - ");
  Serial.print(maxTemp, 1);
  Serial.println(" degC");
}

// Draw a dot with size proportional to temperature
void drawDot(uint8_t x, uint8_t y, uint8_t size) {
  if (size == 0) {
    display.drawPixel(x, y, SSD1306_WHITE);
  } else if (size == 1) {
    display.fillCircle(x, y, 1, SSD1306_WHITE);
  } else if (size == 2) {
    display.fillCircle(x, y, 2, SSD1306_WHITE);
  } else {
    display.fillCircle(x, y, 3, SSD1306_WHITE);
  }
}

// Get dot size (0-3) based on temperature
uint8_t getDotSizeForTemp(float t, float minTemp, float maxTemp) {
  if (maxTemp == minTemp) return 1;
  
  float tRounded = roundf(t / 3.0f) * 3.0f;  // Round to nearest 3 degrees
  float normalized = (tRounded - minTemp) / (maxTemp - minTemp);
  if (normalized < 0) normalized = 0;
  if (normalized > 1) normalized = 1;
  
  if (normalized < 0.25f) return 0;  // tiny: single pixel
  else if (normalized < 0.5f) return 1;  // small: radius 1
  else if (normalized < 0.75f) return 2;  // medium: radius 2
  else return 3;  // large: radius 3
}

// Convert temperature to ASCII character for heatmap display
char getCharForTemp(float t, float minTemp, float maxTemp) {
  if (maxTemp == minTemp) return ' '; // Avoid division by zero
  
  // Normalize temperature to 0-1 range
  float normalized = (t - minTemp) / (maxTemp - minTemp);
  
  // Map to ASCII characters from cold to hot
  if (normalized < 0.2) return '.';
  else if (normalized < 0.4) return '-';
  else if (normalized < 0.6) return '*';
  else if (normalized < 0.8) return '+';
  else return '#';
}

// Draw a scrolling text line inside the visible OLED width.
void drawScrollingText(const char* text, int16_t y, int16_t width) {
  int16_t textWidth = strlen(text) * 6; // Approximate width for 1x font
  int16_t maxOffset = textWidth + width;

  if (textWidth <= width) {
    display.setCursor(VISIBLE_X, y);
    display.print(text);
    headerScroll = 0;
    return;
  }

  display.setCursor(VISIBLE_X - headerScroll, y);
  display.print(text);
  headerScroll += 1;
  if (headerScroll >= maxOffset) {
    headerScroll = 0;
  }
}
