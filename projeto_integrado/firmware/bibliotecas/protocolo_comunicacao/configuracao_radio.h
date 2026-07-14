// configuracao_radio.h
// -----------------------------------------------------------------------------
// FONTE UNICA DE VERDADE dos parametros de radio LoRa P2P. Tanto o transmissor
// quanto o receptor incluem este mesmo arquivo, garantindo que operem com
// parametros identicos. NAO duplique estes valores em outros arquivos.
//
// Valores derivados dos codigos de referencia (Heltec WiFi LoRa 32 V3, SX1262).
//
// ATENCAO REGULATORIA: a frequencia, a potencia, a antena e os parametros de
// operacao devem obedecer a legislacao local (no Brasil, 915 MHz esta na faixa
// ISM 902-928 MHz) e a variante exata da placa. Confirme antes de operar.
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_CONFIGURACAO_RADIO_H
#define PROTOCOLO_CONFIGURACAO_RADIO_H

#include <stdint.h>

namespace protocolo {

// Frequencia central em Hz.
static const uint32_t RADIO_FREQUENCIA_HZ = 915000000UL;

// Potencia de saida em dBm.
static const int8_t RADIO_POTENCIA_TX_DBM = 20;

// Largura de banda: codificacao da API Heltec (0 = 125 kHz, 1 = 250 kHz,
// 2 = 500 kHz). 0 => 125 kHz.
static const uint8_t RADIO_LARGURA_BANDA = 0;

// Spreading Factor (7..12).
static const uint8_t RADIO_SPREADING_FACTOR = 7;

// Coding Rate: codificacao da API Heltec (1 = 4/5, 2 = 4/6, 3 = 4/7, 4 = 4/8).
static const uint8_t RADIO_CODING_RATE = 1;

// Comprimento do preambulo, em simbolos.
static const uint16_t RADIO_PREAMBULO = 8;

// Timeout de simbolo para recepcao (0 = desabilitado).
static const uint16_t RADIO_SYMBOL_TIMEOUT = 0;

// Payload de comprimento fixo desabilitado (usamos comprimento variavel).
static const bool RADIO_PAYLOAD_FIXO = false;

// Inversao de IQ desabilitada.
static const bool RADIO_IQ_INVERTIDO = false;

// Timeout de transmissao em milissegundos (usado no SetTxConfig da Heltec).
static const uint32_t RADIO_TX_TIMEOUT_MS = 3000;

// Validacoes de coerencia em tempo de compilacao.
static_assert(RADIO_SPREADING_FACTOR >= 7 && RADIO_SPREADING_FACTOR <= 12,
              "Spreading Factor fora da faixa valida (7..12)");
static_assert(RADIO_LARGURA_BANDA <= 2,
              "Codigo de largura de banda invalido (0..2)");
static_assert(RADIO_CODING_RATE >= 1 && RADIO_CODING_RATE <= 4,
              "Codigo de coding rate invalido (1..4)");
static_assert(RADIO_POTENCIA_TX_DBM >= 0 && RADIO_POTENCIA_TX_DBM <= 22,
              "Potencia de TX fora da faixa tipica do SX1262 (0..22 dBm)");

}  // namespace protocolo

#endif  // PROTOCOLO_CONFIGURACAO_RADIO_H
