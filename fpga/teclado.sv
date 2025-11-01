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
