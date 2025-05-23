// Mostly copied from https://github.com/rossumur/esp_8_bit/blob/master/src/video_out.h

/* Copyright (c) 2020, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

int _pal_ = 0;

#ifdef ESP_PLATFORM
#include "esp_types.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "esp_err.h"
#include "soc/gpio_reg.h"
#include "soc/rtc.h"
#include "soc/soc.h"
#include "soc/i2s_struct.h"
#include "soc/i2s_reg.h"
#include "soc/ledc_struct.h"
#include "soc/rtc_io_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "rom/lldesc.h"
#include "driver/periph_ctrl.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "driver/i2s.h"

//====================================================================================================
//====================================================================================================
//
// low level HW setup of DAC/DMA/APLL/PWM
//

lldesc_t _dma_desc[4] = {0};
intr_handle_t _isr_handle;

extern "C"
void IRAM_ATTR video_isr(const volatile void* buf);

extern "C"
void IRAM_ATTR on_frame();

#ifdef APU
extern "C"
uint8_t IRAM_ATTR get_audio_sample();
#endif

// simple isr
void IRAM_ATTR i2s_intr_handler_video(void *arg)
{
    if (I2S0.int_st.out_dscr_err) {
        Serial.println("out_dscr_err");
    }    
    if (I2S0.int_st.out_eof)
        video_isr(((lldesc_t*)I2S0.out_eof_des_addr)->buf); // get the next line of video
    I2S0.int_clr.val = I2S0.int_st.val;                     // reset the interrupt
}

static esp_err_t start_dma(int line_width,int samples_per_cc, int ch = 1)
{
    periph_module_enable(PERIPH_I2S0_MODULE);

    // setup interrupt
    if (esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
        i2s_intr_handler_video, 0, &_isr_handle) != ESP_OK)
        return -1;

    // reset conf
    I2S0.conf.val = 1;
    I2S0.conf.val = 0;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_mono = (ch == 2 ? 0 : 1);

    I2S0.conf2.lcd_en = 1;
    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.sample_rate_conf.tx_bits_mod = 16;
    I2S0.conf_chan.tx_chan_mod = (ch == 2) ? 0 : 1;

    // Create TX DMA buffers
    for (int i = 0; i < 2; i++) {
        int n = line_width*2*ch;
        if (n >= 4092) {
            printf("DMA chunk too big:%d\n",n);
            return -1;
        }
        _dma_desc[i].buf = (uint8_t*)heap_caps_calloc(1, n, MALLOC_CAP_DMA);
        if (!_dma_desc[i].buf)
            return -1;
        
        _dma_desc[i].owner = 1;
        _dma_desc[i].eof = 1;
        _dma_desc[i].length = n;
        _dma_desc[i].size = n;
        _dma_desc[i].empty = (uint32_t)(i == 1 ? _dma_desc : _dma_desc+1);
    }
    I2S0.out_link.addr = (uint32_t)_dma_desc;

    //  Setup up the apll: See ref 3.2.7 Audio PLL
    //  f_xtal = (int)rtc_clk_xtal_freq_get() * 1000000;
    //  f_out = xtal_freq * (4 + sdm2 + sdm1/256 + sdm0/65536); // 250 < f_out < 500
    //  apll_freq = f_out/((o_div + 2) * 2)
    //  operating range of the f_out is 250 MHz ~ 500 MHz
    //  operating range of the apll_freq is 16 ~ 128 MHz.
    //  select sdm0,sdm1,sdm2 to produce nice multiples of colorburst frequencies

    //  see calc_freq() for math: (4+a)*10/((2 + b)*2) mhz
    //  up to 20mhz seems to work ok:
    //  rtc_clk_apll_enable(1,0x00,0x00,0x4,0);   // 20mhz for fancy DDS

    if (!_pal_) {
        switch (samples_per_cc) {
            case 3: rtc_clk_apll_enable(1,0x46,0x97,0x4,2);   break;    // 10.7386363636 3x NTSC (10.7386398315mhz)
            case 4: rtc_clk_apll_enable(1,0x46,0x97,0x4,1);   break;    // 14.3181818182 4x NTSC (14.3181864421mhz)
        }
    } else {
        #ifdef PAL_N
          rtc_clk_apll_enable(1,0xD1,0x98,0x04,1);     // 14.32823181mhz ~4x PAL-N
        #else
          rtc_clk_apll_enable(1,0x04,0xA4,0x6,1);     // 17.734476mhz ~4x PAL
        #endif
    }

/*
    if (!_pal_) {
        switch (samples_per_cc) {
            case 3: rtc_clk_apll_coeff_set(2,0x46,0x97,0x4);   break;    // 10.7386363636 3x NTSC (10.7386398315mhz)
            case 4: rtc_clk_apll_coeff_set(1,0x46,0x97,0x4);   break;    // 14.3181818182 4x NTSC (14.3181864421mhz)
        }
    } else {
        rtc_clk_apll_coeff_set(1,0x04,0xA4,0x6);     // 17.734476mhz ~4x PAL
    }

    rtc_clk_apll_enable(true);
*/


    I2S0.clkm_conf.clkm_div_num = 1;            // I2S clock divider’s integral value.
    I2S0.clkm_conf.clkm_div_b = 0;              // Fractional clock divider’s numerator value.
    I2S0.clkm_conf.clkm_div_a = 1;              // Fractional clock divider’s denominator value
    I2S0.sample_rate_conf.tx_bck_div_num = 1;
    I2S0.clkm_conf.clka_en = 1;                 // Set this bit to enable clk_apll.
    I2S0.fifo_conf.tx_fifo_mod = (ch == 2) ? 0 : 1; // 32-bit dual or 16-bit single channel data

    dac_output_enable(DAC_CHANNEL_1);           // DAC, video on GPIO25
    dac_i2s_enable();                           // start DAC!

    I2S0.conf.tx_start = 1;                     // start DMA!
    I2S0.int_clr.val = 0xFFFFFFFF;
    I2S0.int_ena.out_eof = 1;
    I2S0.out_link.start = 1;
    return esp_intr_enable(_isr_handle);        // start interruprs!
}

