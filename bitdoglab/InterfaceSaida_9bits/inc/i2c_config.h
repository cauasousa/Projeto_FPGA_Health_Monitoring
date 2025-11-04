#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Inicializa o barramento I²C com os pinos fornecidos (SDA/SCL)
void i2c_configurar(i2c_inst_t *i2c, uint sda_pin, uint scl_pin);

#endif // I2C_CONFIG_H
