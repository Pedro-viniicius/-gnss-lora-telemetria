// diagnostico.cpp
#include "diagnostico.h"

#include "configuracao_hardware.h"

using namespace protocolo;

static const char* nomeFix(uint8_t tipoFix) {
  switch (tipoFix) {
    case FIX_SEM_FIX:     return "SEM_FIX";
    case FIX_STANDALONE:  return "STANDALONE";
    case FIX_DIFERENCIAL: return "DIFERENCIAL";
    case FIX_RTK_FLOAT:   return "RTK_FLOAT";
    case FIX_RTK_FIXED:   return "RTK_FIXED";
    default:              return "?";
  }
}

void Diagnostico::atualizar(uint32_t agoraMs, const MaquinaEstados& estado,
                            const TelemetriaGnss& gnss, bool gnssValido,
                            uint32_t idadeGnssMs,
                            const ServicoGnss& servicoGnss,
                            const ServicoUartLora& uart,
                            uint16_t sequenciaAtual) {
  if ((uint32_t)(agoraMs - instante_ultimo_ms_) < PERIODO_DIAGNOSTICO_MS) {
    return;
  }
  instante_ultimo_ms_ = agoraMs;

  Serial.println(F("----- DIAGNOSTICO CONTROLADOR -----"));
  Serial.print(F("Estado: "));
  Serial.println(estado.nome());

  Serial.print(F("GNSS fix: "));
  Serial.print(nomeFix(gnss.tipo_fix));
  Serial.print(F(" | sats: "));
  Serial.print(gnss.satelites);
  Serial.print(F(" | valido: "));
  Serial.println(gnssValido ? F("SIM") : F("NAO (antigo/sem fix)"));

  if (gnss.flags & GNSS_FLAG_DADOS_SIMULADOS) {
    Serial.println(F(">>> ATENCAO: DADOS SIMULADOS (modo diagnostico) <<<"));
  }

  Serial.print(F("Lat: "));
  Serial.print(gnss.latitude_1e7 / 1e7, 7);
  Serial.print(F("  Lon: "));
  Serial.print(gnss.longitude_1e7 / 1e7, 7);
  Serial.print(F("  Alt(m): "));
  Serial.println(gnss.altitude_mm / 1000.0, 2);

  Serial.print(F("Precisao horiz.(mm): "));
  if (gnss.flags & GNSS_FLAG_TEM_PRECISAO) {
    Serial.println(gnss.precisao_horizontal_mm);
  } else {
    Serial.println(F("indisponivel"));
  }

  Serial.print(F("Idade GNSS(ms): "));
  if (idadeGnssMs == UINT32_MAX) {
    Serial.println(F("sem dados"));
  } else {
    Serial.println(idadeGnssMs);
  }

  Serial.print(F("GNSS bytes: "));
  Serial.print(servicoGnss.bytesRecebidos());
  Serial.print(F(" | NMEA ok: "));
  Serial.print(servicoGnss.sentencasValidas());
  Serial.print(F(" | invalidas: "));
  Serial.print(servicoGnss.sentencasInvalidas());
  Serial.print(F(" | estouros: "));
  Serial.println(servicoGnss.sentencasEstouradas());

  Serial.print(F("GNSS baud atual: "));
  Serial.print(servicoGnss.baudAtual());
  Serial.print(F(" | varredura baud: "));
  Serial.print(servicoGnss.varreduraBaudAtiva() ? F("ATIVA") : F("PARADA"));
  Serial.print(F(" | bytes neste baud: "));
  Serial.println(servicoGnss.bytesNoBaudAtual());

  Serial.print(F("GNSS GGA/RMC/GST: "));
  Serial.print(servicoGnss.sentencasGga());
  Serial.print(F("/"));
  Serial.print(servicoGnss.sentencasRmc());
  Serial.print(F("/"));
  Serial.print(servicoGnss.sentencasGst());
  Serial.print(F(" | ultima: "));
  Serial.print(servicoGnss.ultimoTipoSentenca());
  Serial.print(F(" | idade ultimo byte(ms): "));
  uint32_t idadeByte = servicoGnss.idadeUltimoByteMs();
  if (idadeByte == UINT32_MAX) {
    Serial.println(F("sem bytes"));
  } else {
    Serial.println(idadeByte);
  }

  Serial.print(F("Enlace LoRa/UART: "));
  Serial.print(uart.conectado() ? F("CONECTADO") : F("DESCONECTADO"));
  Serial.print(F(" | enviados: "));
  Serial.print(uart.pacotesEnviados());
  Serial.print(F(" | rx validos: "));
  Serial.print(uart.quadrosValidos());
  Serial.print(F(" | rx invalidos: "));
  Serial.println(uart.quadrosInvalidos());

  Serial.print(F("UART LoRa bruto: bytes rx: "));
  Serial.print(uart.bytesRecebidos());
  Serial.print(F(" | delimitadores 0x00: "));
  Serial.print(uart.delimitadoresRecebidos());
  Serial.print(F(" | bytes no buffer: "));
  Serial.print(uart.bytesNoBufferAtual());
  Serial.print(F(" | idade ultimo byte(ms): "));
  uint32_t idadeByteLora = uart.idadeUltimoByteMs();
  if (idadeByteLora == UINT32_MAX) {
    Serial.println(F("sem bytes"));
  } else {
    Serial.println(idadeByteLora);
  }

  StatusTransmissor st;
  if (uart.obterUltimoStatus(st)) {
    Serial.print(F("TX status: transmissoes: "));
    Serial.print(st.transmissoes_lora);
    Serial.print(F(" | timeouts: "));
    Serial.print(st.timeouts_lora);
    Serial.print(F(" | descartes: "));
    Serial.println(st.pacotes_descartados);
  }

  Serial.print(F("Sequencia atual: "));
  Serial.println(sequenciaAtual);
  Serial.println(F("Teclado: desabilitado | Cartao SD: desabilitado"));
  Serial.println();
}
