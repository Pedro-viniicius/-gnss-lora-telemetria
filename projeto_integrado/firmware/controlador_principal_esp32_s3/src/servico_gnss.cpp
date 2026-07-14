// servico_gnss.cpp
#include "servico_gnss.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "configuracao_hardware.h"

#if GNSS_CONFIGURAR_COM_SPARKFUN
#include <SparkFun_Unicore_GNSS_Arduino_Library.h>
#endif

using protocolo::TelemetriaGnss;

#if GNSS_CONFIGURAR_COM_SPARKFUN
static UM980 receptor_um980;

static void saidaSparkfunSilenciosa(uint8_t* /*buffer*/, size_t /*length*/) {}

static bool configurarUm980ComSparkFun(HardwareSerial& serial) {
  Serial.println(F("[GNSS] Tentando detectar/configurar UM980 pela biblioteca SparkFun..."));

  bool conectado = false;
  for (uint8_t tentativa = 1; tentativa <= 3; ++tentativa) {
    Serial.print(F("[GNSS] Sonda UM980 SparkFun tentativa "));
    Serial.print(tentativa);
    Serial.println(F("/3"));
    if (receptor_um980.begin(serial, "SFE_Unicore_GNSS_Library",
                             saidaSparkfunSilenciosa)) {
      conectado = true;
      break;
    }
    delay(250);
  }

  if (!conectado) {
    Serial.println(F("[GNSS] UM980 nao respondeu aos comandos SparkFun; mantendo leitura serial bruta"));
    return false;
  }

  Serial.println(F("[GNSS] UM980 respondeu. Aplicando configuracao volatil de bancada..."));

  uint8_t falhas = 0;
  receptor_um980.disableBinaryBeforeFix();
  receptor_um980.nmeaPositionStatus = 0;

  falhas += !receptor_um980.disableOutput();
  falhas += !receptor_um980.setModeRoverSurvey();
  falhas += !receptor_um980.enableConstellation("GPS");
  falhas += !receptor_um980.enableConstellation("GLO");
  falhas += !receptor_um980.enableConstellation("GAL");
  falhas += !receptor_um980.enableConstellation("BDS");
  falhas += !receptor_um980.enableConstellation("QZSS");
  falhas += !receptor_um980.setElevationAngle(15);
  falhas += !receptor_um980.setMinCNO(10);

  // A referencia usa GPGGA como mensagem leve para detectar fix antes de BESTNAV.
  // O nosso parser NMEA usa essa mesma saida para latitude/longitude/altitude.
  falhas += !receptor_um980.setNMEAMessage("GPGGA", 1);

  if (falhas == 0) {
    Serial.println(F("[GNSS] Configuracao UM980 aplicada: rover, multi-constelacao, GPGGA 1 Hz"));
  } else {
    Serial.print(F("[GNSS] Configuracao UM980 parcialmente aplicada; falhas="));
    Serial.println(falhas);
  }

  return true;
}
#endif

static const uint32_t BAUDS_GNSS_DIAGNOSTICO[] = {
    GNSS_BAUD,
    9600UL,
    38400UL,
    57600UL,
    230400UL,
    460800UL,
    921600UL,
};

static const uint8_t QTD_BAUDS_GNSS_DIAGNOSTICO =
    sizeof(BAUDS_GNSS_DIAGNOSTICO) / sizeof(BAUDS_GNSS_DIAGNOSTICO[0]);

ServicoGnss::ServicoGnss()
    : serial_(nullptr),
      linha_pos_(0),
      recebeu_dados_(false),
      instante_ultimo_fix_ms_(0),
      tem_fix_(false),
      simulacao_(false),
      instante_simulacao_ms_(0),
      bytes_recebidos_(0),
      sentencas_validas_(0),
      sentencas_invalidas_(0),
      sentencas_estouradas_(0),
      sentencas_gga_(0),
      sentencas_rmc_(0),
      sentencas_gst_(0),
      instante_ultimo_byte_ms_(0),
      baud_atual_(GNSS_BAUD),
      indice_baud_(0),
      instante_inicio_baud_ms_(0),
      bytes_inicio_baud_(0),
      sentencas_validas_inicio_baud_(0),
      varredura_baud_ativa_(HABILITAR_VARREDURA_BAUD_GNSS != 0),
      baud_confirmado_(false) {
  memset(&dados_, 0, sizeof(dados_));
  memset(linha_, 0, sizeof(linha_));
  ultimo_tipo_sentenca_[0] = '-';
  ultimo_tipo_sentenca_[1] = '-';
  ultimo_tipo_sentenca_[2] = '-';
  ultimo_tipo_sentenca_[3] = '\0';
  dados_.tipo_fix = protocolo::FIX_SEM_FIX;
}

