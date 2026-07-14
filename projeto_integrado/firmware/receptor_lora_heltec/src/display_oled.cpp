// display_oled.cpp — Receptor LoRa
#include "display_oled.h"

#include <stdio.h>

using namespace protocolo;

DisplayOled::DisplayOled()
    : display_(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED) {}

void DisplayOled::vextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void DisplayOled::iniciar() {
  vextON();
  delay(100);
  display_.init();
  display_.setContrast(255);
  display_.setTextAlignment(TEXT_ALIGN_LEFT);
  display_.setFont(ArialMT_Plain_10);
  display_.clear();
  display_.drawString(0, 0, "RX LoRa aguardando...");
  display_.display();
}

static const char* nomeFix(uint8_t t) {
  switch (t) {
    case FIX_SEM_FIX:     return "SEMFIX";
    case FIX_STANDALONE:  return "STD";
    case FIX_DIFERENCIAL: return "DGPS";
    case FIX_RTK_FLOAT:   return "RTKfloat";
    case FIX_RTK_FIXED:   return "RTKfix";
    default:              return "?";
  }
}

void DisplayOled::mostrar(const EstadoDisplayRx& e) {
  char linha[32];
  display_.clear();
  display_.setTextAlignment(TEXT_ALIGN_LEFT);
  display_.setFont(ArialMT_Plain_10);

  if (!e.temDados) {
    display_.drawString(0, 0, "AGUARDANDO 1o pacote");
    display_.display();
    return;
  }

  if (e.timeout) {
    snprintf(linha, sizeof(linha), "TIMEOUT (%lus)",
             (unsigned long)(e.idadeUltimoPacoteMs / 1000UL));
  } else {
    snprintf(linha, sizeof(linha), "OK RSSI:%d SNR:%d", (int)e.rssi, (int)e.snr);
  }
  display_.drawString(0, 0, linha);

  snprintf(linha, sizeof(linha), "Fix:%s Sat:%u", nomeFix(e.gnss.tipo_fix),
           e.gnss.satelites);
  display_.drawString(0, 12, linha);

  snprintf(linha, sizeof(linha), "La:%.5f", e.gnss.latitude_1e7 / 1e7);
  display_.drawString(0, 24, linha);

  snprintf(linha, sizeof(linha), "Lo:%.5f", e.gnss.longitude_1e7 / 1e7);
  display_.drawString(0, 36, linha);

  snprintf(linha, sizeof(linha), "Alt:%.1fm Seq:%u",
           e.gnss.altitude_mm / 1000.0, (unsigned)e.sequencia);
  display_.drawString(0, 48, linha);

  display_.display();
}
