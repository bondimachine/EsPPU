#define PIN_DATA0 15
#define PIN_LATCH 2
#define PIN_CLOCK 4
#define PIN_DATA1 5

#define SET_PINS(x) REG_WRITE(GPIO_OUT_W1TS_REG, x)
#define CLEAR_PINS(x) REG_WRITE(GPIO_OUT_W1TC_REG, x)

#define REP0(X)
#define REP1(X) X
#define REP2(X) REP1(X) X
#define REP3(X) REP2(X) X
#define REP4(X) REP3(X) X
#define REP5(X) REP4(X) X
#define REP6(X) REP5(X) X
#define REP7(X) REP6(X) X
#define REP8(X) REP7(X) X
#define REP9(X) REP8(X) X
#define REP10(X) REP9(X) X

#define REP(HUNDREDS,TENS,ONES,X) \
  REP##HUNDREDS(REP10(REP10(X))) \
  REP##TENS(REP10(X)) \
  REP##ONES(X)

void setup() {
  
  Serial.begin(115200);

  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA0, INPUT);
  pinMode(PIN_DATA1, INPUT);

  digitalWrite(PIN_CLOCK, 1);
}

void read_pad(uint8_t pad, uint8_t pin_data) {
  uint8_t buttons[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  SET_PINS(1 << PIN_LATCH);
  __asm__ __volatile__ (REP(0, 6, 0, "nop; "));
  CLEAR_PINS(1 << PIN_LATCH);

  __asm__ __volatile__ (REP(2, 4, 0, "nop; "));

  SET_PINS(1 << PIN_LATCH);
  __asm__ __volatile__ (REP(0, 6, 0, "nop; "));
  CLEAR_PINS(1 << PIN_LATCH);

  __asm__ __volatile__ (REP(4, 8, 0, "nop; "));

  for(int i = 0; i < 8; i++) {
    SET_PINS(1 << PIN_CLOCK);
    __asm__ __volatile__ (REP(0, 6, 0, "nop; "));
    uint32_t reg = REG_READ(GPIO_IN_REG);
    buttons[i] = ((reg >> pin_data) & 1);
    CLEAR_PINS(1 << PIN_CLOCK);
    digitalWrite(PIN_CLOCK, 1);
    __asm__ __volatile__ (REP(4, 8, 0, "nop; "));
  }

  for(int i = 0; i < 8; i++) {
    Serial.print(buttons[i] == LOW);
  }

  
}

void loop() {

  delay(10);

  read_pad(0, PIN_DATA0);
  Serial.println("");

}