#include "ppu-protocol-local.h"
#include "../pins.h"
#include <stdio.h>


unsigned char YUV[192] = {
	0x66, 0x66, 0x66, 0x00, 0x2a, 0x88, 0x14, 0x12, 
	0xa7, 0x3b, 0x00, 0xa4, 0x5c, 0x00, 0x7e, 0x6e, 
	0x00, 0x40, 0x6c, 0x07, 0x00, 0x56, 0x1d, 0x00, 
	0x33, 0x35, 0x00, 0x0c, 0x48, 0x00, 0x00, 0x52, 
	0x00, 0x00, 0x4f, 0x08, 0x00, 0x40, 0x4d, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xad, 0xad, 0xad, 0x15, 0x5f, 0xd9, 0x42, 0x40, 
	0xff, 0x75, 0x27, 0xfe, 0xa0, 0x1a, 0xcc, 0xb7, 
	0x1e, 0x7b, 0xb5, 0x31, 0x20, 0x99, 0x4e, 0x00, 
	0x6b, 0x6d, 0x00, 0x38, 0x87, 0x00, 0x0d, 0x93, 
	0x00, 0x00, 0x8f, 0x32, 0x00, 0x7c, 0x8d, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0xff, 0x64, 0xb0, 0xff, 0x92, 0x90, 
	0xff, 0xc6, 0x76, 0xff, 0xf2, 0x6a, 0xff, 0xff, 
	0x6e, 0xcc, 0xff, 0x81, 0x70, 0xea, 0x9e, 0x22, 
	0xbc, 0xbe, 0x00, 0x88, 0xd8, 0x00, 0x5c, 0xe4, 
	0x30, 0x45, 0xe0, 0x82, 0x48, 0xcd, 0xde, 0x4f, 
	0x4f, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0xff, 0xc0, 0xdf, 0xff, 0xd3, 0xd2, 
	0xff, 0xe8, 0xc8, 0xff, 0xfa, 0xc2, 0xff, 0xff, 
	0xc4, 0xea, 0xff, 0xcc, 0xc5, 0xf7, 0xd8, 0xa5, 
	0xe4, 0xe5, 0x94, 0xcf, 0xef, 0x96, 0xbd, 0xf4, 
	0xab, 0xb3, 0xf3, 0xcc, 0xb5, 0xeb, 0xf2, 0xb8, 
	0xb8, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


unsigned char nametable1[] = {
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 

	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 

	0x24, 0x24, 0x24, 0x16, 0x0a, 0x1b, 0x12, 0x18, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x20, 0x18, 0x1b, 0x15, 0x0d, 0x24, 
	0x24, 0x1d, 0x12, 0x16, 0x0e, 0x24, 0x24, 0x24, 

	0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, 
	0x00, 0x24, 0x24, 0x2e, 0x29, 0x00, 0x00, 0x24, 
	0x24, 0x24, 0x24, 0x01, 0x28, 0x01, 0x24, 0x24, 
	0x24, 0x24, 0x03, 0x07, 0x07, 0x24, 0x24, 0x24, 
    
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x36, 0x37, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x35, 
	0x25, 0x25, 0x38, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x39, 
	0x3a, 0x3b, 0x3c, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x36, 0x37, 0x36, 0x37, 0x36, 0x37, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x35, 
	0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x38, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x39, 
	0x3a, 0x3b, 0x3a, 0x3b, 0x3a, 0x3b, 0x3c, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x53, 0x54, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x55, 0x56, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x53, 0x54, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x45, 0x45, 0x53, 0x54, 0x45, 0x45, 0x53, 0x54, 
	0x45, 0x45, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x55, 0x56, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x47, 0x47, 0x55, 0x56, 0x47, 0x47, 0x55, 0x56, 
	0x47, 0x47, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x60, 0x61, 0x62, 0x63, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x31, 0x32, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x64, 0x65, 0x66, 0x67, 0x24, 0x24, 0x24, 0x24, 
	0x24, 0x30, 0x26, 0x34, 0x33, 0x24, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x36, 0x37, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x68, 0x69, 0x26, 0x6a, 0x24, 0x24, 0x24, 0x24, 
	0x30, 0x26, 0x26, 0x26, 0x26, 0x33, 0x24, 0x24, 
	0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x35, 
	0x25, 0x25, 0x38, 0x24, 0x24, 0x24, 0x24, 0x24, 
	0x68, 0x69, 0x26, 0x6a, 0x24, 0x24, 0x24, 0x24, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 0xb4, 0xb5, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 
	0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7, 0xb6, 0xb7
};

uint8_t attribute[] = {
	0xaa, 0xaa, 0b11101010, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 
	0x00, 0x88, 0xaa, 0x00, 0x00, 0x80, 0xa0, 0xa0, 
	0x00, 0x00, 0x00, 0x30, 0x00, 0x08, 0x0a, 0x0a, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x30, 0x00, 0xd0, 0xd0, 0x10, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 
};


uint8_t game_palette[] = {
    0x22,0x29,0x1A,0x0F,
    0x22,0x36,0x17,0x0F,  
    0x22,0x30,0x21,0x0F,  
    0x22,0x27,0x17,0x0F
};

void render(int sx, int sy);

int main(int argc, char** argv) {

  	ppuAddr(0x2000);
    for(uint16_t x = 0; x < 960; x++) {
       ppuDataWrite(nametable1[x]);
    }

  	ppuAddr(0x23C0);
    for(uint8_t x = 0; x < 64; x++) {
       ppuDataWrite(attribute[x]);
    }

  	ppuAddr(0x2400);
    for(uint16_t y = 0; y < 30; y++) {
		for(uint16_t x = 0; x < 32; x++) {
			ppuDataWrite(x);
		}
	}

  	ppuAddr(0x27C0);
    for(uint8_t y = 0; y < 8; y++) {
	    for(uint8_t x = 0; x < 8; x++) {
    	   ppuDataWrite(x % 4);
    	}
	}	

    ppuAddr(0x3F00);
    for(uint8_t x = 0; x < 16; x++) {
       ppuDataWrite(game_palette[x]);
    }

    ppuCtrl(0b00010000); // background sprites at 0x1000
    ppuMask(0b00001000); // only background rendering

    FILE* f = fopen("background.chr", "rb");
    fread(chr, 0x2000, 1, f);
    fclose(f);

    // f = fopen("background.ppm", "w+");
    // fprintf(f, "P3\n128 128 3\n");

    // for(int sprite_y = 0; sprite_y < 16; sprite_y++) {
    //    for(int y = 0; y < 8; y++) {
    //        for(int sprite_x = 0; sprite_x < 16; sprite_x++) {
    //           uint8_t* sprite = chr + 0x1000 + (sprite_y * 16 + sprite_x) * 16;
    //           for(int x = 0; x < 8; x++) {
    //              uint8_t color = (sprite[y] >> (7-x) & 1) << 1 | (sprite[y + 8] >> (7-x) & 1);
    //              fprintf(f, "0bd 0bd 0bd\t", color, color, color);
    //           }
    //        }
    //    }
    //    fprintf(f, "\n");
    // }
    // fclose(f);

    // f = fopen("palette.ppm", "w+");
    // fprintf(f, "P3\n32 32 255\n");

    // for(int sprite_y = 0; sprite_y < 4; sprite_y++) {
    //    for(int y = 0; y < 8; y++) {
    //        for(int sprite_x = 0; sprite_x < 4; sprite_x++) {
    //           uint8_t color = palette[sprite_y * 4 + sprite_x];
    //           for(int x = 0; x < 8; x++) {
    //             uint8_t * rgb = YUV + color * 3;
    //             fprintf(f, "%03d %03d %03d\t", rgb[0], rgb[1], rgb[2]);
    //           }
    //        }
    //    }
    //    fprintf(f, "\n");
    // }
    // fclose(f);

	// test fine scrolling, 1 pixel to see if it correctly subdivides tiles
	for(int sy = 0; sy < 8; sy++) {
		render(0, sy);
	}
	// one tile at a time, full screen
	for(int sy = 8; sy < 240; sy+=8) {
		render(0, sy);
	}

	for(int sx = 0; sx < 8; sx++) {
		render(sx, 0);
	}
	for(int sx = 8; sx < 255; sx+=8) {
		render(sx, 0);
	}

	// test 8th bit
    ppuCtrl(0b00010001);

	for(int sx = 0; sx < 255; sx+=8) {
		render(sx, 0);
	}

    ppuCtrl(0b00010000);

	// test combined 
	for(int s = 0; s < 128; s+=8) {
		render(s, s);
	}


    return 0;
}

int frame_count = 0;
void render(int sx, int sy) {
	ppuScroll(sx % 256, sy % 256);
	char filename[30];
	snprintf(filename, 30, "nametabletest%03d.ppm", frame_count++);
	FILE *f = fopen(filename, "w+");
	fprintf(f, "P3\n256 240 255\n");
	uint8_t line_buffer[256];
	
	for (int y = 0; y < 240; y++) {
		nes_ppu_scanline(line_buffer, y);

		for (int x = 0; x < 256; x++) {
			uint8_t * rgb = YUV + line_buffer[x] * 3;
			fprintf(f, "%03d %03d %03d\t", rgb[0], rgb[1], rgb[2]);
			// fprintf(f, "%03d %03d %03d\t", line_buffer[x], line_buffer[x], line_buffer[x]);
		}
		fprintf(f, "\n");
	}

	fclose(f);	
}

