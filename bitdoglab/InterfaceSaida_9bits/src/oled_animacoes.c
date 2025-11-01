#include "oled_animacoes.h"
#include "numeros_display.h" // usa oled_exibir_digito_grande()
#include <stdio.h>           // necessário para snprintf
#include <string.h>


void oled_animar_entrada_direita(ssd1306_t *oled, uint16_t numero, uint8_t x_inicial, uint8_t espacamento, uint32_t delay_ms) {
    char texto[5];
    snprintf(texto, sizeof(texto), "%04u", numero); // garante 4 dígitos com zeros à esquerda

    for (int i = 0; i < 4; i++) {
        oled_clear(oled);

        for (int j = 0; j <= i; j++) {
            uint8_t digito = texto[3 - i + j] - '0';
            int x = x_inicial + espacamento * j;
            oled_exibir_digito_grande(oled, digito, x);
        }

        oled_render(oled);
        sleep_ms(delay_ms);
    }
}
