// diagnostico.cpp — Receptor LoRa
#include "diagnostico.h"

using namespace protocolo;

static const char* nomeFix(uint8_t t) {
  switch (t) {
    case FIX_SEM_FIX:     return "SEM_FIX";
    case FIX_STANDALONE:  return "STANDALONE";
    case FIX_DIFERENCIAL: return "DIFERENCIAL";
    case FIX_RTK_FLOAT:   return "RTK_FLOAT";
    case FIX_RTK_FIXED:   return "RTK_FIXED";
    default:              return "?";
  }
}

static const char* nomeTipo(uint8_t t) {
  switch (t) {
    case MSG_TELEMETRIA_GNSS: return "TELEMETRIA_GNSS";
    case MSG_EVENTO_TECLADO:  return "EVENTO_TECLADO";
    case MSG_COMANDO:         return "COMANDO";
    case MSG_HEARTBEAT:       return "HEARTBEAT";
    default:                  return "OUTRO";
  }
}

void DiagnosticoRx::imprimirPacote(const Pacote& pkt, int16_t rssi, int8_t snr,
                                   bool duplicado, bool faltante,
                                   const DecodificadorPacotes& dec) {
  Serial.println(F("===== PACOTE RECEBIDO ====="));
  Serial.print(F("Tipo: "));
  Serial.print(nomeTipo(pkt.tipo));
  Serial.print(F(" | seq: "));
  Serial.print(pkt.sequencia);
  Serial.print(F(" | RSSI: "));
  Serial.print(rssi);
  Serial.print(F(" dBm | SNR: "));
  Serial.println(snr);

  if (duplicado) {
    Serial.println(F(">>> SEQUENCIA DUPLICADA <<<"));
  }
  if (faltante) {
    Serial.println(F(">>> PACOTES FALTANTES (salto de sequencia) <<<"));
  }

  if (pkt.tipo == MSG_TELEMETRIA_GNSS) {
    TelemetriaGnss t;
    if (lerTelemetria(pkt, t)) {
      if (t.flags & GNSS_FLAG_DADOS_SIMULADOS) {
        Serial.println(F(">>> ATENCAO: DADOS SIMULADOS (nao sao GNSS real) <<<"));
      }
      if (t.flags & GNSS_FLAG_DADOS_ANTIGOS) {
        Serial.println(F(">>> Dados de posicao ANTIGOS (stale) <<<"));
      }
      Serial.print(F("Fix: "));
      Serial.print(nomeFix(t.tipo_fix));
      Serial.print(F(" | Satelites: "));
      Serial.println(t.satelites);
      Serial.print(F("Latitude:  "));
      Serial.println(t.latitude_1e7 / 1e7, 7);
      Serial.print(F("Longitude: "));
      Serial.println(t.longitude_1e7 / 1e7, 7);
      Serial.print(F("Altitude(m): "));
      Serial.println(t.altitude_mm / 1000.0, 2);
      if (t.flags & GNSS_FLAG_TEM_PRECISAO) {
        Serial.print(F("Precisao horiz.(mm): "));
        Serial.println(t.precisao_horizontal_mm);
      }
      Serial.print(F("Uptime controlador(ms): "));
      Serial.println(t.uptime_ms);
      char c = (t.ultimo_comando_teclado >= 32 && t.ultimo_comando_teclado < 127)
                   ? (char)t.ultimo_comando_teclado
                   : '-';
      Serial.print(F("Ultimo comando teclado: "));
      Serial.println(c);
    }
  } else if (pkt.tipo == MSG_EVENTO_TECLADO) {
    EventoTeclado ev;
    if (lerEventoTeclado(pkt, ev)) {
      Serial.print(F("Tecla: "));
      Serial.print((char)ev.tecla);
      Serial.print(F(" | tipo_evento: "));
      Serial.print(ev.tipo_evento);
      Serial.print(F(" | valor: "));
      Serial.println(ev.valor_numerico);
    }
  } else if (pkt.tipo == MSG_HEARTBEAT) {
    Serial.print(F("Heartbeat uptime(ms): "));
    Serial.println(lerU32LE(&pkt.payload[0]));
  }

  Serial.print(F("Contadores -> validos: "));
  Serial.print(dec.validos());
  Serial.print(F(" | duplicados: "));
  Serial.print(dec.duplicados());
  Serial.print(F(" | faltantes: "));
  Serial.print(dec.faltantes());
  Serial.print(F(" | invalidos: "));
  Serial.println(dec.invalidos());
  Serial.println();
}

void DiagnosticoRx::imprimirInvalido(ResultadoDecodificacao erro, int16_t rssi,
                                     int8_t snr, const DecodificadorPacotes& dec) {
  Serial.print(F("[PACOTE INVALIDO] "));
  Serial.print(textoResultado(erro));
  Serial.print(F(" | RSSI: "));
  Serial.print(rssi);
  Serial.print(F(" | SNR: "));
  Serial.print(snr);
  Serial.print(F(" | total invalidos: "));
  Serial.println(dec.invalidos());
}
