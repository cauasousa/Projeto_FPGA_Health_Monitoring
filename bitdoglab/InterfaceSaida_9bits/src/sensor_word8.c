#include "sensor_word8.h"

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "tarefa_word_8.h" // para g_word8_value e WORD8_MASK

// Se definido, usa o driver AHT10 para ler a temperatura via I2C.
// Defina USE_AHT10 nas flags do compilador para ativar a leitura real.

#include "aht10.h"
#include "hardware/i2c.h"

static void task_sensor_word8(void *pv)
{
    const uint32_t period_ms = (uint32_t)(uintptr_t)pv; // passamos period_ms via pv
    const TickType_t dt = pdMS_TO_TICKS(period_ms);

    i2c_inst_t *I2C_PORT = i2c0;
    const uint I2C_SDA_PIN = 0u;
    const uint I2C_SCL_PIN = 1u;

    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    if (!aht10_init(I2C_PORT))
    {
        // se falhar, mantém um valor 0 e fica em loop (poderíamos trocar para simulação)
        g_word8_value = 0;
        for (;;)
            vTaskDelay(dt);
    }

    for (;;)
    {
        aht10_data_t d;
        if (aht10_read_data(I2C_PORT, &d))
        {
            // Usa apenas a temperatura (em °C). Mapear para 0..100 exatamente.
            int temp = (int)roundf(d.temperature);
            if (temp < 0)
                temp = 0;
            if (temp > 100)
                temp = 100;
            g_word8_value = (uint16_t)(temp & WORD8_MASK);
        }
        else
        {
            // leitura falhou -> opcionalmente manter anterior ou set 0
            // aqui apenas mantém valor anterior
        }
        vTaskDelay(dt);
    }
}

void criar_tarefa_sensor_word8(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms)
{
    // passamos period_ms como pv do task para simplificar
    TaskHandle_t th = NULL;
    BaseType_t ok = xTaskCreate(task_sensor_word8, "sensor_word8", 768, (void *)(uintptr_t)period_ms, prio, &th);
    configASSERT(ok == pdPASS);
    vTaskCoreAffinitySet(th, core_mask);
}
