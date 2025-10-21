// tarefa_bcd8.c — Converte os 8 LSB (B0..B7) de g_word9_value em dois dígitos BCD
// Fonte dos bits: tarefa_word_9.c (g_word9_value / word9_get)

#include "tarefa_bcd8.h"
#include "tarefa_word_9.h"   // word9_get(), WORD9_MASK

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef BCD8_PERIOD_MS
#define BCD8_PERIOD_MS 50u  // taxa de verificação/atualização
#endif

// Publicações globais
volatile uint8_t g_bcd_tens    = 0;    // 0..9 quando válido
volatile uint8_t g_bcd_units   = 0;    // 0..9 quando válido
volatile uint8_t g_bcd_decimal = 0xFF; // 0..99 se válido; 0xFF se inválido
volatile bool    g_bcd_valid   = false;

static inline bool bcd_nibble_valido(uint8_t nib) {
    return nib < 10u;
}

static void task_bcd8(void *arg) {
    (void)arg;

    // (Opcional) Espera USB para logs
    for (int i = 0; i < 50 && !stdio_usb_connected(); ++i) {
        printf("[BCD8] aguardando USB...\n");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    printf("[BCD8] usando 8 LSB (B0..B7): D7..D4 = dezena, D3..D0 = unidade\n");

    uint8_t ultimo_b = 0xFF;
    const TickType_t dt = pdMS_TO_TICKS(BCD8_PERIOD_MS);

    for (;;) {
        vTaskDelay(dt);

        // Snapshot dos 8 LSB do barramento (alinhado/atômico em 16b no M0+)
        uint8_t b = (uint8_t)(word9_get() & 0xFFu);

        if (b != ultimo_b) {
            ultimo_b = b;

            uint8_t dez = (uint8_t)((b >> 4) & 0x0Fu);
            uint8_t und = (uint8_t)( b       & 0x0Fu);

            bool valido = bcd_nibble_valido(dez) && bcd_nibble_valido(und);

            // Publicações (cada store é de 8 bits; suficiente na prática)
            g_bcd_tens    = dez;
            g_bcd_units   = und;
            g_bcd_valid   = valido;
            g_bcd_decimal = valido ? (uint8_t)(dez * 10u + und) : 0xFFu;

            // Log de depuração
            printf("[BCD8] byte=0x%02X  dez=%u  und=%u  %s  dec=%s\n",
                   b, dez, und, valido ? "OK" : "INV",
                   valido ? "atualizado" : "—");
        }
    }
}

void criar_tarefa_bcd8(UBaseType_t prio, UBaseType_t core_mask) {
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_bcd8, "bcd8", 768, NULL, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
