// main.c
#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// ==== OLED ====
#include "oled_display.h"
#include "oled_context.h"

// ==== Tarefas ====
#include "tarefa_word_8.h"
#include "tarefa_display_word_8.h"
#include "sensor_word8.h"
#include "tarefa_sensors_i2c.h"

// Adiciona o header público da tarefa UART (remove declaração manual)
#include "tarefa_enviar_uart.h"

// ==== Núcleos (RP2040) ====
#define CORE0_MASK ((UBaseType_t)(1u << 0))
#define CORE1_MASK ((UBaseType_t)(1u << 1))

// ==== Prioridades ====
// Maior -> menor: WORD8 >= OLED
#define PRIO_WORD8 (tskIDLE_PRIORITY + 2)
#define PRIO_OLED (tskIDLE_PRIORITY + 1)

// === UART Interface ===
#define UART_IF uart0
#define BAUD_RATE 9600
#define UART_IF_TX 16
#define UART_IF_RX 17

#define HEADER_BYTE 0xAA

// === Parser 12 bits ===
static volatile uint8_t s_rx_state = 0; // 0=header,1=HI,2=LO
volatile uint16_t s_rx_word = 0;        // [11:0]   // <-- tornada global (extern)
volatile bool s_rx_ready = false;       // <-- tornada global (extern)

// === Estado e seleção atuais ===
static uint8_t current_sensor = 0;
static uint8_t current_sala = 0;

// guarda IRQ usado pela UART (não-static para permitir desabilitar em outras unidades)
int s_uart_irq = -1;

// função criada em tarefa_enviar_uart.c
// void criar_tarefa_enviar_uart(UBaseType_t prio, UBaseType_t core_mask);

// === Imprime 12 bits recebido ===
static void print_word_bits(uint16_t w)
{
    char bits[16] = {0};
    for (int i = 11; i >= 0; --i)
        bits[11 - i] = ((w >> i) & 1) ? '1' : '0';
    printf("RX 12b: %c%c%c%c %c%c%c%c %c%c%c%c  (0x%03X)\n",
           bits[0], bits[1], bits[2], bits[3],
           bits[4], bits[5], bits[6], bits[7],
           bits[8], bits[9], bits[10], bits[11],
           w & 0xFFF);
}

// === IRQ UART ===
// === IRQ UART (ISR leve, sem printf) ===
static void on_uart_rx(void)
{
    static uint32_t s_rx_ts_ms = 0;
    const uint32_t RX_TIMEOUT_MS = 200u;

    while (uart_is_readable(UART_IF))
    {
        int rv = uart_getc(UART_IF);
        if (rv < 0)
            break;
        uint8_t b = (uint8_t)rv;

        // timestamp para timeout (liga-se com to_ms_since_boot(), rápido o suficiente)
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // se estiver esperando HI->LO e o tempo excedeu, reseta o estado
        if (s_rx_state == 1 && (now - s_rx_ts_ms) > RX_TIMEOUT_MS)
        {
            s_rx_state = 0;
        }

        switch (s_rx_state)
        {
        case 0:
            // aceita tanto o header clássico (0xAA) quanto um HI "nibble" enviado
            // diretamente nos 4 LSB (ex.: 0x09). Se receber nibble, já salva como HI.
            if (b == HEADER_BYTE)
            {
                s_rx_state = 1;
                s_rx_ts_ms = now;
            }
            else if ((b & 0xF0) == 0)
            {
                // byte com 0 nos 4 MSB: trata como HI contendo o nibble em bits[3:0]
                s_rx_word = ((uint16_t)(b & 0x0F)) << 8; // coloca nibble em bits11..8
                s_rx_state = 2;                          // aguarda o LO seguinte
            }
            break;
        case 1:
            // guarda HI temporariamente nos 8 bits altos (vindo após header)
            s_rx_word = ((uint16_t)b) << 8;
            s_rx_state = 2;
            s_rx_ts_ms = now;
            break;
        case 2:
        {
            uint8_t hi = (s_rx_word >> 8) & 0xFF;
            uint8_t lo = b;
            // sempre usar os 4 LSB de 'hi' como nibble (S1S0 A1A0)
            uint8_t nibble = hi & 0x0F;
            uint16_t valor12b = ((uint16_t)nibble << 8) | (uint16_t)lo;

            s_rx_word = valor12b & 0x0FFF; // garante 12 bits
            s_rx_ready = true;
            s_rx_state = 0;
            break;
        }
        }
    }
}

int main(void)
{
    // stdio_init_all();
    sleep_ms(500);

    stdio_usb_init();
    // sleep_ms(500);

    // UART Interface
    uart_init(UART_IF, BAUD_RATE);
    gpio_set_function(UART_IF_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_IF_RX, GPIO_FUNC_UART);
    uart_set_format(UART_IF, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_IF, true);
    int UART_IRQ = (UART_IF == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_IF, true, false);

    s_uart_irq = UART_IRQ;

    // while (!stdio_usb_connected()) { tight_loop_contents(); }

    // printf("=== INICIANDO SISTEMA: WORD6 + OLED + BOTOES + LED_VERDE ===\n");

    // OLED
    if (!oled_init(&oled))
    {
        printf("Falha ao inicializar OLED!\n");
        while (true)
        {
            tight_loop_contents();
        }
    }

    mutex_oled = xSemaphoreCreateMutex();
    configASSERT(mutex_oled != NULL);

    // Tarefa dos 6 bits -> núcleo 0 (escreve nos pinos)
    criar_tarefa_word_8(PRIO_WORD8, CORE0_MASK, true);
    // // Tarefa simulada do sensor -> atualiza g_word8_value periodicamente (5s)
    // criar_tarefa_sensor_word8(PRIO_WORD8, CORE0_MASK, 5000u);

    // Tarefa que alterna leituras I2C (AHT10 <-> MAX30102) a cada 100ms
    criar_tarefa_sensors_i2c(PRIO_WORD8, CORE0_MASK, 100u);

    // OLED -> núcleo 1
    criar_tarefa_display_word_8(PRIO_OLED, CORE1_MASK);

    // Tarefa que processa solicitações UART e envia leituras quando solicitadas
    criar_tarefa_enviar_uart(PRIO_WORD8, CORE0_MASK);

    vTaskStartScheduler();

    // Nunca deve chegar aqui
    while (true)
    {
        tight_loop_contents();
    }
}
