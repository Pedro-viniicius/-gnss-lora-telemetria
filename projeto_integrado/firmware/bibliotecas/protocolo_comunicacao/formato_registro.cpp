// formato_registro.cpp
#include "formato_registro.h"

#include <stdio.h>

namespace protocolo {

// Formata um inteiro escalado por 1e7 (graus) em texto "-dd.ddddddd".
static void grausE7ParaTexto(char* out, size_t cap, int32_t valor) {
  bool negativo = valor < 0;
  // Usa int64 para tratar com seguranca o valor minimo de int32.
  int64_t abs64 = negativo ? -(int64_t)valor : (int64_t)valor;
  unsigned long inteiro = (unsigned long)(abs64 / 10000000);
  unsigned long frac = (unsigned long)(abs64 % 10000000);
  snprintf(out, cap, "%s%lu.%07lu", negativo ? "-" : "", inteiro, frac);
}

// Formata milimetros em metros "-m.mmm".
static void milimetrosParaTexto(char* out, size_t cap, int32_t mm) {
  bool negativo = mm < 0;
  int64_t abs64 = negativo ? -(int64_t)mm : (int64_t)mm;
  unsigned long inteiro = (unsigned long)(abs64 / 1000);
  unsigned long frac = (unsigned long)(abs64 % 1000);
  snprintf(out, cap, "%s%lu.%03lu", negativo ? "-" : "", inteiro, frac);
}

static char comandoImprimivel(uint8_t c) {
  return (c >= 32 && c < 127) ? (char)c : '-';
}

size_t cabecalhoCsv(char* buf, size_t cap) {
  int n = snprintf(
      buf, cap,
      "tipo;uptime_ms;sequencia;latitude;longitude;altitude_m;fix;satelites;"
      "precisao_mm;comando_tecla;valor_ou_flags\n");
  if (n < 0 || (size_t)n >= cap) {
    return 0;
  }
  return (size_t)n;
}

size_t formatarLinhaTelemetriaCsv(char* buf, size_t cap,
                                  const TelemetriaGnss& t, uint16_t sequencia) {
  char lat[16];
  char lon[16];
  char alt[16];
  grausE7ParaTexto(lat, sizeof(lat), t.latitude_1e7);
  grausE7ParaTexto(lon, sizeof(lon), t.longitude_1e7);
  milimetrosParaTexto(alt, sizeof(alt), t.altitude_mm);

  int n = snprintf(buf, cap, "TELEMETRIA;%lu;%u;%s;%s;%s;%u;%u;%u;%c;%u\n",
                   (unsigned long)t.uptime_ms, (unsigned)sequencia, lat, lon,
                   alt, (unsigned)t.tipo_fix, (unsigned)t.satelites,
                   (unsigned)t.precisao_horizontal_mm,
                   comandoImprimivel(t.ultimo_comando_teclado),
                   (unsigned)t.flags);
  if (n < 0 || (size_t)n >= cap) {
    return 0;
  }
  return (size_t)n;
}

size_t formatarLinhaEventoCsv(char* buf, size_t cap, const EventoTeclado& e,
                              uint16_t sequencia) {
  // Colunas GNSS ficam vazias; comando_tecla = tecla; valor_ou_flags = valor.
  int n = snprintf(buf, cap, "EVENTO;%lu;%u;;;;;;;%c;%lu\n",
                   (unsigned long)e.uptime_ms, (unsigned)sequencia,
                   comandoImprimivel(e.tecla), (unsigned long)e.valor_numerico);
  if (n < 0 || (size_t)n >= cap) {
    return 0;
  }
  return (size_t)n;
}

}  // namespace protocolo
