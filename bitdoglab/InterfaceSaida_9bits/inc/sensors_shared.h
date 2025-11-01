#ifndef SENSORS_SHARED_H
#define SENSORS_SHARED_H

#include <stdint.h>

// Valor de temperatura lido do AHT10 (°C). Atualizado pela tarefa I2C.
extern volatile float g_aht_temperature;

#endif // SENSORS_SHARED_H
