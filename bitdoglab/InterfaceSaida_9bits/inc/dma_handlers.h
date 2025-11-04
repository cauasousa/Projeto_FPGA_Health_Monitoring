#ifndef DMA_HANDLERS_H
#define DMA_HANDLERS_H

#ifdef __cplusplus
extern "C" {
#endif

// ISR do DMA IRQ 1 (notifica a tarefa do joystick via FreeRTOS)
void dma_handler_joy_xy(void);

#ifdef __cplusplus
}
#endif

#endif
