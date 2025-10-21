// tarefa_word_9.c — LÊ 9 bits (B0..B8) das GPIOs e publica em g_word9_value
// Mapa LSB→MSB: B0=GP18, B1=GP19, B2=GP20, B3=GP4, B4=GP9, B5=GP8, B6=GP16, B7=GP17, B8=GP28

#include "tarefa_word_9.h"

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/gpio.h"
#include "hardware/regs/sio.h"
#include "hardware/structs/sio.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define PERIOD_MS     50u            // intervalo entre amostras -> confirmação ~100 ms

// Ative para usar leitura “em bloco” via SIO (reduz skew entre bits)
#ifndef WORD9_FAST_SIO
#define WORD9_FAST_SIO 1
#endif

// Valor global publicado (9 bits 0..8)
volatile uint16_t g_word9_value = 0;   // 0..0x1FF (B0..B8)

// Flag interna (arquivo) para configurar pull-up
static bool s_word9_pullup_enabled = true;

// Tabela de pinos na ordem lógica B0..B8 (LSB→MSB) — usa macros do header
static const uint8_t GPIO_WORD_PINS[9] = {
    GPIO_WORD_B0, // B0
    GPIO_WORD_B1, // B1
    GPIO_WORD_B2, // B2
    GPIO_WORD_B3, // B3
    GPIO_WORD_B4, // B4
    GPIO_WORD_B5, // B5
    GPIO_WORD_B6, // B6
    GPIO_WORD_B7, // B7
    GPIO_WORD_B8  // B8
};

// --- API principal ---
uint16_t word9_get(void) {
    // Em RP2040, leitura alinhada de 16b é atômica.
    return (uint16_t)(g_word9_value & WORD9_MASK);
}

// --- Utilidades ---
static inline void config_input(uint pin, bool pullup) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_disable_pulls(pin);
    if (pullup) gpio_pull_up(pin);
}

// Impressão MSB..LSB (B8..B0) para diagnóstico
static void print_bits9(uint16_t v) {
    printf("BIN: %c%c%c%c%c%c%c%c%c  (MSB..LSB)\n",
           (v&(1u<<8))?'1':'0',
           (v&(1u<<7))?'1':'0',
           (v&(1u<<6))?'1':'0',
           (v&(1u<<5))?'1':'0',
           (v&(1u<<4))?'1':'0',
           (v&(1u<<3))?'1':'0',
           (v&(1u<<2))?'1':'0',
           (v&(1u<<1))?'1':'0',
           (v&(1u<<0))?'1':'0');
}

// Compacta B0..B8 -> bits 0..8 do retorno
static inline uint16_t read_word9_now(void) {
#if WORD9_FAST_SIO
    // Leitura única do registrador de entradas dos GPIOs
    uint32_t in = sio_hw->gpio_in;
    uint16_t v  = 0;
    v |= (uint16_t)(((in >> GPIO_WORD_B0) & 1u) << 0);
    v |= (uint16_t)(((in >> GPIO_WORD_B1) & 1u) << 1);
    v |= (uint16_t)(((in >> GPIO_WORD_B2) & 1u) << 2);
    v |= (uint16_t)(((in >> GPIO_WORD_B3) & 1u) << 3);
    v |= (uint16_t)(((in >> GPIO_WORD_B4) & 1u) << 4);
    v |= (uint16_t)(((in >> GPIO_WORD_B5) & 1u) << 5);
    v |= (uint16_t)(((in >> GPIO_WORD_B6) & 1u) << 6);
    v |= (uint16_t)(((in >> GPIO_WORD_B7) & 1u) << 7);
    v |= (uint16_t)(((in >> GPIO_WORD_B8) & 1u) << 8);
    return v;
#else
    // Versão simples: um gpio_get por bit
    uint16_t v = 0;
    for (uint8_t i = 0; i < 9; i++) {
        v |= (uint16_t)((gpio_get(GPIO_WORD_PINS[i]) & 1u) << i);
    }
    return v;
#endif
}

// --- Tarefa de leitura com confirmação por dupla amostra ---
static void task_word9_in(void *pv) {
    (void)pv;

    // Aguarda USB para logs ficarem visíveis (opcional)
    for (int i = 0; i < 50 && !stdio_usb_connected(); ++i) {
        printf("[Word9-IN] aguardando USB...\n");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("[Word9-IN] iniciando leitura com pull-up %s\n", s_word9_pullup_enabled ? "ON" : "OFF");
    printf("[Word9-IN] Pinos LSB->MSB:\n");
    printf("  B0=GP%u  B1=GP%u  B2=GP%u  B3=GP%u  B4=GP%u\n",
           GPIO_WORD_PINS[0], GPIO_WORD_PINS[1], GPIO_WORD_PINS[2], GPIO_WORD_PINS[3], GPIO_WORD_PINS[4]);
    printf("  B5=GP%u  B6=GP%u  B7=GP%u  B8=GP%u\n",
           GPIO_WORD_PINS[5], GPIO_WORD_PINS[6], GPIO_WORD_PINS[7], GPIO_WORD_PINS[8]);

    // Configura as 9 GPIOs como ENTRADA (com ou sem pull-up)
    for (uint8_t i = 0; i < 9; i++) {
        config_input(GPIO_WORD_PINS[i], s_word9_pullup_enabled);
    }

    // Snapshot inicial (confirmação simples)
    uint16_t last = (uint16_t)(read_word9_now() & WORD9_MASK);
    g_word9_value = last;

    printf("[Word9-IN] inicial: 0x%03X  ", g_word9_value);
    print_bits9(g_word9_value);

    const TickType_t dt   = pdMS_TO_TICKS(PERIOD_MS);
    TickType_t lastBeat   = xTaskGetTickCount();
    const TickType_t beat = pdMS_TO_TICKS(1000);

    for (;;) {
        vTaskDelay(dt);
        uint16_t v1 = read_word9_now();
        vTaskDelay(dt);
        uint16_t v2 = read_word9_now();

        if (v1 == v2) {
            uint16_t cur = (uint16_t)(v2 & WORD9_MASK);
            if (cur != g_word9_value) {
                g_word9_value = cur; // store de 16b (atômico no M0+)
                printf("[Word9-IN] mudou -> 0x%03X  ", cur);
                print_bits9(cur);
            }
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - lastBeat) >= beat) {
            lastBeat = now;
            uint16_t cur = word9_get();
            printf("[Word9-IN] atual: 0x%03X  ", cur);
            print_bits9(cur);
        }
    }
}

// Criador oficial (usa a flag de pull-up)
void criar_tarefa_word9(UBaseType_t prio, UBaseType_t core_mask, bool use_pullup) {
    s_word9_pullup_enabled = use_pullup;

    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_word9_in, "word9_in", 768, NULL, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
