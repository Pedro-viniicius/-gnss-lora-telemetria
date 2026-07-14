// crc16.cpp
// Implementacao bit-a-bit do CRC16-CCITT (0x1021, init 0xFFFF).
#include "crc16.h"

namespace protocolo {

uint16_t atualizarCrc16(uint16_t crcAtual, uint8_t byte) {
  crcAtual ^= (uint16_t)byte << 8;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    if (crcAtual & 0x8000u) {
      crcAtual = (uint16_t)((crcAtual << 1) ^ 0x1021u);
    } else {
      crcAtual = (uint16_t)(crcAtual << 1);
    }
  }
  return crcAtual;
}

uint16_t calcularCrc16(const uint8_t* dados, size_t tamanho) {
  uint16_t crc = 0xFFFFu;
  for (size_t i = 0; i < tamanho; ++i) {
    crc = atualizarCrc16(crc, dados[i]);
  }
  return crc;
}

}  // namespace protocolo
