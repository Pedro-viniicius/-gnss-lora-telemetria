// formato_registro.h
// -----------------------------------------------------------------------------
// Formatacao das linhas de log em CSV para o cartao microSD. C++ puro (sem
// Arduino) e testavel no host. Usa snprintf com verificacao de capacidade
// (nunca estoura; retorna 0 em caso de estouro). Separador de campos: ';'.
//
// Colunas (11):
//   tipo;uptime_ms;sequencia;latitude;longitude;altitude_m;fix;satelites;
//   precisao_mm;comando_tecla;valor_ou_flags
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_FORMATO_REGISTRO_H
#define PROTOCOLO_FORMATO_REGISTRO_H

#include <stddef.h>
#include <stdint.h>

#include "tipos_comandos.h"
#include "tipos_gnss.h"

namespace protocolo {

// Escreve o cabecalho CSV (com '\n'). Retorna bytes escritos (sem o NUL) ou 0.
size_t cabecalhoCsv(char* buf, size_t cap);

// Formata uma linha de telemetria GNSS. Retorna bytes escritos ou 0 (estouro).
size_t formatarLinhaTelemetriaCsv(char* buf, size_t cap,
                                  const TelemetriaGnss& t, uint16_t sequencia);

// Formata uma linha de evento de teclado. Retorna bytes escritos ou 0.
size_t formatarLinhaEventoCsv(char* buf, size_t cap, const EventoTeclado& e,
                              uint16_t sequencia);

}  // namespace protocolo

#endif  // PROTOCOLO_FORMATO_REGISTRO_H
