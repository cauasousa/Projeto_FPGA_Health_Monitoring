// tarefa_word_9.h — Leitura de 9 bits (B0..B8) nas GPIOs indicadas
// Mapa LSB→MSB padrão: B0=GP18, B1=GP19, B2=GP20, B3=GP4, B4=GP9, B5=GP8, B6=GP16, B7=GP17, B8=GP28
// Você pode sobrescrever GPIO_WORD_Bx antes de incluir este header.

#ifndef TAREFA_WORD_9_H
#define TAREFA_WORD_9_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"  // UBaseType_t

#ifdef __cplusplus
extern "C" {
#endif

// --- Mapeamento padrão dos 9 bits (pode ser sobrescrito externamente) ---
#ifndef GPIO_WORD_B0
#define GPIO_WORD_B0 18
#endif
#ifndef GPIO_WORD_B1
#define GPIO_WORD_B1 19
#endif
#ifndef GPIO_WORD_B2
#define GPIO_WORD_B2 20
#endif
#ifndef GPIO_WORD_B3
#define GPIO_WORD_B3 4
#endif
#ifndef GPIO_WORD_B4
#define GPIO_WORD_B4 9
#endif
#ifndef GPIO_WORD_B5
#define GPIO_WORD_B5 8
#endif
#ifndef GPIO_WORD_B6
#define GPIO_WORD_B6 16
#endif
#ifndef GPIO_WORD_B7
#define GPIO_WORD_B7 17
#endif
#ifndef GPIO_WORD_B8
#define GPIO_WORD_B8 28
#endif

// Máscara dos 9 bits válidos (B0..B8)
#define WORD9_MASK 0x01FFu

// Valor publicado pela tarefa de leitura: 9 bits (0..8), ativo-alto, LSB em B0.
extern volatile uint16_t g_word9_value;

// Getter principal (retorna g_word9_value & WORD9_MASK).
uint16_t word9_get(void);

// Cria a tarefa de leitura dos 9 bits.
// - prio: prioridade da task
// - core_mask: afinidade de core (RP2040 SMP)
// - use_pullup: true = habilita pull-up interno em todas as entradas B0..B8
void criar_tarefa_word9(UBaseType_t prio, UBaseType_t core_mask, bool use_pullup);

// --------- Compatibilidade retro ---------
// Mantidos para não quebrar código legado:
static inline uint16_t word6_get(void) { return word9_get(); }
static inline void criar_tarefa_word6(UBaseType_t p, UBaseType_t m, bool u) { criar_tarefa_word9(p, m, u); }

// Helper: extrai bit k (0..8) de um valor.
static inline uint8_t word9_bit(uint16_t v, uint8_t k) {
    return (uint8_t)((v >> (k % 9u)) & 1u);
}

#ifdef __cplusplus
}
#endif

#endif // TAREFA_WORD_9_H
