FROM ubuntu:22.04

RUN apt-get update && apt-get install -y curl git && rm -rf /var/lib/apt/lists/*
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

WORKDIR /workspace
COPY . /workspace/

RUN arduino-cli config init && \
    arduino-cli core install esp32:esp32@2.0.14 && \
    arduino-cli lib install Adafruit\ MLX90640 && \
    arduino-cli lib install Adafruit\ SSD1306 && \
    arduino-cli lib install Adafruit\ GFX\ Library && \
    arduino-cli lib install Adafruit\ ST7789\ Library

CMD ["sh", "-c", "echo 'MLX90640 Arduino Build Environment Ready. Sketches available:'; find /workspace -name '*.ino' -type f; echo ''; echo 'Run: docker run --rm -v /dev:/dev --privileged mlx90640-thermal-camera flash-c6 /dev/ttyUSB0'"]
