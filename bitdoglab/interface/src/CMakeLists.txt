add_executable(InterfaceEntradaBCD
    main.c
    i2c_config.c
    oled_display.c
    ssd1306_text.c
    tarefa_word_9.c
    numeros_display.c
    oled_animacoes.c
    oled_context.c
    tarefa_led_verde.c
    digitos_grandes_utils.c 
    numeros_grandes.c
    tarefa_bcd8.c
    tarefa_display_duplo_bcd.c
)

# Corrige a saída para build/ em vez de build/src/
set_target_properties(InterfaceEntradaBCD PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_sources(InterfaceEntradaBCD PRIVATE
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(InterfaceEntradaBCD PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc

)

target_link_libraries(InterfaceEntradaBCD 
    pico_stdlib 
    FreeRTOS-Kernel
    hardware_adc
    hardware_i2c
    hardware_gpio
    hardware_dma 
    hardware_pwm 
    hardware_pio 
    hardware_clocks
)

pico_add_extra_outputs(InterfaceEntradaBCD)
