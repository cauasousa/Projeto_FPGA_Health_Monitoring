// tarefa_display_duplo_bcd.c — Exibe dois números grandes (dezena/unidade) centralizados no OLED
// Fonte: tarefa_bcd8 (g_bcd_tens, g_bcd_units, g_bcd_valid)

#include "tarefa_display_duplo_bcd.h"

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "oled_display.h"
#include "oled_context.h"
#include "ssd1306_text.h"
#include "numeros_grandes.h"
#include "digitos_grandes_utils.h"

#include "tarefa_bcd8.h"    // g_bcd_tens, g_bcd_units, g_bcd_valid

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Disponibilizados pelo seu projeto
extern SemaphoreHandle_t mutex_oled;
extern ssd1306_t oled;

#ifndef DISP_DUPLO_PERIOD_MS
#define DISP_DUPLO_PERIOD_MS 80u
#endif

// Largura do glyph "grande" (ajuste se sua fonte tiver outra largura)
#ifndef DIGITO_GRANDE_W
#define DIGITO_GRANDE_W 25u
#endif

// Espaço mínimo entre os dois dígitos
#ifndef DIGITO_GRANDE_GAP
#define DIGITO_GRANDE_GAP 4u
#endif

static void desenhar_duplo_centralizado(uint8_t dez, uint8_t und, bool valido)
{
    oled_clear(&oled);

    // Cabeçalho (opcional): mostra estado de validade
    ssd1306_draw_utf8_multiline(
        oled.ram_buffer, 0, 0,
        valido ? "BCD" : "INV",
        oled.width, oled.height
    );

    // Larguras
    const uint8_t W   = (uint8_t)DIGITO_GRANDE_W;
    const uint8_t GAP = (uint8_t)DIGITO_GRANDE_GAP;

    // Largura ocupada pelo par de dígitos
    const uint16_t pair_w = (uint16_t)(W + GAP + W);

    // Posição X inicial para centralizar o par
    uint8_t x0 = 0;
    if (oled.width > pair_w) {
        x0 = (uint8_t)((oled.width - pair_w) / 2);
    }
    const uint8_t xL = x0;
    const uint8_t xR = (uint8_t)(x0 + W + GAP);

    // Bitmaps
    const uint8_t *bmpL;
    const uint8_t *bmpR;

    if (valido && dez < 10 && und < 10) {
        bmpL = numeros_grandes[dez];
        bmpR = numeros_grandes[und];
    } else {
        // fallback simples: usa '0' e '0' quando inválido (ou crie um glyph '-')
        bmpL = numeros_grandes[0];
        bmpR = numeros_grandes[0];
    }

    // Desenho dos dois dígitos
    exibir_digito_grande(&oled, xL, bmpL);
    exibir_digito_grande(&oled, xR, bmpR);

    // Envia ao display
    oled_render(&oled);
}

static void task_display_duplo_bcd(void *arg)
{
    (void)arg;
    printf("[OLED] Duplo BCD centralizado (dezena/unidade)\n");

    // Estado para evitar redesenho desnecessário
    uint8_t ultimo_dez = 0xFF;
    uint8_t ultimo_und = 0xFF;
    bool    ultimo_ok  = false;

    const TickType_t dt = pdMS_TO_TICKS(DISP_DUPLO_PERIOD_MS);

    for (;;) {
        vTaskDelay(dt);

        // Snapshot dos globais de BCD (cada store é 8b/1b → ok)
        uint8_t dez = g_bcd_tens;
        uint8_t und = g_bcd_units;
        bool    ok  = g_bcd_valid;

        if (dez != ultimo_dez || und != ultimo_und || ok != ultimo_ok) {
            ultimo_dez = dez;
            ultimo_und = und;
            ultimo_ok  = ok;

            if (xSemaphoreTake(mutex_oled, pdMS_TO_TICKS(100))) {
                desenhar_duplo_centralizado(dez, und, ok);
                xSemaphoreGive(mutex_oled);
            }

            printf("[OLED] BCD %s  dez=%u und=%u\n", ok ? "OK" : "INV", dez, und);
        }
    }
}

void criar_tarefa_display_duplo_bcd(UBaseType_t prio, UBaseType_t core_mask)
{
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_display_duplo_bcd, "disp_duplo_bcd", 1024, NULL, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
