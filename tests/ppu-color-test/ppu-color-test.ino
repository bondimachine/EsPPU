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

int sound_frame = -1;


uint8_t main_theme_trg_timer[] =  { 0xAB, 0x00, 0x53, 0x00, 0x1D, 0x00, 0x3A, 0xAB, 0x00, 0xAB, 0x53, 0x00, 0x1D, 0x00, 0xAB, 0x00, 0x40, 0x00, 0xFE, 0x00, 0xD5, 0x00, 0xAB, 0x40, 0x00, 0x40, 0xFE, 0x00, 0xD5, 0x00, 0xC4, 0x00, 0xAB, 0x00, 0x53, 0x00, 0x1D, 0x00, 0x3A, 0xAB, 0x00, 0xAB, 0x53, 0x00, 0x1D, 0x00, 0xAB, 0x00, 0x40, 0x00, 0x7C, 0x00, 0xC4, 0x00, 0x40, 0x7C, 0x00, 0xC4, 0x3A, 0x00, 0xFC, 0x00, 0xC4, 0x00 };
uint8_t main_theme_trg_length[] = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0X01, 0x00, 0X01, 0x00, 0X00, 0x00, 0x00, 0x00, 0X01, 0x01, 0x00, 0x01, 0X00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00 };

uint8_t enemy_hit_sq1_timer[] =  {0x35, 0x43, 0x54, 0x6A, 0x6A, 0x86, 0xA9, 0xD5, 0xD5, 0x0D, 0x53, 0xAB, 0xAB, 0x1A, 0xA6, 0x57 };
uint8_t enemy_hit_sq1_length[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0X01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03 };

uint8_t water_splash_noise_period[] =  { 0x08, 0x05, 0x05, 0x0C, 0x0C };

uint8_t game_over_sq0_timer[]  = {0xD5, 0x00, 0x1D, 0x1D, 0x0D, 0x00, 0x53, 0x40, 0x1D, 0xFE, 0xE2, 0x00, 0xD5, 0x00, 0x00};
uint8_t game_over_sq0_length[] = {0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t game_over_sq1_timer[]  = {0xAB, 0x00, 0x3A, 0x3A, 0x1A, 0x00, 0xA6, 0x80, 0x3A, 0xFC, 0xC4, 0x00, 0xAB, 0x00, 0x00};
uint8_t game_over_sq1_length[] = {0x01, 0x00, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00};

uint8_t game_over_trg_timer[]  = {0xAB, 0x00, 0x53, 0x00, 0x3A, 0x00, 0x53, 0x00, 0xC4, 0x00, 0x7C, 0x00, 0xAB, 0x00, 0x00};
uint8_t game_over_trg_length[] = {0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};


void IRAM_ATTR onNMI(); // forward

#define PIN_LF 39 // VN
#define PIN_RT 36
#define PIN_UP 35
#define PIN_DN 34 // VP


void setup() {
    
    Serial.begin(115200);

    pinMode(PIN_LF, INPUT_PULLUP);
    pinMode(PIN_RT, INPUT_PULLUP);
    pinMode(PIN_UP, INPUT_PULLUP); 
    pinMode(PIN_DN, INPUT_PULLUP);

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
    busSetRead();

}

volatile uint32_t frame_count = 0;

bool debounce = false;

bool inline is_sound_step(int start, int end, int length) {
    return (start <= sound_frame && sound_frame < end) && (sound_frame % length == 0);
}

void render_sound_frame() {
    if (sound_frame == 0) {
        busWrite(0x4015, 0x0F); // ApuStatus

        busWrite(0x4000, 0xCA); // Sq0Duty
        busWrite(0x4001, 0x00); // Sq0Sweep
    
        busWrite(0x4004, 0xC4); // Sq1Duty
        busWrite(0x4005, 0x00); // Sq1Sweep
    
        busWrite(0x4008, 0x20); // TrgLinear
    
        busWrite(0x400C, 0x00); // NoiseVolume
        busWrite(0x400E, 0x00); // NoisePeriod    
    }

    if (is_sound_step(0, 640, 10)) {
        uint8_t i = sound_frame / 10;
        uint8_t timer = main_theme_trg_timer[i];
        if (timer) {
            busWrite(0x400A, main_theme_trg_timer[i]); 
            busWrite(0x400B, main_theme_trg_length[i] | 0x10); 
            busWrite(0x400F, 0x01); // NoiseLength 
        }
    }
    sound_frame++;
    if (sound_frame > 1000) {
        sound_frame = -1;
    }
}

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
        uint8_t foreground = 0x1F;
        if (color < 0x10 || ((color % 0x10) >= 0xD)) {
            foreground = 0x30;
        }

        ppuAddr(0x3F13);
        ppuDataWrite(foreground);

        // set scroll
        ppuScroll(0, 0);
      
        // set emphasis
        ppuMask(ppu_emphasis);

        busDataWrite(0);
        busAddr(0);
        busSetRead();

        if (sound_frame > -1) {
            render_sound_frame();
        }

    }

    bool updated = false;

    if (digitalRead(PIN_LF) == LOW) {
        if (!debounce) {
          color = ((color - 1) & 0x0F) | (color & 0x30);
          Serial.println("left");
          updated = true;
          debounce = true;
        } else if (digitalRead(PIN_RT) == LOW) {
          if (sound_frame != 0) {
            sound_frame = 0;
            Serial.println("sound start");
          }
        }
    } else if (digitalRead(PIN_RT) == LOW) {
        if (!debounce) {
          color = ((color + 1) & 0x0F) | (color & 0x30);
          Serial.println("right");
          updated = true;
          debounce = true;
        }
    } else if (digitalRead(PIN_UP) == LOW) {
        if (!debounce) {
          color = ((color + 0xF0) & 0x3F);
          Serial.println("up");
          updated = true;
          debounce = true;
        }  
    } else if (digitalRead(PIN_DN) == LOW) {
        if (!debounce) {
          color = ((color + 0x10) & 0x3F);
          Serial.println("down");
          updated = true; 
          debounce = true;
        }  
    } else {
      debounce = false;
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
            case 'm':
                sound_frame = 0;
                break;
            default:
                return;    
        }

        updated = true;
    }

    if (updated) {
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

