// servico_uart_lora.cpp
#include "servico_uart_lora.h"

#include <string.h>

using namespace protocolo;

ServicoUartLora::ServicoUartLora()
    : serial_(nullptr),
      rx_pos_(0),
      instante_ultimo_quadro_ms_(0),
      recebeu_algo_(false),
      tem_status_(false),
      quadros_validos_(0),
      quadros_invalidos_(0),
      pacotes_enviados_(0) {
  memset(&ultimo_status_, 0, sizeof(ultimo_status_));
}

void ServicoUartLora::iniciar(HardwareSerial& serial) {
  serial_ = &serial;
  serial_->begin(LORA_BAUD, SERIAL_8N1, LORA_PINO_RX, LORA_PINO_TX);
  rx_pos_ = 0;
}

bool ServicoUartLora::enviarPacote(const Pacote& pkt) {
  if (serial_ == nullptr) {
    return false;
  }
  uint8_t quadro[TAMANHO_MAXIMO_QUADRO_COBS];
  size_t n = enquadrarParaUart(pkt, quadro, sizeof(quadro));
  if (n == 0) {
    return false;
  }
  serial_->write(quadro, n);
  ++pacotes_enviados_;
  return true;
}

void ServicoUartLora::atualizar() {
  if (serial_ == nullptr) {
    return;
  }
  while (serial_->available() > 0) {
    uint8_t b = (uint8_t)serial_->read();
    if (b == 0x00) {
      // Fim de quadro.
      if (rx_pos_ > 0) {
        processarQuadro(rx_, rx_pos_);
      }
      rx_pos_ = 0;
    } else {
      if (rx_pos_ < sizeof(rx_)) {
        rx_[rx_pos_++] = b;
      } else {
        // Quadro grande demais: descarta (corrompido) e reinicia.
        rx_pos_ = 0;
        ++quadros_invalidos_;
      }
    }
  }
}

void ServicoUartLora::processarQuadro(const uint8_t* quadro, size_t tamanho) {
  Pacote pkt;
  ResultadoDecodificacao r = desenquadrarDeUart(quadro, tamanho, pkt, true);
  if (r != DECODE_OK) {
    ++quadros_invalidos_;
    return;
  }
  ++quadros_validos_;
  recebeu_algo_ = true;
  instante_ultimo_quadro_ms_ = millis();

  if (pkt.tipo == MSG_STATUS_TRANSMISSOR) {
    if (lerStatusTransmissor(pkt, ultimo_status_)) {
      tem_status_ = true;
    }
  }
  // MSG_CONFIRMACAO_UART e demais tipos apenas confirmam atividade do enlace.
}

bool ServicoUartLora::conectado() const {
  if (!recebeu_algo_) {
    return false;
  }
  return (uint32_t)(millis() - instante_ultimo_quadro_ms_) <= LORA_TIMEOUT_MS;
}

bool ServicoUartLora::obterUltimoStatus(StatusTransmissor& saida) const {
  if (!tem_status_) {
    return false;
  }
  saida = ultimo_status_;
  return true;
}
