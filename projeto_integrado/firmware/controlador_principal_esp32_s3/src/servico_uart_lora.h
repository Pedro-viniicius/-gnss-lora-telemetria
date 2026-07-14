// servico_uart_lora.h
// -----------------------------------------------------------------------------
// Enlace UART full-duplex entre o controlador (mestre) e a Heltec TX.
//   - envio: pacotes validados sao enquadrados com COBS + delimitador 0x00;
//   - recepcao: quadros COBS sao desenquadrados e validados; STATUS_TRANSMISSOR
//     e CONFIRMACAO_UART sao consumidos aqui;
//   - saude do enlace: considera "conectado" enquanto chegam quadros validos
//     dentro de LORA_TIMEOUT_MS.
// Buffers fixos; nunca escreve alem da capacidade.
// -----------------------------------------------------------------------------
#ifndef SERVICO_UART_LORA_H
#define SERVICO_UART_LORA_H

#include <Arduino.h>

#include "configuracao_hardware.h"
#include "protocolo.h"

class ServicoUartLora {
 public:
  ServicoUartLora();

  void iniciar(HardwareSerial& serial);

  // Envia um pacote ja montado. Retorna true se o quadro foi escrito.
  bool enviarPacote(const protocolo::Pacote& pkt);

  // Leitura nao-bloqueante dos quadros vindos da Heltec.
  void atualizar();

  // Enlace considerado ativo (recebeu quadro valido dentro do timeout).
  bool conectado() const;

  // Ultimo status recebido do transmissor.
  bool temStatus() const { return tem_status_; }
  bool obterUltimoStatus(protocolo::StatusTransmissor& saida) const;

  // Contadores de saude do enlace.
  uint32_t quadrosValidos() const { return quadros_validos_; }
  uint32_t quadrosInvalidos() const { return quadros_invalidos_; }
  uint32_t pacotesEnviados() const { return pacotes_enviados_; }
  uint32_t bytesRecebidos() const { return bytes_recebidos_; }
  uint32_t delimitadoresRecebidos() const { return delimitadores_recebidos_; }
  size_t bytesNoBufferAtual() const { return rx_pos_; }
  uint32_t idadeUltimoByteMs() const;

 private:
  HardwareSerial* serial_;

  // Buffer de montagem do quadro (conteudo COBS, sem o delimitador).
  uint8_t rx_[protocolo::TAMANHO_MAXIMO_QUADRO_COBS];
  size_t rx_pos_;

  uint32_t instante_ultimo_quadro_ms_;
  bool recebeu_algo_;

  bool tem_status_;
  protocolo::StatusTransmissor ultimo_status_;

  uint32_t quadros_validos_;
  uint32_t quadros_invalidos_;
  uint32_t pacotes_enviados_;
  uint32_t bytes_recebidos_;
  uint32_t delimitadores_recebidos_;
  uint32_t instante_ultimo_byte_ms_;

  void processarQuadro(const uint8_t* quadro, size_t tamanho);
};

#endif  // SERVICO_UART_LORA_H
