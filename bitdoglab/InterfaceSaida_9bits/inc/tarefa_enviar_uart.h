#ifndef TAREFA_ENVIAR_UART_H
#define TAREFA_ENVIAR_UART_H

#include "FreeRTOS.h"
#include "task.h"
extern volatile bool s_rx_ready;

#ifdef __cplusplus
extern "C"
{
#endif

    void criar_tarefa_enviar_uart(UBaseType_t prio, UBaseType_t core_mask);

#ifdef __cplusplus
}
#endif

#endif // TAREFA_ENVIAR_UART_H
