// uart_rx.sv
// UART Receiver - SystemVerilog
// Clock: 25 MHz | Baud rate: 9600
// CLKS_PER_BIT = 25_000_000 / 9600 = 2604

module uart_rx #(
    parameter int CLKS_PER_BIT = 2604  // Ajuste para 25 MHz / 9600 baud
)(
    input  logic       i_clk,         // Clock do sistema (25 MHz)
    input  logic       i_rst_n,       // Reset assíncrono ativo baixo
    input  logic       i_rx_serial,   // Linha RX (entrada UART)
    
    output logic       o_rx_dv,       // Pulso de 1 ciclo: dado recebido
    output logic [7:0] o_rx_byte      // Byte recebido
);

    // ----------------------------------------------------------------
    // Estados da máquina
    // ----------------------------------------------------------------
    typedef enum logic [2:0] {
        IDLE         = 3'b000,
        START_BIT    = 3'b001,
        DATA_BITS    = 3'b010,
        STOP_BIT     = 3'b011,
        CLEANUP      = 3'b100
    } state_t;

    state_t state;

    // ----------------------------------------------------------------
    // Registradores internos
    // ----------------------------------------------------------------
    logic [$clog2(CLKS_PER_BIT)-1:0] clk_count;
    logic [2:0] bit_index;
    logic [7:0] rx_data;
    logic rx_dv_reg;

    // Dupla amostragem do sinal RX
    logic rx_sync_0, rx_sync_1;

    // ----------------------------------------------------------------
    // Evita metastabilidade (sincronização dupla)
    // ----------------------------------------------------------------
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            rx_sync_0 <= 1'b1;
            rx_sync_1 <= 1'b1;
        end else begin
            rx_sync_0 <= i_rx_serial;
            rx_sync_1 <= rx_sync_0;
        end
    end

    // ----------------------------------------------------------------
    // Máquina de estados UART RX
    // ----------------------------------------------------------------
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            state      <= IDLE;
            clk_count  <= '0;
            bit_index  <= '0;
            rx_data    <= '0;
            o_rx_dv    <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    o_rx_dv    <= 1'b0;
                    clk_count  <= '0;
                    bit_index  <= '0;

                    if (rx_sync_1 == 1'b0)
                        state <= START_BIT;
                end

                START_BIT: begin
                    if (clk_count == (CLKS_PER_BIT-1)/2) begin
                        if (rx_sync_1 == 1'b0) begin
                            clk_count <= '0;
                            state     <= DATA_BITS;
                        end else begin
                            state <= IDLE;
                        end
                    end else begin
                        clk_count <= clk_count + 1'b1;
                    end
                end

                DATA_BITS: begin
                    if (clk_count < CLKS_PER_BIT-1) begin
                        clk_count <= clk_count + 1'b1;
                    end else begin
                        clk_count <= '0;
                        rx_data[bit_index] <= rx_sync_1;

                        if (bit_index < 7)
                            bit_index <= bit_index + 1'b1;
                        else
                            state <= STOP_BIT;
                    end
                end

                STOP_BIT: begin
                    if (clk_count < CLKS_PER_BIT-1) begin
                        clk_count <= clk_count + 1'b1;
                    end else begin
                        o_rx_dv   <= 1'b1;
                        clk_count <= '0;
                        state     <= CLEANUP;
                    end
                end

                CLEANUP: begin
                    state   <= IDLE;
                    o_rx_dv <= 1'b0;
                end

                default: state <= IDLE;
            endcase
        end
    end

    assign o_rx_byte = rx_data;

endmodule
