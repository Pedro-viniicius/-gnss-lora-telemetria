// protocolo.h
// -----------------------------------------------------------------------------
// Protocolo binario versionado, compartilhado pelos tres firmwares.
//
// Formato do pacote (little-endian, sem depender de padding/alinhamento):
//
//   Offset  Tam  Campo
//   ------  ---  ---------------------------------------------------------------
//    0       1   magic[0] = 0xAA
//    1       1   magic[1] = 0x55
//    2       1   versao (VERSAO_PROTOCOLO)
//    3       1   tipo (ver TipoMensagem)
//    4       1   id_remetente
//    5       2   sequencia (uint16)
//    7       2   tamanho_payload (uint16)
//    9       1   flags
//   10       N   payload (N = tamanho_payload)
//   10+N     2   crc16 (CRC16-CCITT sobre os bytes 0..10+N-1)
//
// O mesmo pacote validado e transportado tanto como carga LoRa quanto (apos
// enquadramento COBS) pelo enlace UART.
//
// Arquivo em C++ puro (sem Arduino): compilavel no firmware e no host (g++).
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_PROTOCOLO_H
#define PROTOCOLO_PROTOCOLO_H

#include <stddef.h>
#include <stdint.h>

#include "cobs.h"
#include "crc16.h"
#include "tipos_comandos.h"
#include "tipos_gnss.h"

namespace protocolo {

// --- Constantes de formato --------------------------------------------------
static const uint8_t  MAGIC_0 = 0xAA;
static const uint8_t  MAGIC_1 = 0x55;
static const uint8_t  VERSAO_PROTOCOLO = 1;

static const size_t   TAMANHO_CABECALHO = 10;  // ate o inicio do payload
static const size_t   TAMANHO_CRC = 2;
static const size_t   MAX_PAYLOAD = 64;         // limite de payload da aplicacao
static const size_t   TAMANHO_MINIMO_PACOTE = TAMANHO_CABECALHO + TAMANHO_CRC;
static const size_t   TAMANHO_MAXIMO_PACOTE =
    TAMANHO_CABECALHO + MAX_PAYLOAD + TAMANHO_CRC;  // 76

// Buffer maximo de um quadro UART (pacote apos COBS + delimitador 0x00).
static const size_t   TAMANHO_MAXIMO_QUADRO_COBS =
    TAMANHO_MAXIMO_PACOTE + (TAMANHO_MAXIMO_PACOTE / 254u) + 2u;  // folga

// Identificadores de remetente conhecidos.
enum IdRemetente : uint8_t {
  ID_CONTROLADOR = 0x10,
  ID_TRANSMISSOR = 0x20,
  ID_RECEPTOR    = 0x30
};

// --- Estrutura de pacote decodificado ---------------------------------------
struct Pacote {
  uint8_t  versao;
  uint8_t  tipo;
  uint8_t  id_remetente;
  uint16_t sequencia;
  uint8_t  flags;
  uint16_t tamanho_payload;
  uint8_t  payload[MAX_PAYLOAD];
};

// --- Resultado da decodificacao ---------------------------------------------
enum ResultadoDecodificacao : uint8_t {
  DECODE_OK = 0,
  DECODE_MUITO_CURTO,        // menor que o pacote minimo
  DECODE_MAGIC_INVALIDO,     // bytes magicos incorretos
  DECODE_VERSAO_INVALIDA,    // versao de protocolo desconhecida
  DECODE_TAMANHO_INVALIDO,   // tamanho_payload impossivel (> MAX_PAYLOAD)
  DECODE_TRUNCADO,           // buffer menor que cabecalho+payload+crc
  DECODE_CRC_INVALIDO,       // CRC nao confere
  DECODE_TIPO_DESCONHECIDO   // tipo desconhecido (apenas em modo estrito)
};

// Texto legivel (PT-BR) para um resultado de decodificacao.
const char* textoResultado(ResultadoDecodificacao r);

// --- Helpers de serializacao little-endian ----------------------------------
void escreverU16LE(uint8_t* destino, uint16_t valor);
void escreverU32LE(uint8_t* destino, uint32_t valor);
void escreverI32LE(uint8_t* destino, int32_t valor);
uint16_t lerU16LE(const uint8_t* origem);
uint32_t lerU32LE(const uint8_t* origem);
int32_t  lerI32LE(const uint8_t* origem);

// --- Codificacao / decodificacao de pacote ----------------------------------
// Serializa 'pkt' (magic + cabecalho + payload + crc) em 'saida'. Retorna o
// numero total de bytes escritos, ou 0 em caso de falha (payload grande demais
// ou capacidade insuficiente).
size_t codificarPacote(const Pacote& pkt, uint8_t* saida, size_t capacidade);

// Valida e decodifica um pacote binario cru (sem COBS) de 'dados'. Preenche
// 'saida' quando o resultado e DECODE_OK. Em 'modoEstrito', rejeita tipos de
// mensagem desconhecidos.
ResultadoDecodificacao decodificarPacote(const uint8_t* dados, size_t tamanho,
                                         Pacote& saida, bool modoEstrito);

// --- Enquadramento UART (COBS + delimitador 0x00) ---------------------------
// Codifica 'pkt' como pacote binario e o enquadra com COBS, acrescentando o
// delimitador 0x00 ao final. Retorna o numero de bytes do quadro, ou 0 em falha.
size_t enquadrarParaUart(const Pacote& pkt, uint8_t* saida, size_t capacidade);

// Decodifica um quadro UART (conteudo COBS SEM o delimitador 0x00) em um pacote
// validado. Retorna o resultado da decodificacao.
ResultadoDecodificacao desenquadrarDeUart(const uint8_t* quadro,
                                          size_t tamanhoQuadro, Pacote& saida,
                                          bool modoEstrito);

// --- Montadores/leitores de payloads tipados --------------------------------
void montarPacoteTelemetria(Pacote& pkt, uint8_t idRemetente, uint16_t sequencia,
                            const TelemetriaGnss& telemetria);
bool lerTelemetria(const Pacote& pkt, TelemetriaGnss& saida);

void montarPacoteEventoTeclado(Pacote& pkt, uint8_t idRemetente,
                               uint16_t sequencia, const EventoTeclado& evento);
bool lerEventoTeclado(const Pacote& pkt, EventoTeclado& saida);

void montarPacoteStatusTransmissor(Pacote& pkt, uint8_t idRemetente,
                                   uint16_t sequencia,
                                   const StatusTransmissor& status);
bool lerStatusTransmissor(const Pacote& pkt, StatusTransmissor& saida);

// Monta um HEARTBEAT simples (payload = uptime_ms em 4 bytes).
void montarPacoteHeartbeat(Pacote& pkt, uint8_t idRemetente, uint16_t sequencia,
                           uint32_t uptime_ms);

}  // namespace protocolo

#endif  // PROTOCOLO_PROTOCOLO_H
