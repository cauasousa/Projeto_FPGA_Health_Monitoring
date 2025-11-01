// ============================================================================
// UART Transmitter - SystemVerilog
// Conversão e otimização do código original em Verilog (Nandland)
//
// Clock: 25 MHz
// Baud rate: 9600
// CLKS_PER_BIT = 25_000_000 / 9600 ≈ 2604
//
// Transmite 8 bits + 1 start bit + 1 stop bit (formato 8N1)
//
// Autor: Adaptado para SystemVerilog por Antonio Cláudio (2025)
// ============================================================================

module uart_tx #(
    parameter int CLKS_PER_BIT = 2604  // 25 MHz / 9600 baud
)(
    input  logic       i_clk,        // Clock do sistema
    input  logic       i_rst_n,      // Reset assíncrono ativo baixo
    input  logic       i_tx_dv,      // Pulso para iniciar transmissão
    input  logic [7:0] i_tx_byte,    // Byte a transmitir
    
    output logic       o_tx_active,  // Indica transmissão em andamento
    output logic       o_tx_serial,  // Linha de saída serial
    output logic       o_tx_done     // Pulso de 1 ciclo quando terminar
);

    // ------------------------------------------------------------------------
    // Definição dos estados
    // ------------------------------------------------------------------------
    typedef enum logic [2:0] {
        IDLE         = 3'b000,
        START_BIT    = 3'b001,
        DATA_BITS    = 3'b010,
        STOP_BIT     = 3'b011,
        CLEANUP      = 3'b100
    } state_t;

    state_t state;

    // ------------------------------------------------------------------------
    // Registradores internos
    // ------------------------------------------------------------------------
    logic [$clog2(CLKS_PER_BIT)-1:0] clk_count;
    logic [2:0] bit_index;
    logic [7:0] tx_data;

    // ------------------------------------------------------------------------
    // Máquina de estados UART TX
    // ------------------------------------------------------------------------
    always_ff @(posedge i_clk or negedge i_rst_n) begin
        if (!i_rst_n) begin
            state       <= IDLE;
            o_tx_serial <= 1'b1;  // Linha ociosa em HIGH
            o_tx_done   <= 1'b0;
            o_tx_active <= 1'b0;
            clk_count   <= '0;
            bit_index   <= '0;
            tx_data     <= '0;
        end else begin
            case (state)
                // ------------------------------------------------------------
                // Estado IDLE: linha em HIGH, aguardando novo dado
                // ------------------------------------------------------------
                IDLE: begin
                    o_tx_serial <= 1'b1;
                    o_tx_done   <= 1'b0;
                    o_tx_active <= 1'b0;
                    clk_count   <= '0;
                    bit_index   <= '0;

                    if (i_tx_dv) begin
                        tx_data     <= i_tx_byte;
                        o_tx_active <= 1'b1;
                        state       <= START_BIT;
                    end
                end

                // ------------------------------------------------------------
                // START_BIT: envia bit de início (0)
                // ------------------------------------------------------------
                START_BIT: begin
                    o_tx_serial <= 1'b0;

                    if (clk_count < CLKS_PER_BIT - 1)
                        clk_count <= clk_count + 1'b1;
                    else begin
                        clk_count <= '0;
                        state     <= DATA_BITS;
                    end
                end

                // ------------------------------------------------------------
                // DATA_BITS: transmite 8 bits de dados (LSB primeiro)
                // ------------------------------------------------------------
                DATA_BITS: begin
                    o_tx_serial <= tx_data[bit_index];

                    if (clk_count < CLKS_PER_BIT - 1)
                        clk_count <= clk_count + 1'b1;
                    else begin
                        clk_count <= '0;
                        if (bit_index < 7)
                            bit_index <= bit_index + 1'b1;
                        else begin
                            bit_index <= '0;
                            state     <= STOP_BIT;
                        end
                    end
                end

                // ------------------------------------------------------------
                // STOP_BIT: envia bit de parada (1)
                // ------------------------------------------------------------
                STOP_BIT: begin
                    o_tx_serial <= 1'b1;

                    if (clk_count < CLKS_PER_BIT - 1)
                        clk_count <= clk_count + 1'b1;
                    else begin
                        clk_count   <= '0;
                        o_tx_done   <= 1'b1;
                        o_tx_active <= 1'b0;
                        state       <= CLEANUP;
                    end
                end

                // ------------------------------------------------------------
                // CLEANUP: 1 ciclo de limpeza antes de voltar ao IDLE
                // ------------------------------------------------------------
                CLEANUP: begin
                    o_tx_done <= 1'b0;
                    state     <= IDLE;
                end

                default: state <= IDLE;
            endcase
        end
    end

endmodule
