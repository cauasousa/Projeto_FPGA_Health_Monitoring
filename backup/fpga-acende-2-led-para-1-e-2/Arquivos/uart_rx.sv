module uart_rx #(
    parameter CLK_FREQ = 25_000_000,
    parameter BAUD_RATE = 9600
)(
    input  logic       clk,
    input  logic       rst_n,
    input  logic       rx,
    output logic [7:0] rx_data,
    output logic       rx_valid
);

    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    
    typedef enum logic [2:0] {
        IDLE  = 3'b000,
        START = 3'b001,
        DATA  = 3'b010,
        STOP  = 3'b011
    } state_t;
    
    state_t state;
    logic [15:0] clk_count;
    logic [2:0]  bit_index;
    logic [7:0]  rx_data_reg;
    logic        rx_sync1, rx_sync2;
    
    // Sincronização do sinal RX
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rx_sync1 <= 1'b1;
            rx_sync2 <= 1'b1;
        end else begin
            rx_sync1 <= rx;
            rx_sync2 <= rx_sync1;
        end
    end
    
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            rx_data <= 8'h00;
            rx_valid <= 1'b0;
            clk_count <= 0;
            bit_index <= 0;
        end else begin
            rx_valid <= 1'b0;
            
            case (state)
                IDLE: begin
                    clk_count <= 0;
                    bit_index <= 0;
                    
                    if (rx_sync2 == 1'b0) begin
                        state <= START;
                    end
                end
                
                START: begin
                    if (clk_count < (CLKS_PER_BIT - 1) / 2) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        if (rx_sync2 == 1'b0) begin
                            clk_count <= 0;
                            state <= DATA;
                        end else begin
                            state <= IDLE;
                        end
                    end
                end
                
                DATA: begin
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        rx_data_reg[bit_index] <= rx_sync2;
                        
                        if (bit_index < 7) begin
                            bit_index <= bit_index + 1;
                        end else begin
                            bit_index <= 0;
                            state <= STOP;
                        end
                    end
                end
                
                STOP: begin
                    if (clk_count < CLKS_PER_BIT - 1) begin
                        clk_count <= clk_count + 1;
                    end else begin
                        clk_count <= 0;
                        rx_data <= rx_data_reg;
                        rx_valid <= 1'b1;
                        state <= IDLE;
                    end
                end
            endcase
        end
    end

endmodule
