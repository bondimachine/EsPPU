#define PIN_D7 7
#define PIN_D6 6

#include "../nes_ppu.h"

inline uint8_t busRead(uint16_t addr) {
    return nes_ppu_command(addr, 0, false);
};

inline void busWrite(uint16_t addr, uint8_t data) {
    nes_ppu_command(addr, data, true);
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

void oamDma(uint8_t* data, uint16_t size) {
    oamAddr(0);
    for (uint16_t i = 0; i < size; i++) {
        oamDataWrite(data[i]);
    }
    for (uint16_t i = size; i < 256; i++) {
        oamDataWrite(0);
    }
} 