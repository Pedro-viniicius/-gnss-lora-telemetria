// configuracao_hardware.h — Receptor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// Este no nao usa UART externa: recebe apenas por LoRa e mostra no OLED/Serial.
// O mapa dos pinos internos confirmados da Heltec V3 fica centralizado em
// pinos_heltec_v3.h. GPIOs do radio e do OLED sao tratados como reservados.
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_RX_H
#define CONFIGURACAO_HARDWARE_RX_H

#include <Arduino.h>

#include "pinos_heltec_v3.h"

// Sem pacote LoRa por este tempo => estado "AGUARDANDO (timeout)".
constexpr uint32_t RECEPTOR_TIMEOUT_MS = 10000UL;

// Periodo de atualizacao do OLED (ms).
constexpr uint32_t PERIODO_OLED_MS = 500UL;

// Periodo de impressao de diagnostico no USB quando ocioso (ms).
constexpr uint32_t PERIODO_DIAGNOSTICO_MS = 2000UL;

static_assert(PIN_LORA_NSS == 8 && PIN_LORA_SCK == 9 &&
                  PIN_LORA_MOSI == 10 && PIN_LORA_MISO == 11 &&
                  PIN_LORA_RST == 12 && PIN_LORA_BUSY == 13 &&
                  PIN_LORA_DIO1 == 14,
              "Mapa interno do radio LoRa Heltec V3 divergente");
static_assert(PIN_OLED_SDA == 17 && PIN_OLED_SCL == 18 && PIN_OLED_RST == 21,
              "Mapa interno do OLED Heltec V3 divergente");
static_assert(pinoReservadoRadioHeltec(PIN_LORA_NSS) &&
                  pinoReservadoRadioHeltec(PIN_LORA_DIO1),
              "GPIOs do radio LoRa devem permanecer reservados");

#endif  // CONFIGURACAO_HARDWARE_RX_H
