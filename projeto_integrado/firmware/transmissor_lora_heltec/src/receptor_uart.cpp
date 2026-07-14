// receptor_uart.cpp — Transmissor LoRa
#include "receptor_uart.h"

#include <string.h>

using namespace protocolo;

ReceptorUart::ReceptorUart()
    : serial_(nullptr),
      rx_pos_(0),
      quadros_validos_(0),
      quadros_invalidos_(0),
      bytes_recebidos_(0),
      delimitadores_recebidos_(0),
      ultima_sequencia_(0),
      ultimo_tipo_(0),
      instante_ultimo_quadro_ms_(0),
      instante_ultimo_byte_ms_(0),
      recebeu_algo_(false),
      fila_ini_(0),
      fila_fim_(0),
      fila_qtd_(0) {}

void ReceptorUart::iniciar(HardwareSerial& serial) {
  serial_ = &serial;
  serial_->begin(CONTROLADOR_BAUD, SERIAL_8N1, CONTROLADOR_PINO_RX,
                 CONTROLADOR_PINO_TX);
  rx_pos_ = 0;
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
  Serial.print(F("[UART-CONTROLADOR] UART iniciada | RX GPIO"));
  Serial.print(CONTROLADOR_PINO_RX);
  Serial.print(F(" | TX GPIO"));
  Serial.print(CONTROLADOR_PINO_TX);
  Serial.print(F(" | baud "));
  Serial.println(CONTROLADOR_BAUD);
#endif
}

void ReceptorUart::atualizar() {
  if (serial_ == nullptr) {
    return;
  }
  while (serial_->available() > 0) {
    uint8_t b = (uint8_t)serial_->read();
    ++bytes_recebidos_;
    instante_ultimo_byte_ms_ = millis();
    if (b == 0x00) {
      ++delimitadores_recebidos_;
      if (rx_pos_ > 0) {
        processarQuadro(rx_, rx_pos_);
      }
      rx_pos_ = 0;
    } else {
      if (rx_pos_ < sizeof(rx_)) {
        rx_[rx_pos_++] = b;
      } else {
        rx_pos_ = 0;
        ++quadros_invalidos_;
      }
    }
  }
}

void ReceptorUart::processarQuadro(const uint8_t* quadro, size_t tamanho) {
  Pacote pkt;
  ResultadoDecodificacao r = desenquadrarDeUart(quadro, tamanho, pkt, true);
  if (r != DECODE_OK) {
    ++quadros_invalidos_;
    return;
  }
  ++quadros_validos_;
  recebeu_algo_ = true;
  instante_ultimo_quadro_ms_ = millis();
  ultima_sequencia_ = pkt.sequencia;
  ultimo_tipo_ = pkt.tipo;
  enfileirar(pkt);
}

void ReceptorUart::enfileirar(const Pacote& pkt) {
  if (fila_qtd_ >= CAP_FILA) {
    // Fila cheia: descarta o mais antigo.
    fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_FILA);
    --fila_qtd_;
  }
  fila_[fila_fim_] = pkt;
  fila_fim_ = (uint8_t)((fila_fim_ + 1) % CAP_FILA);
  ++fila_qtd_;
}

bool ReceptorUart::obterPacote(Pacote& saida) {
  if (fila_qtd_ == 0) {
    return false;
  }
  saida = fila_[fila_ini_];
  fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_FILA);
  --fila_qtd_;
  return true;
}

void ReceptorUart::enviarBrutos(const uint8_t* dados, size_t tamanho) {
  if (serial_ != nullptr && dados != nullptr && tamanho > 0) {
    serial_->write(dados, tamanho);
  }
}

bool ReceptorUart::controladorConectado() const {
  if (!recebeu_algo_) {
    return false;
  }
  return (uint32_t)(millis() - instante_ultimo_quadro_ms_) <=
         CONTROLADOR_TIMEOUT_MS;
}

uint32_t ReceptorUart::idadeUltimoQuadroMs() const {
  if (!recebeu_algo_) {
    return UINT32_MAX;
  }
  return (uint32_t)(millis() - instante_ultimo_quadro_ms_);
}

uint32_t ReceptorUart::idadeUltimoByteMs() const {
  if (bytes_recebidos_ == 0) {
    return UINT32_MAX;
  }
  return (uint32_t)(millis() - instante_ultimo_byte_ms_);
}
