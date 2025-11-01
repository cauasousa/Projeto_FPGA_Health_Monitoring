#ifndef OLED_CONTEXT_H
#define OLED_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

// Garantir tipos inteiros/booleanos antes de outros includes
#include <stdint.h>
#include <stdbool.h>

// === Correção essencial ===
// FreeRTOS.h deve ser incluído ANTES de semphr.h
#include "FreeRTOS.h"
#include "semphr.h"

#include "oled_display.h" // Define ssd1306_t e funções OLED
#include "tela_display.h" // Define TipoTela

// Variáveis globais (apenas declarações, sem inicialização)
extern volatile TipoTela tela_atual;
extern ssd1306_t oled;

// Mutex para proteger acesso concorrente ao OLED
extern SemaphoreHandle_t mutex_oled;

#ifdef __cplusplus
}
#endif

#endif // OLED_CONTEXT_H
