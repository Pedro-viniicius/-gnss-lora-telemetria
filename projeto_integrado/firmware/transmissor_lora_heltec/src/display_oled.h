// display_oled.h — Transmissor LoRa
// -----------------------------------------------------------------------------
// Encapsula o OLED SSD1306 da Heltec V3 (HT_SSD1306Wire) e o controle de Vext.
// Mostra o estado do enlace UART, ultima sequencia, estado de TX, erros, ultimo
// tipo de pacote e a idade do ultimo pacote valido do controlador.
// -----------------------------------------------------------------------------
#ifndef DISPLAY_OLED_TX_H
#define DISPLAY_OLED_TX_H

#include <Arduino.h>

#include "HT_SSD1306Wire.h"

struct EstadoDisplayTx {
  bool uartConectado;
  uint16_t ultimaSequencia;
  uint8_t ultimoTipo;
  uint8_t estadoTx;        // protocolo::EstadoTransmissor
  uint32_t transmissoes;
  uint32_t timeouts;
  uint32_t quadrosInvalidos;
  uint32_t idadeUltimoQuadroMs;
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
