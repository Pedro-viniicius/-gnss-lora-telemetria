// display_oled.h — Transmissor LoRa
// -----------------------------------------------------------------------------
// Encapsula o OLED SSD1306 da Heltec V3 (HT_SSD1306Wire) e o controle de Vext.
// Mostra no transmissor as informacoes GNSS vindas do controlador ESP32-S3
// mestre, alem de um resumo curto do enlace UART/TX. O transmissor nao calcula
// posicao; ele apenas exibe a ultima TELEMETRIA_GNSS recebida pela UART.
// -----------------------------------------------------------------------------
#ifndef DISPLAY_OLED_TX_H
#define DISPLAY_OLED_TX_H

#include <Arduino.h>

#include "HT_SSD1306Wire.h"
#include "tipos_gnss.h"

struct EstadoDisplayTx {
  bool uartConectado;
  uint16_t ultimaSequencia;
  uint8_t ultimoTipo;
  uint8_t estadoTx;        // protocolo::EstadoTransmissor
  uint32_t transmissoes;
  uint32_t timeouts;
  uint32_t quadrosInvalidos;
  uint32_t idadeUltimoQuadroMs;
  bool temGnss;
  protocolo::TelemetriaGnss gnss;
  uint16_t sequenciaGnss;
  uint32_t idadeGnssMs;
};

class DisplayOled {
 public:
  DisplayOled();

  // Liga o Vext e inicializa o display.
  void iniciar();

  // Redesenha a tela com o estado informado.
  void mostrar(const EstadoDisplayTx& e);

 private:
  SSD1306Wire display_;
  void vextON();
};

#endif  // DISPLAY_OLED_TX_H
