#include <Arduino.h>
#include "pins.h"
#include "font.h"
#include <LittleFS.h>

#include "esp_task_wdt.h"

#define AUDIO_PIN   33

#include "video_out.h"
#include "nes_ppu.h"

volatile uint32_t command_buffer[COMMAND_BUFFER_SIZE];
volatile uint32_t command_buffer_write_index = 0xFFFFFFFF; // we increment first, then write
volatile uint32_t command_buffer_read_index = 0xFFFFFFFF;



// this is dark magic that results of running this https://github.com/rossumur/esp_8_bit/blob/55eb7b86eda290d96b02c38c3e787efb8ae6a8c0/src/emu_nofrendo.cpp#L127 
uint32_t nes_4_phase[64] = {
    0x2C2C2C2C,0x241D1F26,0x221D2227,0x1F1D2426,0x1D1F2624,0x1D222722,0x1D24261F,0x1F26241D,
    0x2227221D,0x24261F1D,0x26241D1F,0x27221D22,0x261F1D24,0x14141414,0x14141414,0x14141414,
    0x38383838,0x2C25272E,0x2A252A2F,0x27252C2E,0x25272E2C,0x252A2F2A,0x252C2E27,0x272E2C25,
    0x2A2F2A25,0x2C2E2725,0x2E2C2527,0x2F2A252A,0x2E27252C,0x1F1F1F1F,0x15151515,0x15151515,
    0x45454545,0x3A33353C,0x3732373C,0x35333A3C,0x33353C3A,0x32373C37,0x333A3C35,0x353C3A33,
    0x373C3732,0x3A3C3533,0x3C3A3335,0x3C373237,0x3C35333A,0x2B2B2B2B,0x16161616,0x16161616,
    0x45454545,0x423B3D44,0x403B4045,0x3D3B4244,0x3B3D4442,0x3B404540,0x3B42443D,0x3D44423B,
    0x4045403B,0x42443D3B,0x44423B3D,0x45403B40,0x443D3B42,0x39393939,0x17171717,0x17171717,
};

// PAL yuyv table, must be in RAM
uint32_t _nes_yuv_4_phase_pal[] = {
    0x31313131,0x2D21202B,0x2720252D,0x21212B2C,0x1D23302A,0x1B263127,0x1C293023,0x202B2D22,
    0x262B2722,0x2C2B2122,0x2F2B1E23,0x31291F27,0x30251F2A,0x18181818,0x19191919,0x19191919,
    0x3D3D3D3D,0x34292833,0x2F282D34,0x29283334,0x252B3732,0x232E392E,0x2431382B,0x28333429,
    0x2D342F28,0x33342928,0x3732252A,0x392E232E,0x382B2431,0x24242424,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x42373540,0x3C373B40,0x36374040,0x3337433F,0x3139433B,0x323D4338,0x35414237,
    0x3B423D35,0x41413736,0x453F3238,0x473C313B,0x4639323F,0x2F2F2F2F,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x48413D45,0x42404345,0x3D3F4644,0x3B3D4543,0x3B3E4542,0x3B42453F,0x3E47463E,
    0x434A453E,0x46483E3D,0x4843393E,0x4A403842,0x4B403944,0x3E3E3E3E,0x1B1B1B1B,0x1B1B1B1B,
    //odd
    0x31313131,0x20212D2B,0x2520272D,0x2B21212C,0x30231D2A,0x31261B27,0x30291C23,0x2D2B2022,
    0x272B2622,0x212B2C22,0x1E2B2F23,0x1F293127,0x1F25302A,0x18181818,0x19191919,0x19191919,
    0x3D3D3D3D,0x28293433,0x2D282F34,0x33282934,0x372B2532,0x392E232E,0x3831242B,0x34332829,
    0x2F342D28,0x29343328,0x2532372A,0x232E392E,0x242B3831,0x24242424,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x35374240,0x3B373C40,0x40373640,0x4337333F,0x4339313B,0x433D3238,0x42413537,
    0x3D423B35,0x37414136,0x323F4538,0x313C473B,0x3239463F,0x2F2F2F2F,0x1A1A1A1A,0x1A1A1A1A,
    0x49494949,0x3D414845,0x43404245,0x463F3D44,0x453D3B43,0x453E3B42,0x45423B3F,0x46473E3E,
    0x454A433E,0x3E48463D,0x3943483E,0x38404A42,0x39404B44,0x3E3E3E3E,0x1B1B1B1B,0x1B1B1B1B,
};

static const uint8_t dataPins[] = {PIN_D0, PIN_D1, PIN_D2, PIN_D3, PIN_D4, PIN_D5, PIN_D6, PIN_D7};
static const uint8_t addressPins[] = {PIN_A0, PIN_A1, PIN_A2};

extern void* ld_include_xt_nmi;

uint8_t** back_buffer_lines;

volatile bool new_frame = false;
volatile bool new_frame_ready = false;

volatile uint32_t frame_count = 0;
volatile uint32_t isr_frames = 0;

void on_frame() {    
    // this is on ISR so we don't do much work here
    if (new_frame_ready) {
      new_frame_ready = false;
      uint8_t** current = _lines;
      _lines = back_buffer_lines;
      back_buffer_lines = current;
    }
    isr_frames++;
    new_frame = true;
}

char* msg = "Welcome to EsPPU";
void render_welcome() {
    int len = strlen(msg);
    int center = (42 - len) / 2;
    for(int x = 0; x < len; x++) {
      draw_char(_lines, msg[x], x+center, 15, 0x30, 0x0E);
      draw_char(back_buffer_lines, msg[x], x+center, 15, 0x30, 0x0E);
    }
}


