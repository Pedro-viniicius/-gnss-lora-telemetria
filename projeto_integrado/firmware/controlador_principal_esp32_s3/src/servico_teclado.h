// servico_teclado.h
// -----------------------------------------------------------------------------
// Servico do teclado matricial 4x4. Responsabilidades:
//   - varredura nao-bloqueante da matriz (linhas = saidas, colunas = entradas
//     com pull-up);
//   - debounce por tempo e prevencao de repeticao acidental (exige soltar a
//     tecla antes de registrar a proxima);
//   - buffer numerico com limite fixo (sem overflow);
//   - geracao de eventos de teclado em uma pequena fila.
//
// Mapeamento de teclas (padrao):
//   0-9 : acumula digitos no buffer numerico
//   *   : cancela/limpa a entrada  (EVT_CANCELAMENTO)
//   #   : confirma o numero        (EVT_CONFIRMACAO_NUMERO)
//   A   : muda o modo de operacao  (EVT_MUDANCA_MODO)
//   B   : apaga o ultimo digito
//   C   : limpa a entrada
//   D   : forca telemetria imediata (EVT_TELEMETRIA_FORCADA)
// -----------------------------------------------------------------------------
#ifndef SERVICO_TECLADO_H
#define SERVICO_TECLADO_H

#include <Arduino.h>

#include "configuracao_hardware.h"
#include "tipos_comandos.h"

class ServicoTeclado {
 public:
  static const uint8_t QTD_MODOS = 3;

  ServicoTeclado();

  // Configura os pinos das linhas/colunas.
  void iniciar();

  // Varredura nao-bloqueante; deve ser chamada com frequencia no loop.
  void atualizar();

  // Fila de eventos.
  bool temEvento() const;
  bool obterEvento(protocolo::EventoTeclado& saida);

  // Ultimo comando numerico confirmado (ASCII do ultimo digito, ou 0). Usado na
  // telemetria periodica.
  uint8_t ultimoComandoConfirmado() const { return ultimo_comando_; }

  // Modo de operacao atual (0..QTD_MODOS-1).
  uint8_t modoAtual() const { return modo_atual_; }

  // Injeta uma tecla como se tivesse sido pressionada (modo diagnostico/teste).
  void injetarTecla(char tecla);

 private:
  static const uint8_t CAP_FILA = 8;

  const uint8_t pinos_linha_[TECLADO_LINHAS];
  const uint8_t pinos_coluna_[TECLADO_COLUNAS];
  const char mapa_[TECLADO_LINHAS][TECLADO_COLUNAS];

  char buffer_[TECLADO_MAX_DIGITOS + 1];
  uint8_t buffer_len_;
  uint8_t modo_atual_;
  uint8_t ultimo_comando_;

  // Debounce.
  char tecla_bruta_;         // ultima tecla lida (pode estar instavel)
  char tecla_estavel_;       // ultima tecla ja debounced (0 = nenhuma)
  uint32_t instante_mudanca_ms_;

  // Fila circular de eventos.
  protocolo::EventoTeclado fila_[CAP_FILA];
  uint8_t fila_ini_;
  uint8_t fila_fim_;
  uint8_t fila_qtd_;

  char varrer();                       // retorna a tecla pressionada ou 0
  void tratarTecla(char tecla);        // aplica a semantica da tecla
  void enfileirar(char tecla, uint8_t tipoEvento, uint32_t valor,
                  uint8_t tamEntrada);
};

#endif  // SERVICO_TECLADO_H
