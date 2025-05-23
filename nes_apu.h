#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct channel {
    uint8_t enabled; // $4015 xxxDNT21
    // 1 = Infinite play, 0 = One-shot. If 1, the length counter will be frozen at its current value, and the envelope will repeat forever
    uint8_t infinite_play;
    uint8_t length_counter; // $4003 LLLLLxxx

    uint16_t timer; // high $4003 xxxxTTT, lo $4002 TTTTTTTT
    int32_t timer_count;
};

struct envelope {

    // f C is set the volume will be a constant. If clear, an envelope will be used, starting at volume 15 and lowering to 0 over time.
    uint8_t constant_volume; // $4000 xxxCxxxx

    // Sets the direct volume if constant, otherwise controls the rate which the envelope lowers.
    uint8_t volume; // $4000 xxxxVVVV

    uint8_t decay;
    uint8_t reset;
    uint8_t divider;

};

struct sweep {
    uint8_t enabled; // $4001 Exxxxxxx
    uint8_t period; // $4001 xPPPxxxx
    uint8_t negate; // $4001 xxxxNxxx
    uint8_t shift; // $4001 xxxxxSSS
    uint8_t divider;
    uint8_t reload;
};

struct pulse_channel {

    struct channel channel;
    struct envelope envelope;
    struct sweep sweep;

    // The width of the pulse is controlled by the duty bits in $4000/$4004  
    uint8_t duty; // $4000 DDxxxxxx
    uint8_t sequence;

};

struct triangle_channel {

    struct channel channel;

    // This reload value will be applied to the linear counter on the next frame counter tick, but only if its reload flag is set
    uint8_t linear_counter_reload; // $4008 xRRRRRRR
    uint8_t linear_counter_reload_flag;
    uint8_t linear_counter; // $4008 xRRRRRRR

    int8_t sequence;

};

struct noise_channel {
    struct channel channel;
    struct envelope envelope;

    // If bit 7 of $400E is set, the period of the random bit generation is drastically shortened, producing a buzzing tone
    uint8_t noise_mode; // $400E Lxxxxxxx
    
    uint16_t shift_register;

};

struct pulse_channel pulse1;
struct pulse_channel pulse2;
struct triangle_channel triangle;
struct noise_channel noise;

uint8_t frame_counter_mode_5_step = 0; // $4017 Mxxxxxxxxx 0 = 4-step, 1 = 5-step
uint16_t frame_counter = 0;
uint8_t frame_counter_step = 0;

