#include "../nes_apu.h"
#include <stdio.h>

#include <math.h>
#include <stdint.h>
#include <string.h>


struct wav_header {
  char riff[4];           /* "RIFF"                                  */
  int32_t flength;        /* file length in bytes                    */
  char wave[4];           /* "WAVE"                                  */
  char fmt[4];            /* "fmt "                                  */
  int32_t chunk_size;     /* size of FMT chunk in bytes (usually 16) */
  int16_t format_tag;     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM */
  int16_t num_chans;      /* 1=mono, 2=stereo                        */
  int32_t srate;          /* Sampling rate in samples per second     */
  int32_t bytes_per_sec;  /* bytes per second = srate*bytes_per_samp */
  int16_t bytes_per_samp; /* 2=16-bit mono, 4=16-bit stereo          */
  int16_t bits_per_samp;  /* Number of bits per sample               */
  char data[4];           /* "data"                                  */
  int32_t dlength;        /* data length in bytes (filelength - 44)  */
};

void save_wav(const char* filename, uint8_t* buffer, uint32_t size) {

  struct wav_header wavh;

  strncpy(wavh.riff, "RIFF", 4);
  strncpy(wavh.wave, "WAVE", 4);
  strncpy(wavh.fmt, "fmt ", 4);
  strncpy(wavh.data, "data", 4);

  const int header_length = sizeof(struct wav_header);
  wavh.chunk_size = 16;
  wavh.format_tag = 1;
  wavh.num_chans = 1;
  wavh.srate = 22050;
  wavh.bits_per_samp = 8;
  wavh.bytes_per_sec = wavh.srate;
  wavh.bytes_per_samp = 1;

  wavh.dlength = size;
  wavh.flength = wavh.dlength + header_length;

  FILE *fp = fopen(filename, "w");
  fwrite(&wavh, 1, header_length, fp);
  fwrite(buffer, 1, size, fp);

}

inline uint8_t apu(uint16_t address, uint8_t data) {
    return nes_apu_command(address, data, true);
}

uint8_t main_theme_trg_timer[] =  { 0xAB, 0x53, 0x1D, 0x3A, 0xAB, 0xAB, 0x53, 0x1D, 0xAB, 0x40, 0xFE, 0xD5, 0xAB, 0x40, 0x40, 0xFE, 0xD5, 0xC4, 0xAB, 0x53, 0x1D, 0x3A, 0xAB, 0xAB, 0x53, 0x1D, 0xAB, 0x40, 0x7C, 0xC4, 0x40, 0x7C, 0xC4, 0x3A, 0xFC, 0xC4 };
uint8_t main_theme_trg_length[] = { 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0X01, 0X01, 0X01, 0x00, 0X01, 0x01, 0x01, 0X00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01 };

uint8_t enemy_hit_sq1_timer[] =  {0x35, 0x43, 0x54, 0x6A, 0x6A, 0x86, 0xA9, 0xD5, 0xD5, 0x0D, 0x53, 0xAB, 0xAB, 0x1A, 0xA6, 0x57 };
uint8_t enemy_hit_sq1_length[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0X01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03 };

uint8_t water_splash_noise_period[] =  { 0x08, 0x05, 0x05, 0x0C, 0x0C };

uint8_t game_over_sq0_timer[]  = {0xD5, 0x1D, 0x1D, 0x0D, 0x53, 0x40, 0x1D, 0xFE, 0xE2, 0xD5};
uint8_t game_over_sq0_length[] = {0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00};

uint8_t game_over_sq1_timer[]  = {0xAB, 0x3A, 0x40, 0x1A, 0xA6, 0x80, 0x3A, 0xFC, 0xC4, 0xAB};
uint8_t game_over_sq1_length[] = {0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01};

uint8_t game_over_trg_timer[]  = {0xAB, 0x53, 0x3A, 0x53, 0xC4, 0xC4, 0x7C, 0x7C, 0xAB, 0xAB};
uint8_t game_over_trg_length[] = {0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};


#define BUFFER_SIZE (70 * 368 * 10)
uint8_t buffer[BUFFER_SIZE];
uint32_t buffer_count = 0;

void frame() {

    //  1.789773Mhz / 22.050Khz ~ 81 clocks per sample
    //  22050 / 60 ~ 367 samples per frame

    for (int frame = 0; frame < 10; frame++) {
        for (int sample = 0; sample < 368; sample++) {
            for (int clock = 0; clock < 81; clock++) {
                apu_clock();
            }
            buffer[buffer_count++] = apu_sample();
        }
    }

}

int main() {

    apu(0x4015, 0x0F); // ApuStatus

    apu(0x4000, 0xCA); // Sq0Duty
    apu(0x4001, 0x00); // Sq0Sweep

    apu(0x4004, 0xC4); // Sq1Duty
    apu(0x4005, 0x00); // Sq1Sweep

    apu(0x4008, 0x20); // TrgLinear

    apu(0x400C, 0x00); // NoiseVolume
    apu(0x400E, 0x00); // NoisePeriod

    // main level theme
    for (int i = 0; i < 36; i++) {
        apu(0x400A, main_theme_trg_timer[i]); 
        apu(0x400B, main_theme_trg_length[i] | 0x10); 
        frame();
    }

    // jump sound
    apu(0x4005, 0xAC); // Sq1Sweep
    apu(0x4006, 0xAB); // Sq1Timer
    apu(0x4007, 0x21); // Sq1Length    
    frame();

    apu(0x4006, 0x7C); // Sq1Timer
    apu(0x4007, 0x21); // Sq1Length    
    frame();

    apu(0x4005, 0xB3); // Sq1Sweep
    apu(0x4006, 0x57); // Sq1Timer
    apu(0x4007, 0x23); // Sq1Length    
    frame();


    // enemy hit 
    apu(0x4005, 0xB3); // Sq1Sweep

    for (int i = 0; i < 16; i++) {
        apu(0x4006, enemy_hit_sq1_timer[i]); 
        apu(0x4007, enemy_hit_sq1_length[i] | 0x20); 
        frame();
    }

    // water splash 
    for (int i = 0; i < 5; i++) {
        apu(0x400C, 0x0A); // NoiseVolume 
        apu(0x400E, water_splash_noise_period[i]); 
        apu(0x400F, 0x40); // NoiseLength 
        frame();
    }

    for (int i = 0; i < 10; i++) {
        apu(0x4002, game_over_sq0_timer[i]); 
        apu(0x4003, game_over_sq0_length[i] | 0x20); 
        apu(0x4006, game_over_sq1_timer[i]); 
        apu(0x4007, game_over_sq1_length[i] | 0x20); 
        apu(0x400A, game_over_trg_timer[i]); 
        apu(0x400B, game_over_trg_length[i] | 0x10); 
        apu(0x400F, 0x01); // NoiseLength 
        frame();
    }
    
    save_wav("aputest.wav", buffer, BUFFER_SIZE);

    return 0;
}



