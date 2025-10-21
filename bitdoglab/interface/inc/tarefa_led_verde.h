#pragma once
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

void criar_tarefa_led_verde(UBaseType_t prio, UBaseType_t core_mask);

#ifdef __cplusplus
}
#endif
