// main.c — Leitura de 2 bits e exibição no OLED

#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// ==== OLED ====
#include "oled_display.h"
#include "oled_context.h"
#include "ssd1306_text.h"
#include "numeros_grandes.h"
#include "digitos_grandes_utils.h"

// ==== Definições dos pinos ====
#define GPIO_BIT0 18 // B0 = GP18
#define GPIO_BIT1 19 // B1 = GP19

// ==== Núcleos (RP2040) ====
#define CORE0_MASK ((UBaseType_t)(1u << 0))
#define CORE1_MASK ((UBaseType_t)(1u << 1))

// ==== Prioridades ====
#define PRIO_LEITURA (tskIDLE_PRIORITY + 3)
#define PRIO_DISPLAY (tskIDLE_PRIORITY + 2)

// ==== Objetos globais do OLED (definidos em outro TU) ====
extern SemaphoreHandle_t mutex_oled;
extern ssd1306_t oled;

// Variável global para armazenar o valor lido (0-3)
volatile uint8_t g_valor_2bits = 0;

// ==== Tarefa de Leitura de 2 bits ====
static void task_leitura_2bits(void *arg)
{
    (void)arg;

    printf("[LEITURA] Iniciando leitura de 2 bits (B0=GP18, B1=GP19)\n");

    // Configura os pinos como entrada com pull-down (estado normal = 0)
    gpio_init(GPIO_BIT0);
    gpio_set_dir(GPIO_BIT0, GPIO_IN);
    // gpio_pull_down(GPIO_BIT0);

    gpio_init(GPIO_BIT1);
    gpio_set_dir(GPIO_BIT1, GPIO_IN);
    // gpio_pull_down(GPIO_BIT1);

    uint8_t ultimo_valor = 0xFF;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(50));

        // Lê os 2 bits
        uint8_t bit0 = gpio_get(GPIO_BIT0) ? 1 : 0;
        uint8_t bit1 = gpio_get(GPIO_BIT1) ? 1 : 0;

        // Combina em um valor de 0 a 3
        uint8_t valor = (bit1 << 1) | bit0;

        if (valor != ultimo_valor)
        {
            g_valor_2bits = valor;
            ultimo_valor = valor;
            printf("[LEITURA] Novo valor: %u (B1=%u B0=%u)\n", valor, bit1, bit0);
        }
    }
}

// ==== Tarefa de Display ====
static void task_display_2bits(void *arg)
{
    (void)arg;

    printf("[DISPLAY] Iniciando exibição no OLED\n");

    uint8_t ultimo_valor = 0xFF;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(80));

        uint8_t valor = g_valor_2bits;

        if (valor != ultimo_valor)
        {
            ultimo_valor = valor;

            if (xSemaphoreTake(mutex_oled, pdMS_TO_TICKS(100)))
            {
                oled_clear(&oled);

                // Cabeçalho
                ssd1306_draw_utf8_multiline(
                    oled.ram_buffer, 0, 0,
                    "2 BITS",
                    oled.width, oled.height);

                // Exibe o número grande centralizado
                if (valor < 10)
                {
                    const uint8_t *bitmap = numeros_grandes[valor];
                    uint8_t x_pos = (oled.width - 25) / 2; // 25 é a largura do dígito
                    exibir_digito_grande(&oled, x_pos, bitmap);
                }

                oled_render(&oled);
                xSemaphoreGive(mutex_oled);
            }

            printf("[DISPLAY] Exibindo valor: %u\n", valor);
        }
    }
}

int main(void)
{
    stdio_init_all();

    printf("=== INICIANDO: LEITURA DE 2 BITS + DISPLAY OLED ===\n");

    // Inicializa OLED
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

    // Configura GPIO20 como saída em nível alto
    gpio_init(20);
    gpio_set_dir(20, GPIO_OUT);
    gpio_put(20, 1);
    printf("GPIO20 configurado como HIGH\n");

    // ----- Tarefas -----
    // 1) Leitura de 2 bits
    TaskHandle_t th_leitura = NULL;
    BaseType_t ok1 = xTaskCreate(task_leitura_2bits, "leitura2", 512, NULL,
                                 PRIO_LEITURA, &th_leitura);
    configASSERT(ok1 == pdPASS);
    vTaskCoreAffinitySet(th_leitura, CORE0_MASK);

    // 2) Display
    TaskHandle_t th_display = NULL;
    BaseType_t ok2 = xTaskCreate(task_display_2bits, "display2", 768, NULL,
                                 PRIO_DISPLAY, &th_display);
    configASSERT(ok2 == pdPASS);
    vTaskCoreAffinitySet(th_display, CORE1_MASK);

    // Scheduler
    vTaskStartScheduler();

    // Nunca deve chegar aqui
    while (true)
    {
        tight_loop_contents();
    }
}
