#include <Arduino.h>

#include "pins.h"

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};
static const uint8_t addressPins[] = {PIN_A0, PIN_A1, PIN_A2};

hw_timer_t *nmiTimer = NULL;


bool nmi = false;

volatile bool frame = false;
volatile uint32_t frame_count = 0;


extern void* ld_include_xt_nmi;

volatile uint32_t command_buffer[COMMAND_BUFFER_SIZE];
volatile uint32_t command_buffer_write_index = 0xFFFFFFFF; // we increment first, then write
volatile uint32_t command_buffer_read_index = 0xFFFFFFFF;


void IRAM_ATTR onFrame();

void setup() {

  setCpuFrequencyMhz(240);

  Serial.begin(115200);
  Serial.print("hello - running on core: ");
  Serial.println(xPortGetCoreID());

  pinMode(PIN_RW, INPUT);
  pinMode(PIN_INT, OUTPUT_OPEN_DRAIN);
  digitalWrite(PIN_INT, HIGH);
  
  for (uint8_t pin = 0; pin < 8; pin++) {
    pinMode(dataPins[pin], INPUT);
  }

  for (uint8_t pin = 0; pin < 3; pin++) {
    pinMode(addressPins[pin], INPUT);
  }

  pinMode(PIN_CS, INPUT);
  pinMode(PIN_CLK, INPUT);

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << PIN_CLK),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_POSEDGE
  };

  gpio_config(&io_conf);

  // the 26 comes from here https://github.com/espressif/esp-idf/blob/0f0068fff3ab159f082133aadfa9baf4fc0c7b8d/components/esp_hw_support/port/esp32/esp_cpu_intr.c#L170 

  intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 26);
  ESP_INTR_ENABLE( 26 );

  nmiTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(nmiTimer, &onFrame, true);
  timerAlarmWrite(nmiTimer, 16666, true); // ~ 60hz
  timerAlarmEnable(nmiTimer);

  // nmiTimer = timerBegin(60000);
  // timerAttachInterrupt(nmiTimer, &onFrame);
  // timerAlarm(nmiTimer, 1000, true, 0); // ~ 60hz

}

void loop() {

    if (frame) {
      frame = false;
      if (nmi) {
        digitalWrite(PIN_INT, LOW);
      }
      frame_count++;
      if(frame_count == 60) {
        Serial.print("buffer ");
        Serial.print(command_buffer_write_index + 1);
        Serial.print(" delta ");
        Serial.println(command_buffer_write_index - command_buffer_read_index);
        frame_count = 0;
      }
    }

    // != instead of < because we will overflow
    
    if (command_buffer_read_index != command_buffer_write_index) {

      uint32_t reg = command_buffer[(++command_buffer_read_index) % COMMAND_BUFFER_SIZE];

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
            nmi = data & (1 << 7);
            if (!nmi) {
              digitalWrite(PIN_INT, HIGH);
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
            digitalWrite(PIN_INT, HIGH);
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

      // Serial.print(command_buffer_read_index);
      // Serial.print(" ");
      // Serial.print(command);
      // Serial.print(" ");
      // Serial.print(rw);
      // Serial.print(" ");
      // Serial.println(data, HEX);

    }
}

void IRAM_ATTR onFrame() {
  frame = true;
}