// configuracao_hardware.h — Transmissor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// FONTE UNICA DE VERDADE da pinagem especifica deste no. Os pinos internos da
// Heltec V3 ficam centralizados em pinos_heltec_v3.h e sao validados por
// static_assert para impedir conflito com a UART externa do controlador.
//
// GPIO1 NAO deve ser usado como TX externo nesta placa: pelo mapa confirmado,
// ele e reservado para leitura de bateria. Nesta montagem, o par correto da
// Heltec para a UART com o controlador e o USB/UART da placa:
//   U0TXD = GPIO43
//   U0RXD = GPIO44
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_TX_H
#define CONFIGURACAO_HARDWARE_TX_H

#include <Arduino.h>

#include "pinos_heltec_v3.h"

// UART de ligacao com o controlador (full-duplex).
// Heltec RX GPIO44 (U0RXD) <- TX GPIO18 do controlador.
// Heltec TX GPIO43 (U0TXD) -> RX GPIO17 do controlador.
constexpr int CONTROLADOR_UART_NUM = 1;       // instancia Serial1
constexpr int CONTROLADOR_PINO_RX  = PIN_UART_USB_RX;
constexpr int CONTROLADOR_PINO_TX  = PIN_UART_USB_TX;
constexpr unsigned long CONTROLADOR_BAUD = 115200UL;  // deve casar com o controlador

// O diagnostico Serial da Heltec transmissora fica desligado por padrao para
// nao disputar pinos nem injetar texto no enlace binario com o controlador. O
// OLED integrado continua mostrando o estado local.
#ifndef HABILITAR_SERIAL_TX_DIAGNOSTICO
#define HABILITAR_SERIAL_TX_DIAGNOSTICO 0
#endif

// Timeout do enlace UART: sem quadros do controlador por este tempo => estado
// "controlador desconectado" no OLED.
constexpr uint32_t CONTROLADOR_TIMEOUT_MS = 4000UL;

// Periodo de envio do STATUS_TRANSMISSOR de volta ao controlador (ms).
constexpr uint32_t PERIODO_STATUS_MS = 1000UL;

// Periodo de atualizacao do OLED (ms).
constexpr uint32_t PERIODO_OLED_MS = 500UL;

// Watchdog de transmissao: se o radio ficar "ocupado" mais que isto sem
// callback, considera timeout e libera (recuperacao).
constexpr uint32_t TX_WATCHDOG_MS = 4000UL;

static_assert(CONTROLADOR_PINO_RX != CONTROLADOR_PINO_TX,
              "UART do controlador: RX e TX nao podem ser iguais");
static_assert(!pinoReservadoRadioHeltec(CONTROLADOR_PINO_RX) &&
                  !pinoReservadoRadioHeltec(CONTROLADOR_PINO_TX),
              "UART do controlador nao pode usar GPIOs reservados do radio LoRa");
static_assert(!pinoReservadoOledHeltec(CONTROLADOR_PINO_RX) &&
                  !pinoReservadoOledHeltec(CONTROLADOR_PINO_TX),
              "UART do controlador nao pode usar GPIOs reservados do OLED");
static_assert(!pinoReservadoSistemaHeltec(CONTROLADOR_PINO_RX) &&
                  !pinoReservadoSistemaHeltec(CONTROLADOR_PINO_TX),
              "UART do controlador nao pode usar GPIOs de Vext, LED, botao, bateria ou ADC");
static_assert(CONTROLADOR_PINO_TX == PIN_UART_USB_TX &&
                  CONTROLADOR_PINO_RX == PIN_UART_USB_RX,
              "UART da Heltec deve usar U0TXD=GPIO43 e U0RXD=GPIO44 nesta montagem confirmada");

#endif  // CONFIGURACAO_HARDWARE_TX_H
