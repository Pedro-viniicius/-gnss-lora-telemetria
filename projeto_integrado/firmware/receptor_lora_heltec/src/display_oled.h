// display_oled.h — Receptor LoRa
// -----------------------------------------------------------------------------
// OLED SSD1306 (HT_SSD1306Wire) do receptor. Mostra posicao, tipo de fix,
// satelites, sequencia, RSSI/SNR e estado da conexao (com timeout).
// -----------------------------------------------------------------------------
#ifndef DISPLAY_OLED_RX_H
#define DISPLAY_OLED_RX_H

#include <Arduino.h>

#include "HT_SSD1306Wire.h"
#include "protocolo.h"

struct EstadoDisplayRx {
  bool temDados;                    // ja recebeu algum pacote valido
  bool timeout;                     // sem pacote dentro do periodo configurado
  protocolo::TelemetriaGnss gnss;   // ultimo snapshot de telemetria
  uint16_t sequencia;
  int16_t rssi;
  int8_t snr;
  uint32_t idadeUltimoPacoteMs;
};

class DisplayOled {
 public:
  DisplayOled();
  void iniciar();
  void mostrar(const EstadoDisplayRx& e);

 private:
  SSD1306Wire display_;
  void vextON();
};

#endif  // DISPLAY_OLED_RX_H
