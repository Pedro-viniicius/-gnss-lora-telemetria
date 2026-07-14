// servico_gnss.cpp
#include "servico_gnss.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "configuracao_hardware.h"

using protocolo::TelemetriaGnss;

ServicoGnss::ServicoGnss()
    : serial_(nullptr),
      linha_pos_(0),
      recebeu_dados_(false),
      instante_ultimo_fix_ms_(0),
      tem_fix_(false),
      simulacao_(false),
      instante_simulacao_ms_(0) {
  memset(&dados_, 0, sizeof(dados_));
  memset(linha_, 0, sizeof(linha_));
  dados_.tipo_fix = protocolo::FIX_SEM_FIX;
}

void ServicoGnss::iniciar(HardwareSerial& serial) {
  serial_ = &serial;
  serial_->begin(GNSS_BAUD, SERIAL_8N1, GNSS_PINO_RX, GNSS_PINO_TX);
  linha_pos_ = 0;
}

void ServicoGnss::habilitarSimulacao(bool habilitar) {
  simulacao_ = habilitar;
}

void ServicoGnss::atualizar() {
  if (simulacao_) {
    // Gera dados sinteticos periodicamente, marcados como simulados.
    if ((uint32_t)(millis() - instante_simulacao_ms_) >= 1000UL) {
      instante_simulacao_ms_ = millis();
      gerarSimulacao();
    }
    return;
  }

  if (serial_ == nullptr) {
    return;
  }

  // Leitura nao-bloqueante: consome o que estiver disponivel neste ciclo.
  while (serial_->available() > 0) {
    char c = (char)serial_->read();
    processarByte(c);
  }
}

void ServicoGnss::processarByte(char c) {
  if (c == '\r') {
    return;  // ignorado
  }
  if (c == '\n') {
    if (linha_pos_ > 0) {
      linha_[linha_pos_] = '\0';
      processarLinha(linha_, linha_pos_);
    }
    linha_pos_ = 0;
    return;
  }
  if (linha_pos_ < TAM_LINHA - 1) {
    linha_[linha_pos_++] = c;
  } else {
    // Sentenca maior que o buffer: descarta e reinicia (sentenca corrompida).
    linha_pos_ = 0;
  }
}

bool ServicoGnss::checksumValido(const char* linha, size_t tamanho) const {
  if (tamanho < 4 || linha[0] != '$') {
    return false;
  }
  // Procura o '*'.
  size_t asterisco = 0;
  bool achou = false;
  for (size_t i = 1; i < tamanho; ++i) {
    if (linha[i] == '*') {
      asterisco = i;
      achou = true;
      break;
    }
  }
  if (!achou || asterisco + 2 >= tamanho) {
    return false;
  }
  uint8_t calc = 0;
  for (size_t i = 1; i < asterisco; ++i) {
    calc ^= (uint8_t)linha[i];
  }
  // Converte os 2 digitos hex apos o '*'.
  auto hexDigito = [](char h) -> int {
    if (h >= '0' && h <= '9') return h - '0';
    if (h >= 'A' && h <= 'F') return 10 + (h - 'A');
    if (h >= 'a' && h <= 'f') return 10 + (h - 'a');
    return -1;
  };
  int alto = hexDigito(linha[asterisco + 1]);
  int baixo = hexDigito(linha[asterisco + 2]);
  if (alto < 0 || baixo < 0) {
    return false;
  }
  uint8_t recebido = (uint8_t)((alto << 4) | baixo);
  return calc == recebido;
}

// Divide a linha (modificando-a) em campos separados por ','. Interrompe no '*'.
// Retorna o numero de campos preenchidos.
static int dividirCampos(char* linha, char* campos[], int maxCampos) {
  int n = 0;
  char* p = linha;
  campos[n++] = p;
  while (*p != '\0' && n < maxCampos) {
    if (*p == '*') {
      *p = '\0';
      break;
    }
    if (*p == ',') {
      *p = '\0';
      campos[n++] = p + 1;
    }
    ++p;
  }
  return n;
}

