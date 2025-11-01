#ifndef TAREFA_WORD_8_H
#define TAREFA_WORD_8_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"

// Máscara para 8 bits (B0..B7)
#define WORD8_MASK  (0xFFu)

// Agora a palavra é 16-bit (usa 8 bits válidos)
extern volatile uint16_t g_word8_value;

// Getter padronizado (mascara os 8 bits)
static inline uint16_t word8_get(void) {
    return (uint16_t)(g_word8_value & WORD8_MASK);
}

// Assinatura mantida
void criar_tarefa_word_8(UBaseType_t prio, UBaseType_t core_mask, bool use_pullup);

#endif /* TAREFA_WORD8_H */
