#ifndef OLED_ANIMACOES_H
#define OLED_ANIMACOES_H

#include "oled_display.h"

// Anima números da direita para a esquerda (efeito progressivo)
void oled_animar_entrada_direita(ssd1306_t *oled, uint16_t numero, uint8_t x_inicial, uint8_t espacamento, uint32_t delay_ms);

#endif
