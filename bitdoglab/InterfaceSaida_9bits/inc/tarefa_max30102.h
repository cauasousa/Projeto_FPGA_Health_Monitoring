// inc/tarefa_max30102.h
#ifndef TAREFA_MAX30102_H
#define TAREFA_MAX30102_H

#include "FreeRTOS.h"

void criar_tarefa_max30102(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms);

#endif // TAREFA_MAX30102_H
