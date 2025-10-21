#include "FreeRTOS.h"
#include "oled_display.h"
#include "oled_context.h"
#include "tela_display.h"

volatile TipoTela tela_atual = TELA_DISPLAY;


// Variáveis globais definidas aqui
ssd1306_t oled;
SemaphoreHandle_t mutex_oled = NULL;

