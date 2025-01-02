#include <Arduino.h>

#include "pins.h"

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};
static const uint8_t addressPins[] = {PIN_A0, PIN_A1, PIN_A2};


void setup() {
  Serial.begin(115200);

  pinMode(PIN_RW, INPUT);
  
  for (uint8_t pin = 0; pin < 8; pin++) {
    pinMode(dataPins[pin], INPUT);
  }

  for (uint8_t pin = 0; pin < 3; pin++) {
    pinMode(addressPins[pin], INPUT);
  }

  pinMode(PIN_CS, INPUT_PULLUP);

  pinMode(PIN_CLK, INPUT);

}

bool clockConsumed = false;

void loop() {
  if (digitalRead(PIN_CS) == LOW && digitalRead(PIN_CLK) == HIGH) {
    if (!clockConsumed) {
      clockConsumed = true;
      uint16_t address = 0x2000;
      for (uint8_t pin = 0; pin < 3; pin++) {
        address |= (digitalRead(addressPins[pin]) == HIGH) << pin;
      }

      uint8_t data = 0;

      for (uint8_t pin = 0; pin < 8; pin++) {
        data |= (digitalRead(dataPins[pin]) == HIGH) << pin;
      }

      Serial.print(address, HEX);
      Serial.print("  ");
      Serial.print(data, HEX);
      Serial.println();
    } 
  } else if (clockConsumed) {
    clockConsumed = false;
  }
}