void video_init_hw(int line_width, int samples_per_cc)
{
    // setup apll 4x NTSC or PAL colorburst rate
    if (start_dma(line_width,samples_per_cc,1) != 0) {
      Serial.println("can't start DMA");
    }

    // Now ideally we would like to use the decoupled left DAC channel to produce audio
    // But when using the APLL there appears to be some clock domain conflict that causes
    // nasty digitial spikes and dropouts. You are also limited to a single audio channel.
    // So it is back to PWM/PDM and a 1 bit DAC for us. Good news is that we can do stereo
    // if we want to and have lots of different ways of doing nice noise shaping etc.

    // PWM audio out of pin 33 -> can be anything
    // lots of other ways, PDM by hand over I2S1, spi circular buffer etc
    // but if you would like stereo the led pwm seems like a fine choice
    // needs a simple rc filter (1k->1.2k resistor & 10nf->15nf cap work fine)

    // 33 ----/\/\/\/----|------- a out
    //          1k       |
    //                  ---
    //                  --- 10nf
    //                   |
    //                   v gnd

#ifdef APU
    ledcSetup(0,124800,8);    // 15600 hz * 8 bit = 124800
     ledcAttachPin(PIN_AOUT, 0);
    // ledcAttach(PIN_AOUT, 124800,8);
    ledcWrite(0,0);
#endif
}

// send an audio sample every scanline (15720hz for ntsc, 15600hz for PAL)
inline void IRAM_ATTR audio_sample(uint8_t s)
{
    auto& reg = LEDC.channel_group[0].channel[0];
    reg.duty.duty = s; // 25 bit (21.4)
    reg.conf0.sig_out_en = 1; // This is the output enable control bit for channel
    reg.conf1.duty_start = 1; // When duty_num duty_cycle and duty_scale has been configured. these register won't take effect until set duty_start. this bit is automatically cleared by hardware
    reg.conf0.clk_en = 1;
}

//  Appendix