uint8_t length_counter_mapping[] = { 10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
                                     12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

#ifndef PAL
uint16_t noise_period[] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
#define FRAME_COUNTER_STEP 3728
#else
uint16_t noise_period[] = { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778 };
#define FRAME_COUNTER_STEP 4156
#endif

const uint8_t pulse_duties[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};


inline uint8_t timer_step(struct channel* channel, uint8_t steps) {

    uint8_t triggers = 0;

    if (channel->enabled && channel->timer > 0) {
        channel->timer_count -= steps;
        while (channel->timer_count <= 0) {
            channel->timer_count += channel->timer;
            triggers++;
        }
    }

    return triggers;
}

inline void length_step(struct channel* channel) {
    if (!channel->infinite_play && channel->length_counter > 0) {
        channel->length_counter--;
    }
}

inline void envelope_step(struct envelope* envelope, uint8_t infinite_play) {
    if (envelope->reset) {
        envelope->reset = 0;
        envelope->decay = 15;
        envelope->divider = envelope->volume;
    } else if (envelope->divider > 0) {
        envelope->divider--;
    } else {
        envelope->divider = envelope->volume;
        if (envelope->decay > 0) {
            envelope->decay--;
        } else if (infinite_play) {
            envelope->decay = 15;
        }
    }
}

inline void sweep_step(struct sweep* sweep, struct channel* channel, int8_t comp1) {
    if(sweep->enabled) {
        if(sweep->divider == 0 && sweep->shift > 0) {
            uint16_t change = channel->timer >> sweep->shift;
            if (sweep->negate) {
                change += comp1;
                if (change > channel->timer) {
                    channel->timer = 0;
                } else {
                    channel->timer -= change;
                }
            } else {
                channel->timer += change;
            }
        }
        if(sweep->reload || sweep->divider == 0) {
            sweep->divider = sweep->period;
            sweep->reload = 0;
        } else {
            sweep->divider--;
        }
    }
}

inline void triangle_linear_counter_step() {
    if (triangle.linear_counter_reload_flag) {
        triangle.linear_counter = triangle.linear_counter_reload;
        if (!triangle.channel.infinite_play) {
            triangle.linear_counter_reload_flag = 0;
        }
    } else if (triangle.linear_counter > 0) {
        triangle.linear_counter--;
    }
}


void apu_clock(uint8_t clocks) {

    // traingle is twice the clock
    uint8_t triangle_steps = timer_step(&triangle.channel, clocks*2);
    if (triangle_steps > 0) {
        triangle.sequence = (triangle.sequence + triangle_steps) % 32;
    }
    
    uint8_t pulse1_steps = timer_step(&pulse1.channel, clocks);
    if (pulse1_steps > 0) {
        pulse1.sequence = (pulse1.sequence + pulse1_steps) % 8;
    }

    uint8_t pulse2_steps = timer_step(&pulse2.channel, clocks);
    if (pulse2_steps > 0) {
        pulse2.sequence = (pulse2.sequence + pulse2_steps) % 8;
    }

    uint8_t noise_steps = timer_step(&noise.channel, clocks);

    while (noise_steps > 0) {
        uint8_t feedback = (noise.shift_register & 1) ^ ((noise.shift_register >> (noise.noise_mode ? 6 : 1)) & 1);
        noise.shift_register = (noise.shift_register >> 1) | (feedback << 14);
        noise_steps--;
    }

    frame_counter += clocks;

    // assuming clocks < FRAME_COUNTER_STEP
    if (frame_counter >= FRAME_COUNTER_STEP) {
        frame_counter -= FRAME_COUNTER_STEP;
        frame_counter_step++;

        if (!(frame_counter_mode_5_step && frame_counter == 4)) {
            if (frame_counter_step >= 4) {
                frame_counter_step = 0;
            }

            envelope_step(&pulse1.envelope, pulse1.channel.infinite_play);
            envelope_step(&pulse2.envelope, pulse2.channel.infinite_play);
            envelope_step(&noise.envelope, noise.channel.infinite_play);

            triangle_linear_counter_step();

            if (frame_counter_step == 0 || frame_counter_step == 2) {
                length_step(&pulse1.channel);
                length_step(&pulse2.channel);
                length_step(&triangle.channel);
                length_step(&noise.channel);
                sweep_step(&pulse1.sweep, &pulse1.channel, 1);
                sweep_step(&pulse2.sweep, &pulse2.channel, 0);
            }
        }

    }

}

inline uint8_t volume(struct envelope* envelope) {
    return envelope->constant_volume ? envelope->volume : envelope->decay;
}

inline uint8_t pulse_sequence_value(struct pulse_channel* pulse) {
    return pulse_duties[pulse->duty][pulse->sequence];
}

uint8_t apu_sample() {
    uint8_t tnd_out = 0;

    /*
    output = pulse_out + tnd_out
    
    pulse_out = 0.00752 * (pulse1 + pulse2)
    
    tnd_out = 0.00851 * triangle + 0.00494 * noise + 0.00335 * dmc
    */ 
    
    uint8_t pulse_out = 0;
    if (pulse1.channel.enabled && (pulse1.channel.length_counter > 0) && (pulse1.channel.timer >= 8)) {
        pulse_out = pulse_sequence_value(&pulse1) * volume(&pulse1.envelope);
    }

    if (pulse2.channel.enabled && (pulse2.channel.length_counter > 0) && (pulse2.channel.timer >= 8)) {
        pulse_out += pulse_sequence_value(&pulse2) * volume(&pulse2.envelope);
    }

    if (triangle.channel.enabled && (triangle.channel.length_counter > 0 && triangle.linear_counter > 0)) {
        int8_t seq = triangle.sequence - 15;
        // the 0 value in the sequence is repeated; so we shift all the positive part to start at 0.
        tnd_out = (seq > 0 ? seq - 1 : -seq) * 3;
    }
    if (noise.channel.enabled && ((noise.channel.length_counter > 0) && (noise.shift_register & 1))) {
        tnd_out += volume(&noise.envelope) * 2;
    }

    return 2*pulse_out + tnd_out;
}

inline void pulse_set_duty(struct pulse_channel* pulse, uint8_t data) {
    pulse->duty = data >> 6;
    pulse->channel.infinite_play = (data >> 5) & 1;
    pulse->envelope.constant_volume = (data >> 4) & 1;
    pulse->envelope.volume = data & 0xF;
}

inline void set_sweep(struct sweep* sweep, uint8_t data) {
    sweep->enabled = data >> 7;
    sweep->period = (data >> 4) & 7;
    sweep->negate = (data >> 3) & 1;
    sweep->shift = data & 7;
    sweep->reload = 1;
}

inline void set_timer_lo(struct channel* channel, uint8_t data) {
    channel->timer = (channel->timer & 0xFF00) | data;
}    

inline void set_timer_hi(struct channel* channel, uint8_t data) {
    channel->timer = ((data & 7) << 8) | (channel->timer & 0xFF);
    channel->length_counter = length_counter_mapping[data >> 3];
    channel->timer_count = channel->timer;
}    

uint8_t nes_apu_command(uint16_t address, uint8_t data, uint8_t write) {

    switch(address) {
        // pulse 1
        case 0x4000:
            pulse_set_duty(&pulse1, data);
            break;
        case 0x4001:
            set_sweep(&pulse1.sweep, data);
            break;
        case 0x4002:
            set_timer_lo(&pulse1.channel, data);
            break;
        case 0x4003:
            set_timer_hi(&pulse1.channel, data);
            pulse1.envelope.reset = 1;
            pulse1.sequence = 0;
            break;

        // pulse 2
        case 0x4004:
            pulse_set_duty(&pulse2, data);
            break;
        case 0x4005:
            set_sweep(&pulse2.sweep, data);
            break;
        case 0x4006:
            set_timer_lo(&pulse2.channel, data);
            break;
        case 0x4007:
            set_timer_hi(&pulse2.channel, data);
            pulse2.envelope.reset = 1;
            pulse2.sequence = 0;
            break;

        // triangle
        case 0x4008:
            triangle.channel.infinite_play = (data >> 7) & 1;
            triangle.linear_counter_reload = data & 0b01111111;
            break;
        case 0x400A:
            set_timer_lo(&triangle.channel, data);
            break;
        case 0x400B:
            set_timer_hi(&triangle.channel, data);
            triangle.linear_counter_reload_flag = 1;
            break;

        // noise
        case 0x400C:
            noise.channel.infinite_play = (data >> 5) & 1;
            noise.envelope.constant_volume = (data >> 4) & 1;
            noise.envelope.volume = data & 0xF;
            break;
        case 0x400E:
            noise.noise_mode = (data >> 7) & 1;
            noise.channel.timer = noise_period[data & 0xF] / 2; // it is expressed in CPU clocks
            noise.channel.timer_count = noise.channel.timer;
            break;
        case 0x400F:
            noise.channel.length_counter = length_counter_mapping[data >> 3];
            noise.envelope.reset = 1;
            break;

        case 0x4015:
            if (write) {
                pulse1.channel.enabled = data & 1;
                pulse1.channel.length_counter = 0;
                pulse2.channel.enabled = (data >> 1) & 1;
                pulse2.channel.length_counter = 0;
                triangle.channel.enabled = (data >> 2) & 1;
                triangle.channel.length_counter = 0;
                triangle.sequence = 0;
                triangle.linear_counter_reload_flag = 0;
                noise.channel.enabled = (data >> 3) & 1;
                noise.channel.length_counter = 0;
                noise.shift_register = 1;
                break;
            } else {
                return 
                    (pulse1.channel.length_counter > 0) |
                    ((pulse2.channel.length_counter > 0) >> 1) |
                    ((triangle.channel.length_counter > 0) >> 2) |
                    ((noise.channel.length_counter > 0) >> 3);
            }

        case 0x4017:
            frame_counter_mode_5_step = data >> 7;
            // Writing to $4017 with bit 7 set ($80) will immediately clock all of its controlled
            // units at the beginning of the 5-step sequence; with bit 7 clear, only the sequence 
            // is reset without clocking any of its units.
            if (frame_counter_mode_5_step) {
                frame_counter = 0;
                frame_counter_step = 0;
            }
            frame_counter = 0;
            break;

    }
    return 0;
}

void apu_init() {
    pulse1.channel.enabled = 0;
    pulse1.channel.length_counter = 0;
    pulse2.channel.enabled = 0;
    pulse2.channel.length_counter = 0;
    triangle.channel.enabled = 0;
    triangle.channel.length_counter = 0;
    triangle.sequence = 0;
    triangle.linear_counter_reload_flag = 0;
    noise.channel.enabled = 0;
    noise.channel.length_counter = 0;
    noise.shift_register = 1;
}
