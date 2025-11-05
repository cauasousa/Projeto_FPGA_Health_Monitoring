/*

Módulo: fsm_menu
Descrição geral:
Este módulo implementa uma máquina de estados finitos (FSM) responsável
por gerenciar o menu de seleção de sala e sensor em um sistema baseado
em teclado matricial e comunicação UART.

Parâmetros:
- CLK_FREQ: frequência do clock de entrada em Hz (padrão: 25 MHz).

Entradas:
- clk: clock principal do sistema.
- rst_n: sinal de reset ativo em nível baixo.
- R1, R2: sinais de entrada vindos das linhas do teclado.
- col_index: indica qual coluna do teclado está sendo varrida.
- tx_done_pulse: pulso que indica término de transmissão na UART interna.
- ext_tx_done_pulse: pulso que indica término de transmissão na UART externa.

Saídas:
- sel_sala: seleciona a sala desejada (2 bits).
- sel_sensor: seleciona o sensor desejado (2 bits).
- menu_state: indica o estado atual da FSM (menu de sala, menu de sensor ou envio).
- tx_byte1: primeiro byte a ser transmitido, contendo os identificadores
de sensor e sala ({4'b0, sel_sensor, sel_sala}).
- tx_byte2: segundo byte de transmissão (fixo 0x00, usado como dado de requisição).

Estados principais:
- ST_MENU_SALA: permite escolher a sala através das teclas.
- ST_MENU_SENSOR: permite escolher o tipo de sensor.
- ST_SEND_DATA: envia os dados selecionados (sala e sensor) via UART e aguarda
confirmação de transmissão (tx_done_pulse e ext_tx_done_pulse).

Mecanismos internos:
- Detecta pressionamento de teclas R1 e R2 com detecção de borda.
- Implementa um atraso de debounce via contador (menu_delay_counter).
- Alterna automaticamente entre seleção de sala e seleção de sensor.
- Gera bytes de transmissão quando um item é escolhido.

Observações:
- O módulo alterna o estado do menu após a confirmação de envio via UART.
- O parâmetro AUTO_SEND_PERIOD define o tempo para envio automático,
embora nesta versão esteja habilitado mas não seja utilizado.
-------------------------------------------------------------

*/

`timescale 1ns/1ps
`default_nettype none

module fsm_menu #(
    parameter integer CLK_FREQ = 25_000_000
)(
    input  wire clk,
    input  wire rst_n,
    input  wire R1, R2,
    input  wire [1:0] col_index,
    input  wire tx_done_pulse,
    input  wire ext_tx_done_pulse,
    output reg  [1:0] sel_sala,
    output reg  [1:0] sel_sensor,
    output reg  [1:0] menu_state,
    output reg  [7:0] tx_byte1, // Byte 1: {4'b0, sel_sensor, sel_sala}
    output reg  [7:0] tx_byte2  // Byte 2: Dados (fixo 0x00 para requisição)
);

    localparam ST_MENU_SALA   = 2'd0;
    localparam ST_MENU_SENSOR = 2'd1;
    localparam ST_SEND_DATA   = 2'd2;

    reg [1:0] last_menu_state = ST_MENU_SALA;
    reg R1_last = 1'b1, R2_last = 1'b1;
    wire R1_pressed = ~R1 & R1_last;
    wire R2_pressed = ~R2 & R2_last;

    reg [31:0] menu_delay_counter = 0;
    reg menu_delay_active = 0;
    reg [31:0] auto_send_counter = 0;
    localparam AUTO_SEND_PERIOD = CLK_FREQ / 2; 

    localparam MENU_DELAY_CYCLES = CLK_FREQ / 2;

    reg auto_send_enabled = 1'b1;
    
    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            menu_state <= ST_MENU_SALA;
            sel_sala <= 0;
            sel_sensor <= 0;
            R1_last <= 1'b1;
            R2_last <= 1'b1;
            tx_byte1 <= 8'h00; // NOVO
            tx_byte2 <= 8'h00; // NOVO
        end else begin
            R1_last <= R1;
            R2_last <= R2;
            
            if (menu_state == ST_SEND_DATA) begin
                
                tx_byte1 <= {4'b0000, sel_sensor, sel_sala};
                
                tx_byte2 <= tx_byte2; 
            end else begin
                tx_byte1 <= 8'h00;
                tx_byte2 <= 8'h00;
            end
            
            if(menu_delay_active) begin
                if(menu_delay_counter < MENU_DELAY_CYCLES - 1)
                    menu_delay_counter <= menu_delay_counter + 1;
                else
                    menu_delay_active <= 0;
            end else
                menu_delay_counter <= 0;

            case(menu_state)
                ST_MENU_SALA: begin
                    if(!menu_delay_active && (R1_pressed)) begin    
                        case(col_index)
                            2'd0: sel_sala <= 2'b01;
                            2'd1: sel_sala <= 2'b10;
                        endcase
                        last_menu_state <= menu_state;
                        menu_state <= ST_SEND_DATA;
                        menu_delay_active <= 1;
                        sel_sensor <= 2'b00;
                    end
                end

                ST_MENU_SENSOR: begin
                    
                    if(!menu_delay_active) begin
                        
                        if(!R1) begin
                            case (col_index)
                                2'd0: begin
                                    sel_sensor <= 2'b01;
                                    last_menu_state <= menu_state;
                                    menu_state <= ST_SEND_DATA;
                                    menu_delay_active <= 1;
                                end // R1 + C1
                                2'd1: begin 
                                    sel_sensor <= 2'b10;
                                    last_menu_state <= menu_state;
                                    menu_state <= ST_SEND_DATA;
                                    menu_delay_active <= 1;
                                end // R1 + C2
                                default: begin 
                                    sel_sensor <= 2'b10;
                                end
                            endcase
                        end

            
                    end
                end

                ST_SEND_DATA: begin
                    if(tx_done_pulse && ext_tx_done_pulse) begin
                        if(last_menu_state == ST_MENU_SALA) begin
                            menu_state <= ST_MENU_SENSOR;
                        end else begin
                            menu_state <= ST_MENU_SALA;
                        end
                    end
                end
                default: menu_state <= ST_MENU_SALA; 
            endcase

            
        end
    end
endmodule
