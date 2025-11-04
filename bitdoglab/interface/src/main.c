#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// === OLED ===
#include "oled_display.h"
#include "oled_context.h"
#include "ssd1306_text.h"
#include "numeros_grandes.h"
#include "digitos_grandes_utils.h"

// === UART Interface ===
#define UART_IF uart0
#define BAUD_RATE 9600
#define UART_IF_TX 16
#define UART_IF_RX 17

#define HEADER_BYTE 0xAA

// === Parser 12 bits ===
static volatile uint8_t s_rx_state = 0; // 0=header,1=HI,2=LO
static volatile uint16_t s_rx_word = 0; // [11:0]
static volatile bool s_rx_ready = false;

// === Timer Menu Secundário ===
#define MENU_TIMEOUT_MS 15000 // 10s
static uint32_t menu_timer = 0;

// === Estado e seleção atuais ===
static uint8_t current_sensor = 0;
static uint8_t current_sala = 0;

static int s_uart_irq = -1;
// === Helpers OLED ===
static void oled_show_menu_salas(void)
{
    oled_clear(&oled);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 0, "Selecione a Sala:", oled.width, oled.height);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 16, "- A", oled.width, oled.height);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 24, "- B", oled.width, oled.height);
    oled_render(&oled);
}

static void oled_show_menu_sensor(char sala)
{
    char linha[32];
    oled_clear(&oled);
    snprintf(linha, sizeof(linha), "Selecione sensor Sala %c", sala);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 0, linha, oled.width, oled.height);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 16, "- Temp", oled.width, oled.height);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 24, "- BPM", oled.width, oled.height);
    oled_render(&oled);
}

static void oled_show_temp(uint8_t v)
{
    char linha[16];
    oled_clear(&oled);
    snprintf(linha, sizeof(linha), "Temp: %uC", v);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 0, linha, oled.width, oled.height);
    uint8_t x_pos = (oled.width - 25) / 2;
    if (v < 10)
        exibir_digito_grande(&oled, x_pos, numeros_grandes[v]);
    else if (v < 100)
    {
        exibir_digito_grande(&oled, x_pos - 13, numeros_grandes[(v / 10) % 10]);
        exibir_digito_grande(&oled, x_pos + 13, numeros_grandes[v % 10]);
    }
    oled_render(&oled);
}

static void oled_show_bpm(uint8_t v)
{
    char linha[20];
    oled_clear(&oled);
    snprintf(linha, sizeof(linha), "BPM: %u", v);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, 0, linha, oled.width, oled.height);
    uint8_t d2 = (v / 100) % 10, d1 = (v / 10) % 10, d0 = v % 10;
    if (v >= 100)
    {
        exibir_digito_grande(&oled, 10, numeros_grandes[d2]);
        exibir_digito_grande(&oled, 35, numeros_grandes[d1]);
        exibir_digito_grande(&oled, 60, numeros_grandes[d0]);
    }
    else if (v >= 10)
    {
        exibir_digito_grande(&oled, 26, numeros_grandes[d1]);
        exibir_digito_grande(&oled, 51, numeros_grandes[d0]);
    }
    else
    {
        exibir_digito_grande(&oled, (oled.width - 25) / 2, numeros_grandes[d0]);
    }
    oled_render(&oled);
}

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
// ISR leve: sem printf, aceita formato com ou sem HEADER
static void on_uart_rx(void)
{
    while (uart_is_readable(UART_IF))
    {
        int rv = uart_getc(UART_IF);
        printf("RX char: '%c'\n", rv);
        if (rv < 0)
            break;
        uint8_t b = (uint8_t)rv;
        printf("RX byte: 0x%02X\n", b);
        switch (s_rx_state)
        {
        case 0:
            // Se vier HEADER (0xAA), espera o próximo byte como HI.
            if (b == HEADER_BYTE)
            {
                s_rx_state = 1;
            }
            else
            {
                // Se vier byte no formato "0x0N" (nibble válido em LSB), trate como HI sem HEADER.
                // Ex: 0x09 -> nibble = 0x9 -> bits[11:8] = 0x9
                if ((b & 0xF0) == 0 && (b & 0x0F) != 0)
                {
                    s_rx_word = ((uint16_t)(b & 0x0F)) << 8;
                    s_rx_state = 2; // aguarda LO
                }
                else
                {
                    // Byte inesperado: ignore e permaneça em state 0.
                    printf("Byte inesperado em state 0: 0x%02X\n", b);
                }
            }
            break;

        case 1:
            // Primeiro byte após HEADER: pode ser nibble em LSB (0x0N) ou formato antigo com MSB úteis.
            if ((b & 0xF0) == 0)
            {
                s_rx_word = ((uint16_t)(b & 0x0F)) << 8;
            }
            else
            {
                s_rx_word = ((uint16_t)(b & 0xF0)) << 4; // compatibilidade com HI[7:4]
            }
            s_rx_state = 2;
            break;

        case 2:
            // LO recebido → monta palavra 12 bits e sinaliza ready
            s_rx_word = (s_rx_word | (uint16_t)b) & 0x0FFFu;
            s_rx_ready = true;
            s_rx_state = 0;
            break;
        } // switch
    } // while
}

