# Hardware Components

## Waveshare MLX90640-D110

**Thermal Infrared Camera Sensor**

- **Resolution:** 32×24 pixels (768 thermal pixels)
- **Field of View (FOV):** 110° (wide-angle)
- **Temperature Range:** -40°C to +85°C
- **Accuracy:** ±2°C (typical)
- **Refresh Rates:** 2 Hz, 4 Hz, 8 Hz, 16 Hz
- **Interface:** I2C
- **I2C Address:** 0x33 (default, not configurable)
- **Supply Voltage:** 3.3V
- **Current Draw:** ~23mA typical
- **Pixel Size:** ~90µm
- **Thermal Time Constant:** ~140ms

### Pin Configuration

| Pin | Function |
|-----|----------|
| VCC | 3.3V Power |
| GND | Ground |
| SDA | I2C Data (GPIO 5 on ESP32-C3) |
| SCL | I2C Clock (GPIO 6 on ESP32-C3) |

### Mechanical

- **Module Size:** 34mm × 39mm
- **Sensor Array:** 32×24 grid on 14mm² die
- **Window:** IR-transmissive (typically 7–14µm passband)
- **Housing:** Plastic with aluminum heatsink

### Performance Notes

- Requires ambient temperature (Ta) reference for accuracy — read via `mlx.getTa(false)`
- Spatial resolution: ~3.1° per pixel (at 110° FOV ÷ 32 pixels)
- Time lag between scene change and reading: ~140ms + capture interval
- Automatic calibration occurs periodically; slight lag may occur during calibration

## Gorilley C3 Development Board

**ESP32-C3 Microcontroller**

- **SoC:** Espressif ESP32-C3 (single-core RISC-V)
- **Clock Speed:** 160 MHz (standard), 80 MHz (low power)
- **RAM:** 400 KB SRAM
- **Flash:** 4 MB (typical boards; verify yours)
- **GPIO Pins:** 22 total (11 usable with constraints)
- **I2C Interfaces:** 2× I2C (hardware)
- **SPI Interfaces:** 2× SPI
- **UART:** 2× UART
- **ADC:** 6-channel 12-bit SAR ADC
- **USB:** USB 2.0 Device (via serial bridge or native USB on newer variants)

### Pinout (Relevant for This Project)

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 5 | I2C SDA | Used for thermal sensor + OLED |
| GPIO 6 | I2C SCL | Used for thermal sensor + OLED |
| GPIO 20 | USB D- | Leave unconnected if using serial bridge |
| GPIO 19 | USB D+ | Leave unconnected if using serial bridge |
| GND | Ground | Reference all devices to same GND |
| 3V3 | 3.3V Power | Max ~500mA total draw from LDO regulator |

### Power Management

- **Input Voltage:** 5V USB (internal LDO regulates to 3.3V)
- **3.3V Output:** ~500mA max (shared with all 3.3V peripherals)
- **Idle Current:** ~10mA (without sensors)
- **Active Current:** ~80–150mA (with MLX90640 + OLED)

## SSD1306 OLED Display

**128×64 Monochrome OLED Display**

- **Resolution:** 128×64 pixels
- **Color Depth:** 1-bit (on/off only)
- **Interface:** I2C
- **I2C Address:** 0x3C (typical; some variants use 0x3D)
- **Supply Voltage:** 3.3V to 5V (typically 3.3V for this project)
- **Current Draw:** ~5–10mA typical (white display)
- **Contrast:** Adjustable 0–255
- **Refresh Rate:** 60 Hz internal

### Display Mapping (This Project)

| Property | Value |
|----------|-------|
| Full Buffer | 128×64 pixels |
| Visible Area | 72×40 pixels |
| Visible Offset | X: 28px, Y: 24px (centered) |
| Thermal Image | 32 cols × 24 rows |
| Pixel Scaling | ~2.25 display pixels per thermal pixel |

### Dimensions

- **Module Size:** ~27mm × 27mm (small form factor)
- **Display Area:** ~27mm × 14mm
- **Weight:** ~2g

## Power Budget Summary

| Component | Voltage | Current (Typical) | Notes |
|-----------|---------|-------------------|-------|
| ESP32-C3 (active) | 3.3V | 80mA | CPU + peripherals |
| MLX90640-D110 | 3.3V | 23mA | 4 Hz capture |
| SSD1306 (full white) | 3.3V | 10mA | Max brightness |
| **Total** | 3.3V | **113mA** | Normal operation |

**USB Bus Power:** 5V @ 500mA (standard USB-C)
- LDO regulator: 5V → 3.3V (~80% efficiency)
- Sufficient headroom for all components

## I2C Bus Configuration

**Shared I2C Bus (Bus 0):**
- SDA: GPIO 5
- SCL: GPIO 6
- Clock Speed: 100 kHz (default; can increase to 400 kHz)
- Pull-up Resistors: Internal (~20kΩ typical, may need external 4.7kΩ if bus is noisy)

**Device Addresses:**
- MLX90640: 0x33 (thermal sensor)
- SSD1306: 0x3C (OLED display)

Both devices coexist peacefully on the same bus with no address conflicts.

## Recommended External Components

For production builds, consider adding:

- **4.7kΩ pull-up resistors** on SDA/SCL (if I2C bus is long or noisy)
- **100µF capacitor** on 3.3V rail (decoupling near ESP32)
- **10µF capacitor** on 3.3V rail (near thermal sensor and OLED)
- **Micro USB hub** (if powering multiple high-current devices)

## Revision History

| Date | Board | MLX90640 Module | SSD1306 | Notes |
|------|-------|-----------------|---------|-------|
| Current | Gorilley C3 | Waveshare MLX90640-D110 | Generic SSD1306 | Tested and verified |

All three components are widely available and compatible with Arduino IDE via community libraries.
