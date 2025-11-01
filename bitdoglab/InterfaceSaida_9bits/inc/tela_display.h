// tela_display.h
#ifndef TELA_DISPLAY_H
#define TELA_DISPLAY_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

// Enum com as telas disponíveis
typedef enum {
    TELA_DISPLAY = 0,
    TELA_AX,
    TELA_AY,
    TELA_AZ,
    TELA_GX,
    TELA_GY,
    TELA_GZ,
    TELA_TEMP,
    TOTAL_TELAS
} TipoTela;

// Índice global da tela atual
extern volatile TipoTela tela_atual;

// Mutex opcional (se for necessário proteger o acesso em outros contextos)
extern SemaphoreHandle_t mutex_tela;

#endif
