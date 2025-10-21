#include "FreeRTOS.h"
#include "task.h"
#include "oled_display.h"
#include "ssd1306_text.h"
#include "oled_context.h"
#include "buffer_sensor.h"
#include "tela_display.h"
#include "numeros_display.h"
#include "mpu.h"

#include <stdio.h>
#include <string.h>

#define INTERVALO_DISPLAY_MS 250

void exibir_valor_grande(const char *titulo, const char *coordenada, const char *unidade, const char *valor) {
    oled_clear(&oled);
    oled_centralizar_texto(&oled, titulo, 1);  // Linha 0
    oled_centralizar_texto(&oled, coordenada, 2);  // Linha 2
    oled_centralizar_texto(&oled, unidade, 4); // Linha 4
    oled_exibir_valor_medio(&oled, valor);     // Linhas 5 a 7
    oled_render(&oled);
}

void exibir_tela_display(const sensor_dataf_t *dado) {
    oled_clear(&oled);

    char linha[32];
    int y = 0;

    snprintf(linha, sizeof(linha), "%s", nome_sensor_global);
    ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;

    snprintf(linha, sizeof(linha), "AX: %.2f g", dado->ax); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "AY: %.2f g", dado->ay); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "AZ: %.2f g", dado->az); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "GX: %.2f º/s", dado->gx); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "GY: %.2f º/s", dado->gy); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "GZ: %.2f º/s", dado->gz); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height); y += 8;
    snprintf(linha, sizeof(linha), "Temp: %.2f ºC", dado->temp_c); ssd1306_draw_utf8_multiline(oled.ram_buffer, 0, y, linha, oled.width, oled.height);

    oled_render(&oled);
}

void tarefa_display_oled(void *params) {
    sensor_dataf_t media;

    printf("[tarefa_display_oled] Iniciada\n");

    while (true) {
        if (buffer_sensor_calcular_media(&media)) {
            if (xSemaphoreTake(mutex_oled, pdMS_TO_TICKS(100))) {
                switch (tela_atual) {
                    case TELA_DISPLAY:
                        exibir_tela_display(&media);
                        break;
                    case TELA_AX: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.ax);
                        exibir_valor_grande("ACELERÔMETRO", "Ax", "(g)", valor);
                        break;
                    }
                    case TELA_AY: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.ay);
                        exibir_valor_grande("ACELERÔMETRO","Ay", "(g)", valor);
                        break;
                    }
                    case TELA_AZ: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.az);
                        exibir_valor_grande("ACELERÔMETRO", "Az","(g)", valor);
                        break;
                    }
                    case TELA_GX: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.gx);
                        exibir_valor_grande("GIROSCÓPIO","Gx", "(º/s)", valor);
                        break;
                    }
                    case TELA_GY: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.gy);
                        exibir_valor_grande("GIROSCÓPIO", "Gy","(º/s)", valor);
                        break;
                    }
                    case TELA_GZ: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.gz);
                        exibir_valor_grande("GIROSCÓPIO", "Gz","(º/s)", valor);
                        break;
                    }
                    case TELA_TEMP: {
                        char valor[8];
                        snprintf(valor, sizeof(valor), "%+7.2f", media.temp_c);
                        exibir_valor_grande("TEMPERATURA", "TEMP", "(ºC)", valor);
                        break;
                    }
                    default:
                        oled_clear(&oled);
                        oled_centralizar_texto(&oled, "Tela inválida", 3);
                        oled_render(&oled);
                        break;
                }

                xSemaphoreGive(mutex_oled);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVALO_DISPLAY_MS));
    }
}
