// tela_display.c
#include "tela_display.h"

volatile TipoTela tela_atual = TELA_DISPLAY;
SemaphoreHandle_t mutex_tela = NULL;
