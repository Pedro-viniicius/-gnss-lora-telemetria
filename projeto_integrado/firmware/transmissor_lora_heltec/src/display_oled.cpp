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
    case MSG_COMANDO:            return "COMANDO";
    case MSG_HEARTBEAT:          return "HEARTBEAT";
    case MSG_STATUS_TRANSMISSOR: return "STATUS";
    default:                     return "-";
  }
}

static const char* nomeFix(uint8_t t) {
  switch (t) {
    case FIX_SEM_FIX:     return "SEM_FIX";
    case FIX_STANDALONE:  return "SINGLE";
    case FIX_DIFERENCIAL: return "DGNSS";
    case FIX_RTK_FLOAT:   return "RTK_FLT";
    case FIX_RTK_FIXED:   return "RTK_FIX";
    default:              return "?";
  }
}

static void grausE7ParaTexto(char* out, size_t cap, int32_t valor) {
  bool negativo = valor < 0;
  int64_t abs64 = negativo ? -(int64_t)valor : (int64_t)valor;
  unsigned long inteiro = (unsigned long)(abs64 / 10000000L);
  unsigned long frac = (unsigned long)(abs64 % 10000000L);
  snprintf(out, cap, "%s%lu.%07lu", negativo ? "-" : "", inteiro, frac);
}

static void altitudeParaTexto(char* out, size_t cap, int32_t altitudeMm) {
  bool negativo = altitudeMm < 0;
  int64_t abs64 = negativo ? -(int64_t)altitudeMm : (int64_t)altitudeMm;
  unsigned long metros = (unsigned long)(abs64 / 1000L);
  unsigned long decimos = (unsigned long)((abs64 % 1000L) / 100L);
  snprintf(out, cap, "%s%lu.%lum", negativo ? "-" : "", metros, decimos);
}

void DisplayOled::mostrar(const EstadoDisplayTx& e) {
  char linha[32];
  display_.clear();
  display_.setTextAlignment(TEXT_ALIGN_LEFT);
  display_.setFont(ArialMT_Plain_10);

  snprintf(linha, sizeof(linha), "UART:%s TX:%s",
           e.uartConectado ? "OK" : "--", nomeEstadoTx(e.estadoTx));
  display_.drawString(0, 0, linha);

  if (!e.temGnss) {
    snprintf(linha, sizeof(linha), "Aguardando GPS...");
    display_.drawString(0, 12, linha);

    snprintf(linha, sizeof(linha), "Seq:%u Tipo:%s", (unsigned)e.ultimaSequencia,
             nomeTipo(e.ultimoTipo));
    display_.drawString(0, 24, linha);

    snprintf(linha, sizeof(linha), "Inv:%lu OK:%lu",
             (unsigned long)e.quadrosInvalidos, (unsigned long)e.transmissoes);
    display_.drawString(0, 36, linha);

    if (e.idadeUltimoQuadroMs == UINT32_MAX) {
      snprintf(linha, sizeof(linha), "Ult. pacote: --");
    } else {
      snprintf(linha, sizeof(linha), "Ult. pacote: %lus",
               (unsigned long)(e.idadeUltimoQuadroMs / 1000UL));
    }
    display_.drawString(0, 48, linha);
    display_.display();
    return;
  }

  snprintf(linha, sizeof(linha), "Fix:%s Sat:%u", nomeFix(e.gnss.tipo_fix),
           (unsigned)e.gnss.satelites);
  display_.drawString(0, 12, linha);

  char lat[18];
  char lon[18];
  grausE7ParaTexto(lat, sizeof(lat), e.gnss.latitude_1e7);
  grausE7ParaTexto(lon, sizeof(lon), e.gnss.longitude_1e7);

  snprintf(linha, sizeof(linha), "Lat:%s", lat);
  display_.drawString(0, 24, linha);

  snprintf(linha, sizeof(linha), "Lon:%s", lon);
  display_.drawString(0, 36, linha);

  char alt[14];
  altitudeParaTexto(alt, sizeof(alt), e.gnss.altitude_mm);
  snprintf(linha, sizeof(linha), "A:%s S:%u", alt, (unsigned)e.sequenciaGnss);
  display_.drawString(0, 48, linha);

  display_.display();
}
