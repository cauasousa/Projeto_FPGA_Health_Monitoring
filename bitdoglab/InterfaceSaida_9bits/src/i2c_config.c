#include "i2c_config.h"

// Inicializa o I2C com pull-up e função nos pinos
void i2c_configurar(i2c_inst_t *i2c, uint sda_pin, uint scl_pin) {
    i2c_init(i2c, 400 * 1000); // 400 kHz

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // Opcional: imprime qual barramento foi configurado
    printf("I2C configurado: %s → SDA=%u, SCL=%u\n",(i2c == i2c0 ? "i2c0" : "i2c1"), sda_pin, scl_pin);
}