// Converte "ddmm.mmmm" (lat) ou "dddmm.mmmm" (lon) + hemisferio para graus x 1e7.
// floor(bruto/100) extrai os graus corretamente em ambos os formatos.
static int32_t parseCoordenada(const char* campo, char hemisferio) {
  if (campo == nullptr || campo[0] == '\0') {
    return 0;
  }
  double bruto = strtod(campo, nullptr);  // ex.: 2107.4567
  double graus = floor(bruto / 100.0);
  double minutos = bruto - graus * 100.0;
  double decimal = graus + minutos / 60.0;
  if (hemisferio == 'S' || hemisferio == 's' || hemisferio == 'W' ||
      hemisferio == 'w') {
    decimal = -decimal;
  }
  return (int32_t)llround(decimal * 1e7);
}

void ServicoGnss::processarLinha(char* linha, size_t tamanho) {
  if (!checksumValido(linha, tamanho)) {
    return;  // sentenca corrompida: descarta silenciosamente
  }
  // Tipo de sentenca esta nos offsets 3..5 (apos '$' + talker de 2 chars).
  if (tamanho < 6) {
    return;
  }
  const char* tipo = linha + 3;
  if (strncmp(tipo, "GGA", 3) == 0) {
    parseGGA(linha);
  } else if (strncmp(tipo, "RMC", 3) == 0) {
    parseRMC(linha);
  } else if (strncmp(tipo, "GST", 3) == 0) {
    parseGST(linha);
  }
}

void ServicoGnss::parseGGA(char* linha) {
  char* campos[16];
  int n = dividirCampos(linha, campos, 16);
  // GGA: 0=tipo 1=hora 2=lat 3=N/S 4=lon 5=E/W 6=qualidade 7=sats 8=HDOP 9=alt
  if (n < 10) {
    return;
  }
  int qualidade = (campos[6][0] != '\0') ? atoi(campos[6]) : 0;
  uint8_t tipoFix;
  switch (qualidade) {
    case 0:  tipoFix = protocolo::FIX_SEM_FIX;     break;
    case 1:  tipoFix = protocolo::FIX_STANDALONE;  break;
    case 2:  tipoFix = protocolo::FIX_DIFERENCIAL; break;
    case 4:  tipoFix = protocolo::FIX_RTK_FIXED;   break;
    case 5:  tipoFix = protocolo::FIX_RTK_FLOAT;   break;
    default: tipoFix = protocolo::FIX_STANDALONE;  break;
  }
  dados_.tipo_fix = tipoFix;
  dados_.satelites = (campos[7][0] != '\0') ? (uint8_t)atoi(campos[7]) : 0;

  recebeu_dados_ = true;

  if (qualidade > 0 && campos[2][0] != '\0' && campos[4][0] != '\0') {
    dados_.latitude_1e7 = parseCoordenada(campos[2], campos[3][0]);
    dados_.longitude_1e7 = parseCoordenada(campos[4], campos[5][0]);
    if (campos[9][0] != '\0') {
      dados_.altitude_mm = (int32_t)llround(strtod(campos[9], nullptr) * 1000.0);
      dados_.flags |= protocolo::GNSS_FLAG_TEM_ALTITUDE;
    }
    tem_fix_ = true;
    instante_ultimo_fix_ms_ = millis();
  } else {
    tem_fix_ = false;
  }
}

void ServicoGnss::parseRMC(char* linha) {
  // RMC traz status/velocidade; usamos apenas para confirmar validade.
  char* campos[14];
  int n = dividirCampos(linha, campos, 14);
  if (n < 3) {
    return;
  }
  recebeu_dados_ = true;
  // campos[2] = 'A' (valido) ou 'V' (invalido). Nao sobrescreve o fix do GGA,
  // apenas registra que ha fluxo de dados.
}

