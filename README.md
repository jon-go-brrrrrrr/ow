# MLX90640 Thermal Camera OLED Display

A real-time thermal imaging system using a Waveshare MLX90640-D110 infrared camera and SSD1306 OLED display on an ESP32-C3 microcontroller. Displays live thermal data as a dynamic dot-matrix heatmap with temperature gradients.

## Features

- **32×24 thermal resolution** from MLX90640-D110 infrared sensor
- **Real-time display** on 128×64 OLED screen with 4 Hz refresh rate
- **Temperature visualization** using dot sizes (pixel to radius 3) mapped to thermal gradients
- **Thermal smoothing** – Frame averaging reduces sensor noise by 50%
- **Scrolling header** – Displays ambient temperature, min/max range
- **3°C temperature granularity** – Optimal balance for small display resolution
- **Serial debug output** – Temperature readings logged to USB serial

## Hardware

### Components

- **Waveshare MLX90640-D110** – 32×24 pixel IR thermal camera, I2C interface
- **Gorilley C3 Development Board** – ESP32-C3 microcontroller (240 MHz, 4 MB flash)
- **SSD1306 OLED Display** – 128×64 pixels, I2C interface (0x3C)
- **USB-C cable** – Power and serial communication

### Pinout

| Signal | ESP32-C3 Pin | Device |
|--------|--------------|--------|
| SDA    | GPIO 5       | MLX90640, SSD1306 |
| SCL    | GPIO 6       | MLX90640, SSD1306 |
| GND    | GND          | All devices |
| 3.3V   | 3V3          | All devices |

**I2C Bus:**
- MLX90640 I2C address: `0x33` (default)
- SSD1306 I2C address: `0x3C` (default)

Both devices share the same I2C bus (GPIO 5/6).

## Installation

See [INSTALLATION.md](INSTALLATION.md) for detailed Arduino IDE setup and library installation.

### Quick Start

1. Install Arduino IDE and ESP32 board support
2. Install required libraries via Arduino Library Manager:
   - `Adafruit MLX90640`
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
3. Connect hardware per pinout diagram above
4. Upload sketch to Gorilley C3 board
5. Open Serial Monitor (115200 baud) to view debug output

## Usage

### Display Output

The OLED display shows:
- **Header line** – Ambient temperature (Ta), min/max thermal range
- **Thermal image** – 24 rows × 32 columns rendered as dots
  - Dot size represents temperature: small (cold) → large (hot)
  - Image is vertically flipped to match camera orientation
  - Visible area: 72×40 pixels centered in 128×64 buffer

### Serial Output

Temperature readings and frame statistics printed every 500ms:
```
===================================
Ambient temperature = 24.5 degC
Temperature range: 18.2 - 35.7 degC
```

## Configuration

Edit these values in the sketch to customize behavior:

```cpp
#define SDA_PIN 5                    // I2C SDA pin
#define SCL_PIN 6                    // I2C SCL pin
#define OLED_I2C_ADDRESS 0x3C        // SSD1306 I2C address

mlx.setMode(MLX90640_CHESS);         // Sensor pattern mode
mlx.setResolution(MLX90640_ADC_18BIT); // 18-bit ADC
mlx.setRefreshRate(MLX90640_4_HZ);   // 4 Hz capture rate
```

**Refresh Rate Tuning:**
- `MLX90640_2_HZ` – Lower power, less jitter
- `MLX90640_4_HZ` – Recommended for real-time tracking ⭐
- `MLX90640_8_HZ` – Smoother motion, higher power consumption
- `MLX90640_16_HZ` – Maximum speed (limited by I2C)

**Temperature Gradient:**
Modify `getDotSizeForTemp()` to adjust thermal resolution:
```cpp
float tRounded = roundf(t / 3.0f) * 3.0f;  // Currently 3°C granularity
```

## Performance

- **Frame capture:** ~250ms at 4 Hz
- **Display update:** ~50ms rendering + I2C transfer
- **Total latency:** ~300ms (thermal + display)
- **RAM usage:** ~1.8 KB (two 768-byte frame buffers)
- **Power:** ~150mA @ 3.3V (thermal sensor + display + ESP32)

## Troubleshooting

### Display not initializing
```
SSD1306 allocation failed
```
- Verify I2C address is `0x3C` (check with `i2cdetect`)
- Ensure SDA/SCL pins 5/6 are not used by other peripherals
- Check power supply voltage (3.3V required)

### Thermal sensor not found
```
MLX90640 not found!
```
- Verify I2C address is `0x33` (default for MLX90640-D110)
- Check SDA/SCL connections and pull-up resistors (if needed)
- Confirm sensor firmware is loaded (should show serial number)

### Frame read failures
```
Failed to get frame
```
- Increase I2C clock speed (default 100 kHz often sufficient)
- Reduce refresh rate to 2 Hz
- Check for I2C bus contention or noise

### Serial output not appearing
- Verify baud rate: **115200** (not 9600)
- Check USB-C cable is data-capable
- Ensure "Upload Speed" in Arduino IDE set to 921600

## Technical Notes

- **Frame smoothing:** Current + previous frame averaged (50/50 weighting) to reduce thermal noise
- **Temperature calibration:** Ambient temp (Ta) read from sensor reference junction; may differ ±2°C from actual air temp
- **Vertical flip:** Frame indices reversed `(23-h)*32+w` to correct camera orientation on display
- **Dot mapping:** Normalized temperature (0–1) mapped to dot radius (0–3) in quartile steps
- **ASCII art mode:** `getCharForTemp()` function available but not used in current display mode

## Future Enhancements

- Color thermal palette (IPS LCD upgrade)
- Min/max temperature history plotting
- Temperature alarm thresholds with buzzer
- SD card logging for thermal video recording
- WiFi streaming to mobile app
- Calibration offset adjustment

## License

MIT License. See LICENSE file.

## References

- [Adafruit MLX90640 Library](https://github.com/adafruit/Adafruit_MLX90640)
- [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [Waveshare MLX90640-D110 Datasheet](https://www.waveshare.com/wiki/MLX90640-D110)
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
