// display_oled.cpp — Transmissor LoRa
#include "display_oled.h"

#include <stdio.h>

#include "tipos_comandos.h"

using namespace protocolo;

// Mesma construcao dos codigos de referencia (endereco 0x3c, I2C do OLED).
DisplayOled::DisplayOled()
    : display_(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED) {}

void DisplayOled::vextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);  // LOW liga a alimentacao do OLED na Heltec
}

void DisplayOled::iniciar() {
  vextON();
  delay(100);  // assentamento da alimentacao do OLED (uma unica vez, no boot)
  display_.init();
  display_.setContrast(255);
  display_.setTextAlignment(TEXT_ALIGN_LEFT);
  display_.setFont(ArialMT_Plain_10);
  display_.clear();
  display_.drawString(0, 0, "TX LoRa iniciando...");
  display_.display();
}

static const char* nomeEstadoTx(uint8_t e) {
  switch (e) {
    case TX_OCIOSO:       return "OCIOSO";
    case TX_TRANSMITINDO: return "TX...";
    case TX_TIMEOUT:      return "TIMEOUT";
    default:              return "?";
  }
}

static const char* nomeTipo(uint8_t t) {
  switch (t) {
    case MSG_TELEMETRIA_GNSS:    return "TELEMETRIA";
    case MSG_EVENTO_TECLADO:     return "TECLADO";
    case MSG_COMANDO:            return "COMANDO";
    case MSG_HEARTBEAT:          return "HEARTBEAT";
    case MSG_STATUS_TRANSMISSOR: return "STATUS";
    default:                     return "-";
  }
}

void DisplayOled::mostrar(const EstadoDisplayTx& e) {
  char linha[32];
  display_.clear();
  display_.setTextAlignment(TEXT_ALIGN_LEFT);
  display_.setFont(ArialMT_Plain_10);

  snprintf(linha, sizeof(linha), "UART: %s",
           e.uartConectado ? "CONECTADO" : "SEM SINAL");
  display_.drawString(0, 0, linha);

  snprintf(linha, sizeof(linha), "Seq:%u Tipo:%s", (unsigned)e.ultimaSequencia,
           nomeTipo(e.ultimoTipo));
  display_.drawString(0, 12, linha);

  snprintf(linha, sizeof(linha), "TX:%s ok:%lu to:%lu", nomeEstadoTx(e.estadoTx),
           (unsigned long)e.transmissoes, (unsigned long)e.timeouts);
  display_.drawString(0, 24, linha);

  snprintf(linha, sizeof(linha), "Quadros inval.: %lu",
           (unsigned long)e.quadrosInvalidos);
  display_.drawString(0, 36, linha);

  if (e.idadeUltimoQuadroMs == UINT32_MAX) {
    snprintf(linha, sizeof(linha), "Ult. pacote: --");
  } else {
    snprintf(linha, sizeof(linha), "Ult. pacote: %lu ms",
             (unsigned long)e.idadeUltimoQuadroMs);
  }
  display_.drawString(0, 48, linha);

  display_.display();
}
