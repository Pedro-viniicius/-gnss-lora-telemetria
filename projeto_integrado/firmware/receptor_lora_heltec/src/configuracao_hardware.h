// configuracao_hardware.h — Receptor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// Este no nao usa UART externa: recebe apenas por LoRa e mostra no OLED/Serial.
// Pinos de OLED/SX1262/Vext vem da variante da placa (biblioteca Heltec).
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_RX_H
#define CONFIGURACAO_HARDWARE_RX_H

#include <Arduino.h>

// Sem pacote LoRa por este tempo => estado "AGUARDANDO (timeout)".
#define RECEPTOR_TIMEOUT_MS   10000UL

// Periodo de atualizacao do OLED (ms).
#define PERIODO_OLED_MS       500UL

// Periodo de impressao de diagnostico no USB quando ocioso (ms).
#define PERIODO_DIAGNOSTICO_MS 2000UL

#endif  // CONFIGURACAO_HARDWARE_RX_H
