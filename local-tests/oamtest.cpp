#include "ppu-protocol-local.h"
#include "../pins.h"
#include <stdio.h>

int main(int argc, char** argv) {

    ppuAddr(0x3F00);
    for(uint8_t x = 0; x < 2; x++) {
        for(uint8_t y = 0; y < 16; y++) {
            ppuDataWrite(y);
        }
    }

  	ppuAddr(0x2000);
    for(uint8_t y = 16; y > 0; y--) {
        for(uint8_t x = 0; x < 255; x++) {
            ppuDataWrite(0);
        }
    }

    ppuCtrl(0x80);
    ppuMask(0b00011110);

    FILE* f = fopen("../tests/ppu-color-test/test.chr", "rb");
    fread(chr, 0x1000, 1, f);
    fclose(f);

    // f = fopen("chr.ppm", "w+");
    // fprintf(f, "P3\n128 128 3\n");

    // for(int sprite_y = 0; sprite_y < 16; sprite_y++) {
    //     for(int y = 0; y < 8; y++) {
    //         for(int sprite_x = 0; sprite_x < 16; sprite_x++) {
    //             uint8_t* sprite = chr + (sprite_y * 16 + sprite_x) * 16;
    //             for(int x = 0; x < 8; x++) {
    //                 uint8_t color = (sprite[y] >> (7-x) & 1) << 1 | (sprite[y + 8] >> (7-x) & 1);
    //                 fprintf(f, "%d %d %d\t", color, color, color);
    //             }
    //         }
    //     }
    //     fprintf(f, "\n");
    // }
    // fclose(f);

    for (int attr = 0; attr < 8; attr++) {
        for(int base_tile = 0; base_tile < 4; base_tile++) {
            uint8_t oam_fill[256];

            for(int y = 0; y<8; y++) {
                for(int x = 0; x<8; x++) {
                    uint8_t tile = (y*8 + x);
                    uint8_t* oam_tile = oam_fill + tile*4;
                    oam_tile[0] = y*8;
                    oam_tile[1] = tile + base_tile*64;
                    oam_tile[2] = attr < 4 ? attr : ((attr - 4) << 6);
                    oam_tile[3] = x*8;
                }        
            }

            oamDma(oam_fill, 256);

            char filename[16];
            snprintf(filename, 16, "oamtest%d%d.ppm", base_tile, attr);

            f = fopen(filename, "w+");
            fprintf(f, "P3\n256 240 15\n");
            uint8_t line_buffer[256];
            
            for (int y = 0; y < 240; y++) {
                nes_ppu_scanline(line_buffer, y);

                for (int x = 0; x < 256; x++) {
                    uint8_t * rgb = YUV + line_buffer[x] * 3;
                    // fprintf(f, "%03d %03d %03d\t", rgb[0], rgb[1], rgb[2]);
                    fprintf(f, "%03d %03d %03d\t", line_buffer[x], line_buffer[x], line_buffer[x]);
                }
                fprintf(f, "\n");
            }

            fclose(f);
        }
    }

    return 0;
}

