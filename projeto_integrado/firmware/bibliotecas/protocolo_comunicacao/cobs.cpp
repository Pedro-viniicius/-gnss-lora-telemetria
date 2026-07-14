// cobs.cpp
// Implementacao de COBS com verificacao rigorosa de capacidade de destino.
#include "cobs.h"

namespace protocolo {

size_t cobsEncode(const uint8_t* entrada, size_t tamanhoEntrada,
                  uint8_t* saida, size_t capacidadeSaida) {
  if (saida == nullptr) {
    return 0;
  }
  if (tamanhoEntrada > 0 && entrada == nullptr) {
    return 0;
  }
  // O menor quadro possivel ja consome 1 byte (o codigo inicial).
  if (capacidadeSaida < 1) {
    return 0;
  }

  size_t indiceLeitura = 0;
  size_t indiceEscrita = 1;   // reserva 1 byte para o primeiro codigo
  size_t indiceCodigo = 0;    // posicao onde o codigo corrente sera gravado
  uint8_t codigo = 1;

  while (indiceLeitura < tamanhoEntrada) {
    if (entrada[indiceLeitura] == 0) {
      // Fecha o bloco corrente gravando o codigo acumulado.
      saida[indiceCodigo] = codigo;
      codigo = 1;
      if (indiceEscrita >= capacidadeSaida) {
        return 0;
      }
      indiceCodigo = indiceEscrita++;
      indiceLeitura++;
    } else {
      if (indiceEscrita >= capacidadeSaida) {
        return 0;
      }
      saida[indiceEscrita++] = entrada[indiceLeitura++];
      codigo++;
      if (codigo == 0xFF) {
        // Bloco cheio (254 bytes sem zero): inicia um novo bloco.
        saida[indiceCodigo] = codigo;
        codigo = 1;
        if (indiceEscrita >= capacidadeSaida) {
          return 0;
        }
        indiceCodigo = indiceEscrita++;
      }
    }
  }

  // Grava o codigo do ultimo bloco.
  saida[indiceCodigo] = codigo;
  return indiceEscrita;
}

size_t cobsDecode(const uint8_t* entrada, size_t tamanhoEntrada,
                  uint8_t* saida, size_t capacidadeSaida) {
  if (tamanhoEntrada == 0) {
    return 0;
  }
  if (entrada == nullptr || saida == nullptr) {
    return 0;
  }

  size_t indiceLeitura = 0;
  size_t indiceEscrita = 0;

  while (indiceLeitura < tamanhoEntrada) {
    uint8_t codigo = entrada[indiceLeitura];
    // Um 0x00 no interior indica quadro malformado (nao deveria existir).
    if (codigo == 0) {
      return 0;
    }
    // O bloco precisa caber no que resta da entrada.
    if (indiceLeitura + codigo > tamanhoEntrada + 1) {
      return 0;
    }
    indiceLeitura++;

    for (uint8_t i = 1; i < codigo; ++i) {
      if (indiceLeitura >= tamanhoEntrada) {
        return 0;
      }
      if (indiceEscrita >= capacidadeSaida) {
        return 0;
      }
      saida[indiceEscrita++] = entrada[indiceLeitura++];
    }

    // Reinsere o zero suprimido, exceto apos um bloco cheio (0xFF) ou no fim.
    if (codigo != 0xFF && indiceLeitura < tamanhoEntrada) {
      if (indiceEscrita >= capacidadeSaida) {
        return 0;
      }
      saida[indiceEscrita++] = 0;
    }
  }

  return indiceEscrita;
}

}  // namespace protocolo
