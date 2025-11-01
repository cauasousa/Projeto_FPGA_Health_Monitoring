#include "max30102.h"

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define MAX30102_ADDRESS 0x57
#define MEDIA_MOVEL_N 10

// Buffers circulares para armazenar amostras IR e RED
static uint32_t ir_buffer[MEDIA_MOVEL_N];
static uint32_t red_buffer[MEDIA_MOVEL_N];
static int media_index = 0;

// Estado para detecção de picos
static absolute_time_t ultimo_pico = {0};
static bool pico_detectado = false;

// Valores públicos exportados
volatile uint16_t g_max_bpm = 0;   // BPM aproximado (inteiro)
volatile uint8_t g_max_spo2 = 0;   // SpO2 em % (0..100)

// Escrita simples em registrador
// escreve um registrador e retorna true se o escravo ACKou
static bool max30102_write_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = { reg, value };
    int ret = i2c_write_blocking(i2c, MAX30102_ADDRESS, buffer, 2, false);
    if (ret != 2) {
        // write failed (no debug print to keep logs clean)
        return false;
    }
    return true;
}

static bool max30102_read_reg(i2c_inst_t *i2c, uint8_t reg, uint8_t *out)
{
    int w = i2c_write_blocking(i2c, MAX30102_ADDRESS, &reg, 1, true);
    if (w != 1) {
        // write (register address) failed/NAK
        return false;
    }
    int r = i2c_read_blocking(i2c, MAX30102_ADDRESS, out, 1, false);
    return (r == 1);
}

// para depuração, imprime alguns registradores importantes
static void max30102_dump_registers(i2c_inst_t *i2c)
{
    // intentionally empty: debug register dump disabled to reduce serial noise
    (void)i2c;
}

// Leitura simples de FIFO (6 bytes => RED(3) + IR(3))
static bool max30102_read_fifo_once(i2c_inst_t *i2c, uint32_t *ir, uint32_t *red)
{
    uint8_t reg = 0x07; // FIFO_DATA
    uint8_t data[6];
    // Aponta para registrador FIFO_DATA
    if (i2c_write_blocking(i2c, MAX30102_ADDRESS, &reg, 1, true) != 1) return false;
    int res = i2c_read_blocking(i2c, MAX30102_ADDRESS, data, 6, false);
    if (res != 6) return false;
    // Cada valor 3 bytes: RED then IR
    *red = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    *ir  = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    return true;
}

bool max30102_init(i2c_inst_t *i2c)
{
    // Configura registradores básicos (similar ao código de exemplo)
    // Zerando ponteiros FIFO
    // Zerando ponteiros FIFO
    (void)max30102_write_reg(i2c, 0x02, 0x00); // FIFO_WR_PTR
    (void)max30102_write_reg(i2c, 0x03, 0x00); // OVF_COUNTER
    (void)max30102_write_reg(i2c, 0x04, 0x00); // FIFO_RD_PTR
    // Modo SPO2 (ativa RED e IR) -> reg 0x09 (MODE_CONFIG)
    (void)max30102_write_reg(i2c, 0x09, 0x03);
    // SPO2 config: sample rate e pulse width -> reg 0x0A
    (void)max30102_write_reg(i2c, 0x0A, 0x1F);
    // Intensidade LEDs (LED1 = IR, LED2 = RED)
    (void)max30102_write_reg(i2c, 0x0C, 0x7F); // LED1 (IR)
    (void)max30102_write_reg(i2c, 0x0D, 0x7F); // LED2 (RED)

    // Inicializa buffers e estado
    for (int i = 0; i < MEDIA_MOVEL_N; ++i) {
        ir_buffer[i] = 0;
        red_buffer[i] = 0;
    }
    media_index = 0;
    pico_detectado = false;
    ultimo_pico = get_absolute_time();
    g_max_bpm = 0;
    g_max_spo2 = 0;

    return true;
}

static float calcular_media(uint32_t *buffer, int tamanho)
{
    uint64_t soma = 0;
    for (int i = 0; i < tamanho; i++) soma += buffer[i];
    return (float)soma / (float)tamanho;
}

static float calcular_spo2(uint32_t ir_ac, uint32_t ir_dc, uint32_t red_ac, uint32_t red_dc)
{
    if (ir_dc == 0 || red_dc == 0) return 0.0f;
    float R = ((float)red_ac / (float)red_dc) / ((float)ir_ac / (float)ir_dc);
    float spo2 = 110.0f - 25.0f * R;
    if (spo2 > 100.0f) spo2 = 100.0f;
    if (spo2 < 0.0f) spo2 = 0.0f;
    return spo2;
}

void max30102_process_once(i2c_inst_t *i2c)
{
    uint32_t ir, red;
    if (!max30102_read_fifo_once(i2c, &ir, &red)) {
        // se não houver dados, não altera os valores existentes
        return;
    }

    // armazena nas janelas circulares
    ir_buffer[media_index] = ir;
    red_buffer[media_index] = red;
    media_index = (media_index + 1) % MEDIA_MOVEL_N;

    // calcula DC (média) e AC (diferença)
    float ir_media = calcular_media(ir_buffer, MEDIA_MOVEL_N);
    float red_media = calcular_media(red_buffer, MEDIA_MOVEL_N);
    float ir_ac = fabsf((float)ir - ir_media);
    float red_ac = fabsf((float)red - red_media);

    // detecção simples de pico para BPM
    bool acima = ((float)ir > ir_media * 1.01f);
    if (acima && !pico_detectado) {
        pico_detectado = true;
        absolute_time_t agora = get_absolute_time();
        int64_t intervalo_us = absolute_time_diff_us(ultimo_pico, agora);
        if (intervalo_us > 100000) { // Ignora picos muito próximos (<100ms)
            float bpm_f = 60.0f * 1000000.0f / (float)intervalo_us;
            if (bpm_f < 0.0f) bpm_f = 0.0f;
            if (bpm_f > 65535.0f) bpm_f = 65535.0f;
            g_max_bpm = (uint16_t)roundf(bpm_f);
            ultimo_pico = agora;
        }
    } else if (!acima) {
        pico_detectado = false;
    }

    // estima SpO2
    float spo2f = calcular_spo2((uint32_t)ir_ac, (uint32_t)ir_media, (uint32_t)red_ac, (uint32_t)red_media);
    if (spo2f < 0.0f) spo2f = 0.0f;
    if (spo2f > 100.0f) spo2f = 100.0f;
    g_max_spo2 = (uint8_t)roundf(spo2f);
}
