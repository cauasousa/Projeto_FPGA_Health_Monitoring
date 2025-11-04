#ifndef HANDLES_TAREFAS_H
#define HANDLES_TAREFAS_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t handle_display;
extern TaskHandle_t handle_ax;
extern TaskHandle_t handle_ay;
extern TaskHandle_t handle_az;
extern TaskHandle_t handle_gx;
extern TaskHandle_t handle_gy;
extern TaskHandle_t handle_gz;
extern TaskHandle_t handle_temp;
extern TaskHandle_t handle_botao_a;
extern TaskHandle_t handle_leitura_mpu;

#endif
