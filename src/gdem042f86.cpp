#include "gdem042f86.h"

#include <SPI.h>

namespace GDEM042F86 {
namespace {

constexpr uint32_t kSpiHz = 10000000;
constexpr uint32_t kBusyTimeoutMs = 30000;

int8_t pinCs = -1;
int8_t pinDc = -1;
int8_t pinRst = -1;
int8_t pinBusy = -1;

void writeCommand(uint8_t command) {
  digitalWrite(pinCs, LOW);
  digitalWrite(pinDc, LOW);
  SPI.transfer(command);
  digitalWrite(pinCs, HIGH);
}

void writeData(uint8_t data) {
  digitalWrite(pinCs, LOW);
  digitalWrite(pinDc, HIGH);
  SPI.transfer(data);
  digitalWrite(pinCs, HIGH);
}

bool waitUntilIdle(uint32_t timeoutMs = kBusyTimeoutMs) {
  const uint32_t start = millis();
  while (digitalRead(pinBusy) == LOW) {
    if (millis() - start > timeoutMs) {
      Serial.println(F("GDEM042F86 busy timeout."));
      return false;
    }
    delay(1);
    yield();
  }
  return true;
}

void resetPanel() {
  delay(20);
  digitalWrite(pinRst, LOW);
  delay(40);
  digitalWrite(pinRst, HIGH);
  delay(50);
}

bool initPanel(bool fastUpdate) {
  resetPanel();
  if (!waitUntilIdle()) {
    return false;
  }

  writeCommand(0x06);  // BTST
  writeData(0x0F);
  writeData(0x8B);
  writeData(0x9C);
  writeData(0x96);

  writeCommand(0x00);  // PSR
  writeData(0x2F);
  writeData(0x69);

  writeCommand(0x01);  // PWR
  writeData(0x07);
  writeData(0xF0);

  writeCommand(0x50);  // CDI
  writeData(0x37);

  writeCommand(0x61);  // resolution
  writeData(kWidth / 256);
  writeData(kWidth % 256);
  writeData(kHeight / 256);
  writeData(kHeight % 256);

  writeCommand(0x62);
  writeData(0x64);
  writeData(0x53);

  writeCommand(0x65);
  writeData(0x00);
  writeData(0x00);
  writeData(0x00);
  writeData(0x00);

  writeCommand(0x30);
  writeData(0x08);

  writeCommand(0xE9);
  writeData(0x01);

  writeCommand(0x04);  // power on
  if (!waitUntilIdle()) {
    return false;
  }

  if (fastUpdate) {
    writeCommand(0xEF);
    writeData(0x01);

    writeCommand(0xF6);
    writeData(0x15);

    writeCommand(0xEF);
    writeData(0x00);

    writeCommand(0xE0);
    writeData(0x02);

    writeCommand(0xE6);
    writeData(0x5A);

    writeCommand(0xA5);
    if (!waitUntilIdle()) {
      return false;
    }
  }

  return true;
}

bool updatePanel() {
  writeCommand(0x12);
  writeData(0x00);
  return waitUntilIdle();
}

void sleepPanel() {
  writeCommand(0x02);  // power off
  writeData(0x00);
  waitUntilIdle();

  writeCommand(0x07);  // deep sleep
  writeData(0xA5);
}

uint8_t nativeColorFromAppCode(uint8_t code) {
  // App frame format: 0=white, 1=black, 2=red, 3=yellow.
  // GDEM042F86 native format: 0=black, 1=white, 2=yellow, 3=red.
  switch (code & 0x03) {
    case 0:
      return 0x01;
    case 1:
      return 0x00;
    case 2:
      return 0x03;
    case 3:
      return 0x02;
    default:
      return 0x01;
  }
}

uint8_t remapPackedByte(uint8_t packed) {
  return (nativeColorFromAppCode(packed >> 6) << 6) |
         (nativeColorFromAppCode(packed >> 4) << 4) |
         (nativeColorFromAppCode(packed >> 2) << 2) |
         nativeColorFromAppCode(packed);
}

bool writeSolid(uint8_t nativePackedColor, bool fastUpdate) {
  if (!initPanel(fastUpdate)) {
    return false;
  }

  writeCommand(0x10);
  for (size_t i = 0; i < kFrameBytes; ++i) {
    writeData(nativePackedColor);
  }

  const bool ok = updatePanel();
  sleepPanel();
  return ok;
}

}  // namespace

void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t cs, int8_t dc, int8_t rst, int8_t busy) {
  pinCs = cs;
  pinDc = dc;
  pinRst = rst;
  pinBusy = busy;

  pinMode(pinBusy, INPUT);
  pinMode(pinRst, OUTPUT);
  pinMode(pinDc, OUTPUT);
  pinMode(pinCs, OUTPUT);
  digitalWrite(pinCs, HIGH);
  digitalWrite(pinRst, HIGH);

  SPI.begin(sck, miso, mosi, cs);
  SPI.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
}

bool displayFrame(const uint8_t *frame, size_t length, bool fastUpdate) {
  if ((frame == nullptr) || (length != kFrameBytes)) {
    return false;
  }

  if (!initPanel(fastUpdate)) {
    sleepPanel();
    return false;
  }

  writeCommand(0x10);
  for (size_t i = 0; i < kFrameBytes; ++i) {
    writeData(remapPackedByte(frame[i]));
  }

  const bool ok = updatePanel();
  sleepPanel();
  return ok;
}

bool clearWhite() {
  return writeSolid(0x55, false);
}

}  // namespace GDEM042F86
