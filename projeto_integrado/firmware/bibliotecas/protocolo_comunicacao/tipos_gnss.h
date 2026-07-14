// tipos_gnss.h
// -----------------------------------------------------------------------------
// Tipos de dados GNSS usados na telemetria. Coordenadas sao representadas como
// inteiros escalados para evitar ambiguidade de ponto flutuante no protocolo:
//
//   latitude  = graus x 10.000.000   (int32, 1e7)
//   longitude = graus x 10.000.000   (int32, 1e7)
//   altitude  = milimetros           (int32)
//
// IMPORTANTE: a presenca de um UM980 NAO garante precisao centimetrica. O tipo
// de fix (ver TipoFix) indica a qualidade real da solucao. RTK Fixed depende de
// correcoes validas, antena adequada e boa instalacao.
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_TIPOS_GNSS_H
#define PROTOCOLO_TIPOS_GNSS_H

#include <stdint.h>

namespace protocolo {

// Qualidade da solucao de posicao.
enum TipoFix : uint8_t {
  FIX_SEM_FIX      = 0,  // sem posicao valida
  FIX_STANDALONE   = 1,  // solucao autonoma (GNSS puro)
  FIX_DIFERENCIAL  = 2,  // diferencial (DGPS/SBAS)
  FIX_RTK_FLOAT    = 3,  // RTK com ambiguidades ainda nao fixadas
  FIX_RTK_FIXED    = 4   // RTK com ambiguidades fixadas (maxima precisao)
};

// Flags de estado da telemetria GNSS (campo de bits).
enum FlagsGnss : uint8_t {
  GNSS_FLAG_DADOS_SIMULADOS = 0x01,  // dados gerados em modo de diagnostico
  GNSS_FLAG_DADOS_ANTIGOS   = 0x02,  // idade dos dados acima do limite (stale)
  GNSS_FLAG_TEM_ALTITUDE    = 0x04,  // campo de altitude e valido
  GNSS_FLAG_TEM_PRECISAO    = 0x08,  // campo de precisao horizontal e valido
  GNSS_FLAG_SD_ATIVO        = 0x10,  // cartao microSD pronto e gravando
  GNSS_FLAG_SD_FALHA        = 0x20   // cartao microSD ausente/cheio/em falha
};

// Estrutura de telemetria GNSS. Serializada explicitamente (ver protocolo.h);
// NUNCA e transmitida via memcpy direto do struct.
struct TelemetriaGnss {
  uint32_t uptime_ms;                 // tempo desde o boot do controlador
  int32_t  latitude_1e7;              // graus x 1e7
  int32_t  longitude_1e7;             // graus x 1e7
  int32_t  altitude_mm;               // milimetros (elipsoidal/ortometrica)
  uint8_t  tipo_fix;                  // ver TipoFix
  uint8_t  satelites;                 // numero de satelites em uso
  uint16_t precisao_horizontal_mm;    // estimativa de precisao (mm)
  uint8_t  ultimo_comando_teclado;    // ultimo comando confirmado (ASCII/0)
  uint8_t  flags;                     // ver FlagsGnss
};

// Tamanho do payload serializado de TelemetriaGnss (bytes).
// 4 + 4 + 4 + 4 + 1 + 1 + 2 + 1 + 1 = 22
static const uint16_t TAMANHO_PAYLOAD_TELEMETRIA_GNSS = 22;

}  // namespace protocolo

#endif  // PROTOCOLO_TIPOS_GNSS_H
