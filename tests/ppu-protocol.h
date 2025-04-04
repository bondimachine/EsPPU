#include <Arduino.h>
#include "pins.h"

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

static const uint32_t dataPinsMask = 
    (1 << PIN_D0)
    | (1 << PIN_D1)
    | (1 << PIN_D2)
    | (1 << PIN_D3)
    | (1 << PIN_D4)
    | (1 << PIN_D5)
    | (1 << PIN_D6)
    | (1 << PIN_D7);

uint32_t clockCount = 0;

void setupPins() {
    
    setCpuFrequencyMhz(240);

    pinMode(PIN_RW, OUTPUT);
    digitalWrite(PIN_RW, HIGH); // negated

    pinMode(PIN_D0, OUTPUT);
    digitalWrite(PIN_D0, LOW);
    pinMode(PIN_D1, OUTPUT);
    digitalWrite(PIN_D1, LOW);
    pinMode(PIN_D2, OUTPUT);
    digitalWrite(PIN_D2, LOW);
    pinMode(PIN_D3, OUTPUT);
    digitalWrite(PIN_D3, LOW);
    pinMode(PIN_D4, OUTPUT);
    digitalWrite(PIN_D4, LOW);
    pinMode(PIN_D5, OUTPUT);
    digitalWrite(PIN_D5, LOW);
    pinMode(PIN_D6, OUTPUT);
    digitalWrite(PIN_D6, LOW);
    pinMode(PIN_D7, OUTPUT);
    digitalWrite(PIN_D7, LOW);

    pinMode(PIN_A0, OUTPUT);
    digitalWrite(PIN_A0, LOW);
    pinMode(PIN_A1, OUTPUT);
    digitalWrite(PIN_A1, LOW);
    pinMode(PIN_A2, OUTPUT);
    digitalWrite(PIN_A2, LOW);
    pinMode(PIN_A3_OUT, OUTPUT);
    digitalWrite(PIN_A3_OUT, LOW);
    pinMode(PIN_A4_OUT, OUTPUT);
    digitalWrite(PIN_A4_OUT, LOW);
    pinMode(PIN_A5_OUT, OUTPUT);
    digitalWrite(PIN_A5_OUT, LOW);
    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, HIGH);
    pinMode(PIN_AS, OUTPUT);
    digitalWrite(PIN_AS, HIGH);

    pinMode(PIN_CLK, OUTPUT);
    digitalWrite(PIN_CLK, LOW);

    pinMode(PIN_INT, INPUT);

}

#define SET_PINS(x) REG_WRITE(GPIO_OUT_W1TS_REG, x)
#define CLEAR_PINS(x) REG_WRITE(GPIO_OUT_W1TC_REG, x)

inline void busAddr(uint16_t addr, bool write) {

    uint32_t toSet = ((addr & 1) << PIN_A0) 
        | (((addr >> 1) & 1) << PIN_A1) 
        | (((addr >> 2) & 1) << PIN_A2)
        | (((addr >> 3) & 1) << PIN_A3_OUT)
        | (((addr >> 4) & 1) << PIN_A4_OUT)
        | ((!((addr >> 13) & 1)) << PIN_CS)
        | ((!((addr >> 14) & 1)) << PIN_AS)
        | ((!(write)) << PIN_RW);

    uint32_t toClear = ~toSet & (
        (1 << PIN_A0 | 1 << PIN_A1 | 1 << PIN_A2 | 1 << PIN_A3_OUT | 1 << PIN_A4_OUT | 1 << PIN_CS | 1 << PIN_AS | 1 << PIN_RW));

    SET_PINS(toSet);
    CLEAR_PINS(toClear);

    digitalWrite(PIN_A5_OUT, ((addr >> 5) & 1));

}

inline uint8_t busDataRead() {

    uint32_t reg = REG_READ(GPIO_IN_REG);
    uint8_t data = 
        (reg >> PIN_D0 & 1)
        | ((reg >> PIN_D1 & 1) << 1)
        | ((reg >> PIN_D2 & 1) << 2)
        | ((reg >> PIN_D3 & 1) << 3)
        | ((reg >> PIN_D4 & 1) << 4)
        | ((reg >> PIN_D5 & 1) << 5)
        | ((reg >> PIN_D6 & 1) << 6)
        | ((reg >> PIN_D7 & 1) << 7);

    return data;
}

inline void busDataWrite(uint8_t data) {
    REG_WRITE(GPIO_ENABLE_W1TS_REG, dataPinsMask);

    uint32_t toSet = ((data & 1) << PIN_D0) 
        | (((data >> 1) & 1) << PIN_D1) 
        | (((data >> 2) & 1) << PIN_D2)
        | (((data >> 3) & 1) << PIN_D3)
        | (((data >> 4) & 1) << PIN_D4)
        | (((data >> 5) & 1) << PIN_D5)
        | (((data >> 6) & 1) << PIN_D6)
        | (((data >> 7) & 1) << PIN_D7);

    uint32_t toClear = ~toSet & dataPinsMask;

    SET_PINS(toSet);
    CLEAR_PINS(toClear);
}

inline void busClock() {
    SET_PINS(1 << PIN_CLK);
    __asm__ __volatile__ (REP(0, 6, 0, "nop; "));
    CLEAR_PINS(1 << PIN_CLK);
    clockCount++;
}


inline uint8_t busRead(uint16_t addr) {
    busAddr(addr, false);
    busClock();
    // we got up to 10ns since clock down to read the bus
    uint8_t result = busDataRead();
    // we simulate fetching next instruction in the 6502 (it actually takes longer)
    // there is no way to get two consecutive calls to PPU in real world so lets avoid stressing ours
    busAddr(0, false);
    busClock();
    return result;
}

inline void busWrite(uint16_t addr, uint8_t data) {
    busAddr(addr, true);
    busDataWrite(data);
    busClock();
    busAddr(0, false);
    busClock();
};

inline void ppuCtrl(uint8_t bits) {
    busWrite(0x2000, bits);
}

inline void ppuMask(uint8_t bits) {
    busWrite(0x2001, bits);
}

inline uint8_t ppuStatus() {
    // reading PPU status resets the write pair for ppuAddr and ppuScroll
    return busRead(0x2002);
}

inline void oamAddr(uint8_t addr) {
    busWrite(0x2003, addr);
}

inline uint8_t oamDataRead(uint8_t data) {
    return busRead(0x2004);
}

inline void oamDataWrite(uint8_t data) {
    busWrite(0x2004, data);
}

inline void ppuScroll(uint8_t x, uint8_t y) {
    ppuStatus();
    busWrite(0x2005, x);
    busWrite(0x2005, y);
}


inline void ppuAddr(uint16_t addr) {
    ppuStatus();
    busWrite(0x2006, (uint8_t)((addr >> 8) & 0xFF));
    busWrite(0x2006, (uint8_t)(addr & 0xFF));
}

inline void ppuDataWrite(uint8_t data) {
    busWrite(0x2007, data);
}

inline uint8_t ppuDataRead(uint8_t data) {
    return busRead(0x2007);
}

void oamDma(uint8_t* data, uint16_t size) {
    oamAddr(0);
    for (uint16_t i = 0; i < size; i++) {
        oamDataWrite(data[i]);
    }
    for (uint16_t i = size; i < 256; i++) {
        oamDataWrite(0);
    }
}