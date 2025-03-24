#include <stdint.h>
#include <stdio.h>

uint8_t oam[256];
uint8_t oam_addr = 0;

uint16_t vram_addr = 0;
uint8_t vram[2048];

uint8_t chr[8192];

uint8_t palette[32];

uint8_t vram_step = 1;
bool write_latch = false;

uint16_t sprite_8x8_pattern_address = 0;
uint16_t background_pattern_address = 0;
uint8_t sprite_height = 8;

uint8_t scroll_x = 0;
uint16_t scroll_x_high = 0;
uint8_t scroll_y = 0;
uint16_t scroll_y_high = 0;

bool nmi_output = false;
volatile uint32_t vblank = 0;

bool grayscale = false;
bool left8pixels_background = false;
bool left8pixels_sprites = false;
bool background_rendering = false;
bool sprite_rendering = false;
bool emphasize_r = false;
bool emphasize_g = false;
bool emphasize_b = false;

struct __attribute__((packed)) oam_entry {
  uint8_t y;
  uint8_t index;
  uint8_t attr;
  uint8_t x;
};

inline uint8_t pixel_palette_idx(uint8_t right_plane_line, uint8_t left_plane_line, uint8_t x) {
    return (right_plane_line >> (7-x) & 1) << 1 | (left_plane_line >> (7-x) & 1);
}

inline void nes_ppu_scanline(uint8_t* buf, int y) {

    if(background_rendering) {
        // TODO: horizontal mirroring
        uint16_t effective_y = (y + scroll_y + scroll_y_high) % 240;
        uint8_t row = effective_y / 8;
        for (uint16_t x = 0; x < 256; x++) {
            uint16_t effective_x = (x + scroll_x + scroll_x_high) % 512;
            uint16_t table_x = 0;
            if (effective_x >= 256) {
                effective_x -= 256;
                table_x = 0x400;
            };
            uint8_t col = effective_x / 8;

            uint8_t tile_index = vram[table_x + (row * 32) + col];

            uint8_t attribute = vram[table_x + 0x03C0 + ((row / 4) * 8) + col / 4];

            uint8_t* tile = chr + background_pattern_address + tile_index * 16;
            uint8_t line = (effective_y % 8);

            uint8_t right_plane_line = tile[line];
            uint8_t left_plane_line = tile[line + 8];

            // I know there is a bitwise way to figure this, but I got enough for the moment
            uint8_t attribute_shift = (col % 4) < 2 ? 0 : 2;
            if ((row % 4) >= 2) { 
                attribute_shift += 4;
            }
            uint8_t tile_palete_idx = (attribute >> attribute_shift) & 0b11;
            uint8_t* tile_palette = palette + tile_palete_idx * 4;

            uint8_t pixel = pixel_palette_idx(left_plane_line, right_plane_line, effective_x % 8);
            if (pixel) {
                buf[x] = tile_palette[pixel];
            } else {
                buf[x] = palette[0];
            }                
        }

    } else {
        // universal background
        for (int x = 0; x < 256; x++) {
            buf[x] = palette[0];
        }
    }

    // sprites start at line 1
    if (y == 0 || !sprite_rendering) {
      return;
    }

    uint8_t sprites_found = 0;

    // priority goes from last to first
    for(int sprite = 63; sprite >= 0 && sprites_found < 8; sprite--) {
      struct oam_entry* oamsprite = (struct oam_entry*)(oam + sprite*4);
      if (oamsprite->y < y && y <= (oamsprite->y + sprite_height)) {

        // printf("%d %d %d\n", y, sprite, oamsprite->y);

        sprites_found++;

        // 1 - fetch the sprite from CHR
        uint16_t chr_idx = sprite_height == 16 
          ? ((oamsprite->index & 1) << 12) | ((oamsprite->index & 0xFE) * 16)
          : sprite_8x8_pattern_address + oamsprite->index * 16;

        uint8_t* tile = chr + chr_idx;
        
        // 2 - find the exact line we are rendering
        uint8_t line = y - oamsprite->y - 1;
        if(oamsprite->attr & (1 << 7)) { // flip vertically
          line = sprite_height - line - 1;
        }

        if (line > 7) {
          // tall sprite, lines 8-15 come from next tile
          line-=8;
          tile += 16;
        }

        uint8_t right_plane_line = tile[line];
        uint8_t left_plane_line = tile[line + 8];

        // 3 - find the desired palette from attributes
        uint8_t* sprite_palette = palette + 16 + (oamsprite->attr & 3) * 4;

        // 4 - apply other attributes (avoid recalculating them for every column)
        bool background_priority = oamsprite->attr & (1 << 5);
        bool flip_horizontally = oamsprite->attr & (1 << 6);

        // 5 - draw the sprite
        for (int x = 0; x < 8; x++) {
          int screen_x = oamsprite->x + (flip_horizontally ? (7-x) : x);
          if (screen_x >= 256) {
            continue;
          }
          
          uint8_t palette_idx = pixel_palette_idx(right_plane_line, left_plane_line, x);
          if (palette_idx) {
            // TODO: background priority
            uint8_t pixel_color = sprite_palette[palette_idx];
            buf[screen_x] = pixel_color;
          }
        }
      }
    }

    // TODO ppumask
}