/*
static
void calc_freq(double f)
{
    f /= 1000000;
    printf("looking for sample rate of %fmhz\n",(float)f);
    int xtal_freq = 40;
    for (int o_div = 0; o_div < 3; o_div++) {
        float f_out = 4*f*((o_div + 2)*2);          // 250 < f_out < 500
        if (f_out < 250 || f_out > 500)
            continue;
        int sdm = round((f_out/xtal_freq - 4)*65536);
        float apll_freq = 40 * (4 + (float)sdm/65536)/((o_div + 2)*2);    // 16 < apll_freq < 128 MHz
        if (apll_freq < 16 || apll_freq > 128)
            continue;
        printf("f_out:%f %d:0x%06X %fmhz %f\n",f_out,o_div,sdm,apll_freq/4,f/(apll_freq/4));
    }
    printf("\n");
}

static void freqs()
{
    calc_freq(PAL_FREQUENCY*3);
    calc_freq(PAL_FREQUENCY*4);
    calc_freq(NTSC_FREQUENCY*3);
    calc_freq(NTSC_FREQUENCY*4);
    calc_freq(20000000);
}
*/

extern "C"
void* MALLOC32(int x, const char* label)
{
    printf("MALLOC32 %d free, %d biggest, allocating %s:%d\n",
      heap_caps_get_free_size(MALLOC_CAP_32BIT),heap_caps_get_largest_free_block(MALLOC_CAP_32BIT),label,x);
    void * r = heap_caps_malloc(x,MALLOC_CAP_32BIT);
    if (!r) {
        printf("MALLOC32 FAILED allocation of %s:%d!!!!####################\n",label,x);
        esp_restart();
    }
    else
        printf("MALLOC32 allocation of %s:%d %08X\n",label,x,r);
    return r;
}

#else

//====================================================================================================
//====================================================================================================
//  Simulator
//

#define IRAM_ATTR
#define DRAM_ATTR

void video_init_hw(int line_width, int samples_per_cc);

uint32_t xthal_get_ccount() {
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return lo;
    //return ((uint64_t)hi << 32) | lo;
}

void audio_sample(uint8_t s);

#endif

//====================================================================================================
//====================================================================================================


uint32_t cpu_ticks()
{
  return xthal_get_ccount();
}

uint32_t us() {
    return cpu_ticks()/240;
}

// Color clock frequency is 315/88 (3.57954545455)
// DAC_MHZ is 315/11 or 8x color clock
// 455/2 color clocks per line, round up to maintain phase
// HSYNCH period is 44/315*455 or 63.55555..us
// Field period is 262*44/315*455 or 16651.5555us

#define IRE(_x)          ((uint32_t)(((_x)+40)*255/3.3/147.5) << 8)   // 3.3V DAC
#define SYNC_LEVEL       IRE(-40)
#define BLANKING_LEVEL   IRE(0)
#define BLACK_LEVEL      IRE(7.5)
#define GRAY_LEVEL       IRE(50)
#define WHITE_LEVEL      IRE(100)


#define P0 (color >> 16)
#define P1 (color >> 8)
#define P2 (color)
#define P3 (color << 8)

uint8_t** _lines; // filled in by emulator
volatile int _line_counter = 0;
volatile int _frame_counter = 0;

int _active_lines;
int _line_count;

int _line_width;
int _samples_per_cc;
const uint32_t* _palette;
uint16_t _palette_size;

float _sample_rate;

int _hsync;
int _hsync_long;
int _hsync_short;
int _burst_start;
int _burst_width;
int _active_start;

int16_t* _burst0 = 0; // pal bursts
int16_t* _burst1 = 0;

static int usec(float us)
{
    uint32_t r = (uint32_t)(us*_sample_rate);
    return ((r + _samples_per_cc)/(_samples_per_cc << 1))*(_samples_per_cc << 1);  // multiple of color clock, word align
}

#define NTSC_COLOR_CLOCKS_PER_SCANLINE 228       // really 227.5 for NTSC but want to avoid half phase fiddling for now
#define NTSC_FREQUENCY (315000000.0/88)
#define NTSC_LINES 262

#define PAL_COLOR_CLOCKS_PER_SCANLINE 284        // really 283.75 ?
#ifdef PAL_N
  #define PAL_FREQUENCY 3582056.25 // PAL-N
