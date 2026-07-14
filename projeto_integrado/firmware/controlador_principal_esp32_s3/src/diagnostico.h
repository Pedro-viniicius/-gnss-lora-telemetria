// diagnostico.h
// -----------------------------------------------------------------------------
// Impressao periodica de diagnostico pelo USB Serial. Nao bloqueia; imprime a
// cada PERIODO_DIAGNOSTICO_MS. Sem uso de String dinamica em caminho critico.
// -----------------------------------------------------------------------------
#ifndef DIAGNOSTICO_H
#define DIAGNOSTICO_H

#include <Arduino.h>

#include "maquina_estados.h"
#include "protocolo.h"
#include "servico_uart_lora.h"

class Diagnostico {
 public:
  Diagnostico() : instante_ultimo_ms_(0) {}

  // Imprime um resumo se ja passou o periodo. 'gnssValido' indica se o snapshot
  // corresponde a um fix atual (nao antigo).
  void atualizar(uint32_t agoraMs, const MaquinaEstados& estado,
                 const protocolo::TelemetriaGnss& gnss, bool gnssValido,
                 uint32_t idadeGnssMs, const ServicoUartLora& uart,
                 uint8_t modoTeclado, uint16_t sequenciaAtual);

 private:
  uint32_t instante_ultimo_ms_;
};

#endif  // DIAGNOSTICO_H
