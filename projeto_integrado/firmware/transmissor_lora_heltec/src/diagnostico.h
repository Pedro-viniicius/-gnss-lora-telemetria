// diagnostico.h — Transmissor LoRa
// -----------------------------------------------------------------------------
// Impressao periodica dos contadores de saude pelo USB Serial. Nao bloqueia.
// -----------------------------------------------------------------------------
#ifndef DIAGNOSTICO_TX_H
#define DIAGNOSTICO_TX_H

#include <Arduino.h>

class DiagnosticoTx {
 public:
  DiagnosticoTx() : instante_ultimo_ms_(0) {}

  void atualizar(uint32_t agoraMs, bool uartConectado, uint32_t quadrosValidos,
                 uint32_t quadrosInvalidos, uint32_t transmissoes,
                 uint32_t timeouts, uint32_t descartados, uint8_t estadoTx,
                 uint32_t bytesRecebidos, uint32_t delimitadoresRecebidos,
                 size_t bytesNoBuffer, uint32_t idadeUltimoByteMs);

 private:
  uint32_t instante_ultimo_ms_;
};

#endif  // DIAGNOSTICO_TX_H
