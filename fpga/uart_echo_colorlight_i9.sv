// ======================================================================
// Módulo: uart_echo_colorlight_i9
// Descrição: Sistema que integra comunicação UART, teclado matricial,
//             LEDs e uma FSM de menu. Responsável por gerenciar troca
//             de dados entre uma interface UART interna e uma externa,
//             além de exibir informações via LEDs.
// ======================================================================
module uart_echo_colorlight_i9 #(
    parameter CLK_FREQ_HZ = 25_000_000,  // Clock do sistema (25 MHz)
    parameter BAUD_RATE   = 9600         // Taxa de transmissão UART
)(
    input  logic       clk_25mhz,            // Clock principal
    input  logic       reset_n,              // Reset ativo em nível baixo
    input  logic       uart_rx_interface,    // RX da UART de interface
    output logic       uart_tx_interface,    // TX da UART de interface

    input  logic       uart_rx_externo1,     // RX da UART externa
    output logic       uart_tx_externo1,     // TX da UART externa

    input  wire       R1, R2, R3, R4,        // Linhas do teclado
    output reg  [3:0] C,                     // Colunas do teclado
    output logic led_1, 
    output logic led_2,
    output logic led_3,
    output logic led_4,
    output logic led_5,
    output logic led_6,
    output logic led_7,
    output logic led_8
);

    // ----------------- Delay de inicialização -----------------
    // Aguarda alguns ciclos para estabilizar o reset interno.
    logic [7:0] reset_counter = 8'd0;
    logic reset_n_internal = 1'b0;

    always_ff @(posedge clk_25mhz) begin
        if (reset_counter < 8'd255) begin
            reset_counter <= reset_counter + 1'b1;
            reset_n_internal <= 1'b0;
        end else begin
            reset_n_internal <= 1'b1;
        end
    end

    // ----------------- Teclado matricial -----------------
    logic [1:0] col_index; // índice de coluna ativa

    teclado #(.CLK_FREQ(CLK_FREQ_HZ)) u_teclado (
        .clk(clk_25mhz),
        .rst_n(reset_n_internal),
        .C(C),
        .col_index(col_index)
    );
    
    // ----------------- FSM de Menu -----------------
    // Sinais de controle e dados do menu
    logic [1:0] sel_sala, sel_sensor, menu_state;
    logic [7:0] menu_tx_byte1, menu_tx_byte2; 

    // Pulsos de finalização de transmissão UART
    logic tx_done_if_pulse;
    logic tx_done_ext1_pulse;

    // Instância da FSM do menu
    fsm_menu #(.CLK_FREQ(CLK_FREQ_HZ)) u_menu (
        .clk(clk_25mhz),
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
        .i_clk(clk_25mhz),
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

    // ----------------- UART EXTERNA 1 -----------------
    logic       rx_dv_ext1;
    logic [7:0] rx_byte_ext1;
    logic       tx_dv_ext1;
    logic [7:0] tx_byte_ext1;
    logic       tx_active_ext1, tx_done_ext1;

    uart_top #(
        .CLK_FREQ_HZ(CLK_FREQ_HZ),
        .BAUD_RATE(BAUD_RATE)
    ) uart_ext1 (
        .i_clk(clk_25mhz),
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

    // ----------------- Sinais auxiliares -----------------
    reg tx_dv_if_stb_reg;     // strobe de transmissão da UART interface
    reg tx_dv_ext1_stb_reg;   // strobe de transmissão da UART externa
    
    reg        forward_buffer_valid; // flag indicando dado pendente de retransmissão
    reg [7:0]  forward_buffer_byte;  // byte armazenado para forwarding
    reg        tx_forward_mode;      // indica modo de reenvio direto

    // ----------------- FSM de Transmissão -----------------
    localparam ST_WAIT_MENU = 2'd0;
    localparam ST_TX_BYTE1  = 2'd1;
    localparam ST_TX_BYTE2  = 2'd2;
    localparam ST_WAIT_DATA = 2'd3;

    reg [1:0] tx_fsm_state = ST_WAIT_MENU;
    reg       ext_packet_ready = 1'b0;
    reg       tx_source = 1'b0; // 0 = menu, 1 = externo

    always_ff @(posedge clk_25mhz or negedge reset_n_internal) begin
        if (!reset_n_internal) begin
            // Inicialização de todos os sinais e flags
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
            tx_dv_if_stb_reg   <= 1'b0;
            tx_dv_ext1_stb_reg <= 1'b0;
        end else begin
            // Limpeza de pulsos e strobes
            tx_dv_if_stb_reg   <= 1'b0;
            tx_dv_ext1_stb_reg <= 1'b0;
            tx_done_if_pulse   <= 1'b0;
            tx_done_ext1_pulse <= 1'b0;

            // Geração dos pulsos de transmissão concluída
            if (tx_done_if) begin
                tx_done_if_pulse <= 1'b1;
            end
            if (tx_done_ext1) begin
                tx_done_ext1_pulse <= 1'b1;
            end

            // Encaminha dados recebidos externamente para interface, se possível
            if (rx_dv_ext1 && !tx_active_if ) begin
                tx_byte_if         <= rx_byte_ext1;
                tx_dv_if_stb_reg   <= 1'b1; 
            end
            
            // FSM principal de transmissão
            case(tx_fsm_state)
                ST_WAIT_MENU: begin
                    // Espera evento: menu ativo ou pacote externo pronto
                    if (rx_dv_ext1 ) begin
                        tx_byte_if       <= rx_byte_ext1;
                        tx_dv_if_stb_reg <= 1'b1;
                    end
                    
                    if (forward_buffer_valid) begin
                        tx_forward_mode <= 1'b1;
                        tx_source <= 1'b1; 
                        tx_fsm_state <= ST_TX_BYTE1;
                    end else if (ext_packet_ready) begin
                        tx_source <= 1'b1; // externo
                        tx_fsm_state <= ST_TX_BYTE1;
                    end else if (menu_state == 2'd2) begin // menu solicita envio
                        tx_source <= 1'b0;
                        tx_fsm_state <= ST_TX_BYTE1;
                    end
                end
                
                ST_TX_BYTE1: begin
                    // Transmite primeiro byte conforme origem
                    if (!tx_active_if && !tx_active_ext1) begin
                        if (tx_source == 1'b0) begin
                            // Envio simultâneo via menu
                            tx_byte_if   <= menu_tx_byte1;
                            tx_byte_ext1 <= menu_tx_byte1;
                            tx_dv_if_stb_reg   <= 1'b1; 
                            tx_dv_ext1_stb_reg <= 1'b1; 
                        end else begin
                            if (tx_forward_mode) begin
                                // Forward direto de byte externo
                                tx_byte_if   <= forward_buffer_byte;
                                tx_byte_ext1 <= 8'h00;
                                tx_dv_if_stb_reg   <= 1'b1;
                                tx_dv_ext1_stb_reg <= 1'b0;
                            end else begin
                                tx_byte_if   <= menu_tx_byte1;        
                                tx_byte_ext1 <= 8'h00;
                                tx_dv_if_stb_reg   <= 1'b1;
                                tx_dv_ext1_stb_reg <= 1'b0;
                            end
                        end
                        tx_fsm_state <= ST_TX_BYTE2; // Avança para segundo byte
                    end
                end
                
                ST_TX_BYTE2: begin
                    // Transmite segundo byte
                    if (tx_source == 1'b0) begin
                        if (tx_done_if_pulse && tx_done_ext1_pulse) begin 
                            tx_byte_if   <= measurement_byte;
                            tx_byte_ext1 <= measurement_byte;
                            tx_dv_if_stb_reg   <= 1'b1;
                            tx_dv_ext1_stb_reg <= 1'b1;
                            tx_fsm_state <= ST_WAIT_DATA;
                        end
                    end else begin
                        if (tx_forward_mode) begin
                            // Finaliza forwarding
                            if (tx_done_if_pulse) begin
                                forward_buffer_valid <= 1'b0; 
                                tx_forward_mode <= 1'b0;
                                tx_source <= 1'b0;
                                tx_fsm_state <= ST_WAIT_MENU;
                            end
                        end else begin
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
                    // Espera finalização do envio completo
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

            // Atualiza sinais de controle de transmissão
            tx_dv_if   <= tx_dv_if_stb_reg;
            tx_dv_ext1 <= tx_dv_ext1_stb_reg;
        end
    end

    // ----------------- Timeout de resposta externa -----------------
    localparam integer RESPONSE_TIMEOUT_CYCLES = CLK_FREQ_HZ / 1000; // ~1 ms @25MHz 

    // Variáveis de controle de recepção UART externa
    reg [31:0] rx_wait_counter;
    reg        external_single_byte;     
    reg [7:0]  measurement_byte;         

    // FSM de recepção e roteamento de dados externos
    reg [1:0] rx_fsm_state = 2'd0;
    reg [7:0] external_byte1 = 8'h00;
    reg [7:0] external_byte2 = 8'h00;
    reg [7:0] leds_reg;

    always_ff @(posedge clk_25mhz or negedge reset_n_internal) begin
        if (!reset_n_internal) begin
            // Reset de estados e registradores
            rx_fsm_state       <= 2'd0;
            external_byte1     <= 8'h00;
            external_byte2     <= 8'h00;
            ext_packet_ready   <= 1'b0;
            rx_wait_counter    <= 32'd0;
            external_single_byte <= 1'b0;
            measurement_byte   <= 8'h00;
            leds_reg           <= 8'h00;
            forward_buffer_valid <= 1'b0; 
        end else begin
            case (rx_fsm_state)
                2'd0: begin 
                    // Espera o primeiro byte do pacote externo
                    ext_packet_ready <= 1'b0;
                    rx_wait_counter  <= 32'd0;
                    if (rx_dv_ext1) begin
                        external_byte1 <= rx_byte_ext1;
                        forward_buffer_byte  <= rx_byte_ext1;
                        forward_buffer_valid <= 1'b1;
                        rx_wait_counter <= 32'd0;
                        rx_fsm_state <= 2'd1;
                    end
                end

                2'd1: begin 
                    // Espera o segundo byte ou timeout
                    if (rx_dv_ext1) begin
                        external_byte2 <= rx_byte_ext1;
                        external_single_byte <= 1'b0;
                        measurement_byte <= rx_byte_ext1;
                        ext_packet_ready <= 1'b1;
                        forward_buffer_byte  <= rx_byte_ext1;
                        forward_buffer_valid <= 1'b1;
                    end else if (rx_wait_counter < RESPONSE_TIMEOUT_CYCLES) begin
                        rx_wait_counter <= rx_wait_counter + 1;
                    end else begin
                        // Caso receba apenas 1 byte (timeout)
                        external_single_byte <= 1'b1;
                        external_byte2 <= 8'h00;
                        measurement_byte <= external_byte1; 
                        ext_packet_ready <= 1'b1;
                        leds_reg <= external_byte1;
                        rx_fsm_state <= 2'd2;
                    end
                end

                2'd2: begin 
                    // Aguarda liberação antes de voltar ao estado inicial
                    if (!ext_packet_ready) begin
                        rx_fsm_state <= 2'd0;
                    end
                end
                default: rx_fsm_state <= 2'd0;
            endcase
        end
    end

    // ----------------- Mapeamento dos LEDs -----------------
    // Cada bit do registrador leds_reg controla um LED físico.
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
