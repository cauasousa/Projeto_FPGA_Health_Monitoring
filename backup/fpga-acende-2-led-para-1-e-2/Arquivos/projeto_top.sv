`timescale 1ns/1ps
`default_nettype none

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
            2'd0: C = 4'b1110; // coluna 0 ativa (C1)
            2'd1: C = 4'b1101; // coluna 1 ativa (C2)
            2'd2: C = 4'b1011; // coluna 2 ativa (C3)
            2'd3: C = 4'b0111; // coluna 3 ativa (C4)
            default: C = 4'b1111;
        endcase
    end

    // ============================================================
    // LEDS - Acendem quando R1+C1 ou R1+C2 são pressionados
    // ============================================================
    always @(*) begin
        // LEDs padrão apagados
        led_1 = 1'b0;
        led_2 = 1'b0;
        led_3 = 1'b0;
        led_4 = 1'b0;

        // Detecta se R1 está pressionado (nível 0)
        if (!R1) begin
            case (col_index)
                2'd0: led_1 = 1'b1; // R1 + C1
                2'd1: led_2 = 1'b1; // R1 + C2
                default: ;
            endcase
        end
    end
    
endmodule
