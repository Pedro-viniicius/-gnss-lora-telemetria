// diagnostico.h — Receptor LoRa
// -----------------------------------------------------------------------------
// Impressao detalhada, pelo USB Serial, do pacote decodificado (telemetria
// GNSS ou evento de teclado) e dos contadores. Nao bloqueia.
// -----------------------------------------------------------------------------
#ifndef DIAGNOSTICO_RX_H
#define DIAGNOSTICO_RX_H

#include <Arduino.h>

#include "decodificador_pacotes.h"
#include "protocolo.h"

class DiagnosticoRx {
 public:
  DiagnosticoRx() {}

  // Imprime os detalhes de um pacote recem-recebido.
  void imprimirPacote(const protocolo::Pacote& pkt, int16_t rssi, int8_t snr,
                      bool duplicado, bool faltante,
                      const DecodificadorPacotes& dec);

  // Imprime a razao de um pacote invalido.
  void imprimirInvalido(protocolo::ResultadoDecodificacao erro, int16_t rssi,
                        int8_t snr, const DecodificadorPacotes& dec);
};

#endif  // DIAGNOSTICO_RX_H
