#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Tamanho do display
#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_PAGES     (OLED_HEIGHT / 8)
#define OLED_I2C_ADDR  0x3C
#define OLED_I2C_FREQ  400000

// Pinos da OLED (fixos na BitDogLab)
#define OLED_PIN_SDA   14
#define OLED_PIN_SCL   15

// Estrutura do display
typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t pages;
    uint8_t address;
    i2c_inst_t *i2c_port;
    uint8_t *ram_buffer;
    size_t bufsize;
    uint8_t port_buffer[2];
} ssd1306_t;

// Inicialização, renderização e limpeza
bool oled_init(ssd1306_t *oled);
void oled_clear(ssd1306_t *oled);
void oled_render(ssd1306_t *oled);

// Funções de escrita (providas externamente)
void ssd1306_draw_string(uint8_t *buf, int16_t x, int16_t y, const char *texto, uint8_t width, uint8_t height);

extern ssd1306_t oled;


#endif // OLED_DISPLAY_H