void ServicoGnss::parseGST(char* linha) {
  // GST: 0=tipo 1=hora 2=rms 3=maj 4=min 5=orient 6=stdLat 7=stdLon 8=stdAlt
  char* campos[10];
  int n = dividirCampos(linha, campos, 10);
  if (n < 8) {
    return;
  }
  if (campos[6][0] != '\0' && campos[7][0] != '\0') {
    double stdLat = strtod(campos[6], nullptr);
    double stdLon = strtod(campos[7], nullptr);
    double horizontal = sqrt(stdLat * stdLat + stdLon * stdLon);  // metros
    double mm = horizontal * 1000.0;
    if (mm < 0) mm = 0;
    if (mm > 65535.0) mm = 65535.0;
    dados_.precisao_horizontal_mm = (uint16_t)llround(mm);
    dados_.flags |= protocolo::GNSS_FLAG_TEM_PRECISAO;
  }
}

bool ServicoGnss::dadosAntigos() const {
  if (!recebeu_dados_ || instante_ultimo_fix_ms_ == 0) {
    return true;
  }
  return (uint32_t)(millis() - instante_ultimo_fix_ms_) > GNSS_IDADE_MAX_MS;
}

bool ServicoGnss::temFixValido() const {
  return tem_fix_ && !dadosAntigos();
}

uint32_t ServicoGnss::idadeDadosMs() const {
  if (!recebeu_dados_ || instante_ultimo_fix_ms_ == 0) {
    return UINT32_MAX;
  }
  return (uint32_t)(millis() - instante_ultimo_fix_ms_);
}

bool ServicoGnss::obterSnapshot(TelemetriaGnss& saida) {
  // Atualiza flags de idade/simulado sem descartar as flags de conteudo.
  dados_.flags &= (uint8_t)~(protocolo::GNSS_FLAG_DADOS_ANTIGOS |
                             protocolo::GNSS_FLAG_DADOS_SIMULADOS);
  if (dadosAntigos()) {
    dados_.flags |= protocolo::GNSS_FLAG_DADOS_ANTIGOS;
  }
  if (simulacao_) {
    dados_.flags |= protocolo::GNSS_FLAG_DADOS_SIMULADOS;
  }
  dados_.uptime_ms = millis();
  saida = dados_;
  return recebeu_dados_;
}

void ServicoGnss::injetarSentencaNmea(const char* sentenca) {
  if (sentenca == nullptr) {
    return;
  }
  for (const char* p = sentenca; *p != '\0'; ++p) {
    processarByte(*p);
  }
  processarByte('\n');
}

void ServicoGnss::injetarRtcm(const uint8_t* dados, size_t tamanho) {
  // Ponto de extensao: correcoes RTCM cruas seriam escritas no UART do UM980.
  // NAO ha NTRIP/caster/credenciais aqui (fora do escopo atual).
  if (serial_ != nullptr && dados != nullptr && tamanho > 0 && !simulacao_) {
    serial_->write(dados, tamanho);
  }
}

void ServicoGnss::gerarSimulacao() {
  // Coordenada base (Lavras/MG, aproximada) com pequena deriva ciclica.
  static uint32_t contador = 0;
  ++contador;
  double deriva = sin((double)contador * 0.1) * 0.00005;  // ~ +-5 m
  dados_.latitude_1e7 = (int32_t)llround((-21.2255 + deriva) * 1e7);
  dados_.longitude_1e7 = (int32_t)llround((-44.9931 + deriva) * 1e7);
  dados_.altitude_mm = 919000 + (int32_t)(deriva * 1e6);
  dados_.satelites = 12 + (uint8_t)(contador % 6);
  // Alterna entre standalone e RTK float para exercitar os estados.
  dados_.tipo_fix = (contador % 5 == 0) ? protocolo::FIX_RTK_FLOAT
                                        : protocolo::FIX_STANDALONE;
  dados_.precisao_horizontal_mm = (dados_.tipo_fix == protocolo::FIX_RTK_FLOAT)
                                      ? 300
                                      : 2500;
  dados_.flags = protocolo::GNSS_FLAG_TEM_ALTITUDE |
                 protocolo::GNSS_FLAG_TEM_PRECISAO |
                 protocolo::GNSS_FLAG_DADOS_SIMULADOS;
  recebeu_dados_ = true;
  tem_fix_ = true;
  instante_ultimo_fix_ms_ = millis();
}