#else
  #define PAL_FREQUENCY 4433618.75
#endif
#define PAL_LINES 312



void pal_init();

void video_init(const uint32_t* palette, uint16_t palette_size, bool ntsc)
{
    _samples_per_cc = 4;
    _palette = palette;
    _palette_size = palette_size;

    if (ntsc) {
        _sample_rate = 315.0/88 * _samples_per_cc;   // DAC rate
        _line_width = NTSC_COLOR_CLOCKS_PER_SCANLINE*_samples_per_cc;
        _line_count = NTSC_LINES;
        _hsync_long = usec(63.555-4.7);
        _active_start = usec(_samples_per_cc == 4 ? 10 : 10.5);
        _hsync = usec(4.7);
        _pal_ = 0;
    } else {
        pal_init();
        _pal_ = 1;
    }
    
    _active_lines = 240;
    video_init_hw(_line_width,_samples_per_cc);    // init the hardware
}

//===================================================================================================
//===================================================================================================
// PAL

void pal_init()
{
    int cc_width = 4;
    _sample_rate = PAL_FREQUENCY*cc_width/1000000.0;       // DAC rate in mhz
    _line_width = PAL_COLOR_CLOCKS_PER_SCANLINE*cc_width;
    _line_count = PAL_LINES;
    _hsync_short = usec(2);
    _hsync_long = usec(30);
    _hsync = usec(4.7);
    _burst_start = usec(5.6);
    _burst_width = (int)(10*cc_width + 4) & 0xFFFE;
    _active_start = usec(10.4);

    // make colorburst tables for even and odd lines
    _burst0 = new int16_t[_burst_width];
    _burst1 = new int16_t[_burst_width];
    float phase = 2*M_PI/2;
    for (int i = 0; i < _burst_width; i++)
    {
        _burst0[i] = BLANKING_LEVEL + sin(phase + 3*M_PI/4) * BLANKING_LEVEL/1.5;
        _burst1[i] = BLANKING_LEVEL + sin(phase - 3*M_PI/4) * BLANKING_LEVEL/1.5;
        phase += 2*M_PI/cc_width;
    }
}

void IRAM_ATTR blit_pal(uint8_t* src, uint16_t* dst)
{
    uint32_t c,color;
    bool even = _line_counter & 1;
    const uint32_t* p = even ? _palette : _palette + _palette_size;
    int left = 0;
    int right = 256;
    const uint32_t mask = (_palette_size - 1);
    uint8_t c0,c1,c2,c3,c4;
    uint8_t y1,y2,y3;
    dst += 88;

    // 4 pixels over 3 color clocks, 12 samples
    // do the blitting
    for (int i = left; i < right; i += 4) {
        c = *((uint32_t*)(src+i));
        color = p[c & mask];
        dst[0^1] = P0;
        dst[1^1] = P1;
        dst[2^1] = P2;
        color = p[(c >> 8) & mask];
        dst[3^1] = P3;
        dst[4^1] = P0;
        dst[5^1] = P1;
        color = p[(c >> 16) & mask];
        dst[6^1] = P2;
        dst[7^1] = P3;
        dst[8^1] = P0;
        color = p[(c >> 24) & mask];
        dst[9^1] = P1;
        dst[10^1] = P2;
        dst[11^1] = P3;
        dst += 12;
    }
}

void IRAM_ATTR burst_pal(uint16_t* line)
{
    line += _burst_start;
    int16_t* b = (_line_counter & 1) ? _burst0 : _burst1;
    for (int i = 0; i < _burst_width; i += 2) {
        line[i^1] = b[i];
        line[(i+1)^1] = b[i+1];
    }
}

//===================================================================================================
//===================================================================================================
// ntsc tables
// AA AA                // 2 pixels, 1 color clock - atari
// AA AB BB             // 3 pixels, 2 color clocks - nes
// AAA ABB BBC CCC      // 4 pixels, 3 color clocks - sms

// cc == 3 gives 684 samples per line, 3 samples per cc, 3 pixels for 2 cc
// cc == 4 gives 912 samples per line, 4 samples per cc, 2 pixels per cc

