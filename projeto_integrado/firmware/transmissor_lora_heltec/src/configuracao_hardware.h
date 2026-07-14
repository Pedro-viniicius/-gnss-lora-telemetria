// configuracao_hardware.h — Transmissor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// FONTE UNICA DE VERDADE da pinagem especifica deste no. Os pinos de OLED,
// radio SX1262, Vext e LED sao definidos pela variante da placa (fornecidos
// pela biblioteca Heltec). Aqui centralizamos apenas o UART de ligacao com o
// controlador (mestre).
//
// GPIO5/6 sao livres na Heltec V3 (nao conflitam com OLED I2C 17/18/21,
// SX1262 8-14, Vext 36 nem LED 35). Confirme na sua placa.
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_TX_H
#define CONFIGURACAO_HARDWARE_TX_H

#include <Arduino.h>

// UART de ligacao com o controlador (full-duplex).
// Heltec RX (GPIO5) <- controlador TX ; Heltec TX (GPIO6) -> controlador RX.
#define CONTROLADOR_UART_NUM   1            // instancia Serial1
#define CONTROLADOR_PINO_RX    5
#define CONTROLADOR_PINO_TX    6
#define CONTROLADOR_BAUD       115200UL     // deve casar com o controlador

// Timeout do enlace UART: sem quadros do controlador por este tempo => estado
// "controlador desconectado" no OLED.
#define CONTROLADOR_TIMEOUT_MS 4000UL

// Periodo de envio do STATUS_TRANSMISSOR de volta ao controlador (ms).
#define PERIODO_STATUS_MS      1000UL

// Periodo de atualizacao do OLED (ms).
#define PERIODO_OLED_MS        500UL

// Watchdog de transmissao: se o radio ficar "ocupado" mais que isto sem
// callback, considera timeout e libera (recuperacao).
#define TX_WATCHDOG_MS         4000UL

#endif  // CONFIGURACAO_HARDWARE_TX_H
