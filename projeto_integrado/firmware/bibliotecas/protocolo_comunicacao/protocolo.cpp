// protocolo.cpp
// Implementacao do protocolo binario versionado.
#include "protocolo.h"

namespace protocolo {

// --- Helpers de serializacao little-endian ----------------------------------
void escreverU16LE(uint8_t* destino, uint16_t valor) {
  destino[0] = (uint8_t)(valor & 0xFF);
  destino[1] = (uint8_t)((valor >> 8) & 0xFF);
}

void escreverU32LE(uint8_t* destino, uint32_t valor) {
  destino[0] = (uint8_t)(valor & 0xFF);
  destino[1] = (uint8_t)((valor >> 8) & 0xFF);
  destino[2] = (uint8_t)((valor >> 16) & 0xFF);
  destino[3] = (uint8_t)((valor >> 24) & 0xFF);
}

void escreverI32LE(uint8_t* destino, int32_t valor) {
  // Conversao para uint32 preserva o padrao de bits (complemento de dois).
  escreverU32LE(destino, (uint32_t)valor);
}

uint16_t lerU16LE(const uint8_t* origem) {
  return (uint16_t)((uint16_t)origem[0] | ((uint16_t)origem[1] << 8));
}

uint32_t lerU32LE(const uint8_t* origem) {
  return (uint32_t)origem[0] | ((uint32_t)origem[1] << 8) |
         ((uint32_t)origem[2] << 16) | ((uint32_t)origem[3] << 24);
}

int32_t lerI32LE(const uint8_t* origem) {
  return (int32_t)lerU32LE(origem);
}

// --- Texto de resultado -----------------------------------------------------
const char* textoResultado(ResultadoDecodificacao r) {
  switch (r) {
    case DECODE_OK:               return "OK";
    case DECODE_MUITO_CURTO:      return "pacote muito curto";
    case DECODE_MAGIC_INVALIDO:   return "bytes magicos invalidos";
    case DECODE_VERSAO_INVALIDA:  return "versao de protocolo invalida";
    case DECODE_TAMANHO_INVALIDO: return "tamanho de payload invalido";
    case DECODE_TRUNCADO:         return "pacote truncado";
    case DECODE_CRC_INVALIDO:     return "CRC invalido";
    case DECODE_TIPO_DESCONHECIDO:return "tipo de mensagem desconhecido";
    default:                      return "desconhecido";
  }
}

// --- Codificacao ------------------------------------------------------------
size_t codificarPacote(const Pacote& pkt, uint8_t* saida, size_t capacidade) {
  if (saida == nullptr) {
    return 0;
  }
  if (pkt.tamanho_payload > MAX_PAYLOAD) {
    return 0;
  }
  const size_t total = TAMANHO_CABECALHO + pkt.tamanho_payload + TAMANHO_CRC;
  if (capacidade < total) {
    return 0;
  }

  saida[0] = MAGIC_0;
  saida[1] = MAGIC_1;
  saida[2] = pkt.versao;
  saida[3] = pkt.tipo;
  saida[4] = pkt.id_remetente;
  escreverU16LE(&saida[5], pkt.sequencia);
  escreverU16LE(&saida[7], pkt.tamanho_payload);
  saida[9] = pkt.flags;

  for (uint16_t i = 0; i < pkt.tamanho_payload; ++i) {
    saida[TAMANHO_CABECALHO + i] = pkt.payload[i];
  }

  const size_t offsetCrc = TAMANHO_CABECALHO + pkt.tamanho_payload;
  const uint16_t crc = calcularCrc16(saida, offsetCrc);
  escreverU16LE(&saida[offsetCrc], crc);

  return total;
}

// --- Decodificacao ----------------------------------------------------------
ResultadoDecodificacao decodificarPacote(const uint8_t* dados, size_t tamanho,
                                         Pacote& saida, bool modoEstrito) {
  if (dados == nullptr || tamanho < TAMANHO_MINIMO_PACOTE) {
    return DECODE_MUITO_CURTO;
  }
  if (dados[0] != MAGIC_0 || dados[1] != MAGIC_1) {
    return DECODE_MAGIC_INVALIDO;
  }
  const uint8_t versao = dados[2];
  if (versao != VERSAO_PROTOCOLO) {
    return DECODE_VERSAO_INVALIDA;
  }

  const uint16_t tamanhoPayload = lerU16LE(&dados[7]);
  if (tamanhoPayload > MAX_PAYLOAD) {
    return DECODE_TAMANHO_INVALIDO;
  }

  const size_t total = TAMANHO_CABECALHO + tamanhoPayload + TAMANHO_CRC;
  if (tamanho < total) {
    return DECODE_TRUNCADO;
  }

  const size_t offsetCrc = TAMANHO_CABECALHO + tamanhoPayload;
  const uint16_t crcCalculado = calcularCrc16(dados, offsetCrc);
  const uint16_t crcRecebido = lerU16LE(&dados[offsetCrc]);
  if (crcCalculado != crcRecebido) {
    return DECODE_CRC_INVALIDO;
  }

  const uint8_t tipo = dados[3];
  if (modoEstrito && !tipoMensagemConhecido(tipo)) {
    return DECODE_TIPO_DESCONHECIDO;
  }

  saida.versao = versao;
  saida.tipo = tipo;
  saida.id_remetente = dados[4];
  saida.sequencia = lerU16LE(&dados[5]);
  saida.tamanho_payload = tamanhoPayload;
  saida.flags = dados[9];
  for (uint16_t i = 0; i < tamanhoPayload; ++i) {
    saida.payload[i] = dados[TAMANHO_CABECALHO + i];
  }

  return DECODE_OK;
}

// --- Enquadramento UART -----------------------------------------------------
size_t enquadrarParaUart(const Pacote& pkt, uint8_t* saida, size_t capacidade) {
  if (saida == nullptr) {
    return 0;
  }
  uint8_t bruto[TAMANHO_MAXIMO_PACOTE];
  const size_t tamanhoBruto = codificarPacote(pkt, bruto, sizeof(bruto));
  if (tamanhoBruto == 0) {
    return 0;
  }
  // Precisa de espaco para o COBS mais o delimitador 0x00.
  if (capacidade < 1) {
    return 0;
  }
  const size_t tamanhoCobs =
      cobsEncode(bruto, tamanhoBruto, saida, capacidade - 1);
  if (tamanhoCobs == 0) {
    return 0;
  }
  saida[tamanhoCobs] = 0x00;  // delimitador de fim de quadro
  return tamanhoCobs + 1;
}

ResultadoDecodificacao desenquadrarDeUart(const uint8_t* quadro,
                                          size_t tamanhoQuadro, Pacote& saida,
                                          bool modoEstrito) {
  uint8_t bruto[TAMANHO_MAXIMO_PACOTE];
  const size_t tamanhoBruto =
      cobsDecode(quadro, tamanhoQuadro, bruto, sizeof(bruto));
  if (tamanhoBruto == 0) {
    return DECODE_MUITO_CURTO;
  }
  return decodificarPacote(bruto, tamanhoBruto, saida, modoEstrito);
}

// --- Payloads tipados: TelemetriaGnss ---------------------------------------
void montarPacoteTelemetria(Pacote& pkt, uint8_t idRemetente, uint16_t sequencia,
                            const TelemetriaGnss& t) {
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_TELEMETRIA_GNSS;
  pkt.id_remetente = idRemetente;
  pkt.sequencia = sequencia;
  pkt.flags = t.flags;
  pkt.tamanho_payload = TAMANHO_PAYLOAD_TELEMETRIA_GNSS;

  uint8_t* p = pkt.payload;
  escreverU32LE(&p[0], t.uptime_ms);
  escreverI32LE(&p[4], t.latitude_1e7);
  escreverI32LE(&p[8], t.longitude_1e7);
  escreverI32LE(&p[12], t.altitude_mm);
  p[16] = t.tipo_fix;
  p[17] = t.satelites;
  escreverU16LE(&p[18], t.precisao_horizontal_mm);
  p[20] = t.ultimo_comando_teclado;
  p[21] = t.flags;
}

bool lerTelemetria(const Pacote& pkt, TelemetriaGnss& saida) {
  if (pkt.tipo != MSG_TELEMETRIA_GNSS ||
      pkt.tamanho_payload != TAMANHO_PAYLOAD_TELEMETRIA_GNSS) {
    return false;
  }
  const uint8_t* p = pkt.payload;
  saida.uptime_ms = lerU32LE(&p[0]);
  saida.latitude_1e7 = lerI32LE(&p[4]);
  saida.longitude_1e7 = lerI32LE(&p[8]);
  saida.altitude_mm = lerI32LE(&p[12]);
  saida.tipo_fix = p[16];
  saida.satelites = p[17];
  saida.precisao_horizontal_mm = lerU16LE(&p[18]);
  saida.ultimo_comando_teclado = p[20];
  saida.flags = p[21];
  return true;
}

// --- Payloads tipados: EventoTeclado ----------------------------------------
void montarPacoteEventoTeclado(Pacote& pkt, uint8_t idRemetente,
                               uint16_t sequencia, const EventoTeclado& e) {
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_EVENTO_TECLADO;
  pkt.id_remetente = idRemetente;
  pkt.sequencia = sequencia;
  pkt.flags = 0;
  pkt.tamanho_payload = TAMANHO_PAYLOAD_EVENTO_TECLADO;

  uint8_t* p = pkt.payload;
  escreverU32LE(&p[0], e.uptime_ms);
  p[4] = e.tecla;
  p[5] = e.tipo_evento;
  escreverU32LE(&p[6], e.valor_numerico);
  p[10] = e.tamanho_entrada;
}

bool lerEventoTeclado(const Pacote& pkt, EventoTeclado& saida) {
  if (pkt.tipo != MSG_EVENTO_TECLADO ||
      pkt.tamanho_payload != TAMANHO_PAYLOAD_EVENTO_TECLADO) {
    return false;
  }
  const uint8_t* p = pkt.payload;
  saida.uptime_ms = lerU32LE(&p[0]);
  saida.tecla = p[4];
  saida.tipo_evento = p[5];
  saida.valor_numerico = lerU32LE(&p[6]);
  saida.tamanho_entrada = p[10];
  return true;
}

// --- Payloads tipados: StatusTransmissor ------------------------------------
void montarPacoteStatusTransmissor(Pacote& pkt, uint8_t idRemetente,
                                   uint16_t sequencia,
                                   const StatusTransmissor& s) {
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_STATUS_TRANSMISSOR;
  pkt.id_remetente = idRemetente;
  pkt.sequencia = sequencia;
  pkt.flags = 0;
  pkt.tamanho_payload = TAMANHO_PAYLOAD_STATUS_TRANSMISSOR;

  uint8_t* p = pkt.payload;
  escreverU32LE(&p[0], s.uptime_ms);
  escreverU32LE(&p[4], s.quadros_uart_validos);
  escreverU32LE(&p[8], s.quadros_uart_invalidos);
  escreverU32LE(&p[12], s.transmissoes_lora);
  escreverU32LE(&p[16], s.timeouts_lora);
  escreverU32LE(&p[20], s.pacotes_descartados);
  escreverU16LE(&p[24], s.ultima_sequencia);
  p[26] = s.estado_tx;
}

bool lerStatusTransmissor(const Pacote& pkt, StatusTransmissor& saida) {
  if (pkt.tipo != MSG_STATUS_TRANSMISSOR ||
      pkt.tamanho_payload != TAMANHO_PAYLOAD_STATUS_TRANSMISSOR) {
    return false;
  }
  const uint8_t* p = pkt.payload;
  saida.uptime_ms = lerU32LE(&p[0]);
  saida.quadros_uart_validos = lerU32LE(&p[4]);
  saida.quadros_uart_invalidos = lerU32LE(&p[8]);
  saida.transmissoes_lora = lerU32LE(&p[12]);
  saida.timeouts_lora = lerU32LE(&p[16]);
  saida.pacotes_descartados = lerU32LE(&p[20]);
  saida.ultima_sequencia = lerU16LE(&p[24]);
  saida.estado_tx = p[26];
  return true;
}

// --- HEARTBEAT --------------------------------------------------------------
void montarPacoteHeartbeat(Pacote& pkt, uint8_t idRemetente, uint16_t sequencia,
                           uint32_t uptime_ms) {
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_HEARTBEAT;
  pkt.id_remetente = idRemetente;
  pkt.sequencia = sequencia;
  pkt.flags = 0;
  pkt.tamanho_payload = 4;
  escreverU32LE(&pkt.payload[0], uptime_ms);
}

}  // namespace protocolo