#ifdef PERF
#define BEGIN_TIMING()  uint32_t t = cpu_ticks()
#define END_TIMING() t = cpu_ticks() - t; _blit_ticks_min = min(_blit_ticks_min,t); _blit_ticks_max = max(_blit_ticks_max,t);
#define ISR_BEGIN() uint32_t t = cpu_ticks()
#define ISR_END() t = cpu_ticks() - t;_isr_us += (t+120)/240;
uint32_t _blit_ticks_min = 0;
uint32_t _blit_ticks_max = 0;
uint32_t _isr_us = 0;
#else
#define BEGIN_TIMING()
#define END_TIMING()
#define ISR_BEGIN()
#define ISR_END()
#endif

// draw a line of game in NTSC
void IRAM_ATTR blit(uint8_t* src, uint16_t* dst)
{
    uint32_t* d = (uint32_t*)dst;
    const uint32_t* p = _palette;
    uint32_t color,c;
    uint32_t mask = (_palette_size - 1);
    int i;

    BEGIN_TIMING();
    if (_pal_) {
        blit_pal(src,dst);
        END_TIMING();
        return;
    }


    // AAA ABB BBC CCC
    // 4 pixels, 3 color clocks, 4 samples per cc
    // each pixel gets 3 samples, 192 color clocks wide
    for (i = 0; i < 256; i += 4) {
        c = *((uint32_t*)(src+i));
        color = p[c & mask];
        dst[0^1] = P0;
        dst[1^1] = P1;
        dst[2^1] = P2;
        color = p[(c >> 8) & mask];
        dst[3^1] = P3;
        dst[4^1] = P0;
        dst[5^1] = P1;
        color = p[(c >> 16) & mask];
        dst[6^1] = P2;
        dst[7^1] = P3;
        dst[8^1] = P0;
        color = p[(c >> 24) & mask];
        dst[9^1] = P1;
        dst[10^1] = P2;
        dst[11^1] = P3;
        dst += 12;
    }
    END_TIMING();
}

void IRAM_ATTR burst(uint16_t* line)
{
    if (_pal_) {
        burst_pal(line);
        return;
    }

    int i,phase;
    switch (_samples_per_cc) {
        case 4:
            // 4 samples per color clock
            for (i = _hsync; i < _hsync + (4*10); i += 4) {
                line[i+1] = BLANKING_LEVEL;
                line[i+0] = BLANKING_LEVEL + BLANKING_LEVEL/2;
                line[i+3] = BLANKING_LEVEL;
                line[i+2] = BLANKING_LEVEL - BLANKING_LEVEL/2;
            }
            break;
        case 3:
            // 3 samples per color clock
            phase = 0.866025*BLANKING_LEVEL/2;
            for (i = _hsync; i < _hsync + (3*10); i += 6) {
                line[i+1] = BLANKING_LEVEL;
                line[i+0] = BLANKING_LEVEL + phase;
                line[i+3] = BLANKING_LEVEL - phase;
                line[i+2] = BLANKING_LEVEL;
                line[i+5] = BLANKING_LEVEL + phase;
                line[i+4] = BLANKING_LEVEL - phase;
            }
            break;
    }
}

void IRAM_ATTR sync(uint16_t* line, int syncwidth)
{
    for (int i = 0; i < syncwidth; i++)
        line[i] = SYNC_LEVEL;
}

void IRAM_ATTR blanking(uint16_t* line, bool vbl)
{
    int syncwidth = vbl ? _hsync_long : _hsync;
    sync(line,syncwidth);
    for (int i = syncwidth; i < _line_width; i++)
        line[i] = BLANKING_LEVEL;
    if (!vbl)
        burst(line);    // no burst during vbl
}

// Fancy pal non-interlace
// http://martin.hinner.info/vga/pal.html
void IRAM_ATTR pal_sync2(uint16_t* line, int width, int swidth)
{
    swidth = swidth ? _hsync_long : _hsync_short;
    int i;
    for (i = 0; i < swidth; i++)
        line[i] = SYNC_LEVEL;
    for (; i < width; i++)
        line[i] = BLANKING_LEVEL;
}

