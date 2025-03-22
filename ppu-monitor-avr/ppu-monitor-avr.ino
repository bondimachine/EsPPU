#include <Arduino.h>

#define PIN_RW 13
#define PIN_D0 4
#define PIN_D1 5
#define PIN_D2 6
#define PIN_D3 7
#define PIN_D4 8
#define PIN_D5 9
#define PIN_D6 10
#define PIN_D7 11

#define PIN_A2 A2
#define PIN_A1 A1
#define PIN_A0 A0
#define PIN_CS 12
#define PIN_CLK 2

#define PIN_INT 3

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};
static const uint8_t addressPins[] = {PIN_A0, PIN_A1, PIN_A2};

bool nmi = false;

volatile bool frame = false;
volatile uint32_t frame_count = 0;

volatile uint8_t address_buffer[256];
volatile uint8_t data_buffer[256];
volatile uint8_t command_buffer_write_index = 0xFF; // we increment first, then write
volatile uint8_t command_buffer_read_index = 0xFF;


void setup() {

  Serial.begin(115200);
  Serial.print("hello");

  pinMode(PIN_RW, INPUT);
  pinMode(PIN_INT, OUTPUT);
  digitalWrite(PIN_INT, HIGH);
  
  for (uint8_t pin = 0; pin < 8; pin++) {
    pinMode(dataPins[pin], INPUT);
  }

  for (uint8_t pin = 0; pin < 3; pin++) {
    pinMode(addressPins[pin], INPUT);
  }

  pinMode(PIN_CS, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_CLK), onClock, RISING);

  TCCR1A = 0;  // timer1 normal mode
  TCCR1B = (1 << CS11) | (1 << WGM12);  // prescaler /8 (2mhz). CTC
  OCR1A = 33333; // counter set at (2mhz / (1/60 ms)) 

  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);

}

void loop() {

    if (frame) {
      frame = false;
      if (nmi) {
        digitalWrite(PIN_INT, LOW);
      }
      frame_count++;
      if(frame_count == 60) {
        // Serial.print("buffer ");
        // Serial.print(command_buffer_write_index + 1);
        // Serial.print(" delta ");
        // Serial.println(command_buffer_write_index - command_buffer_read_index);
        frame_count = 0;
      }
    }

    // != instead of < because we will overflow
    
    if (command_buffer_read_index != command_buffer_write_index) {

      uint8_t addr = address_buffer[++command_buffer_read_index];
      uint16_t address = 0x2000 | (addr & 0b111);
      uint8_t data = data_buffer[command_buffer_read_index];

      char rw = (addr >> 6 & 1) ? 'R' : 'W';

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

      Serial.print(command_buffer_read_index);
      Serial.print(" ");
      //Serial.print(command);
      Serial.print(address, HEX);
      Serial.print(" ");
      Serial.print(rw);
      Serial.print(" ");
      Serial.println(data, HEX);

    }
}

ISR(TIMER1_COMPA_vect) {
  frame = true;
}  

void onClock() {
 if (!(PINB & (1 << (PIN_CS-8)))) { // CS is negated
    address_buffer[++command_buffer_write_index] = PINC | ((PINB >> (PIN_RW - 8)) & 1) << 6;
    data_buffer[command_buffer_write_index] = (PIND >> PIN_D0) | (PINB << PIN_D0);
 }
}