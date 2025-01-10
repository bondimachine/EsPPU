#include <Arduino.h>

#include "pins.h"

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};
static const uint8_t addressPins[] = {PIN_A0, PIN_A1, PIN_A2};

hw_timer_t *nmiTimer = NULL;


volatile bool nmiOutput = false;
volatile bool nmiUpdated = false;

volatile uint32_t frameCount = 0;
volatile uint32_t commandCount = 0;
volatile uint32_t cycleCount = 0;


void IRAM_ATTR onFrame();

void setup() {

  setCpuFrequencyMhz(240);

  Serial.begin(115200);
  Serial.println("hello");

  pinMode(PIN_RW, INPUT);
  pinMode(PIN_INT, INPUT);
  
  for (uint8_t pin = 0; pin < 8; pin++) {
    pinMode(dataPins[pin], INPUT);
  }

  for (uint8_t pin = 0; pin < 3; pin++) {
    pinMode(addressPins[pin], INPUT);
  }

  pinMode(PIN_CS, INPUT);

  pinMode(PIN_CLK, INPUT);

  nmiTimer = timerBegin(60000);
  timerAttachInterrupt(nmiTimer, &onFrame);
  timerAlarm(nmiTimer, 1000, true, 0); // ~ 60hz

}

void loop() {

  for(;;) {

    uint32_t reg = REG_READ(GPIO_IN_REG);
    cycleCount++;

    if ((!(reg & (1 << PIN_CS))) && (reg & (1 << PIN_CLK))) {
        commandCount++;

        uint16_t address = 0x2000
          | (reg >> PIN_A0 & 1)
          | ((reg >> PIN_A1 & 1) << 1)
          | ((reg >> PIN_A2 & 1) << 2);

        uint8_t data = 
          (reg >> PIN_D0 & 1)
          | ((reg >> PIN_D1 & 1) << 1)
          | ((reg >> PIN_D2 & 1) << 2)
          | ((reg >> PIN_D3 & 1) << 3)
          | ((reg >> PIN_D4 & 1) << 4)
          | ((reg >> PIN_D5 & 1) << 5)
          | ((reg >> PIN_D6 & 1) << 6)
          | ((reg >> PIN_D7 & 1) << 7);  

        char rw = (reg & (1 << PIN_RW)) ? 'W' : 'R';

        const char* command;
        switch(address) {
          case 0x2000:
            command = "PPUCTRL";
            if (rw == 'W') {
              nmiOutput = data & (1 << 7);
              nmiUpdated = true;
              if (!nmiOutput) {
                pinMode(PIN_INT, INPUT);
              }
            } 
            break;
          case 0x2001:
            command = "PPUMASK";
            break;
          case 0x2002:
            command = "PPUSTATUS";
            if (rw == 'R') {
              // clear NMI, set to high impedance
              pinMode(PIN_INT, INPUT);
            }  
            break;
          case 0x2003:
            command = "OAMADDR";
            break;
          case 0x2004:
            command = "OAMDATA";
            break;
          case 0x2005:
            command = "PPUSCROLL";
            break;
          case 0x2006:
            command = "PPUADDR";
            break;
          case 0x2007:
            command = "PPUDATA";
            break;
        }
    }
  }
}

void IRAM_ATTR onFrame() {
  if (nmiOutput) {
    pinMode(PIN_INT, OUTPUT);
    digitalWrite(PIN_INT, LOW);
  }
  frameCount++;
  if(frameCount == 60) {
    Serial.print("commands ");
    Serial.print(commandCount);
    Serial.print(" cycles ");
    Serial.println(cycleCount);
    commandCount = 0;
    cycleCount = 0;
    frameCount = 0;
  }
}