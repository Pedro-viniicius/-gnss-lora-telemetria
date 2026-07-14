// crc16.h
// -----------------------------------------------------------------------------
// Calculo de CRC16-CCITT (variante "FALSE"): polinomio 0x1021, valor inicial
// 0xFFFF, sem reflexao de entrada/saida e sem XOR final.
//
// Vetor de teste conhecido: a mensagem ASCII "123456789" resulta em 0x29B1.
//
// Este arquivo e C++ puro (sem dependencia do Arduino) para poder ser compilado
// tanto no firmware quanto nos testes de host (g++).
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_CRC16_H
#define PROTOCOLO_CRC16_H

#include <stddef.h>
#include <stdint.h>

namespace protocolo {

// Calcula o CRC16-CCITT sobre 'tamanho' bytes de 'dados'.
uint16_t calcularCrc16(const uint8_t* dados, size_t tamanho);

// Versao incremental: permite acumular o CRC em blocos. Passe 0xFFFF como
// 'crcAtual' na primeira chamada.
uint16_t atualizarCrc16(uint16_t crcAtual, uint8_t byte);

}  // namespace protocolo

#endif  // PROTOCOLO_CRC16_H
