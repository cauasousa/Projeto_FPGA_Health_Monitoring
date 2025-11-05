/*

Módulo: teclado
Descrição geral:
Este módulo realiza a varredura (scanning) de um teclado matricial 4x4.
Ele alterna as colunas ativas de forma cíclica, controlando o sinal das
linhas (C) para detectar qual tecla está sendo pressionada.

Parâmetros:
- CLK_FREQ: frequência do clock de entrada em Hz (padrão: 25 MHz).
- SCAN_FREQ: frequência de varredura desejada (padrão: 1000 Hz).

Entradas:
- clk: sinal de clock do sistema.
- rst_n: reset ativo em nível baixo.

Saídas:
- C: vetor de 4 bits que indica qual coluna está ativa no momento.
- col_index: índice da coluna ativa (0 a 3).

Funcionamento resumido:
O módulo utiliza um contador (scan_counter) que controla a taxa de varredura.
A cada período correspondente ao SCAN_CYCLES, o índice de coluna (col_index)
é incrementado, alternando a coluna ativa.
O sinal "C" define qual coluna do teclado está sendo varrida,
enquanto as linhas do teclado podem ser lidas externamente para identificar
as teclas pressionadas.

Observação:
Este módulo não realiza a leitura direta das teclas — apenas gera o padrão
de varredura necessário para o circuito externo detectar as teclas ativas.

---

*/

`timescale 1ns/1ps
`default_nettype none

module teclado #(
    parameter integer CLK_FREQ = 25_000_000,
    parameter integer SCAN_FREQ = 1000
)(
    input  wire clk,
    input  wire rst_n,
    output reg  [3:0] C,
    output reg  [1:0] col_index
);
    localparam integer SCAN_CYCLES = CLK_FREQ / (SCAN_FREQ * 4);

    reg [31:0] scan_counter = 0;

    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            scan_counter <= 0;
            col_index <= 0;
        end else if(scan_counter >= SCAN_CYCLES - 1) begin
            scan_counter <= 0;
            col_index <= col_index + 1'b1;
        end else begin
            scan_counter <= scan_counter + 1'b1;
        end
    end

    always @(*) begin
        case(col_index)
            2'd0: C = 4'b1110;
            2'd1: C = 4'b1101;
            2'd2: C = 4'b1011;
            2'd3: C = 4'b0111;
            default: C = 4'b1111;
        endcase
    end
endmodule
