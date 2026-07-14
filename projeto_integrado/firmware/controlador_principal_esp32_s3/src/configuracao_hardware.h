// configuracao_hardware.h
// -----------------------------------------------------------------------------
// FONTE UNICA DE VERDADE da pinagem e dos parametros de hardware do controlador
// principal (ESP32-S3, mestre do sistema).
//
// >>> ATENCAO: TODOS os pinos abaixo sao PREMISSAS a confirmar fisicamente na
// >>> sua placa. Ajuste aqui, em um unico lugar, sem espalhar numeros de pino
// >>> pelo codigo. Escolha de GPIOs segura para ESP32-S3 (evita strapping 0/3/45/
// >>> 46, USB 19/20 e pinos de flash/PSRAM).
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_H
#define CONFIGURACAO_HARDWARE_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
// UART do GNSS UM980 (HardwareSerial dedicada). NAO use SoftwareSerial no S3.
// ESP32-S3 TX  -> UM980 RX ; ESP32-S3 RX <- UM980 TX ; GND comum.
// ----------------------------------------------------------------------------
#define GNSS_UART_NUM        1            // usa a instancia Serial1 (UART1)
#define GNSS_PINO_RX         18           // RX do ESP32-S3 (recebe TX do UM980)
#define GNSS_PINO_TX         17           // TX do ESP32-S3 (vai ao RX do UM980)
#define GNSS_BAUD            115200UL     // baud padrao do UM980 (confirmar)
#define GNSS_TIMEOUT_INIT_MS 5000UL       // limite p/ detectar dados no boot
#define GNSS_IDADE_MAX_MS    3000UL       // acima disso, dados marcados ANTIGOS
#define GNSS_PINO_PPS        -1           // opcional; -1 = nao usado

// ----------------------------------------------------------------------------
// UART do enlace LoRa (para a Heltec TX), full-duplex. HardwareSerial dedicada.
// ESP32-S3 TX  -> Heltec RX ; ESP32-S3 RX <- Heltec TX ; GND comum ; 3.3 V.
// ----------------------------------------------------------------------------
#define LORA_UART_NUM        2            // usa a instancia Serial2 (UART2)
#define LORA_PINO_RX         16           // RX do ESP32-S3 (recebe TX da Heltec)
#define LORA_PINO_TX         15           // TX do ESP32-S3 (vai ao RX da Heltec)
#define LORA_BAUD            115200UL     // enlace UART controlador <-> Heltec
#define LORA_TIMEOUT_MS      4000UL       // sem status da Heltec => desconectada

// ----------------------------------------------------------------------------
// USB Serial (depuracao/diagnostico).
// ----------------------------------------------------------------------------
#define USB_BAUD             115200UL

// ----------------------------------------------------------------------------
// Teclado matricial 4x4. Linhas sao SAIDAS; colunas sao ENTRADAS com pull-up.
// Layout fisico:
//     1 2 3 A
//     4 5 6 B
//     7 8 9 C
//     * 0 # D
// ----------------------------------------------------------------------------
#define TECLADO_LINHAS       4
#define TECLADO_COLUNAS      4

// Pinos das 4 linhas (saidas).
#define TECLADO_PINO_LINHA_0 4
#define TECLADO_PINO_LINHA_1 5
#define TECLADO_PINO_LINHA_2 6
#define TECLADO_PINO_LINHA_3 7

// Pinos das 4 colunas (entradas com INPUT_PULLUP).
#define TECLADO_PINO_COLUNA_0 9
#define TECLADO_PINO_COLUNA_1 10
#define TECLADO_PINO_COLUNA_2 11
#define TECLADO_PINO_COLUNA_3 12

#define TECLADO_DEBOUNCE_MS   30          // tempo de estabilizacao de tecla
#define TECLADO_MAX_DIGITOS   9           // limite do buffer numerico (sem overflow)

// ----------------------------------------------------------------------------
// LED de status embarcado (opcional). -1 desabilita.
// ----------------------------------------------------------------------------
#ifdef LED_BUILTIN
#define PINO_LED_STATUS      LED_BUILTIN
#else
#define PINO_LED_STATUS      -1
#endif

// ----------------------------------------------------------------------------
// Periodos de agendamento cooperativo (millis()).
// ----------------------------------------------------------------------------
#define PERIODO_TELEMETRIA_MS   2000UL    // envio periodico de telemetria
#define PERIODO_HEARTBEAT_MS    5000UL    // heartbeat quando ocioso
#define PERIODO_DIAGNOSTICO_MS  1000UL    // impressao de diagnostico no USB

// Validacoes basicas de coerencia de pinagem (nao pega tudo, mas evita erros
// grosseiros de duplicidade entre UARTs e teclado).
static_assert(GNSS_PINO_RX != GNSS_PINO_TX, "GNSS RX e TX nao podem ser iguais");
static_assert(LORA_PINO_RX != LORA_PINO_TX, "LoRa RX e TX nao podem ser iguais");
static_assert(GNSS_UART_NUM != LORA_UART_NUM, "GNSS e LoRa em UARTs distintas");

#endif  // CONFIGURACAO_HARDWARE_H
