# Ligação entre BitDogLab Interface (RP2040) e FPGA

Resumo rápido
- Nível lógico: ambos usam 3.3V → não é necessário conversor de nível.
- Sempre conectar GND comum entre RP2040 (BitDogLab) e FPGA.
- Baud padrão: 9600, 8N1 (conforme código).
- Protocolo: pacote = [0xAA][HI][LO] → palavra 12 bits = (HI<<8 | LO) & 0x0FFF
  - Bits [11:10] = seleção do sensor (01 = Temp, 10 = BPM)
  - Bits [9:8]   = seleção da sala (01 = Sala A, 10 = Sala B)
  - Bits [7:0]   = dados do sensor (8 bits)

Conexões principais (BitDogLab Interface I ↔ FPGA)
- BitDogLab TX (RP2040) -> FPGA RX (uart_interface_rx)
- BitDogLab RX (RP2040) <- FPGA TX (uart_interface_tx)
- GND (RP2040) <-> GND (FPGA)
- Pinos FPGA (conforme top.lpf):
  - FPGA uart_interface_rx = PT29A (J4)
  - FPGA uart_interface_tx = PT29B (J5)
- RP2040 default (exemplo usado no código):
  - UART0 TX = GP16 (conectar ao FPGA RX)
  - UART0 RX = GP17 (conectar ao FPGA TX)

Conexões adicionais (UARTs externas no FPGA)
- BitDogLab Externa I  <-> FPGA
  - FPGA RX (externa I)  = PR35A (N17)
  - FPGA TX (externa I)  = PR47D (T17)
  - Se a placa externa tiver um RP2040, conectar TX->FPGA RX e RX->FPGA TX, GND comum.
- BitDoglab Externa II <-> FPGA
  - FPGA RX (externa II) = PB15B (R3)
  - FPGA TX (externa II) = PB18A (T3)

Protocolo de troca de mensagens (fluxo recomendado)
1. Host (RP2040) envia primeiro byte HEADER = 0xAA para sincronizar.
2. Host envia dois bytes: HI, LO.
3. FPGA deve processar (HI<<8|LO) e considerar apenas 12 LSB.
4. Exemplo de mensagens:
   - Início:        0000 0000 0000 → HI=0x00, LO=0x00
   - Menu Sala A:   0001 0000 0000 → 0x0100 (HI=0x01, LO=0x00)
   - Menu Sala B:   0010 0000 0000 → 0x0200
   - Selecionar Temp: 0101 0000 0000 → 0x0500
   - Selecionar BPM:  1001 0000 0000 → 0x0900
   - Dados ex.: sensor temp 0x05 → 01 01 xxxx xxxx (montar conforme mapeamento)

Boas práticas e testes
- Teste 1 (verificação física):
  - Meça continuidade GND entre as duas placas.
  - Meça tensão nos pinos TX/RX (inativo deve ser nível alto 3.3V).
- Teste 2 (loop simples):
  - Configure RP2040 para enviar 0xAA, HI, LO com 10ms entre bytes e observe logs seriais do FPGA.
  - No firmware do FPGA/Top ou no terminal do RP2040, imprima os 12 bits recebidos.
- Timeout/Retry:
  - Implementar timeout no RP2040 (ex.: aguardar resposta < 10s).
  - Se usar eco/ack, definir byte de ACK no protocolo (opcional).
- Pull-ups/pull-downs:
  - Teclado matricial: linhas com pull-ups na FPGA (conforme top.lpf), colunas como saídas ativas-low.
  - UART não precisa de pull resistors adicionais se ambas as placas estiverem em 3.3V e com cabos curtos.

Dicas de debug
- Use um analisador lógico ou osciloscópio para verificar o header 0xAA e os dois bytes seguintes.
- No RP2040, habilite logs USB (stdio_usb) para ver o que foi enviado.
- No FPGA, faça leds de debug acenderem em estados de recepção ou erros de checksum (se implementar).

Mapeamento resumido (conectar ↔):
- RP2040 GP16 (TX)  -> FPGA PT29A (uart_interface_rx)
- RP2040 GP17 (RX)  <- FPGA PT29B (uart_interface_tx)
- RP2040 GND        -> FPGA GND
- (Opcional) RP2040 UART0 configurado em 9600, 8N1

Exemplo de sequência de teste (RP2040 envia):
1. Enviar [0xAA]
2. Enviar [0x01] [0x00]  -> deve mostrar "Selecione dados da Sala A" no OLED do FPGA.
3. Enviar [0x05] [0x64]  -> sensor temp ou BPM com dados (depende dos bits de controle) — verificar exibição.

Observação final
- Confirmar no projeto FPGA (top.lpf e projeto_top.sv) os nomes dos sinais usados (uart_interface_rx/tx, uart_ext1_rx/tx, uart_ext2_rx/tx) — esses nomes foram adotados no código fornecido.
- Se usar cabos longos (>20 cm) considere colocar resistores de pull-down/up ou buffers conforme necessidade.

Fim do README.
