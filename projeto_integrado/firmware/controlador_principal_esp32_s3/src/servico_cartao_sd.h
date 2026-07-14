// servico_cartao_sd.h
// -----------------------------------------------------------------------------
// Servico de armazenamento em cartao microSD (SPI) do controlador. Grava
// telemetria GNSS e eventos de teclado em CSV. Projetado para ser TOLERANTE A
// FALHA e NAO-BLOQUEANTE:
//   - o SD NUNCA e dependencia obrigatoria: se ausente/cheio/corrompido/removido,
//     o resto do sistema continua operando;
//   - as funcoes registrar*() apenas formatam e enfileiram linhas em um buffer
//     de RAM (nao tocam o cartao);
//   - a gravacao real acontece em atualizar(), em blocos limitados e espacados
//     (INTERVALO_FLUSH_SD_MS), com verificacao periodica de saude;
//   - rotaciona arquivos ao atingir TAMANHO_MAXIMO_ARQUIVO_SD_BYTES;
//   - tenta re-montar o cartao periodicamente para se recuperar.
//
// Compile com -D HABILITAR_CARTAO_SD=0 para desabilitar (estado SD_DESABILITADO;
// todas as chamadas viram no-op e o SD nao e usado).
// -----------------------------------------------------------------------------
#ifndef SERVICO_CARTAO_SD_H
#define SERVICO_CARTAO_SD_H

#include <Arduino.h>

#include "configuracao_hardware.h"
#include "protocolo.h"

#if HABILITAR_CARTAO_SD
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#endif

class ServicoCartaoSd {
 public:
  ServicoCartaoSd();

  // Inicializa o barramento SPI e tenta montar o cartao (nao-fatal).
  void iniciar();

  // Executa flush e verificacao de saude quando chega a hora. Nao bloqueia.
  void atualizar(uint32_t agora);

  // Enfileira uma linha de telemetria/evento para gravacao (nao toca o cartao).
  void registrarTelemetria(const protocolo::TelemetriaGnss& t, uint16_t seq);
  void registrarEvento(const protocolo::EventoTeclado& e, uint16_t seq);

  protocolo::EstadoArmazenamento estado() const { return estado_; }
  const char* nomeEstado() const;
  bool habilitado() const { return estado_ != protocolo::SD_DESABILITADO; }
  bool presente() const {
    return estado_ == protocolo::SD_PRONTO || estado_ == protocolo::SD_CHEIO;
  }

  // Flag GNSS de status do SD para embutir na telemetria (ATIVO/FALHA/0).
  uint8_t flagsGnss() const;

  // Preenche a estrutura de status para o pacote STATUS_ARMAZENAMENTO.
  void obterStatus(protocolo::StatusArmazenamento& saida) const;

 private:
  static const uint8_t  CAP_LINHAS = 16;   // linhas no buffer de RAM
  static const uint16_t LINHA_MAX = 160;   // tamanho maximo de uma linha CSV
  static const uint8_t  LINHAS_POR_FLUSH = 8;  // limite de linhas gravadas/ciclo

  // Buffer circular de linhas pre-formatadas.
  char linhas_[CAP_LINHAS][LINHA_MAX];
  uint16_t tam_linha_[CAP_LINHAS];
  uint8_t fila_ini_;
  uint8_t fila_fim_;
  uint8_t fila_qtd_;

  protocolo::EstadoArmazenamento estado_;
  uint32_t instante_flush_ms_;
  uint32_t instante_verificacao_ms_;
  uint32_t arquivos_criados_;
  uint32_t bytes_escritos_total_;
  uint32_t bytes_arquivo_atual_;
  uint32_t registros_descartados_;
  uint32_t kb_livres_;                      // cache atualizado na verificacao

  void enfileirar(const char* linha, size_t n);

#if HABILITAR_CARTAO_SD
  SPIClass spi_;
  File arquivo_;
  bool cartaoDetectado() const;
  bool montar();
  bool abrirNovoArquivo();
  void flush();
  void verificarSaude();
  void atualizarEspacoLivre();
#endif
};

#endif  // SERVICO_CARTAO_SD_H
