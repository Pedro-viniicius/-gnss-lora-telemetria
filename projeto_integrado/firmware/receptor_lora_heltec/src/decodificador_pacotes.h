// decodificador_pacotes.h — Receptor LoRa
// -----------------------------------------------------------------------------
// Valida pacotes recebidos (magic, versao, tipo, tamanho, CRC) e detecta
// sequencias duplicadas e faltantes. Mantem contadores.
// -----------------------------------------------------------------------------
#ifndef DECODIFICADOR_PACOTES_H
#define DECODIFICADOR_PACOTES_H

#include <stdint.h>

#include "protocolo.h"

class DecodificadorPacotes {
 public:
  enum Resultado : uint8_t {
    RES_OK = 0,        // pacote valido e novo
    RES_DUPLICADO,     // sequencia igual a anterior
    RES_INVALIDO       // falhou na validacao (ver ultimoErro)
  };

  DecodificadorPacotes();

  // Processa 'tamanho' bytes crus. Em RES_OK/RES_DUPLICADO, 'saida' e valido.
  // 'faltante' indica salto de sequencia (pacotes perdidos) no caso RES_OK.
  Resultado processar(const uint8_t* dados, size_t tamanho,
                      protocolo::Pacote& saida, bool& faltante);

  protocolo::ResultadoDecodificacao ultimoErro() const { return ultimo_erro_; }

  uint32_t validos() const { return validos_; }
  uint32_t invalidos() const { return invalidos_; }
  uint32_t duplicados() const { return duplicados_; }
  uint32_t faltantes() const { return faltantes_; }

 private:
  bool tem_ultima_seq_;
  uint16_t ultima_seq_;

  protocolo::ResultadoDecodificacao ultimo_erro_;
  uint32_t validos_;
  uint32_t invalidos_;
  uint32_t duplicados_;
  uint32_t faltantes_;
};

#endif  // DECODIFICADOR_PACOTES_H
