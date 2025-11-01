#include "tarefa_sensors_i2c.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#include "aht10.h"
#include "max30102.h"
#include "sensors_shared.h"
#include "tarefa_word_8.h" // for g_word8_value
#include <math.h>
#include <stdio.h>

#define I2C_SDA_PIN 0u
#define I2C_SCL_PIN 1u

// Define os valores globais
volatile float g_aht_temperature = 0.0f;

static void task_sensors_i2c(void *pv)
{
    const uint32_t period_ms = (uint32_t)(uintptr_t)pv; // intervalo entre leituras de cada sensor
    const TickType_t dt = pdMS_TO_TICKS(period_ms);

    i2c_inst_t *i2c = i2c0;

    // Inicializa I2C (bus compartilhado)
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa ambos drivers (assume endereços diferentes no mesmo barramento)
    // Não abortamos se alguma inicialização falhar: o loop continuará
    (void)aht10_init(i2c);
    (void)max30102_init(i2c);

    for (;;) {
        // 1) Ler AHT10 (temperatura) e atualizar global
        aht10_data_t d;
        if (aht10_read_data(i2c, &d)) {
            g_aht_temperature = d.temperature;
            // Mapear temperatura para 0..100 e atualizar saída word8
            int temp = (int)roundf(d.temperature);
            if (temp < 0) temp = 0;
            if (temp > 100) temp = 100;
            g_word8_value = (uint16_t)(temp & WORD8_MASK);
            // printf("[sensors_i2c] AHT10 temp=%.2f -> word8=%u\n", (double)g_aht_temperature, (unsigned)g_word8_value);
        }
        // aguarda o período (por exemplo 10s) antes de ler o outro sensor
        vTaskDelay(dt);

        // 2) Ler MAX30102 (uma iteração) e atualizar seus globals (g_max_bpm/g_max_spo2)
        // processa o MAX30102 várias vezes durante a janela para acumular leituras
        const uint32_t sample_ms = 100u;
        uint32_t loops = (period_ms + sample_ms - 1) / sample_ms;
        for (uint32_t i = 0; i < loops; ++i) {
            max30102_process_once(i2c);
            vTaskDelay(pdMS_TO_TICKS(sample_ms));
        }

        // atualiza saída word8 com o BPM (0..100 clamped)
        uint16_t bpm_mapped = (uint16_t)g_max_bpm;
        if (bpm_mapped > 100u) bpm_mapped = 100u;
        g_word8_value = (uint16_t)(bpm_mapped & WORD8_MASK);
        // printf("[sensors_i2c] MAX30102 bpm=%u -> word8=%u\n", (unsigned)g_max_bpm, (unsigned)g_word8_value);

        // aguarda período antes de voltar a ler o AHT10
        vTaskDelay(dt);
    }
}

void criar_tarefa_sensors_i2c(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms)
{
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_sensors_i2c, "sensors_i2c", 1024, (void *)(uintptr_t)period_ms, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
