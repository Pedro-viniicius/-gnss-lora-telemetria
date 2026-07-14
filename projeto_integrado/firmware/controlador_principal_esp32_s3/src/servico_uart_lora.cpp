// servico_uart_lora.cpp
#include "servico_uart_lora.h"

#include <string.h>

using namespace protocolo;

static const char* nomeTipoMensagem(uint8_t tipo) {
  switch (tipo) {
    case MSG_TELEMETRIA_GNSS:       return "TELEMETRIA_GNSS";
    case MSG_COMANDO:               return "COMANDO";
    case MSG_STATUS_TRANSMISSOR:    return "STATUS_TRANSMISSOR";
    case MSG_CONFIRMACAO_UART:      return "CONFIRMACAO_UART";
    case MSG_ERRO:                  return "ERRO";
    case MSG_HEARTBEAT:             return "HEARTBEAT";
    default:                        return "DESCONHECIDO";
  }
}

ServicoUartLora::ServicoUartLora()
    : serial_(nullptr),
      rx_pos_(0),
      instante_ultimo_quadro_ms_(0),
      recebeu_algo_(false),
      tem_status_(false),
      quadros_validos_(0),
      quadros_invalidos_(0),
      pacotes_enviados_(0),
      bytes_recebidos_(0),
      delimitadores_recebidos_(0),
      instante_ultimo_byte_ms_(0) {
  memset(&ultimo_status_, 0, sizeof(ultimo_status_));
}

void ServicoUartLora::iniciar(HardwareSerial& serial) {
  serial_ = &serial;
  serial_->begin(LORA_BAUD, SERIAL_8N1, LORA_PINO_RX, LORA_PINO_TX);
  rx_pos_ = 0;
#if HABILITAR_LOG_UART_LORA
  Serial.print(F("[UART-LORA] UART iniciada | RX GPIO"));
  Serial.print(LORA_PINO_RX);
  Serial.print(F(" | TX GPIO"));
  Serial.print(LORA_PINO_TX);
  Serial.print(F(" | baud "));
  Serial.println(LORA_BAUD);
#endif
}

bool ServicoUartLora::enviarPacote(const Pacote& pkt) {
  if (serial_ == nullptr) {
    return false;
  }
  uint8_t quadro[TAMANHO_MAXIMO_QUADRO_COBS];
  size_t n = enquadrarParaUart(pkt, quadro, sizeof(quadro));
  if (n == 0) {
#if HABILITAR_LOG_UART_LORA
    Serial.print(F("[UART-LORA] Falha ao enquadrar pacote tipo "));
    Serial.print(nomeTipoMensagem(pkt.tipo));
    Serial.print(F(" seq="));
    Serial.println(pkt.sequencia);
#endif
    return false;
  }
  serial_->write(quadro, n);
  ++pacotes_enviados_;
#if HABILITAR_LOG_UART_LORA
  Serial.print(F("[UART-LORA] TX tipo="));
  Serial.print(nomeTipoMensagem(pkt.tipo));
  Serial.print(F(" seq="));
  Serial.print(pkt.sequencia);
  Serial.print(F(" payload="));
  Serial.print(pkt.tamanho_payload);
  Serial.print(F(" quadro="));
  Serial.print(n);
  Serial.println(F(" bytes"));
#endif
  return true;
}

void ServicoUartLora::atualizar() {
  if (serial_ == nullptr) {
    return;
  }
  while (serial_->available() > 0) {
    uint8_t b = (uint8_t)serial_->read();
    ++bytes_recebidos_;
    instante_ultimo_byte_ms_ = millis();
    if (b == 0x00) {
      ++delimitadores_recebidos_;
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
#if HABILITAR_LOG_UART_LORA
        Serial.println(F("[UART-LORA] RX descartado: quadro maior que o buffer"));
#endif
      }
    }
  }
}

void ServicoUartLora::processarQuadro(const uint8_t* quadro, size_t tamanho) {
  Pacote pkt;
  ResultadoDecodificacao r = desenquadrarDeUart(quadro, tamanho, pkt, true);
  if (r != DECODE_OK) {
    ++quadros_invalidos_;
#if HABILITAR_LOG_UART_LORA
    Serial.print(F("[UART-LORA] RX invalido | resultado="));
    Serial.print((int)r);
    Serial.print(F(" | tamanho COBS="));
    Serial.println(tamanho);
#endif
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
#if HABILITAR_LOG_UART_LORA
  Serial.print(F("[UART-LORA] RX valido tipo="));
  Serial.print(nomeTipoMensagem(pkt.tipo));
  Serial.print(F(" seq="));
  Serial.print(pkt.sequencia);
  Serial.print(F(" payload="));
  Serial.println(pkt.tamanho_payload);
#endif
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

uint32_t ServicoUartLora::idadeUltimoByteMs() const {
  if (bytes_recebidos_ == 0) {
    return UINT32_MAX;
  }
  return (uint32_t)(millis() - instante_ultimo_byte_ms_);
}
