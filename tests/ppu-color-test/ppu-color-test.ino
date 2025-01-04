#include "ppu-protocol.h"

// Adapted from https://forums.nesdev.org/viewtopic.php?p=155593#p155593
uint8_t palette[] = {0x00, 0x16, 0x2D, 0x30};

#define CX 16
#define CY 23


uint8_t oam_fill[] = {
    CY+(0*10), 0xB8, 0, CX+(2*8),
    CY+(0*10), 0xBA, 0, CX+(3*8),
    CY+(1*10), 'C', 0, CX+(0*8),
    CY+(1*10), '0', 0, CX+(2*8),
    CY+(1*10), '0', 0, CX+(3*8),
    CY+(2*10), 0xB9, 0, CX+(2*8),
    CY+(2*10), 0xBB, 0, CX+(3*8),
    CY+(4*10), 0xBA, 0, CX+(2*8),
    CY+(4*10), 0xB8, 0, CX+(3*8),
    CY+(4*10), 0xBB, 0, CX+(4*8),
    CY+(4*10), '+', 0, CX+(5*8),
    CY+(4*10), 0xB1, 0, CX+(6*8),
    CY+(5*10), 'E', 0, CX+(0*8),
    CY+(5*10), '0', 0, CX+(2*8),
    CY+(5*10), '0', 0, CX+(3*8),
    CY+(5*10), '0', 0, CX+(4*8),
    CY+(7*10), 0xB0, 0, CX+(2*8),
    CY+(8*10), 'S', 0, CX+(0*8),
    CY+(8*10), '0', 0, CX+(2*8),
    CY+(10*10),'H', 0, CX+(0*8),
    CY+(10*10),0xB2, 0, CX+(2*8),
    CY+(10*10),0xB3, 0, CX+(3*8),
    CY+(10*10),0xB4, 0, CX+(4*8)
};


void setup() {
    
    setupPins();
    // we don't have an actual DMA, so no need to copy oam_fill

    // setup default palettes
    ppuLatch(0x3F00);
    for(uint8_t y = 16; y > 0; y++) {
        for(uint8_t x = 0; x < 4; x++) {
            ppuDataWrite(palette[x]);
        }
    }

	  //setup nametable
	  ppuLatch(0x2000);
    for(uint8_t y = 16; y > 0; y++) {
        for(uint8_t x = 0; x < 255; x++) {
            ppuDataWrite(0);
        }
    }

    // start NMI
    ppuCtrl(0x80);
}

void loop() { 
}

uint8_t ppuEmphasis = 0b00011110;