// Nota explicativa do parser:
//  - O código do Pico espera 3 bytes: HEADER(0xAA), HI, LO.
//  - Quando recebe HI e LO, monta a palavra de 12 bits assim:
//      valor12b = ((HI & 0xF0) << 4) | LO;
//    Logo:
//      bits[11:10] = sensor_sel
//      bits[9:8]   = sala_sel
//      bits[7:0]   = dado (8 bits: temperatura ou BPM)
//  - No main a variável 'w' contém esses 12 bits (w & 0x0FFF).
//  - O firmware usa esses campos para decidir qual menu/OLED atualizar.
//
// Exemplificando fluxo completo:
// 1) FPGA: botão seleciona Sala A e Sensor Temp -> gera HEADER, HI={sensor=01,sala=01,0000}, LO=dado(25).
// 2) UART transmite 0xAA, HI, LO serialmente.
// 3) Pico: ISR coleta bytes; ao ver 0xAA entra no estado de receber HI e LO.
// 4) Ao completar HI+LO, monta palavra 12b e s_rx_ready=true.
// 5) Loop principal lê s_rx_word, extrai sensor/sala/dado e atualiza OLED.
//
// === MAIN ===
int main(void)
{
    stdio_usb_init();
    sleep_ms(1500);
    printf("\n=== UART 12b RX → OLED ===\n");

    // UART Interface
    uart_init(UART_IF, BAUD_RATE);
    gpio_set_function(UART_IF_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_IF_RX, GPIO_FUNC_UART);
    uart_set_format(UART_IF, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_IF, false);
    int UART_IRQ = (UART_IF == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_IF, true, false);

    s_uart_irq = UART_IRQ;

    // Inicializa OLED
    if (!oled_init(&oled))
    {
        printf("Falha ao inicializar OLED!\n");
        while (true)
            tight_loop_contents();
    }
    oled_show_menu_salas();

    // === Loop principal ===
    enum
    {
        ST_INICIO,
        ST_MENU_SEC,
        ST_LEITURA_SENSOR
    } state = ST_INICIO;
    uint32_t last_time = to_ms_since_boot(get_absolute_time());

    uint16_t last_w = 0xFFFF;
    uint32_t last_process_ms = 0;

    // === Variáveis para temporizador OLED ===
    static uint32_t oled_display_until = 0;
    static enum { OLED_NONE,
                  OLED_TEMP,
                  OLED_BPM } oled_state = OLED_NONE;

    for (;;)
    {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Se ISR marcou ready, copie os dados de forma atômica
        if (s_rx_ready)
        {
            irq_set_enabled(s_uart_irq, false);
            uint16_t w_copy = s_rx_word;
            s_rx_ready = false;
            irq_set_enabled(s_uart_irq, true);

            // Ignorar repetição rápida (debounce lógico)
            if (w_copy == last_w && (now - last_process_ms) < 150)
                continue;

            last_w = w_copy;
            last_process_ms = now;

            // uint16_t w = w_copy & 0x0FFF;

            // uint8_t sensor_sel = (w >> 10) & 0x3; // bits 11:10
            // uint8_t sala_sel = (w >> 8) & 0x3;    // bits 9:8

            uint16_t w = w_copy & 0x0FFFu;
            uint8_t sensor_sel = (uint8_t)((w >> 10) & 0x3);
            uint8_t sala_sel = (uint8_t)((w >> 8) & 0x3);
            uint8_t dado = (uint8_t)(w & 0xFFu);

            printf("Sensor: %u, Sala: %u, Dado: %u\n", sensor_sel, sala_sel, dado);

            // Atualiza OLED e inicia temporizador
            if (sensor_sel != 0)
            {
                if (sensor_sel == 1)
                {
                    oled_show_temp(dado);
                    oled_state = OLED_TEMP;
                }
                else if (sensor_sel == 2)
                {
                    oled_show_bpm(dado);
                    oled_state = OLED_BPM;
                }
                oled_display_until = now + 10000; // mostra por 10s
            }
            else
            {
                // Menu ou seleção de sala
                if (sensor_sel == 0 && sala_sel != 0)
                    oled_show_menu_sensor((sala_sel == 1) ? 'A' : 'B');
                else
                    oled_show_menu_salas();
                oled_state = OLED_NONE;
            }
        }

        // Verifica se expirou o tempo de exibição do dado
        if (oled_state != OLED_NONE && now >= oled_display_until)
        {
            oled_state = OLED_NONE;
            // Volta ao menu principal
            oled_show_menu_salas();
        }

        sleep_ms(5);
    }

    return 0;
}