void ServicoGnss::iniciar(HardwareSerial& serial) {
  serial_ = &serial;
  linha_pos_ = 0;
  indice_baud_ = 0;
  reiniciarUartGnss(GNSS_BAUD);
#if HABILITAR_LOG_SERIAL_DETALHADO
  Serial.print(F("[GNSS] UART iniciada | RX GPIO"));
  Serial.print(GNSS_PINO_RX);
  Serial.print(F(" | TX GPIO"));
  Serial.print(GNSS_PINO_TX);
  Serial.print(F(" | baud "));
  Serial.println(GNSS_BAUD);
#if HABILITAR_VARREDURA_BAUD_GNSS
  Serial.println(F("[GNSS] Varredura de baud ATIVA: se nao houver NMEA valido, testara bauds comuns"));
#endif
#endif

#if GNSS_CONFIGURAR_COM_SPARKFUN
  configurarUm980ComSparkFun(*serial_);
  instante_inicio_baud_ms_ = millis();
  bytes_inicio_baud_ = bytes_recebidos_;
  sentencas_validas_inicio_baud_ = sentencas_validas_;
#endif
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
    ++bytes_recebidos_;
    instante_ultimo_byte_ms_ = millis();
    processarByte(c);
  }

  atualizarVarreduraBaud();
}

void ServicoGnss::reiniciarUartGnss(uint32_t baud) {
  if (serial_ == nullptr) {
    return;
  }
  serial_->setRxBufferSize(GNSS_RX_BUFFER_BYTES);
  serial_->end();
  serial_->begin(baud, SERIAL_8N1, GNSS_PINO_RX, GNSS_PINO_TX);
  baud_atual_ = baud;
  linha_pos_ = 0;
  instante_inicio_baud_ms_ = millis();
  bytes_inicio_baud_ = bytes_recebidos_;
  sentencas_validas_inicio_baud_ = sentencas_validas_;
#if HABILITAR_LOG_SERIAL_DETALHADO
  Serial.print(F("[GNSS] Testando baud "));
  Serial.print(baud_atual_);
  Serial.print(F(" em RX GPIO"));
  Serial.print(GNSS_PINO_RX);
  Serial.print(F(" / TX GPIO"));
  Serial.println(GNSS_PINO_TX);
#endif
}

