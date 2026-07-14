// servico_cartao_sd.cpp
#include "servico_cartao_sd.h"

#include <stdio.h>
#include <string.h>

#include "formato_registro.h"

using namespace protocolo;

ServicoCartaoSd::ServicoCartaoSd()
    : fila_ini_(0),
      fila_fim_(0),
      fila_qtd_(0),
      estado_(SD_DESABILITADO),
      instante_flush_ms_(0),
      instante_verificacao_ms_(0),
      arquivos_criados_(0),
      bytes_escritos_total_(0),
      bytes_arquivo_atual_(0),
      registros_descartados_(0),
      kb_livres_(0)
#if HABILITAR_CARTAO_SD
      ,
      spi_(SD_SPI_HOST)
#endif
{
  memset(tam_linha_, 0, sizeof(tam_linha_));
}

const char* ServicoCartaoSd::nomeEstado() const {
  switch (estado_) {
    case SD_DESABILITADO: return "DESABILITADO";
    case SD_PRONTO:       return "PRONTO";
    case SD_SEM_CARTAO:   return "SEM_CARTAO";
    case SD_CHEIO:        return "CHEIO";
    case SD_FALHA:        return "FALHA";
    default:              return "?";
  }
}

uint8_t ServicoCartaoSd::flagsGnss() const {
  if (estado_ == SD_PRONTO) {
    return GNSS_FLAG_SD_ATIVO;
  }
  if (estado_ == SD_DESABILITADO) {
    return 0;  // sem SD compilado => nao sinaliza nada
  }
  return GNSS_FLAG_SD_FALHA;  // SEM_CARTAO / CHEIO / FALHA
}

void ServicoCartaoSd::obterStatus(StatusArmazenamento& saida) const {
  saida.uptime_ms = millis();
  saida.estado = (uint8_t)estado_;
  saida.presente = presente() ? 1 : 0;
  saida.kb_livres = kb_livres_;
  saida.arquivos_escritos = arquivos_criados_;
  saida.bytes_escritos = bytes_escritos_total_;
  saida.registros_descartados = registros_descartados_;
}

void ServicoCartaoSd::enfileirar(const char* linha, size_t n) {
  if (n == 0) {
    return;
  }
  if (n >= LINHA_MAX) {
    n = LINHA_MAX - 1;
  }
  if (fila_qtd_ >= CAP_LINHAS) {
    // Buffer cheio: descarta a linha mais antiga.
    fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_LINHAS);
    --fila_qtd_;
    ++registros_descartados_;
  }
  memcpy(linhas_[fila_fim_], linha, n);
  tam_linha_[fila_fim_] = (uint16_t)n;
  fila_fim_ = (uint8_t)((fila_fim_ + 1) % CAP_LINHAS);
  ++fila_qtd_;
}

void ServicoCartaoSd::registrarTelemetria(const TelemetriaGnss& t, uint16_t seq) {
  if (estado_ == SD_DESABILITADO) {
    return;
  }
  char linha[LINHA_MAX];
  size_t n = formatarLinhaTelemetriaCsv(linha, sizeof(linha), t, seq);
  enfileirar(linha, n);
}

void ServicoCartaoSd::registrarEvento(const EventoTeclado& e, uint16_t seq) {
  if (estado_ == SD_DESABILITADO) {
    return;
  }
  char linha[LINHA_MAX];
  size_t n = formatarLinhaEventoCsv(linha, sizeof(linha), e, seq);
  enfileirar(linha, n);
}

#if HABILITAR_CARTAO_SD

void ServicoCartaoSd::iniciar() {
  if (SD_POSSUI_DETECCAO) {
    pinMode(PIN_SD_DETECT, INPUT_PULLUP);
  }
  spi_.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  Serial.println(F("[SD] Inicializando cartao microSD..."));
  if (!montar()) {
    Serial.println(F("[SD] Cartao ausente ou falha na montagem "
                     "(o sistema continua sem gravacao)."));
  }
  instante_flush_ms_ = millis();
  instante_verificacao_ms_ = millis();
}

bool ServicoCartaoSd::cartaoDetectado() const {
  // Convencao: contato card-detect vai a GND quando ha cartao inserido.
  return digitalRead(PIN_SD_DETECT) == LOW;
}

void ServicoCartaoSd::atualizarEspacoLivre() {
  uint64_t total = SD.totalBytes();
  uint64_t usado = SD.usedBytes();
  uint64_t livre = (total > usado) ? (total - usado) : 0;
  kb_livres_ = (uint32_t)(livre / 1024ULL);
}

