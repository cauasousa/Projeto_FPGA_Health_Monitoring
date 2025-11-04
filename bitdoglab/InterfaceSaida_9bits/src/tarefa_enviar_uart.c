#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "FreeRTOS.h"
#include "task.h"

#include "tarefa_enviar_uart.h" // <-- novo include

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Se main define UART_IF, usa; caso contrário, fallback para uart0
#ifndef UART_IF
#define UART_IF uart0
#endif

// id desta sala/dispositivo (ajuste conforme necessário)
static const uint8_t id_bitdoglab = 1u;

// Variáveis compartilhadas definidas em main.c
extern volatile uint16_t s_rx_word;
extern volatile bool s_rx_ready;
extern int s_uart_irq;

#define UART_TX_PIN 16

// Leitura dos sensores (variáveis globais em outros módulos)
extern volatile float g_aht_temperature; // definido em tarefa_sensors_i2c.c
extern volatile uint16_t g_max_bpm;      // definido em max30102.c

// monta e envia um pacote 12-bit como 2 bytes (sem HEADER) compatível com o FPGA:
// byte1 = nibble (bits11..8) -> 0000 S1 S0 A1 A0  (4 MSB = 0)
// byte2 = dado (bits7..0)
static void send_12b(uint16_t w)
{
    uint8_t byte1 = (uint8_t)((w >> 8) & 0x0F); // nibble nos 4 LSB
    uint8_t byte2 = (uint8_t)(w & 0xFF);
    uint8_t buf[2] = {byte1, byte2};
    (void)uart_write_blocking(UART_IF, buf, 2);
}

// converte temperatura (°C) para um byte (0..255) e envia pacote com sensor id = 1
static void send_sensor_temp(uint8_t sala)
{
    int temp = (int)roundf(g_aht_temperature);
    // int temp = (int)roundf(64.0f);
    if (temp < 0)
        temp = 0;
    if (temp > 255)
        temp = 255;
    uint8_t data = (uint8_t)temp;
    uint16_t word = ((uint16_t)(1 & 0x3) << 10) | ((uint16_t)(sala & 0x3) << 8) | data;
    // printf("[UART-TX] %u\n", (unsigned)word);
    send_12b(word);
}

// converte BPM para byte (0..255) e envia pacote com sensor id = 2
static void send_sensor_bpm(uint8_t sala)
{
    uint16_t bpm = g_max_bpm;
    // uint16_t bpm = 64;
    if (bpm > 255u)
        bpm = 255u;
    uint8_t data = (uint8_t)bpm;
    uint16_t word = ((uint16_t)(2 & 0x3) << 10) | ((uint16_t)(sala & 0x3) << 8) | data;
    // printf("[UART-TX] %u\n", (unsigned)word);
    send_12b(word);
}

static void task_enviar_uart(void *pv)
{
    (void)pv;

    uint16_t last_w = 0xFFFFu;
    uint32_t last_process_ms = 0;
    uint32_t last_broadcast_ms = to_ms_since_boot(get_absolute_time());
    const uint32_t broadcast_interval_ms = 200u; // envia 1x por segundo

    printf("[UART-TX] tarefa iniciar\n");

    for (;;)
    {

        // verifica se ISR marcou ready (requisição)
        if (s_rx_ready)
        {
            printf("[UART-TX] Requisição recebida\n");
            // copiar de forma atômica desabilitando a IRQ
            if (s_uart_irq >= 0)
                irq_set_enabled(s_uart_irq, false);
            uint16_t w_copy = s_rx_word;
            s_rx_ready = false;
            if (s_uart_irq >= 0)
                irq_set_enabled(s_uart_irq, true);

            // debounce lógico: ignora repetição rápida
            uint32_t now2 = to_ms_since_boot(get_absolute_time());
            if (w_copy == last_w && (now2 - last_process_ms) < 150u)
                continue;

            last_w = w_copy;
            last_process_ms = now2;

            // decodifica 12 bits: bits11..10 sensor, 9..8 sala, 7..0 dado
            uint16_t w = w_copy & 0x0FFFu;
            uint8_t sensor_sel = (uint8_t)((w >> 10) & 0x3);
            uint8_t sala_sel = (uint8_t)((w >> 8) & 0x3);
            uint8_t dado = (uint8_t)(w & 0xFFu);

            printf("[UART-TX] Req sensor=%u sala=%u dado=%u\n",
                   (unsigned)sensor_sel, (unsigned)sala_sel, (unsigned)dado);

            // se for para esta sala, responde com a leitura apropriada (envia 3x em 3s como antes)
            if (sala_sel == id_bitdoglab)
            {
                const int duracao_ms = 301;
                const int intervalo_ms = 300;
                const int iteracoes = duracao_ms / intervalo_ms;

                for (int i = 0; i < iteracoes; i++)
                {
                    if (sensor_sel == 1)
                    {
                        send_sensor_temp(sala_sel);
                        printf("[UART-TX] (%d/%d) Enviado TEMP (%.2f C)\n", i + 1, iteracoes, (double)g_aht_temperature);
                    }
                    else if (sensor_sel == 2)
                    {
                        send_sensor_bpm(sala_sel);
                        printf("[UART-TX] (%d/%d) Enviado BPM (%u)\n", i + 1, iteracoes, (unsigned)g_max_bpm);
                    }
                    vTaskDelay(pdMS_TO_TICKS(intervalo_ms));
                }
            }else{
                // quero enviar a sala e sensor selecionado, mas quero enviar como dados zero
            
                uint16_t word = ((uint16_t)(sensor_sel & 0x3) << 10) | ((uint16_t)(sala_sel & 0x3) << 8) | 0u;
                send_12b(word);
                printf("[UART-TX] Enviado pacote vazio para sensor=%u sala=%u\n",
                   (unsigned)sensor_sel, (unsigned)sala_sel);
                
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // idle curto
    }
}

void criar_tarefa_enviar_uart(UBaseType_t prio, UBaseType_t core_mask)
{
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_enviar_uart, "uart_tx", 1024, NULL, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}