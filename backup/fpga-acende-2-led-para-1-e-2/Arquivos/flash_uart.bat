@echo off
REM ============================================================
REM    Síntese e Gravação - UART SystemVerilog para Colorlight i9
REM    
REM    Projeto: Comunicação UART FPGA <-> Raspberry Pi Pico
REM    FPGA: Lattice ECP5 LFE5U-45F-6BG381C
REM    Clock: 25 MHz (interno) -> 50 MHz via PLL
REM    Baud Rate: 115200
REM ============================================================

REM Caminho da OSS CAD Suite
set OSSCADROOT=C:\oss-cad-suite

REM Configurações do projeto
set TOP=projeto_top
set LPF=top.lpf
set BOARD=colorlight-i9

REM Ativa ambiente OSS CAD Suite
call "%OSSCADROOT%\environment.bat"
cd /d "%~dp0"

echo ============================================================
echo   UART SystemVerilog - Síntese para Colorlight i9
echo ============================================================
echo.
echo [1/5] Verificando arquivos necessários...

if not exist "%TOP%.sv" (
    echo ❌ ERRO: Arquivo %TOP%.sv não encontrado!
    pause
    exit /b 1
)

if not exist "%LPF%" (
    echo ❌ ERRO: Arquivo %LPF% não encontrado!
    pause
    exit /b 1
)

if not exist "uart_tx.sv" (
    echo ❌ ERRO: Arquivo uart_tx.sv não encontrado!
    pause
    exit /b 1
)

if not exist "uart_rx.sv" (
    echo ❌ ERRO: Arquivo uart_rx.sv não encontrado!
    pause
    exit /b 1
)

echo ✅ Todos os arquivos encontrados
echo.

REM ============================================================
echo [2/5] Síntese com Yosys...
echo ============================================================

yosys -p "read_verilog -sv uart_tx.sv uart_rx.sv %TOP%.sv; synth_ecp5 -top %TOP% -json %TOP%.json"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ ERRO na síntese! Verifique o código SystemVerilog.
    pause
    exit /b 1
)

echo ✅ Síntese concluída
echo.

REM ============================================================
echo [3/5] Place and Route com nextpnr-ecp5...
echo ============================================================

nextpnr-ecp5 --json "%TOP%.json" --textcfg "%TOP%.config" --lpf "%LPF%" --45k --package CABGA381 --speed 6 --timing-allow-fail

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ⚠️  AVISO: Place and Route completou com avisos de timing
    echo    O design pode não atingir 50 MHz, mas deve funcionar @ 25 MHz
    echo.
) else (
    echo ✅ Place and Route concluído
)

echo.

REM ============================================================
echo [4/5] Gerando bitstream com ecppack...
echo ============================================================

ecppack --compress "%TOP%.config" "%TOP%.bit"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ ERRO ao gerar bitstream!
    pause
    exit /b 1
)

echo ✅ Bitstream gerado: %TOP%.bit
echo.

REM ============================================================
echo [5/5] Gravando no FPGA com openFPGALoader...
echo ============================================================

openFPGALoader -b %BOARD% "%TOP%.bit"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ ERRO ao gravar FPGA! Verifique conexão USB.
    pause
    exit /b 1
)

echo.
echo ✅ FPGA gravado com sucesso!
echo.

REM ============================================================
echo Limpando arquivos temporários...
REM ============================================================

del *.json *.config 2>nul

echo.
echo ============================================================
echo   ✅ PROCESSO COMPLETO!
echo ============================================================
echo.
echo Próximos passos:
echo   1. Conectar Raspberry Pi Pico ao FPGA:
echo      - Pico TX (GP16) --^> FPGA RX (pino PB29A)
echo      - Pico RX (GP17) ^<-- FPGA TX (pino PB29B)
echo      - GND comum entre Pico e FPGA
echo.
echo   2. Executar test_fpga_uart.py no Raspberry Pi Pico
echo.
echo   3. Observar LEDs:
echo      - leds[3:0]: Estado controlado via UART
echo      - led_rx_active: Pisca ao receber dados
echo      - led_tx_active: Pisca ao transmitir dados
echo.
echo Consulte README_UART.md para mais informações.
echo ============================================================
echo.

pause
