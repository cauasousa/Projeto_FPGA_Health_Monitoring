#ifndef SENSOR_WORD8_H
#define SENSOR_WORD8_H

#include <stdint.h>
#include "FreeRTOS.h"

// Cria tarefa que lê (ou simula) um sensor analógico e atualiza os pinos word8
// period_ms: intervalo de leitura em ms
void criar_tarefa_sensor_word8(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms);

#endif // SENSOR_WORD8_H
