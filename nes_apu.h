#include <stdint.h>
#include <stdlib.h>

uint8_t length_counter_mapping[] = { 10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
                                     12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

struct pulse_channel {
    uint8_t enabled; // $4015 xxxxxx21 

    // The width of the pulse is controlled by the duty bits in $4000/$4004  
    uint8_t pulse_width; // $4000 DDxxxxxx

    // 1 = Infinite play, 0 = One-shot. If 1, the length counter will be frozen at its current value, and the envelope will repeat forever
    uint8_t infinite_play; // $4000 xxLxxxxx

    // f C is set the volume will be a constant. If clear, an envelope will be used, starting at volume 15 and lowering to 0 over time.
    uint8_t constant_volume; // $4000 xxxCxxxx

    // Sets the direct volume if constant, otherwise controls the rate which the envelope lowers.
    uint8_t volume; // $4000 xxxxVVVV

    uint8_t sweep_enabled; // $4001 Exxxxxxx
    uint8_t sweep_period; // $4001 xPPPxxxx
    uint8_t sweep_negate; // $4001 xxxxNxxx
    uint8_t sweep_shift; // $4001 xxxxxSSS

    uint16_t timer; // high $4003 xxxxTTT, lo $4002 TTTTTTTT
    uint8_t length_counter; // $4003 LLLLLxxx

};

struct triangle_channel {
    uint8_t enabled; // $4015 xxxxxTxx 

    // This bit controls both the length counter and linear counter at the same time.
    // When set this will stop the length counter in the same way as for the pulse/noise channels.
    // When set it prevents the linear counter's internal reload flag from clearing, which effectively halts it if $400B is written after setting C.
    uint8_t infinite_play; // $4008 Cxxxxxxx

    // This reload value will be applied to the linear counter on the next frame counter tick, but only if its reload flag is set
    uint8_t linear_counter_reload; // $4008 xRRRRRRR
    uint8_t linear_counter_reload_flag;
    uint8_t linear_counter; // $4008 xRRRRRRR

    uint16_t timer; // high $400B xxxxTTT, lo $400A TTTTTTTT
    uint8_t length_counter; // $400B LLLLLxxx

    uint16_t timer_count;
    int8_t sequence;
};

struct noise_channel {
    uint8_t enabled; // $4015 xxxxNxxx 

    // 1 = Infinite play, 0 = One-shot. If 1, the length counter will be frozen at its current value, and the envelope will repeat forever
    uint8_t infinite_play; // $4000 xxLxxxxx

    // f C is set the volume will be a constant. If clear, an envelope will be used, starting at volume 15 and lowering to 0 over time.
    uint8_t constant_volume; // $4000 xxxCxxxx

    // Sets the direct volume if constant, otherwise controls the rate which the envelope lowers.
    uint8_t volume; // $4000 xxxxVVVV

    // If bit 7 of $400E is set, the period of the random bit generation is drastically shortened, producing a buzzing tone
    uint8_t noise_loop; // $400E Lxxxxxxx
    
    // The frequency of the noise is determined by a 4-bit value in $400E, which loads a period from a lookup table
    uint8_t noise_period; // $400E xxxxPPPP

    uint8_t length_counter; // $400F LLLLLxxx

};

struct pulse_channel pulse1;
struct pulse_channel pulse2;
struct triangle_channel triangle;
struct noise_channel noise;

uint8_t frame_counter_mode_5_step = 0; // $4017 Mxxxxxxxxx 0 = 4-step, 1 = 5-step
uint16_t frame_counter = 0;
uint8_t frame_counter_step = 0;
uint8_t even_clock = 1;

#define FRAME_COUNTER_STEP 3728 // 4156 pal

inline void apu_clock() {
    if (triangle.enabled) {
        triangle.timer_count--;
        if (triangle.timer_count == 0) {
            triangle.timer_count = triangle.timer;
            triangle.sequence++;
            if (triangle.sequence>15) {
                triangle.sequence = -15;
            }
        }
    }

    if (even_clock) {
        even_clock = 0;

        frame_counter++;
        if (frame_counter == FRAME_COUNTER_STEP) {
            frame_counter_step++;
            if (!(frame_counter_mode_5_step && frame_counter == 4)) {
                // envelope step
                if (triangle.linear_counter_reload_flag) {
                    triangle.linear_counter = triangle.linear_counter_reload;
                    if (!triangle.infinite_play) {
                        triangle.linear_counter_reload_flag = 0;
                    }
                } else if (triangle.linear_counter > 0) {
                    triangle.linear_counter--;
                }

                bool last_step = frame_counter_step >= 4;
                if (frame_counter_step == 2 || last_step) {
                    // length & sweep step
                    if (!pulse1.infinite_play && pulse1.length_counter > 0) {
                        pulse1.length_counter--;
                    }
                    if (!pulse2.infinite_play && pulse2.length_counter > 0) {
                        pulse2.length_counter--;
                    }
                    if (!triangle.infinite_play && triangle.length_counter > 0) {
                        triangle.length_counter--;
                    }
                    if (!noise.infinite_play && noise.length_counter > 0) {
                        noise.length_counter--;
                    }
                }
                if (last_step) {
                    frame_counter_step = 0;
                }
            }
            frame_counter = 0;
        }


    } else {
        even_clock = 1;
    }
}

uint8_t apu_sample() {
    uint8_t tnd_out = 0;

    /*
    output = pulse_out + tnd_out
    
    pulse_out = 0.00752 * (pulse1 + pulse2)
    
    tnd_out = 0.00851 * triangle + 0.00494 * noise + 0.00335 * dmc
    */    
    if (triangle.enabled && (triangle.length_counter > 0 && triangle.linear_counter > 0)) {
        tnd_out = abs(triangle.sequence) * 16;
    }

    return tnd_out;
}

inline uint8_t nes_apu_command(uint16_t address, uint8_t data, bool write) {

    switch(address) {
        // pulse 1
        case 0x4000:
            pulse1.pulse_width = data >> 6;
            pulse1.infinite_play = (data >> 5) & 1;
            pulse1.constant_volume = (data >> 4) & 1;
            pulse1.volume = data & 0xF;
            break;
        case 0x4001:
            pulse1.sweep_enabled = data >> 7;
            pulse1.sweep_period = (data >> 4) & 7;
            pulse1.sweep_negate = (data >> 3) & 1;
            pulse1.sweep_shift = data & 7;
            break;
        case 0x4002:
            pulse1.timer = (pulse1.timer & 0xFF00) | data;
            break;
        case 0x4003:
            pulse1.timer = ((data & 7) << 8) | (pulse1.timer & 0xFF);
            pulse1.length_counter = length_counter_mapping[data >> 3];
            break;

        // pulse 2
        case 0x4004:
            pulse2.pulse_width = data >> 6;
            pulse2.infinite_play = (data >> 5) & 1;
            pulse2.constant_volume = (data >> 4) & 1;
            pulse2.volume = data & 0xF;
            break;
        case 0x4005:
            pulse2.sweep_enabled = data >> 7;
            pulse2.sweep_period = (data >> 4) & 7;
            pulse2.sweep_negate = (data >> 3) & 1;
            pulse2.sweep_shift = data & 7;
            break;
        case 0x4006:
            pulse2.timer = (pulse2.timer & 0xFF00) | data;
            break;
        case 0x4007:
            pulse2.timer = ((data & 7) << 8) | (pulse2.timer & 0xFF);
            pulse2.length_counter = length_counter_mapping[data >> 3];
            break;

        // triangle
        case 0x4008:
            triangle.infinite_play = (data >> 7) & 1;
            triangle.linear_counter_reload = data & 0b01111111;
            break;
        case 0x400A:
            triangle.timer = (triangle.timer & 0xFF00) | data;
            break;
        case 0x400B:
            triangle.timer = ((data & 7) << 8) | (triangle.timer & 0xFF);
            triangle.length_counter = length_counter_mapping[data >> 3];
            triangle.timer_count = triangle.timer;
            triangle.linear_counter_reload_flag = 1;
            break;

        // noise
        case 0x400C:
            noise.infinite_play = (data >> 5) & 1;
            noise.constant_volume = (data >> 4) & 1;
            noise.volume = data & 0xF;
            break;

        case 0x400E:
            noise.noise_loop = (data >> 7) & 1;
            noise.noise_period = data & 0xF;
            break;

        case 0x400F:
            noise.length_counter = length_counter_mapping[data >> 3];
            break;

        case 0x4015:
            if (write) {
                pulse1.enabled = data & 1;
                pulse1.length_counter = 0;
                pulse2.enabled = (data >> 1) & 1;
                pulse2.length_counter = 0;
                triangle.enabled = (data >> 2) & 1;
                triangle.length_counter = 0;
                triangle.sequence = -15;
                triangle.linear_counter_reload_flag = 0;
                noise.enabled = (data >> 3) & 1;
                noise.length_counter = 0;
                break;
            } else {
                return 
                    (pulse1.length_counter > 0) |
                    ((pulse2.length_counter > 0) >> 1) |
                    ((triangle.length_counter > 0) >> 2) |
                    ((noise.length_counter > 0) >> 3);
            }

        case 0x4017:
            frame_counter_mode_5_step = data >> 7;
            // Writing to $4017 with bit 7 set ($80) will immediately clock all of its controlled
            // units at the beginning of the 5-step sequence; with bit 7 clear, only the sequence 
            // is reset without clocking any of its units.
            if (frame_counter_mode_5_step) {
                frame_counter = 0;
                frame_counter_step = 0;
                even_clock = 1;
            }
            frame_counter = 0;
            break;

    }
    return 0;
}
