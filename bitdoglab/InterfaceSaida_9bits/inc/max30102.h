#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

bool max30102_init(i2c_inst_t *i2c);
// Executa uma iteração de leitura/Processamento (lê FIFO e atualiza estatísticas)
void max30102_process_once(i2c_inst_t *i2c);

// Valores públicos (atualizados pela task/driver)
extern volatile uint16_t g_max_bpm;   // BPM aproximado
extern volatile uint8_t g_max_spo2;   // SpO2 em % (0..100)

#endif // MAX30102_H