inline uint8_t nes_ppu_command(uint16_t address, uint8_t data, bool write) {
    switch(address) {
        case 0x2000:
            // PPUCTRL
            if (write) {
                scroll_x_high = (data & 1) ? 256 : 0;
                scroll_y_high = (data & 2) ? 256 : 0;
                vram_step = data & (1 << 2) ? 32 : 1;
                sprite_8x8_pattern_address = data & (1 << 3) ? 0x1000 : 0;
                background_pattern_address = data & (1 << 4) ? 0x1000 : 0;
                sprite_height = data & (1 << 5) ? 16 : 8;
                nmi_output = data & (1 << 7);
            }
            break;
        case 0x2001:
            // PPUMASK
            if (write) {
                grayscale = data & 1;
                left8pixels_background = data & (1 << 1);
                left8pixels_sprites = data & (1 << 2);
                background_rendering = data & (1 << 3);
                sprite_rendering = data & (1 << 4);
                // TODO swap R/G in PAL? 
                emphasize_r = data & (1 << 5);
                emphasize_g = data & (1 << 6);
                emphasize_b = data & (1 << 7);
            }
            break;
        case 0x2002:
            // PPUSTATUS
            if (!write) {
                vblank = 0;
            }
            write_latch = false;
            break;
        case 0x2003:
            // OAMADDR
            if (write) {
                oam_addr = data;
            }
            break;
        case 0x2004:
            // OAMDATA
            if (write) {
                oam[oam_addr] = data;
                oam_addr++;
            }
            break;
        case 0x2005:
            // PPUSCROLL
            if (!write_latch) {
                scroll_x = data;
                write_latch = true;
            } else {
                scroll_y = data;
                write_latch = false;
            }
            break;
            break;
        case 0x2006:
            // PPUADDR
            if (!write_latch) {
                vram_addr = data << 8;
                write_latch = true;
            } else {
                vram_addr |= data;
                write_latch = false;
            }
            break;
        case 0x2007:
            // PPUDATA
            if (write) {
                if (vram_addr < 0x2000) {
                    chr[vram_addr] = data;
                } else if (vram_addr < 0x3000) {
                    // TODO horizonal mirroring
                    uint16_t index = (vram_addr - 0x2000) % 2048;
                    vram[index] = data;
                } else if (vram_addr < 0x3F00) {
                    // weird. this shouldn't be unused
                } else {
                    uint16_t index = (vram_addr - 0x3F00) % 32;
                    palette[index] = data;
                }
                vram_addr += vram_step;
            }
            break;
    }
    return data;
}