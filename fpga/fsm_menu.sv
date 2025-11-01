`timescale 1ns/1ps
`default_nettype none


// Gerencia o estado do menu e as seleções de sala/sensor.

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
    // NOVAS SAÍDAS: Bytes a serem transmitidos
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

    localparam MENU_DELAY_CYCLES = CLK_FREQ / 2;

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

            if(menu_delay_active) begin
                if(menu_delay_counter < MENU_DELAY_CYCLES - 1)
                    menu_delay_counter <= menu_delay_counter + 1;
                else
                    menu_delay_active <= 0;
            end else
                menu_delay_counter <= 0;

            case(menu_state)
                ST_MENU_SALA: begin
                    if(!menu_delay_active && (R1_pressed || R2_pressed)) begin
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
                    // Usar mesmo debounce/menu_delay_active que foi usado para seleção de sala.
                    if(!menu_delay_active && (R1_pressed || R2_pressed)) begin
                        case(col_index)
                            2'd0: sel_sensor <= 2'b01;
                            2'd1: sel_sensor <= 2'b10;
                        endcase
                        last_menu_state <= menu_state;
                        menu_state <= ST_SEND_DATA;
                        menu_delay_active <= 1;
                    end
                end

                ST_SEND_DATA: begin
                    if(tx_done_pulse && ext_tx_done_pulse) begin
                        if(last_menu_state == ST_MENU_SALA)
                            menu_state <= ST_MENU_SENSOR;
                        else
                            menu_state <= ST_MENU_SALA;
                    end
                end
                default: menu_state <= ST_MENU_SALA; // NOVO: Adicione default para robustez
            endcase

            // Geração dos Bytes
            if (menu_state == ST_SEND_DATA) begin
                // Byte 1: {0000, sel_sensor, sel_sala}  (explicitamente com 4 MSBs zero)
                tx_byte1 <= {4'b0000, sel_sensor, sel_sala};
                // Byte 2: 0x00 (Payload de requisição)
                tx_byte2 <= 8'h00; 
            end else begin
                tx_byte1 <= 8'h00;
                tx_byte2 <= 8'h00;
            end
        end
    end
endmodule
