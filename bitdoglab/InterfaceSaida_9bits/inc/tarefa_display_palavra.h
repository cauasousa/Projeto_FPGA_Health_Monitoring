#ifndef TAREFA_DISPLAY_PALAVRA_H
#define TAREFA_DISPLAY_PALAVRA_H

#include "FreeRTOS.h"
#include "task.h"

// Cria a tarefa de OLED (núcleo 1) que exibe a palavra 6 bits
void criar_tarefa_display_palavra(UBaseType_t prio, UBaseType_t core_mask);

#endif