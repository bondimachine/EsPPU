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

uint16_t nametable_address = 0x2000;
uint16_t sprite_8x8_pattern_address = 0;
uint16_t background_pattern_address = 0;
uint8_t sprite_height = 8;

bool nmi_output = false;
bool nmi_clear = false;

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


inline void nes_ppu_scanline(uint8_t* buf, int y) {

    if(background_rendering) {
        // TODO: background
        // Fetch a nametable entry from nametable_address.
        // Fetch the corresponding attribute table entry from nametable_address + $03C0 and increment the current VRAM address within the same row.
        // Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
        // Fetch the high-order byte of this sliver from an address 8 bytes higher.
        // Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data using priority.
        
        for (int x = 0; x < 256; x++) {
            buf[x] = palette[0];
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
          
          uint8_t palette_idx = (right_plane_line >> (7-x) & 1) << 1 | (left_plane_line >> (7-x) & 1);
          if (palette_idx) {
            // TODO: background priority
            uint8_t pixel_color = sprite_palette[palette_idx];
            buf[screen_x] = pixel_color;
          }
        }
      }
    }

    // TODO ppumask
    // TODO scroll
}

inline uint8_t nes_ppu_command(uint16_t address, uint8_t data, bool write) {
    switch(address) {
        case 0x2000:
            // PPUCTRL
            if (write) {
                nametable_address = 0x2000 + 0x0400 * (data & 3); // this is some shift but I'm being lazy
                vram_step = data & (1 << 2) ? 32 : 1;
                sprite_8x8_pattern_address = data & (1 << 3) ? 0x1000 : 0;
                background_pattern_address = data & (1 << 4) ? 0x1000 : 0;
                sprite_height = data & (1 << 5) ? 16 : 8;
                nmi_output = data & (1 << 7);

                if (!nmi_output) {
                    nmi_clear = false;
                }
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
                nmi_clear = false;
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
            }
            break;
        case 0x2005:
            // PPUSCROLL
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