// tarefa_display_duplo_bcd.h — Mostra dois números grandes (dezena/unidade) centralizados no OLED

#ifndef TAREFA_DISPLAY_DUPLO_BCD_H
#define TAREFA_DISPLAY_DUPLO_BCD_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

// Cria a task que exibe dois dígitos grandes centralizados (usa a tarefa_bcd8 como fonte)
void criar_tarefa_display_duplo_bcd(UBaseType_t prio, UBaseType_t core_mask);

#ifdef __cplusplus
}
#endif

#endif // TAREFA_DISPLAY_DUPLO_BCD_H