void ServicoGnss::atualizarVarreduraBaud() {
#if HABILITAR_VARREDURA_BAUD_GNSS
  if (serial_ == nullptr || simulacao_ || baud_confirmado_) {
    return;
  }

  if (sentencas_validas_ > sentencas_validas_inicio_baud_) {
    baud_confirmado_ = true;
    varredura_baud_ativa_ = false;
#if HABILITAR_LOG_SERIAL_DETALHADO
    Serial.print(F("[GNSS] NMEA valido encontrado. Baud confirmado: "));
    Serial.println(baud_atual_);
#endif
    return;
  }

  const uint32_t agora = millis();
  if ((uint32_t)(agora - instante_inicio_baud_ms_) < GNSS_TEMPO_TESTE_BAUD_MS) {
    return;
  }

  const uint32_t bytesNesteBaud = bytes_recebidos_ - bytes_inicio_baud_;
#if HABILITAR_LOG_SERIAL_DETALHADO
  Serial.print(F("[GNSS] Sem NMEA valido em "));
  Serial.print(baud_atual_);
  Serial.print(F(" baud"));
  if (bytesNesteBaud > 0) {
    Serial.print(F(" ("));
    Serial.print(bytesNesteBaud);
    Serial.print(F(" bytes brutos; pode ser baud errado ou protocolo binario)"));
  } else {
    Serial.print(F(" (0 bytes recebidos)"));
  }
  Serial.println();
#endif

  indice_baud_ = (uint8_t)((indice_baud_ + 1) % QTD_BAUDS_GNSS_DIAGNOSTICO);
  reiniciarUartGnss(BAUDS_GNSS_DIAGNOSTICO[indice_baud_]);
#endif
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
    ++sentencas_estouradas_;
#if HABILITAR_LOG_SERIAL_DETALHADO
    Serial.println(F("[GNSS] Sentenca NMEA excedeu o buffer e foi descartada"));
#endif
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
#if HABILITAR_LOG_NMEA_BRUTO
  Serial.print(F("[GNSS:NMEA] "));
  Serial.println(linha);
#endif
  if (!checksumValido(linha, tamanho)) {
    ++sentencas_invalidas_;
#if HABILITAR_LOG_SERIAL_DETALHADO
    Serial.println(F("[GNSS] Sentenca NMEA descartada: checksum/formato invalido"));
#endif
    return;  // sentenca corrompida: descarta silenciosamente
  }
  // Tipo de sentenca esta nos offsets 3..5 (apos '$' + talker de 2 chars).
  if (tamanho < 6) {
    ++sentencas_invalidas_;
    return;
  }
  const char* tipo = linha + 3;
  ++sentencas_validas_;
  ultimo_tipo_sentenca_[0] = tipo[0];
  ultimo_tipo_sentenca_[1] = tipo[1];
  ultimo_tipo_sentenca_[2] = tipo[2];
  ultimo_tipo_sentenca_[3] = '\0';
  if (strncmp(tipo, "GGA", 3) == 0) {
    ++sentencas_gga_;
    parseGGA(linha);
  } else if (strncmp(tipo, "RMC", 3) == 0) {
    ++sentencas_rmc_;
    parseRMC(linha);
  } else if (strncmp(tipo, "GST", 3) == 0) {
    ++sentencas_gst_;
    parseGST(linha);
#if HABILITAR_LOG_SERIAL_DETALHADO
  } else {
    Serial.print(F("[GNSS] Sentenca valida sem uso direto: "));
    Serial.println(ultimo_tipo_sentenca_);
#endif
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
#if HABILITAR_LOG_SERIAL_DETALHADO
  Serial.print(F("[GNSS:GGA] qualidade="));
  Serial.print(qualidade);
  Serial.print(F(" | fix="));
  Serial.print(dados_.tipo_fix);
  Serial.print(F(" | sats="));
  Serial.print(dados_.satelites);
  Serial.print(F(" | lat="));
  Serial.print(dados_.latitude_1e7 / 10000000.0, 7);
  Serial.print(F(" | lon="));
  Serial.print(dados_.longitude_1e7 / 10000000.0, 7);
  Serial.print(F(" | alt_m="));
  Serial.println(dados_.altitude_mm / 1000.0, 3);
#endif
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
#if HABILITAR_LOG_SERIAL_DETALHADO
  Serial.print(F("[GNSS:RMC] status="));
  Serial.println(campos[2][0] == '\0' ? '-' : campos[2][0]);
#endif
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
#if HABILITAR_LOG_SERIAL_DETALHADO
    Serial.print(F("[GNSS:GST] precisao horizontal estimada(mm)="));
    Serial.println(dados_.precisao_horizontal_mm);
#endif
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

uint32_t ServicoGnss::idadeUltimoByteMs() const {
  if (instante_ultimo_byte_ms_ == 0) {
    return UINT32_MAX;
  }
  return (uint32_t)(millis() - instante_ultimo_byte_ms_);
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
  ++sentencas_validas_;
  ++sentencas_gga_;
  ultimo_tipo_sentenca_[0] = 'S';
  ultimo_tipo_sentenca_[1] = 'I';
  ultimo_tipo_sentenca_[2] = 'M';
  ultimo_tipo_sentenca_[3] = '\0';
}
