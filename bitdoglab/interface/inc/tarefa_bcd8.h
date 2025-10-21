// tarefa_bcd8.h — Converte 8 LSB (B0..B7) de g_word9_value em 2 dígitos BCD

#ifndef TAREFA_BCD8_H
#define TAREFA_BCD8_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"  // UBaseType_t

#ifdef __cplusplus
extern "C" {
#endif

// Publicações globais (consumidas por outras tasks, ex.: OLED)
extern volatile uint8_t g_bcd_tens;      // 0..9 quando válido
extern volatile uint8_t g_bcd_units;     // 0..9 quando válido
extern volatile uint8_t g_bcd_decimal;   // 0..99 quando válido; 0xFF quando inválido
extern volatile bool    g_bcd_valid;     // true se ambos nibbles ∈ [0..9]

// Cria a tarefa de conversão BCD8.
// Depende de tarefa_word_9 (word9_get/g_word9_value) já em execução.
void criar_tarefa_bcd8(UBaseType_t prio, UBaseType_t core_mask);

// Getters opcionais (segurança de máscara e tipo)
static inline uint8_t bcd8_get_dezena(void) { return g_bcd_tens; }
static inline uint8_t bcd8_get_unidade(void) { return g_bcd_units; }
static inline uint8_t bcd8_get_decimal(void) { return g_bcd_decimal; }
static inline bool    bcd8_is_valido(void)   { return g_bcd_valid; }

#ifdef __cplusplus
}
#endif

#endif // TAREFA_BCD8_H
