// cobs.h
// -----------------------------------------------------------------------------
// Enquadramento de bytes usando COBS (Consistent Overhead Byte Stuffing).
// O byte 0x00 e reservado como delimitador de fim de quadro. O COBS remove
// todos os 0x00 do interior do pacote, permitindo usar 0x00 como marcador
// inequivoco de fronteira de quadro no enlace UART.
//
// Todas as funcoes recebem a capacidade do buffer de destino e NUNCA escrevem
// alem dela: retornam 0 (falha) em vez de estourar.
//
// Arquivo em C++ puro (sem Arduino) para ser testavel no host.
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_COBS_H
#define PROTOCOLO_COBS_H

#include <stddef.h>
#include <stdint.h>

namespace protocolo {

// Pior caso de expansao do COBS: 1 byte de overhead a cada 254 bytes, mais o
// byte de codigo inicial. Use este helper para dimensionar buffers de destino.
inline size_t tamanhoMaximoCobs(size_t tamanhoEntrada) {
  return tamanhoEntrada + (tamanhoEntrada / 254u) + 1u;
}

// Codifica 'tamanhoEntrada' bytes de 'entrada' em 'saida'. NAO acrescenta o
// delimitador 0x00 final (isso e responsabilidade de quem transmite o quadro).
// Retorna o numero de bytes escritos em 'saida', ou 0 em caso de falha
// (capacidade insuficiente ou parametros invalidos).
size_t cobsEncode(const uint8_t* entrada, size_t tamanhoEntrada,
                  uint8_t* saida, size_t capacidadeSaida);

// Decodifica um quadro COBS (SEM o delimitador 0x00 final) de 'entrada' para
// 'saida'. Retorna o numero de bytes escritos em 'saida', ou 0 em caso de
// falha (quadro malformado ou capacidade insuficiente).
size_t cobsDecode(const uint8_t* entrada, size_t tamanhoEntrada,
                  uint8_t* saida, size_t capacidadeSaida);

}  // namespace protocolo

#endif  // PROTOCOLO_COBS_H
