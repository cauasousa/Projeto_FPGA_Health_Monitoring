module uart_echo_colorlight_i9 #(
    parameter CLK_FREQ_HZ = 25_000_000,  // Clock do sistema (25 MHz)
    parameter BAUD_RATE   = 9600        // Taxa de transmissão
)(
    input  logic       clk_50mhz,
    input  logic       reset_n,
    input  logic       uart_rx_interface,
    output logic       uart_tx_interface,

    input  logic       uart_rx_externo1,
    output logic       uart_tx_externo1,

    input  wire       R1, R2, R3, R4,
    output reg  [3:0] C,
    output logic led_1, 
    output logic led_2,
    output logic led_3,
    output logic led_4,
    output logic led_5,
    output logic led_6,
    output logic led_7,
    output logic led_8
);

    // Delay para inicialização do sistema
    logic [7:0] reset_counter = 8'd0;
    logic reset_n_internal = 1'b0;

    always_ff @(posedge clk_50mhz) begin
        if (reset_counter < 8'd255) begin
            reset_counter <= reset_counter + 1'b1;
            reset_n_internal <= 1'b0;
        end else begin
            reset_n_internal <= 1'b1;
        end
    end

    // ----------------- Teclado -----------------
    logic [1:0] col_index;

    teclado #(.CLK_FREQ(CLK_FREQ_HZ)) u_teclado (
        .clk(clk_50mhz),
        .rst_n(reset_n_internal),
        .C(C),
        .col_index(col_index)
    );
    
    // menu FSM
    // --- SINAIS DO MENU ---
    logic [1:0] sel_sala, sel_sensor, menu_state;
    logic [7:0] menu_tx_byte1, menu_tx_byte2; // recebe tx_byte1/tx_byte2 do fsm_menu

    // --- UART TX DV/DONE (pulsos usados pelo menu) ---
    logic tx_done_if_pulse;
    logic tx_done_ext1_pulse;

    // Instância do menu: usar clk_50mhz e reset_n_internal; mapear corretamente sinais
    fsm_menu #(.CLK_FREQ(CLK_FREQ_HZ)) u_menu (
        .clk(clk_50mhz),
        .rst_n(reset_n_internal),
        .R1(R1),
        .R2(R2),
        .col_index(col_index),
        .tx_done_pulse(tx_done_if_pulse),
        .ext_tx_done_pulse(tx_done_ext1_pulse),
        .sel_sala(sel_sala),
        .sel_sensor(sel_sensor),
        .menu_state(menu_state),
        .tx_byte1(menu_tx_byte1),
        .tx_byte2(menu_tx_byte2)
    );
    // ----------------- UART INTERFACE -----------------
    logic       rx_dv_if;
    logic [7:0] rx_byte_if;
    logic       tx_dv_if;
    logic [7:0] tx_byte_if;
    logic       tx_active_if, tx_done_if;

    uart_top #(
        .CLK_FREQ_HZ(CLK_FREQ_HZ),
        .BAUD_RATE(BAUD_RATE)
    ) uart_if (
        .i_clk(clk_50mhz),
        .i_rst_n(reset_n_internal),
        .i_uart_rx(uart_rx_interface),
        .o_uart_tx(uart_tx_interface),
        .i_tx_dv(tx_dv_if),
        .i_tx_byte(tx_byte_if),
        .o_tx_active(tx_active_if),
        .o_tx_done(tx_done_if),
        .o_rx_dv(rx_dv_if),
        .o_rx_byte(rx_byte_if)
    );

    // ----------------- UART EXTERNO1 -----------------
    logic       rx_dv_ext1;
    logic [7:0] rx_byte_ext1;
    logic       tx_dv_ext1;
    logic [7:0] tx_byte_ext1;
    logic       tx_active_ext1, tx_done_ext1;

    uart_top #(
        .CLK_FREQ_HZ(CLK_FREQ_HZ),
        .BAUD_RATE(BAUD_RATE)
    ) uart_ext1 (
        .i_clk(clk_50mhz),
        .i_rst_n(reset_n_internal),
        .i_uart_rx(uart_rx_externo1),
        .o_uart_tx(uart_tx_externo1),
        .i_tx_dv(tx_dv_ext1),
        .i_tx_byte(tx_byte_ext1),
        .o_tx_active(tx_active_ext1),
        .o_tx_done(tx_done_ext1),
        .o_rx_dv(rx_dv_ext1),
        .o_rx_byte(rx_byte_ext1)
    );

    reg tx_dv_if_stb_reg;
    reg tx_dv_ext1_stb_reg;

    // --- Forward buffer: encaminhar bytes recebidos externamente para a interface imediatamente
    reg        forward_buffer_valid;
    reg [7:0]  forward_buffer_byte;
    reg        tx_forward_mode; // 1 = enviar forward buffer (single-byte flow)

    // --- NOVO ROTEAMENTO DE DADOS ---
    localparam ST_WAIT_MENU = 2'd0;
    localparam ST_TX_BYTE1  = 2'd1;
    localparam ST_TX_BYTE2  = 2'd2;
    localparam ST_WAIT_DATA = 2'd3;

    reg [1:0] tx_fsm_state = ST_WAIT_MENU;

    // Novo: sinaliza pacote externo pronto para transmissão (rx_fsm escreve, tx_fsm consome)
    reg       ext_packet_ready = 1'b0;
    reg       tx_source = 1'b0; // 0 = menu, 1 = external

    always_ff @(posedge clk_50mhz or negedge reset_n_internal) begin
        if (!reset_n_internal) begin
            tx_fsm_state <= ST_WAIT_MENU;
            tx_dv_if     <= 1'b0;
            tx_byte_if   <= 8'h00;
            tx_dv_ext1   <= 1'b0;
            tx_byte_ext1 <= 8'h00;
            tx_done_if_pulse   <= 1'b0;
            tx_done_ext1_pulse <= 1'b0;
            ext_packet_ready <= 1'b0;
            tx_source <= 1'b0;
            forward_buffer_valid <= 1'b0;
            forward_buffer_byte  <= 8'h00;
            tx_forward_mode      <= 1'b0;
            // init strobes
            tx_dv_if_stb_reg   <= 1'b0;
            tx_dv_ext1_stb_reg <= 1'b0;
        end else begin
            // Reset pulsos de 
              // Default: limpa strobes no início do ciclo (serão setadas quando precisar)
            tx_dv_if_stb_reg   <= 1'b0;
            tx_dv_ext1_stb_reg <= 1'b0;

            // Reset pulsos de Done (só um ciclo)
            tx_done_if_pulse   <= 1'b0;
            tx_done_ext1_pulse <= 1'b0;

            
            if (tx_done_if) begin
                tx_done_if_pulse <= 1'b1;
            end
            if (tx_done_ext1) begin
                tx_done_ext1_pulse <= 1'b1;
                // Removido: tx_done_if_pulse <= 1'b1;  // agora não forçamos mapeamento
            end
            // ----------------------------------------
            // Envio imediato ao receber da externa:
            // Use o strobe tx_dv_if_stb_reg (não atribua tx_dv_if diretamente,
            // pois ele é atualizado no final do always_ff a partir do strobe).
            // Verifica se os UARTs estão livres antes de disparar.
            if (rx_dv_ext1 && !tx_active_if ) begin
                tx_byte_if         <= rx_byte_ext1;
                tx_dv_if_stb_reg   <= 1'b1; // 1-ciclo strobe para uart_if
                // não altera tx_fsm_state aqui; mantemos FSM intacta
            end
            // Lógica FSM de Transmissão (único lugar que dirige tx_dv_if/tx_byte_if)
            case(tx_fsm_state)
                ST_WAIT_MENU: begin
                    // Prioridade máxima: se dados externos chegarem e UARTs livres, envie imediato via strobe
                    if (rx_dv_ext1 ) begin
                        tx_byte_if       <= rx_byte_ext1;
                        tx_dv_if_stb_reg <= 1'b1;
                    end
                    // Prioridade: se houver byte imediato a encaminhar, dispare envio para interface
                    if (forward_buffer_valid) begin
                        tx_forward_mode <= 1'b1;
                        tx_source <= 1'b1; // tratar como "external" para fluxo
                        tx_fsm_state <= ST_TX_BYTE1;
                    end else
                    if (ext_packet_ready) begin
                        tx_source <= 1'b1; // external
                        tx_fsm_state <= ST_TX_BYTE1;
                    end else if (menu_state == 2'd2) begin // ST_SEND_DATA (menu)
                        tx_source <= 1'b0; // menu
                        tx_fsm_state <= ST_TX_BYTE1;
                    end
                end
                
                ST_TX_BYTE1: begin
                    // Inicia transmissão do Byte 1 (origem depende de tx_source)
                    if (!tx_active_if && !tx_active_ext1) begin
                        if (tx_source == 1'b0) begin
                            // menu: enviar PARA AMBAS UARTs (interface + externa)
                            
                            tx_byte_if   <= menu_tx_byte1;
                            
                            tx_byte_ext1 <= menu_tx_byte1;
                             tx_dv_if_stb_reg   <= 1'b1; // 1-ciclo
                            tx_dv_ext1_stb_reg <= 1'b1; // 1-ciclo
                        end else begin
                            if (tx_forward_mode) begin
                                // encaminhar byte buffered diretamente PARA INTERFACE (single-byte forward)
                                tx_byte_if   <= forward_buffer_byte;
                                tx_byte_ext1 <= 8'h00;
                                tx_dv_if_stb_reg   <= 1'b1;
                               tx_dv_ext1_stb_reg <= 1'b0;
                            end else begin
                                // external response: encaminhar seleção primeiro PARA INTERFACE
                                tx_byte_if   <= menu_tx_byte1;        // mantem a seleção como primeiro byte
                                tx_byte_ext1 <= 8'h00;
                                tx_dv_if_stb_reg   <= 1'b1;
                                tx_dv_ext1_stb_reg <= 1'b0;
                            end
                        end
                        tx_fsm_state <= ST_TX_BYTE2; // Próximo passo
                    end
                end
                
                ST_TX_BYTE2: begin
                    // Inicia transmissão do Byte 2 após ambos terem sido enviados se menu OR quando tx_done pulso
                    if (tx_source == 1'b0) begin
                        // menu: aguarda conclusão de AMBAS as transmissões antes de enviar o segundo byte
                        if (tx_done_if_pulse && tx_done_ext1_pulse) begin
                            
                            tx_byte_if   <= measurement_byte;
                            
                            tx_byte_ext1 <= measurement_byte;
                            tx_dv_if_stb_reg   <= 1'b1;
                            tx_dv_ext1_stb_reg <= 1'b1;
                            tx_fsm_state <= ST_WAIT_DATA;
                        end
                    end else begin
                        if (tx_forward_mode) begin
                            // aguardamos conclusão da transmissão do forward_byte na interface
                            if (tx_done_if_pulse) begin
                                forward_buffer_valid <= 1'b0; // consumido
                                tx_forward_mode <= 1'b0;
                                tx_source <= 1'b0;
                                tx_fsm_state <= ST_WAIT_MENU;
                            end
                        end else begin
                            // external: enviar measurement_byte PARA INTERFACE após byte1 enviado
                            if (tx_done_if_pulse) begin
                                tx_byte_if   <= measurement_byte;
                                tx_byte_ext1 <= 8'h00;
                                tx_dv_if_stb_reg   <= 1'b1;
                                tx_dv_ext1_stb_reg <= 1'b0;
                                tx_fsm_state <= ST_WAIT_DATA;
                            end
                        end
                    end
                end
                
                ST_WAIT_DATA: begin
                   
                    if (tx_source == 1'b1) begin
                        if (tx_done_if_pulse) begin
                            ext_packet_ready <= 1'b0;
                            tx_source <= 1'b0;
                            tx_fsm_state <= ST_WAIT_MENU;
                        end
                    end else begin
                        if (menu_state != 2'd2) begin 
                            tx_fsm_state <= ST_WAIT_MENU;
                        end
                    end
                end
                
                default: tx_fsm_state <= ST_WAIT_MENU;
            endcase
            // Atualiza os sinais físicos i_tx_dv para as instâncias uart_top (strobes de 1 ciclo)
            tx_dv_if   <= tx_dv_if_stb_reg;
            tx_dv_ext1 <= tx_dv_ext1_stb_reg;
        end
        

    end

    // --- PARAMETROS PARA TIMEOUT (detectar resposta de 1 byte) ---
    localparam integer RESPONSE_TIMEOUT_CYCLES = CLK_FREQ_HZ / 1000; // ~1 ms @ 25MHz (ajuste se necessário)

    // Variáveis para RX externo melhoradas
    reg [31:0] rx_wait_counter;
    reg        external_single_byte;      // 1 = single-byte response, 0 = two-byte response expected
    reg [7:0]  measurement_byte;         // valor final a ser enviado como segundo byte

    // --- Roteamento de dados (Doglab Externa -> Interface) ---
    // rx_fsm captura 1 ou 2 bytes; se só 1 byte chegar dentro do timeout, trata como single-byte.
    reg [1:0] rx_fsm_state = 2'd0;
    reg [7:0] external_byte1 = 8'h00;
    reg [7:0] external_byte2 = 8'h00;

    // --- NEW: registrador de LEDs (atualiza somente quando pacote externo confirmado) ---
    reg [7:0] leds_reg;

    always_ff @(posedge clk_50mhz or negedge reset_n_internal) begin
        if (!reset_n_internal) begin
            rx_fsm_state       <= 2'd0;
            external_byte1     <= 8'h00;
            external_byte2     <= 8'h00;
            ext_packet_ready   <= 1'b0;
            rx_wait_counter    <= 32'd0;
            external_single_byte <= 1'b0;
            measurement_byte   <= 8'h00;
            // init leds
            leds_reg           <= 8'h00;
           forward_buffer_valid <= 1'b0; // garantir reset consistente se não inicializado acima
        end else begin
            case (rx_fsm_state)
                2'd0: begin // Aguarda Byte 1 da Externa
                    ext_packet_ready <= 1'b0;
                    rx_wait_counter  <= 32'd0;
                    if (rx_dv_ext1) begin
                        external_byte1 <= rx_byte_ext1;
                        // sempre bufferizar o byte recebido para encaminhar à interface
                        forward_buffer_byte  <= rx_byte_ext1;
                        forward_buffer_valid <= 1'b1;
                        // começa a janela para receber um possível segundo byte
                        rx_wait_counter <= 32'd0;
                        rx_fsm_state <= 2'd1;
                    end
                end

                2'd1: begin // Espera segundo byte ou timeout -> decide single/two byte
                    if (rx_dv_ext1) begin
                        // Recebeu segundo byte
                        external_byte2 <= rx_byte_ext1;
                        external_single_byte <= 1'b0;
                        measurement_byte <= rx_byte_ext1;
                        ext_packet_ready <= 1'b1;
                        // Atualiza LEDs com o valor confirmado
                        // leds_reg <= rx_byte_ext1;
                        // menu_state <= 2'd2;
                        forward_buffer_byte  <= rx_byte_ext1;
                        forward_buffer_valid <= 1'b1;
                        // tx_byte2 <= qual dados é o valor do segundo pacote?
                    end else if (rx_wait_counter < RESPONSE_TIMEOUT_CYCLES) begin
                        rx_wait_counter <= rx_wait_counter + 1;
                    end else begin
                        external_single_byte <= 1'b1;
                        external_byte2 <= 8'h00;
                        measurement_byte <= external_byte1; 
                        ext_packet_ready <= 1'b1;
                        leds_reg <= external_byte1;
                        // tx_byte2 <= qual dados é o valor do segundo pacote?
                        rx_fsm_state <= 2'd2;
                        // menu_state <= 2'd2;
                    end
                end

                2'd2: begin // Aguardando tx_fsm consumir pacote (ext_packet_ready cleared)
                    if (!ext_packet_ready) begin
                        rx_fsm_state <= 2'd0;
                    end
                end

                default: rx_fsm_state <= 2'd0;
            endcase
            
        end
        
    end

    // Mapear leds_reg para saídas físicas (led_1 = LSB)
    always_comb begin
        led_1 = leds_reg[0];
        led_2 = leds_reg[1];
        led_3 = leds_reg[2];
        led_4 = leds_reg[3];
        led_5 = leds_reg[4];
        led_6 = leds_reg[5];
        led_7 = leds_reg[6];
        led_8 = leds_reg[7];
    end

    // --- Fim do Arquivo ---
endmodule