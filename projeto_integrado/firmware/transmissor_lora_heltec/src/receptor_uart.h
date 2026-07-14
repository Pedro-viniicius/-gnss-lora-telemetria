// receptor_uart.h — Transmissor LoRa
// -----------------------------------------------------------------------------
// Recebe quadros COBS do controlador pelo UART, desenquadra e valida (magic,
// tamanho, versao, tipo, CRC). Mantem contadores de quadros validos/invalidos
// e disponibiliza o ultimo pacote valido em uma pequena fila.
// -----------------------------------------------------------------------------
#ifndef RECEPTOR_UART_H
#define RECEPTOR_UART_H

#include <Arduino.h>

#include "configuracao_hardware.h"
#include "protocolo.h"

class ReceptorUart {
 public:
  ReceptorUart();

  void iniciar(HardwareSerial& serial);

  // Leitura nao-bloqueante; deve ser chamada com frequencia.
  void atualizar();

  // Retira da fila o proximo pacote valido. Retorna true se havia um.
  bool obterPacote(protocolo::Pacote& saida);

  // Envia bytes crus de volta ao controlador (usado para status/ACK).
  void enviarBrutos(const uint8_t* dados, size_t tamanho);

  bool controladorConectado() const;

  uint32_t quadrosValidos() const { return quadros_validos_; }
  uint32_t quadrosInvalidos() const { return quadros_invalidos_; }
  uint32_t bytesRecebidos() const { return bytes_recebidos_; }
  uint32_t delimitadoresRecebidos() const { return delimitadores_recebidos_; }
  size_t bytesNoBufferAtual() const { return rx_pos_; }
  uint16_t ultimaSequencia() const { return ultima_sequencia_; }
  uint8_t  ultimoTipo() const { return ultimo_tipo_; }
  uint32_t idadeUltimoQuadroMs() const;
  uint32_t idadeUltimoByteMs() const;

 private:
  static const uint8_t CAP_FILA = 4;

  HardwareSerial* serial_;
  uint8_t rx_[protocolo::TAMANHO_MAXIMO_QUADRO_COBS];
  size_t rx_pos_;

  uint32_t quadros_validos_;
  uint32_t quadros_invalidos_;
  uint32_t bytes_recebidos_;
  uint32_t delimitadores_recebidos_;
  uint16_t ultima_sequencia_;
  uint8_t  ultimo_tipo_;
  uint32_t instante_ultimo_quadro_ms_;
  uint32_t instante_ultimo_byte_ms_;
  bool recebeu_algo_;

  protocolo::Pacote fila_[CAP_FILA];
  uint8_t fila_ini_;
  uint8_t fila_fim_;
  uint8_t fila_qtd_;

  void processarQuadro(const uint8_t* quadro, size_t tamanho);
  void enfileirar(const protocolo::Pacote& pkt);
};

#endif  // RECEPTOR_UART_H
