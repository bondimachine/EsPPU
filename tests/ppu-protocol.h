#include <Arduino.h>
#include "pins.h"

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};

void setupPins() {
    pinMode(PIN_RW, OUTPUT);

    for (uint8_t pin = 0; pin < 8; pin++) {
        pinMode(dataPins[pin], OUTPUT);
    }

    pinMode(PIN_A0, OUTPUT);
    pinMode(PIN_A1, OUTPUT);
    pinMode(PIN_A2, OUTPUT);
    pinMode(PIN_CS, OUTPUT);

    pinMode(PIN_CLK, OUTPUT);

    pinMode(PIN_INT, INPUT);

}

void busAddr(uint16_t addr) {
    digitalWrite(PIN_A0, addr & 1);
    digitalWrite(PIN_A1, addr & 2);
    digitalWrite(PIN_A2, addr & 4);
    digitalWrite(PIN_CS, !(addr & 0x2000));
}

inline void busSetRead() {
    digitalWrite(PIN_RW, 0);
}

uint8_t busDataRead() {
    uint8_t data = 0;

    for (uint8_t pin = 0; pin < 8; pin++) {
        data |= (digitalRead(dataPins[pin]) == HIGH) << pin;
    }

    return data;
}

void busDataWrite(uint8_t data) {
    digitalWrite(PIN_RW, 1);

    for (uint8_t pin = 0; pin < 8; pin++) {
        digitalWrite(dataPins[pin], (data & (1 << pin)));
    }
}

void busClock() {
    digitalWrite(PIN_CLK, 1);
    NOP();
    delayMicroseconds(1);
    digitalWrite(PIN_CLK, 0);
}


inline uint8_t busRead(uint16_t addr) {
    busAddr(addr);
    busSetRead();
    busClock();
    return busDataRead();
}

inline uint8_t busWrite(uint16_t addr, uint8_t data) {
    busAddr(addr);
    busDataWrite(data);
    busClock();
};

inline uint8_t ppuCtrl(uint8_t bits) {
    return busWrite(0x2000, bits);
}

inline uint8_t ppuMask(uint8_t bits) {
    return busWrite(0x2001, bits);
}

inline uint8_t ppuStatus() {
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
    busAddr(0x2006);
    busDataWrite(x);
    busClock();
    busDataWrite(y);
    busClock();
}


inline void ppuAddr(uint16_t addr) {
    busAddr(0x2006);
    busDataWrite((uint8_t)((addr >> 8) & 0xFF));
    busClock();
    busDataWrite((uint8_t)(addr & 0xFF));
    busClock();
}

inline void ppuDataWrite(uint8_t data) {
    busWrite(0x2007, data);
}

inline uint8_t ppuDataRead(uint8_t data) {
    return busRead(0x2007);
}

inline void ppuLatch(uint16_t addr) {
    ppuStatus();
    ppuAddr(addr);
}

void oamDma(uint8_t* data, uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        oamAddr(i);
        oamDataWrite(data[i]);
    }
}