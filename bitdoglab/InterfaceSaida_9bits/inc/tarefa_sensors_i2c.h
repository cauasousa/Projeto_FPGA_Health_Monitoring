// inc/tarefa_sensors_i2c.h
#ifndef TAREFA_SENSORS_I2C_H
#define TAREFA_SENSORS_I2C_H

#include "FreeRTOS.h"

void criar_tarefa_sensors_i2c(UBaseType_t prio, UBaseType_t core_mask, uint32_t period_ms);

#endif // TAREFA_SENSORS_I2C_H
