#include "tarefa_max30102.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#include "max30102.h"
#include "tarefa_word_8.h" // g_word8_value, WORD8_MASK

#include <stdio.h>

#define I2C_SDA_PIN 0u
#define I2C_SCL_PIN 1u

static void task_max30102(void *pv)
{
    const uint32_t period_ms = (uint32_t)(uintptr_t)pv;
    const TickType_t dt = pdMS_TO_TICKS(period_ms);

    i2c_inst_t *i2c = i2c0;

    // Inicializa I2C (assume o extensor já presente no barramento)
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o driver MAX30102
    (void)max30102_init(i2c);

    for (;;) {
        // Executa uma iteração do driver (lê FIFO e atualiza g_max_bpm/g_max_spo2)
        max30102_process_once(i2c);

        // Mapear BPM para 0..100 e escrever em g_word8_value
        uint16_t bpm = (uint16_t)g_max_bpm;
        if (bpm > 100u) bpm = 100u;
        g_word8_value = (uint16_t)(bpm & WORD8_MASK);

        vTaskDelay(dt);
    }
}

void criar_tarefa_max30102(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms)
{
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_max30102, "max30102", 1024, (void *)(uintptr_t)period_ms, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
