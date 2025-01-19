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

uint8_t* oam_tile_ch = oam_fill + ( 3*4)+1;
uint8_t* oam_tile_cl = oam_fill + ( 4*4)+1;
uint8_t* oam_tile_b  = oam_fill + (13*4)+1;
uint8_t* oam_tile_g  = oam_fill + (14*4)+1;
uint8_t* oam_tile_r  = oam_fill + (15*4)+1;
uint8_t* oam_tile_s  = oam_fill + (18*4)+1;

volatile uint8_t ppu_emphasis = 0b00011110;
volatile uint8_t color = 0;

volatile bool frame = false;

void IRAM_ATTR onNMI(); // forward

void setup() {
    
    Serial.begin(115200);

    setupPins();
    
    // setup oam (this is different from what original code does)
    oamDma(oam_fill, 23*4);

    // setup default palettes
    Serial.println("setup default palettes");
    ppuAddr(0x3F00);
    for(uint8_t y = 16; y > 0; y--) {
        for(uint8_t x = 0; x < 4; x++) {
            ppuDataWrite(palette[x]);
        }
    }

	//setup nametable
    Serial.println("setup nametable");
	ppuAddr(0x2000);
    for(uint8_t y = 16; y > 0; y--) {
        for(uint8_t x = 0; x < 255; x++) {
            ppuDataWrite(0);
        }
    }

    // start NMI
    attachInterrupt(digitalPinToInterrupt(PIN_INT), onNMI, FALLING);
    Serial.println("start nmi");
    ppuCtrl(0x80);

    ppuMask(ppu_emphasis);

    Serial.print("setup clocks: ");
    Serial.print(clockCount);

    busDataWrite(0);
    busAddr(0);

}

volatile uint32_t frame_count = 0;

void loop() {
    if (frame) {
        frame = false;
        if ((frame_count % 60) == 1) {
            Serial.print("frame ");
            Serial.print(frame_count);
            Serial.print(" clocks: ");
            Serial.println(clockCount);
        }

        // update sprites
        oamDma(oam_fill, 23*4);

        // set background color
        ppuAddr(0x3F00);
        ppuDataWrite(color);

        // set foreground color
        uint8_t foreground = 0x30;
        if (color <= 0x20) {
            if((ppu_emphasis & 1) || (color & 0x0F) < 0x0E) {
                // if not in greyscale mode, E/F columns are all black
                foreground = 0x0F;
            }
        }

        ppuAddr(0x3F13);
        ppuDataWrite(foreground);

        // set scroll
        ppuScroll(0, 0);
      
        // set emphasis
        ppuMask(ppu_emphasis);

        busDataWrite(0);
        busAddr(0);
    }

    if (Serial.available() > 0) {
        switch(Serial.read()) {
            case 'w':
                color = ((color + 0xF0) & 0x3F);
                break;
            case 's':
                color = ((color + 0x10) & 0x3F);
                break;
            case 'a':
                color = ((color - 1) & 0x0F) | (color & 0x30);
                break;
            case 'd':
                color = ((color + 1) & 0x0F) | (color & 0x30);
                break;
            case ' ':
                ppu_emphasis ^= 1;
                break;
            case 'A':
                ppu_emphasis ^= 0b10000000;
                break;
            case 'W':
                ppu_emphasis ^= 0b01000000;
                break;
            case 'D':
                ppu_emphasis ^= 0b00100000;
                break;
            case 'S':
                ppu_emphasis ^= 0b00011111;
                break;
            case '.':
                ppu_emphasis ^= 0b00010100;
                break;
            default:
                return;    
        }

        // redraw sprites
        *oam_tile_ch = (color >> 4) | 0xA0;
        *oam_tile_cl = (color & 0x0F) | 0xA0;
        *oam_tile_b = ((ppu_emphasis >> 2) & 1) | 0xA0;
        *oam_tile_g = ((ppu_emphasis >> 3) & 1) | 0xA0;
        *oam_tile_r = ((ppu_emphasis >> 4) & 1) | 0xA0;
        *oam_tile_s = (ppu_emphasis & 1) | 0xA0;

        Serial.print("color: ");
        Serial.print(color, HEX);
        Serial.print(" emphasis: ");
        Serial.println(ppu_emphasis, HEX);
    }

}

void IRAM_ATTR onNMI() {
    frame = true;
    frame_count++;
}