bool ServicoCartaoSd::abrirNovoArquivo() {
  char nome[24];
  uint32_t idx = 1;
  for (; idx <= 9999; ++idx) {
    snprintf(nome, sizeof(nome), "/LOG%04lu.CSV", (unsigned long)idx);
    if (!SD.exists(nome)) {
      break;
    }
  }
  if (idx > 9999) {
    Serial.println(F("[SD] Limite de arquivos atingido; tratando como CHEIO."));
    estado_ = SD_CHEIO;
    return false;
  }

  arquivo_ = SD.open(nome, FILE_WRITE);
  if (!arquivo_) {
    Serial.println(F("[SD] Falha ao abrir arquivo de log; marcando FALHA."));
    estado_ = SD_FALHA;
    return false;
  }

  // Arquivo novo: grava o cabecalho CSV.
  char cab[128];
  size_t nc = cabecalhoCsv(cab, sizeof(cab));
  if (nc > 0) {
    arquivo_.write((const uint8_t*)cab, nc);
    arquivo_.flush();
    bytes_escritos_total_ += nc;
  }
  bytes_arquivo_atual_ = (uint32_t)arquivo_.size();
  ++arquivos_criados_;

  Serial.print(F("[SD] Gravando em "));
  Serial.println(nome);
  return true;
}

bool ServicoCartaoSd::montar() {
  if (SD_POSSUI_DETECCAO && !cartaoDetectado()) {
    estado_ = SD_SEM_CARTAO;
    return false;
  }
  if (!SD.begin(PIN_SD_CS, spi_, FREQUENCIA_SPI_SD_HZ, PONTO_MONTAGEM_SD)) {
    estado_ = SD_SEM_CARTAO;
    return false;
  }
  if (SD.cardType() == CARD_NONE) {
    SD.end();
    estado_ = SD_SEM_CARTAO;
    return false;
  }
  atualizarEspacoLivre();
  if (!abrirNovoArquivo()) {
    return false;  // estado ja definido em abrirNovoArquivo
  }
  estado_ = SD_PRONTO;
  Serial.print(F("[SD] Cartao montado. Livre(KB): "));
  Serial.println(kb_livres_);
  return true;
}

void ServicoCartaoSd::flush() {
  if (estado_ != SD_PRONTO || fila_qtd_ == 0) {
    return;
  }
  uint8_t escritas = 0;
  while (fila_qtd_ > 0 && escritas < LINHAS_POR_FLUSH) {
    size_t n = tam_linha_[fila_ini_];
    size_t w = arquivo_.write((const uint8_t*)linhas_[fila_ini_], n);
    if (w != n) {
      Serial.println(F("[SD] Erro de gravacao; marcando FALHA."));
      arquivo_.close();
      estado_ = SD_FALHA;
      return;
    }
    bytes_escritos_total_ += n;
    bytes_arquivo_atual_ += n;
    fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_LINHAS);
    --fila_qtd_;
    ++escritas;
  }
  arquivo_.flush();

  if (bytes_arquivo_atual_ >= TAMANHO_MAXIMO_ARQUIVO_SD_BYTES) {
    Serial.println(F("[SD] Tamanho maximo atingido; rotacionando arquivo."));
    arquivo_.close();
    abrirNovoArquivo();  // estado tratado internamente
  }
}

void ServicoCartaoSd::verificarSaude() {
  // Card-detect (opcional): remocao => SEM_CARTAO.
  if (SD_POSSUI_DETECCAO && !cartaoDetectado()) {
    if (estado_ == SD_PRONTO || estado_ == SD_CHEIO) {
      arquivo_.close();
      SD.end();
    }
    estado_ = SD_SEM_CARTAO;
    return;
  }

  if (estado_ == SD_PRONTO || estado_ == SD_CHEIO) {
    atualizarEspacoLivre();
    uint64_t livre = (uint64_t)kb_livres_ * 1024ULL;
    if (livre < ESPACO_LIVRE_MINIMO_SD_BYTES) {
      if (estado_ != SD_CHEIO) {
        Serial.println(F("[SD] Espaco livre abaixo do minimo; cartao CHEIO."));
      }
      estado_ = SD_CHEIO;
    } else if (estado_ == SD_CHEIO) {
      Serial.println(F("[SD] Espaco liberado; voltando a gravar."));
      estado_ = SD_PRONTO;
    }
    return;
  }

  // SEM_CARTAO ou FALHA: tenta recuperar re-montando.
  if (estado_ == SD_SEM_CARTAO || estado_ == SD_FALHA) {
    SD.end();
    if (montar()) {
      Serial.println(F("[SD] Cartao recuperado."));
    }
  }
}

void ServicoCartaoSd::atualizar(uint32_t agora) {
  if (estado_ == SD_DESABILITADO) {
    return;
  }
  if ((uint32_t)(agora - instante_flush_ms_) >= INTERVALO_FLUSH_SD_MS) {
    instante_flush_ms_ = agora;
    flush();
  }
  if ((uint32_t)(agora - instante_verificacao_ms_) >= INTERVALO_VERIFICACAO_SD_MS) {
    instante_verificacao_ms_ = agora;
    verificarSaude();
  }
}

#else  // HABILITAR_CARTAO_SD == 0

void ServicoCartaoSd::iniciar() {
  estado_ = SD_DESABILITADO;
  Serial.println(F("[SD] Suporte a cartao microSD desabilitado em compilacao."));
}

void ServicoCartaoSd::atualizar(uint32_t agora) {
  (void)agora;  // nada a fazer
}

#endif  // HABILITAR_CARTAO_SD
