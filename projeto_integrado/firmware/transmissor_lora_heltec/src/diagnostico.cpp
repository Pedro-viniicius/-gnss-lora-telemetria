// diagnostico.cpp — Transmissor LoRa
#include "diagnostico.h"

#include "configuracao_hardware.h"
#include "tipos_comandos.h"

using namespace protocolo;

static const char* nomeEstado(uint8_t e) {
  switch (e) {
    case TX_OCIOSO:       return "OCIOSO";
    case TX_TRANSMITINDO: return "TRANSMITINDO";
    case TX_TIMEOUT:      return "TIMEOUT";
    default:              return "?";
  }
}

void DiagnosticoTx::atualizar(uint32_t agoraMs, bool uartConectado,
                              uint32_t quadrosValidos, uint32_t quadrosInvalidos,
                              uint32_t transmissoes, uint32_t timeouts,
                              uint32_t descartados, uint8_t estadoTx,
                              uint32_t bytesRecebidos,
                              uint32_t delimitadoresRecebidos,
                              size_t bytesNoBuffer,
                              uint32_t idadeUltimoByteMs) {
  if ((uint32_t)(agoraMs - instante_ultimo_ms_) < PERIODO_STATUS_MS) {
    return;
  }
  instante_ultimo_ms_ = agoraMs;

  Serial.println(F("----- DIAGNOSTICO TRANSMISSOR -----"));
  Serial.print(F("UART controlador: "));
  Serial.println(uartConectado ? F("CONECTADO") : F("DESCONECTADO"));
  Serial.print(F("Quadros UART - validos: "));
  Serial.print(quadrosValidos);
  Serial.print(F(" | invalidos: "));
  Serial.println(quadrosInvalidos);
  Serial.print(F("UART bruto - bytes rx: "));
  Serial.print(bytesRecebidos);
  Serial.print(F(" | delimitadores 0x00: "));
  Serial.print(delimitadoresRecebidos);
  Serial.print(F(" | bytes no buffer: "));
  Serial.print(bytesNoBuffer);
  Serial.print(F(" | idade ultimo byte(ms): "));
  if (idadeUltimoByteMs == UINT32_MAX) {
    Serial.println(F("sem bytes"));
  } else {
    Serial.println(idadeUltimoByteMs);
  }
  Serial.print(F("LoRa - transmissoes: "));
  Serial.print(transmissoes);
  Serial.print(F(" | timeouts: "));
  Serial.print(timeouts);
  Serial.print(F(" | descartados: "));
  Serial.println(descartados);
  Serial.print(F("Estado TX: "));
  Serial.println(nomeEstado(estadoTx));
  Serial.println();
}
