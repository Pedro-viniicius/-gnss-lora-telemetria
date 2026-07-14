// decodificador_pacotes.cpp — Receptor LoRa
#include "decodificador_pacotes.h"

using namespace protocolo;

DecodificadorPacotes::DecodificadorPacotes()
    : tem_ultima_seq_(false),
      ultima_seq_(0),
      ultimo_erro_(DECODE_OK),
      validos_(0),
      invalidos_(0),
      duplicados_(0),
      faltantes_(0) {}

DecodificadorPacotes::Resultado DecodificadorPacotes::processar(
    const uint8_t* dados, size_t tamanho, Pacote& saida, bool& faltante) {
  faltante = false;

  ResultadoDecodificacao r = decodificarPacote(dados, tamanho, saida, true);
  ultimo_erro_ = r;
  if (r != DECODE_OK) {
    ++invalidos_;
    return RES_INVALIDO;
  }

  // Deteccao de duplicata / faltante por numero de sequencia.
  if (tem_ultima_seq_) {
    if (saida.sequencia == ultima_seq_) {
      ++duplicados_;
      return RES_DUPLICADO;
    }
    uint16_t esperada = (uint16_t)(ultima_seq_ + 1);
    if (saida.sequencia != esperada) {
      // Salto: um ou mais pacotes foram perdidos. O rollover 65535->0 cai no
      // caso "esperada" e nao e contado como faltante.
      faltante = true;
      ++faltantes_;
    }
  }

  ultima_seq_ = saida.sequencia;
  tem_ultima_seq_ = true;
  ++validos_;
  return RES_OK;
}
