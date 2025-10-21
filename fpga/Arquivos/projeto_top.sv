`timescale 1ns/1ps
`default_nettype none

module top (
    input  wire        clk,      // Clock principal
    input  wire        R1,       // Linha 1 do teclado
    input  wire        R3,       // Linha 3 do teclado
    output wire        C2,       // Coluna 2 do teclado
    output wire [1:0]  S         // Saída de estado do teclado
);

    // ============================================================
    // Sincronização das entradas (duplo registrador)
    // ============================================================
    reg [1:0] s_R1 = 2'b00;
    reg [1:0] s_R3 = 2'b00;

    always @(posedge clk) begin
        s_R1 <= {s_R1[0], R1};
        s_R3 <= {s_R3[0], R3};
    end

    wire R1_s = s_R1[1];
    wire R3_s = s_R3[1];

    // ============================================================
    // Parâmetros de debounce
    // ============================================================
    parameter integer DEBOUNCE_LIMIT = 25000; // 1 ms @ 50 MHz
    localparam integer COUNTER_WIDTH = $clog2(DEBOUNCE_LIMIT);

    // ============================================================
    // Registradores de estado
    // ============================================================
    reg [COUNTER_WIDTH-1:0] count = 0;
    reg R1_stable = 1'b0;
    reg R3_stable = 1'b0;

    // ============================================================
    // Debounce unificado com prioridade (R1 > R3)
    // ============================================================
    always @(posedge clk) begin
        if (R1_s != R1_stable) begin
            // Se R1 mudou → faz debounce
            if (count == DEBOUNCE_LIMIT - 1) begin
                R1_stable <= R1_s;
                R3_stable <= 1'b0;   // zera o outro
                count <= 0;
            end else begin
                count <= count + 1;
            end

        end else if (R3_s != R3_stable) begin
            // Se R3 mudou → faz debounce
            if (count == DEBOUNCE_LIMIT - 1) begin
                R3_stable <= R3_s;
                R1_stable <= 1'b0;   // zera o outro
                count <= 0;
            end else begin
                count <= count + 1;
            end

        end else begin
            // Nenhuma mudança → reseta contador
            count <= 0;
        end
    end

    // ============================================================
    // Coluna ativa (HIGH)
    // ============================================================
    assign C2 = 1'b1;

    // ============================================================
    // Estado de saída (codificação)
    // ============================================================
    reg [1:0] key_state = 2'b00;

    always @(posedge clk) begin
        if (R1_stable)
            key_state <= 2'b01;
        else if (R3_stable)
            key_state <= 2'b10;
        else
            key_state <= 2'b00;
    end

    assign S = key_state;

endmodule