void setup() {
    setCpuFrequencyMhz(240);
    Serial.begin(115200);
    Serial.println("EsPPU");

    pinMode(PIN_RW, INPUT);
    pinMode(PIN_INT, OUTPUT_OPEN_DRAIN);
    digitalWrite(PIN_INT, HIGH);
    
    for (uint8_t pin = 0; pin < 8; pin++) {
      pinMode(dataPins[pin], INPUT);
    }

    for (uint8_t pin = 0; pin < 3; pin++) {
      pinMode(addressPins[pin], INPUT);
    }

    pinMode(PIN_CS, INPUT);

    pinMode(PIN_CLK, INPUT);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_CLK),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    gpio_config(&io_conf);

    uint8_t* _front_buffer = (uint8_t*)calloc(240*256, 1);
    uint8_t* _back_buffer = (uint8_t*)calloc(240*256, 1);
    _lines = (uint8_t**) malloc(240 * sizeof(uint8_t*));
    back_buffer_lines = (uint8_t**) malloc(240 * sizeof(uint8_t*));
    for (int y = 0; y < 240; y++) {
      _lines[y] = _front_buffer + y*256;
      back_buffer_lines[y] = _back_buffer + y*256;
    }

    if (!LittleFS.begin(false)) {
      msg = "LittleFS Mount Failed";
      Serial.println(msg);
    }

    File chr_file = LittleFS.open("/test.chr");

    if (!chr_file) {
      msg = "can't open test.chr";
      Serial.println(msg);
    }

    int x = 0;
    while(chr_file.available()) {
      chr[x++] = chr_file.read();
    }

    render_welcome();
    new_frame_ready = true;

    video_init(nes_4_phase, 64, true);

    xTaskCreatePinnedToCore(render, "render", 1024, NULL, configMAX_PRIORITIES - 1, NULL, 0);

}

void render_new_frame() {

  if (!(sprite_rendering || background_rendering)) {
    render_welcome();
    return;
  }

  for (int y = 0; y < 240; y++) {
    nes_ppu_scanline(back_buffer_lines[y], y);
  }

}

void loop() {

  Serial.println("starting bus read");

  // the 25 comes from here https://github.com/espressif/esp-idf/blob/0f0068fff3ab159f082133aadfa9baf4fc0c7b8d/components/esp_hw_support/port/esp32/esp_cpu_intr.c#L170 
  intr_matrix_set(xPortGetCoreID(), ETS_GPIO_INTR_SOURCE, 25);
  ESP_INTR_ENABLE( 25 );

  for(;;) {

    if (command_buffer_read_index != command_buffer_write_index) {

      // TODO: we've got to do something about read commands, we can't answer whenver we want...

      uint32_t reg = command_buffer[(++command_buffer_read_index) % COMMAND_BUFFER_SIZE];

      uint16_t address = 0x2000
        | (reg >> PIN_A0 & 1)
        | ((reg >> PIN_A1 & 1) << 1)
        | ((reg >> PIN_A2 & 1) << 2);

      uint8_t data = 
        (reg >> PIN_D0 & 1)
        | ((reg >> PIN_D1 & 1) << 1)
        | ((reg >> PIN_D2 & 1) << 2)
        | ((reg >> PIN_D3 & 1) << 3)
        | ((reg >> PIN_D4 & 1) << 4)
        | ((reg >> PIN_D5 & 1) << 5)
        | ((reg >> PIN_D6 & 1) << 6)
        | ((reg >> PIN_D7 & 1) << 7);  

      bool write = (reg & (1 << PIN_RW));

      nes_ppu_command(address, data, write);

    }
    if (nmi_clear) {
      nmi_clear = false;
      digitalWrite(PIN_INT, HIGH);
    }
  }  

}

void render(void* ignored) {

  esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));

  for(;;) {
    if (new_frame && !new_frame_ready) {

      new_frame = false;

      render_new_frame();
      new_frame_ready = true;

      frame_count++;
      if(isr_frames >= 60) {
        isr_frames = 0;
        Serial.print("buffer ");
        Serial.print(command_buffer_write_index + 1);
        Serial.print(" delta ");
        Serial.print(command_buffer_write_index - command_buffer_read_index);
        Serial.print(" fps ");
        Serial.println(frame_count);
        frame_count = 0;
      }

      if (nmi_output) {
        digitalWrite(PIN_INT, LOW);
      }
    }  

  }
}

/*
void init_intr(void* ignored) {
    vTaskDelete( NULL );
}

void poll_bus(void* ignored) {
  for(;;) {

    uint32_t reg = REG_READ(GPIO_IN_REG);

    if (reg & (1 << PIN_CLK) && !(reg & (1 << PIN_CS))) {

        uint16_t address = 0x2000
          | (reg >> PIN_A0 & 1)
          | ((reg >> PIN_A1 & 1) << 1)
          | ((reg >> PIN_A2 & 1) << 2);

        uint8_t data = 
          (reg >> PIN_D0 & 1)
          | ((reg >> PIN_D1 & 1) << 1)
          | ((reg >> PIN_D2 & 1) << 2)
          | ((reg >> PIN_D3 & 1) << 3)
          | ((reg >> PIN_D4 & 1) << 4)
          | ((reg >> PIN_D5 & 1) << 5)
          | ((reg >> PIN_D6 & 1) << 6)
          | ((reg >> PIN_D7 & 1) << 7);  

        bool write = (reg & (1 << PIN_RW));

        nes_ppu_command(address, data, write);

        command_buffer_read_index++;

      }    
    }
}
*/