# Installation Guide

## Prerequisites

- Arduino IDE 1.8.13 or later (or Arduino IDE 2.0+)
- USB-C cable (data-capable, not power-only)
- Gorilley C3 Development Board
- MLX90640-D110 thermal sensor
- SSD1306 OLED display
- 3.3V power supply (USB from development board)

## Arduino IDE Setup

### 1. Install ESP32 Board Support

1. Open **Arduino IDE** → **File** → **Preferences** (or **Arduino IDE** → **Settings** on macOS)
2. In **Additional Boards Manager URLs**, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**
4. Go to **Tools** → **Board Manager**
5. Search for `esp32` and click **Install** (latest version, typically 2.0.x or higher)
6. Wait for installation to complete (~2 min)

### 2. Install Required Libraries

Open **Sketch** → **Include Library** → **Manage Libraries** (or use Ctrl+Shift+I):

#### Library 1: Adafruit MLX90640
- **Search:** `MLX90640`
- **Author:** Adafruit
- **Install:** Latest version (typically 1.1.x)

#### Library 2: Adafruit SSD1306
- **Search:** `SSD1306`
- **Author:** Adafruit
- **Install:** Latest version (typically 2.5.x)

#### Library 3: Adafruit GFX Library
- **Search:** `GFX`
- **Author:** Adafruit
- **Install:** Latest version (typically 1.11.x)
- *(Usually auto-installed as dependency of SSD1306; verify it's present)*

#### Library 4: Adafruit BusIO (Optional, often auto-installed)
- **Search:** `BusIO`
- **Author:** Adafruit
- **Install:** Latest version
- *(Dependency for MLX90640; may auto-install)*

### 3. Select Board and Port

1. **Tools** → **Board** → **esp32** → Select `ESP32C3 Dev Module` (or `Gorilley C3` if listed)
2. **Tools** → **Upload Speed** → `921600`
3. **Tools** → **CPU Frequency** → `160 MHz`
4. **Tools** → **Flash Size** → `4MB`
5. **Tools** → **Partition Scheme** → `Default 4MB with spiffs`
6. **Tools** → **Port** → Select your USB port (e.g., `/dev/ttyUSB0` or `COM3`)

### 4. Verify Installation

1. Copy the entire sketch code to Arduino IDE
2. Click **Verify** (checkmark icon) to compile
3. Should complete without errors in ~10 seconds
4. If compilation fails, check:
   - All three Adafruit libraries installed
   - Board selected as ESP32C3 (not generic ESP32)
   - No conflicting library versions

## Hardware Connection

### Wiring Diagram

```
Gorilley C3           MLX90640-D110
──────────────────────────────────
GPIO 5 (SDA) ────────── SDA
GPIO 6 (SCL) ────────── SCL
3V3 ─────────────────── VCC
GND ─────────────────── GND

Gorilley C3           SSD1306 OLED
──────────────────────────────────
GPIO 5 (SDA) ────────── SDA
GPIO 6 (SCL) ────────── SCL
3V3 ─────────────────── VCC (or 5V)
GND ─────────────────── GND
```

### Connection Checklist

- [ ] MLX90640 SDA → ESP32-C3 GPIO 5
- [ ] MLX90640 SCL → ESP32-C3 GPIO 6
- [ ] SSD1306 SDA → ESP32-C3 GPIO 5 (shared I2C bus)
- [ ] SSD1306 SCL → ESP32-C3 GPIO 6 (shared I2C bus)
- [ ] All GND pins connected together
- [ ] All VCC/3V3 pins connected to ESP32-C3 3V3 output
- [ ] USB-C cable connected to ESP32-C3 for power and serial

## First Upload

1. Connect USB-C cable to Gorilley C3
2. In Arduino IDE, click **Upload** (arrow icon)
3. Wait 30–60 seconds for compilation and upload
4. When complete, serial monitor should automatically open
5. Set baud rate to **115200** (bottom-right dropdown)
6. Observe OLED display initialization messages
7. After ~2 seconds, thermal image should appear on OLED

### Expected Output

**Serial Monitor:**
```
Adafruit MLX90640 OLED Display
Found Adafruit MLX90640
Serial number: 401A02D011CC
MLX90640 initialized successfully
===================================
Ambient temperature = 24.5 degC
Temperature range: 18.2 - 35.7 degC
```

**OLED Display:**
- Initialization message: "Init MLX90640..." → "MLX90640 Found!"
- Live thermal image with scrolling header (Ta, min–max temps)

## Troubleshooting

### Upload Fails: "Failed to connect to ESP32"

**Solution:**
1. Verify USB cable is **data-capable** (not power-only)
2. Try a different USB port on your computer
3. Manually enter bootloader mode:
   - Hold **BOOT** button on ESP32-C3
   - Press **RESET** button (or USB cable)
   - Release **BOOT** button
   - Try uploading again
4. If still failing, check device manager for COM port conflicts

### Serial Monitor Shows Gibberish

**Solution:**
1. Verify baud rate is set to **115200** (not 9600)
2. Check USB cable connection is secure
3. Restart Arduino IDE
4. Unplug/replug USB cable

### "SSD1306 allocation failed"

**Solution:**
1. Verify I2C address is `0x3C` in code
2. Run I2C scanner sketch to confirm device presence:
   ```cpp
   Wire.begin(5, 6);
   Wire.beginTransmission(0x3C);
   if (Wire.endTransmission() == 0) {
     Serial.println("SSD1306 found at 0x3C");
   }
   ```
3. Check SDA/SCL connections (swap if needed)
4. Verify 3.3V power is stable (check with multimeter)

### "MLX90640 not found!"

**Solution:**
1. Verify I2C address is `0x33` in code
2. Confirm sensor connections:
   - SDA to GPIO 5
   - SCL to GPIO 6
   - VCC to 3V3
   - GND to GND
3. Check for I2C pull-up resistors (may need external 4.7kΩ)
4. Add delay in setup: `delay(500)` after `Wire.begin()` to allow sensors to power-on

### No Output on OLED

**Solution:**
1. Check OLED contrast setting in code (default is 128, max 255)
2. Verify OLED is receiving power (backlight may be dimly lit)
3. Test OLED alone with Adafruit example sketch
4. Check if vertical flip is needed: modify `(23 - h) * 32 + w` to `h * 32 + w` in loop

### Frame Read Failures ("Failed to get frame")

**Solution:**
1. Reduce refresh rate from 4 Hz to 2 Hz:
   ```cpp
   mlx.setRefreshRate(MLX90640_2_HZ);  // vs MLX90640_4_HZ
   ```
2. Increase I2C clock speed (if needed):
   ```cpp
   Wire.begin(5, 6, 400000);  // 400 kHz instead of 100 kHz
   ```
3. Add 10µF decoupling capacitors on 3.3V rail near each sensor

## Advanced Configuration

### Changing I2C Pins

To use different GPIO pins for I2C:

1. Modify sketch:
   ```cpp
   #define SDA_PIN 8   // Change from 5
   #define SCL_PIN 9   // Change from 6
   Wire.begin(SDA_PIN, SCL_PIN);
   ```
2. Update physical connections accordingly
3. Recompile and upload

### Adjusting Temperature Range

To change the min/max displayed range, modify:
```cpp
mlx.setRefreshRate(MLX90640_8_HZ);   // 4 Hz → 8 Hz (faster, more power)
mlx.setResolution(MLX90640_ADC_16BIT); // 18-bit → 16-bit (faster, less precise)
```

### Custom Colormaps (Future)

When upgrading to color display, implement in `getDotSizeForTemp()`:
```cpp
// Future: Replace with RGB value mapping
// if (normalized < 0.25) return 0xFF0000;  // Red for hot
// if (normalized < 0.5) return 0xFFFF00;   // Yellow
// if (normalized < 0.75) return 0x00FF00;  // Green
// return 0x0000FF;  // Blue for cold
```

## Performance Tuning

| Setting | Performance | Power | Thermal Lag |
|---------|-------------|-------|------------|
| 2 Hz refresh | Smooth | Low | High |
| 4 Hz refresh | Good (recommended) | Medium | Medium |
| 8 Hz refresh | Very smooth | High | Low |
| 16 Hz refresh | Max (I2C limited) | Very high | Very low |

For battery-powered applications, use 2 Hz. For real-time thermal tracking, use 4 Hz or higher.

## Next Steps

1. Verify both OLED and thermal image display correctly
2. Test temperature range by holding warm/cold objects near sensor
3. Adjust `tRounded` granularity (currently 3°C) for desired thermal resolution
4. Add `.gitignore` and push to GitHub repository

---

**Need Help?** Check [README.md](README.md) for common issues or [COMPONENTS.md](COMPONENTS.md) for hardware specs.
