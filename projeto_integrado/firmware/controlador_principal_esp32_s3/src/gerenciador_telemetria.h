// gerenciador_telemetria.h
// -----------------------------------------------------------------------------
// Monta pacotes de telemetria GNSS periodicos e gerencia o contador global de
// sequencia usado pelos pacotes enviados pelo controlador.
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

  // Monta um pacote de telemetria a partir de um snapshot do GNSS. O campo
  // legado "ultimo_comando_teclado" vai zerado nesta versao sem teclado.
  void montarTelemetria(protocolo::TelemetriaGnss snapshot,
                        protocolo::Pacote& saida) {
    snapshot.ultimo_comando_teclado = 0;
    protocolo::montarPacoteTelemetria(saida, protocolo::ID_CONTROLADOR,
                                      proximaSequencia(), snapshot);
  }

 private:
  uint16_t sequencia_;
};

#endif  // GERENCIADOR_TELEMETRIA_H