uint8_t DRAM_ATTR _sync_type[8] = {0,0,0,3,3,2,0,0};
void IRAM_ATTR pal_sync(uint16_t* line, int i)
{
    uint8_t t = _sync_type[i-304];
    pal_sync2(line,_line_width/2, t & 2);
    pal_sync2(line+_line_width/2,_line_width/2, t & 1);
}

uint8_t _audio_buffer;

// test pattern, must be ram
uint8_t _sin64[64] = {
    0x20,0x22,0x25,0x28,0x2B,0x2E,0x30,0x33,
    0x35,0x37,0x38,0x3A,0x3B,0x3C,0x3D,0x3D,
    0x3D,0x3D,0x3D,0x3C,0x3B,0x3A,0x38,0x37,
    0x35,0x33,0x30,0x2E,0x2B,0x28,0x25,0x22,
    0x20,0x1D,0x1A,0x17,0x14,0x11,0x0F,0x0C,
    0x0A,0x08,0x07,0x05,0x04,0x03,0x02,0x02,
    0x02,0x02,0x02,0x03,0x04,0x05,0x07,0x08,
    0x0A,0x0C,0x0F,0x11,0x14,0x17,0x1A,0x1D,
};
uint8_t _x;

// test the fancy DAC
void IRAM_ATTR test_wave(volatile void* vbuf, int t = 1)
{
    uint16_t* buf = (uint16_t*)vbuf;
    int n = _line_width;
    switch (t) {
        case 0: // f/64 sinewave
            for (int i = 0; i < n; i += 2) {
                buf[0^1] = GRAY_LEVEL + (_sin64[_x++ & 0x3F] << 8);
                buf[1^1] = GRAY_LEVEL + (_sin64[_x++ & 0x3F] << 8);
                buf += 2;
            }
            break;
        case 1: // fast square wave
            for (int i = 0; i < n; i += 2) {
                buf[0^1] = GRAY_LEVEL - (0x10 << 8);
                buf[1^1] = GRAY_LEVEL + (0x10 << 8);
                buf += 2;
            }
            break;
    }
}

// Wait for blanking before starting drawing
// avoids tearing in our unsynchonized world
#ifdef ESP_PLATFORM
void video_sync()
{
  if (!_lines)
    return;
  int n = 0;
  if (_pal_) {
    if (_line_counter < _active_lines)
      n = (_active_lines - _line_counter)*1000/15600;
  } else {
    if (_line_counter < _active_lines)
      n = (_active_lines - _line_counter)*1000/15720;
  }
  vTaskDelay(n+1);
}
#endif

// Workhorse ISR handles audio and video updates
extern "C"
void IRAM_ATTR video_isr(const volatile void* vbuf)
{
    if (!_lines)
        return;

    ISR_BEGIN();

#ifdef APU
    audio_sample(get_audio_sample());
#endif

    int i = _line_counter++;
    uint16_t* buf = (uint16_t*)vbuf;
    if (_pal_) {
        // pal
        if (i < 32) {
            blanking(buf,false);                // pre render/black 0-32
        } else if (i < _active_lines + 32) {    // active video 32-272
            sync(buf,_hsync);
            burst(buf);
            blit(_lines[i-32],buf + _active_start);
        } else if (i < 304) {                   // post render/black 272-304
            if (i < 272)                        // slight optimization here, once you have 2 blanking buffers
                blanking(buf,false);
        } else {
            pal_sync(buf,i);                    // 8 lines of sync 304-312
        }
    } else {
        // ntsc
        if (i < _active_lines) {                // active video
            sync(buf,_hsync);
            burst(buf);
            blit(_lines[i],buf + _active_start);

        } else if (i < (_active_lines + 5)) {   // post render/black
            blanking(buf,false);

        } else if (i < (_active_lines + 8)) {   // vsync
            blanking(buf,true);

        } else {                                // pre render/black
            blanking(buf,false);
        }
    }

    if (_line_counter == _line_count) {
        _line_counter = 0;                      // frame is done
        _frame_counter++;
        on_frame();
    }

    ISR_END();
}