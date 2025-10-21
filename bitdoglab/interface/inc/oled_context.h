#ifndef OLED_CONTEXT_H
#define OLED_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "oled_display.h"   // Define ssd1306_t e funções OLED
#include "semphr.h"         // Para 
#include "tela_display.h"
extern volatile TipoTela tela_atual;


// Variável global do display OLED
extern ssd1306_t oled;

// Mutex para proteger acesso concorrente ao OLED
extern SemaphoreHandle_t mutex_oled;

#ifdef __cplusplus
}
#endif

#endif // OLED_CONTEXT_H
