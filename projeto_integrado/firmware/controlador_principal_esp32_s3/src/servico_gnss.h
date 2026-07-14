// servico_gnss.h
// -----------------------------------------------------------------------------
// Servico de leitura do GNSS UM980 por HardwareSerial dedicada.
//
// Estrategia: o UM980 emite sentencas NMEA (GGA/RMC/GST) por padrao. Este
// servico faz um parser NMEA proprio, nao-bloqueante e com buffers fixos:
//   - le os bytes disponiveis a cada atualizar() sem esperar (sem delay);
//   - valida o checksum de cada sentenca (sentencas corrompidas sao descartadas);
//   - converte coordenadas para inteiros escalados (graus x 1e7, altitude em mm);
//   - marca a idade dos dados e NUNCA reaproveita coordenada antiga como atual.
//
// Modo de simulacao (diagnostico): quando habilitado, gera sentencas sinteticas
// internamente e marca os dados com GNSS_FLAG_DADOS_SIMULADOS.
//
// Ponto de extensao RTCM: metodo injetarRtcm() reservado para futura injecao de
// correcoes (nao implementa NTRIP/caster; sem credenciais).
// -----------------------------------------------------------------------------
#ifndef SERVICO_GNSS_H
#define SERVICO_GNSS_H

#include <Arduino.h>

#include "protocolo.h"

class ServicoGnss {
 public:
  ServicoGnss();

  // Inicializa a UART do GNSS. 'serial' deve ser uma HardwareSerial dedicada.
  void iniciar(HardwareSerial& serial);

  // Deve ser chamado com frequencia no loop (nao bloqueia).
  void atualizar();

  // true se ja chegou algum dado valido do GNSS desde o boot.
  bool recebeuDados() const { return recebeu_dados_; }

  // true se ha um fix valido (tipo_fix > SEM_FIX) e os dados nao estao antigos.
  bool temFixValido() const;

  // true se os dados atuais passaram do limite de idade (stale).
  bool dadosAntigos() const;

  // Preenche 'saida' com o snapshot mais recente (ja com flags de idade/simulado
  // atualizadas). Retorna true se ha ao menos dados recebidos.
  bool obterSnapshot(protocolo::TelemetriaGnss& saida);

  // Idade, em ms, do ultimo dado de posicao valido (UINT32_MAX se nunca houve).
  uint32_t idadeDadosMs() const;

  // Habilita/desabilita a simulacao de dados GNSS (modo diagnostico).
  void habilitarSimulacao(bool habilitar);
  bool simulacaoHabilitada() const { return simulacao_; }

  // Injeta uma sentenca NMEA completa (util para testes/diagnostico).
  void injetarSentencaNmea(const char* sentenca);

  // Ponto de extensao para futura injecao de correcoes RTCM (bytes crus) no
  // UM980. Atualmente apenas repassa ao UART do GNSS, se configurado.
  void injetarRtcm(const uint8_t* dados, size_t tamanho);

 private:
  static const size_t TAM_LINHA = 128;   // maximo de uma sentenca NMEA

  HardwareSerial* serial_;
  char linha_[TAM_LINHA];
  size_t linha_pos_;

  protocolo::TelemetriaGnss dados_;      // ultimo snapshot consolidado
  bool recebeu_dados_;
  uint32_t instante_ultimo_fix_ms_;      // millis() do ultimo fix valido
  bool tem_fix_;                         // ultimo GGA indicou fix?
  bool simulacao_;
  uint32_t instante_simulacao_ms_;

  void processarByte(char c);
  void processarLinha(char* linha, size_t tamanho);
  bool checksumValido(const char* linha, size_t tamanho) const;
  void parseGGA(char* linha);
  void parseRMC(char* linha);
  void parseGST(char* linha);
  void gerarSimulacao();
};

#endif  // SERVICO_GNSS_H
