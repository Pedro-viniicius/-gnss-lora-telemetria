// gerenciador_telemetria.h
// -----------------------------------------------------------------------------
// Monta pacotes de telemetria GNSS periodicos e gerencia o contador global de
// sequencia usado por TODOS os pacotes enviados pelo controlador (telemetria e
// eventos de teclado compartilham o mesmo espaco de sequencia).
// -----------------------------------------------------------------------------
#ifndef GERENCIADOR_TELEMETRIA_H
#define GERENCIADOR_TELEMETRIA_H

#include <stdint.h>

#include "protocolo.h"

class GerenciadorTelemetria {
 public:
  GerenciadorTelemetria() : sequencia_(0) {}

  // Proximo numero de sequencia (incrementa a cada chamada; faz rollover em 16 bits).
  uint16_t proximaSequencia() { return ++sequencia_; }

  uint16_t sequenciaAtual() const { return sequencia_; }

  // Monta um pacote de telemetria a partir de um snapshot do GNSS, injetando o
  // ultimo comando de teclado confirmado e um numero de sequencia novo.
  void montarTelemetria(protocolo::TelemetriaGnss snapshot,
                        uint8_t ultimoComandoTeclado,
                        protocolo::Pacote& saida) {
    snapshot.ultimo_comando_teclado = ultimoComandoTeclado;
    protocolo::montarPacoteTelemetria(saida, protocolo::ID_CONTROLADOR,
                                      proximaSequencia(), snapshot);
  }

 private:
  uint16_t sequencia_;
};

#endif  // GERENCIADOR_TELEMETRIA_H
