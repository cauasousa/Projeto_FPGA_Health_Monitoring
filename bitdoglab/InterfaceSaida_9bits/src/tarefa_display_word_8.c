// tarefa_display_word_8.c — mostra "BIT N" (linha 0) e o dígito grande 0/1 do bit N
#include "tarefa_display_word_8.h"
#include <string.h>

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "oled_display.h"
#include "oled_context.h"
#include "ssd1306_text.h"
#include "numeros_grandes.h"
#include "digitos_grandes_utils.h"

#include "tarefa_word_8.h"              // WORD8_MASK / word8_get()
#include "max30102.h"
#include "sensors_shared.h"
#include <math.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

extern SemaphoreHandle_t mutex_oled;
extern ssd1306_t oled;
extern volatile uint16_t g_word8_value;   // 8 bits em uso

#define PERIOD_MS  80u

static void desenhar_bitN_grande_com_header(uint8_t sel, uint8_t v01)
{
    // 1) limpa quadro
    oled_clear(&oled);

    // 2) mostra TEMPERATURA (AHT10) no topo e BPM (MAX30102) abaixo; ambos centralizados
    uint16_t w = (uint16_t)(g_word8_value & WORD8_MASK);

    char tempbuf[32];
    if (isnan((double)g_aht_temperature)) {
        snprintf(tempbuf, sizeof(tempbuf), "TEMP: -- C");
    } else {
        int temp_i = (int)roundf(g_aht_temperature);
        snprintf(tempbuf, sizeof(tempbuf), "TEMP: %d C", temp_i);
    }
    size_t temp_len = strlen(tempbuf);
    uint16_t temp_w = (uint16_t)(temp_len * 6u);
    uint8_t x_temp = (oled.width > temp_w) ? (uint8_t)((oled.width - temp_w) / 2u) : 0u;
    ssd1306_draw_utf8_multiline(oled.ram_buffer, x_temp-3, 8, tempbuf, oled.width, oled.height);

    char bpmbuf[24];
    uint16_t bpm = (uint16_t)g_max_bpm;
    if (bpm == 0) {
        snprintf(bpmbuf, sizeof(bpmbuf), "BPM: --");
    } else {
        if (bpm > 999u) bpm = 999u;
        snprintf(bpmbuf, sizeof(bpmbuf), "BPM: %u", (unsigned)bpm);
    }
    size_t bpm_len = strlen(bpmbuf);
    uint16_t bpm_w = (uint16_t)(bpm_len * 6u);
    uint8_t x_bpm = (oled.width > bpm_w) ? (uint8_t)((oled.width - bpm_w) / 2u) : 0u;
    // desenha BPM (y = 26)
    ssd1306_draw_utf8_multiline(oled.ram_buffer, x_bpm-3, 26, bpmbuf, oled.width, oled.height);

    // 3) mostra a representação BINÁRIA (8 bits) centralizada abaixo com rótulo
    char binbuf[16];
    const char *label = "BIN: ";
    memcpy(binbuf, label, 5);
    for (int i = 0; i < 8; ++i) {
        binbuf[5 + i] = (w & (1u << (7 - i))) ? '1' : '0';
    }
    binbuf[13] = '\0';
    size_t bin_len = 5 + 8; // "BIN: " + 8 chars
    uint16_t bin_w = (uint16_t)(bin_len * 6u);
    uint8_t x_bin = (oled.width > bin_w) ? (uint8_t)((oled.width - bin_w) / 2u) : 0u;
    // desenha binário (y = 44)
    ssd1306_draw_utf8_multiline(oled.ram_buffer, x_bin-4, 44, binbuf, oled.width, oled.height);

    // 4) envia ao display
    oled_render(&oled);
}

static void task_display_word_8(void *arg)
{
    (void)arg;
    // printf("[OLED] mostrando TEMP e BIN (sem joystick)\n");

    const TickType_t dt = pdMS_TO_TICKS(PERIOD_MS);
    uint16_t last_w = 0xFFFFu;

    for (;;) {
        uint16_t w = (uint16_t)(g_word8_value & WORD8_MASK);
        if (w != last_w) {
            last_w = w;
            if (xSemaphoreTake(mutex_oled, pdMS_TO_TICKS(100))) {
                // desenha usando o valor atual; sel/v01 não são usados na rotina
                desenhar_bitN_grande_com_header(0, (uint8_t)(w & 1u));
                xSemaphoreGive(mutex_oled);
            }
        }

        vTaskDelay(dt);
    }
}

void criar_tarefa_display_word_8(UBaseType_t prio, UBaseType_t core_mask)
{
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_display_word_8, "disp_bitN", 1024, NULL, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
