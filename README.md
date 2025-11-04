# Projeto FPGA — Conexões (match .lpf)

Resumo
- Este README lista as conexões de pinos usadas pelo projeto, espelhando o arquivo `uart_colorlight_i9.lpf`.
- A seção de LEDs foi omitida conforme solicitado.

Clock
- clk_50mhz : P3 (oscillador principal / 25 MHz no .lpf)
  - Observação: o .lpf informa 25 MHz; ajuste de clocks/PLL pode ser necessário caso módulos esperem 50 MHz.

Reset
- reset_n : D1 (botão de reset, ativo baixo, pull-up)

UART — Interface (Bitdoglab Interface I)
- uart_tx_interface : J5 (saída TX do FPGA)
- uart_rx_interface : J4 (entrada RX para o FPGA)

UART — Externa I (Bitdoglab Externa I)
- uart_tx_externo1 : T17 (saída TX do FPGA para dispositivo externo)
- uart_rx_externo1 : N17 (entrada RX do FPGA a partir do dispositivo externo)

UART — Externa II (se presente)
- uart_ext2_tx : T3
- uart_ext2_rx : T2

Teclado 4x4 (linhas e colunas)
- Linhas (entradas com pull-up):
  - R1 : B19
  - R2 : A18
  - R3 : C2
  - R4 : B20
- Colunas (saídas, varredura ativa baixa):
  - C[0] : E2
  - C[1] : A19
  - C[2] : B18
  - C[3] : B1

Observações rápidas
- As rotas e nomes acima refletem o conteúdo do arquivo `uart_colorlight_i9.lpf`.
- Se o módulo UART no código assume 50 MHz e o .lpf fornece 25 MHz, verifique/ajuste `CLKS_PER_BIT` ou adicione PLL para gerar o clock esperado.
- Ignorei a parte de LEDs conforme pedido.

Fim.
