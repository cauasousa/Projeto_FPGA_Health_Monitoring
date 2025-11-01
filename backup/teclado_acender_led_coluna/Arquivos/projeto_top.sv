`timescale 1ns/1ps
`default_nettype none


// Acende baseado na coluna do teclado 4x4 pressionada.

module projeto_top (
    input  wire       clk,
    input  wire       rst_n,
    // Teclado 4x4
    input  wire       R1, R2, R3, R4,    // linhas (entradas com pull-up)
    output reg  [3:0] C,                 // colunas (saídas, ativa baixa)
    // LEDs
    output reg        led_1,
    output reg        led_2,
    output reg        led_3,
    output reg        led_4
);

    // ============================================================
    // PARÂMETROS
    // ============================================================
    localparam integer CLK_FREQ    = 25_000_000; // 25 MHz
    localparam integer SCAN_FREQ   = 1000;       // Frequência de varredura (~1 kHz)
    localparam integer SCAN_CYCLES = CLK_FREQ / (SCAN_FREQ * 4);

    // ============================================================
    // SINAIS INTERNOS
    // ============================================================
    reg [31:0] scan_counter = 0;
    reg [1:0]  col_index    = 0;
    reg [1:0]  col_pressed  = 0;    // coluna detectada
    wire [3:0] row_inputs;

    assign row_inputs = {R4, R3, R2, R1};

    // ============================================================
    // VARREDURA DAS COLUNAS
    // ============================================================
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            scan_counter <= 0;
            col_index    <= 0;
        end else begin
            if (scan_counter >= SCAN_CYCLES - 1) begin
                scan_counter <= 0;
                col_index <= col_index + 1'b1;
            end else begin
                scan_counter <= scan_counter + 1'b1;
            end
        end
    end

    // ============================================================
    // ATIVAÇÃO DA COLUNA (1 ativa por vez, nível baixo)
    // ============================================================
    always @(*) begin
        case (col_index)
            2'd0: C = 4'b1110; // coluna 0 ativa
            2'd1: C = 4'b1101; // coluna 1 ativa
            2'd2: C = 4'b1011; // coluna 2 ativa
            2'd3: C = 4'b0111; // coluna 3 ativa
            default: C = 4'b1111;
        endcase
    end

    // ============================================================
    // DETECÇÃO DE TECLA PRESSIONADA
    // ============================================================
    wire key_pressed = (row_inputs != 4'b1111); // alguma linha em 0?

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            col_pressed <= 2'b00;
        end else begin
            if (key_pressed)
                col_pressed <= col_index; // salva a coluna atual
        end
    end

    // ============================================================
    // LEDS - Indicam coluna da tecla pressionada
    // ============================================================
    always @(*) begin
        led_1 = key_pressed && (col_pressed == 2'd0);
        led_2 = key_pressed && (col_pressed == 2'd1);
        led_3 = key_pressed && (col_pressed == 2'd2);
        led_4 = key_pressed && (col_pressed == 2'd3);
    end

endmodule
