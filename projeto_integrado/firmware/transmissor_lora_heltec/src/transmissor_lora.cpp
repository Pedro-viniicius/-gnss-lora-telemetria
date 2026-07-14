// transmissor_lora.cpp — Transmissor LoRa
#include "transmissor_lora.h"

#include <string.h>

#include "LoRaWan_APP.h"
#include "configuracao_hardware.h"
#include "configuracao_radio.h"

using namespace protocolo;

static RadioEvents_t s_radioEvents;

TransmissorLora* TransmissorLora::instancia_ = nullptr;

TransmissorLora::TransmissorLora()
    : tx_len_(0),
      ocupado_(false),
      estado_(TX_OCIOSO),
      transmissoes_(0),
      timeouts_(0),
      instante_envio_ms_(0) {}

void TransmissorLora::iniciar() {
  instancia_ = this;

  // Nao chamamos Mcu.begin() aqui: nas versoes atuais da biblioteca Heltec essa
  // chamada dispara uma validacao de licenca e reinicia a placa quando a licenca
  // nao esta provisionada. Para LoRa P2P usamos diretamente a API Radio.*.

  s_radioEvents.TxDone = TransmissorLora::aoTxDone;
  s_radioEvents.TxTimeout = TransmissorLora::aoTxTimeout;
  Radio.Init(&s_radioEvents);
  Radio.SetChannel(RADIO_FREQUENCIA_HZ);
  Radio.SetTxConfig(MODEM_LORA, RADIO_POTENCIA_TX_DBM, 0, RADIO_LARGURA_BANDA,
                    RADIO_SPREADING_FACTOR, RADIO_CODING_RATE, RADIO_PREAMBULO,
                    RADIO_PAYLOAD_FIXO, true /*crcOn*/, 0, 0,
                    RADIO_IQ_INVERTIDO, RADIO_TX_TIMEOUT_MS);

  ocupado_ = false;
  estado_ = TX_OCIOSO;
}

bool TransmissorLora::enviar(const uint8_t* dados, size_t tamanho) {
  if (ocupado_) {
    return false;  // transmissao em andamento: nao sobrescreve o buffer
  }
  if (dados == nullptr || tamanho == 0 || tamanho > sizeof(tx_)) {
    return false;
  }
  memcpy(tx_, dados, tamanho);
  tx_len_ = tamanho;
  ocupado_ = true;
  estado_ = TX_TRANSMITINDO;
  instante_envio_ms_ = millis();
  Radio.Send(tx_, (uint8_t)tx_len_);
  return true;
}

void TransmissorLora::atualizar() {
  Radio.IrqProcess();

  // Watchdog: se ficou ocupado tempo demais sem callback, recupera.
  if (ocupado_ &&
      (uint32_t)(millis() - instante_envio_ms_) > TX_WATCHDOG_MS) {
    Radio.Sleep();
    ocupado_ = false;
    estado_ = TX_TIMEOUT;
    ++timeouts_;
  }
}

void TransmissorLora::aoTxDone() {
  if (instancia_ == nullptr) {
    return;
  }
  instancia_->ocupado_ = false;
  instancia_->estado_ = TX_OCIOSO;
  ++instancia_->transmissoes_;
}

void TransmissorLora::aoTxTimeout() {
  if (instancia_ == nullptr) {
    return;
  }
  Radio.Sleep();
  instancia_->ocupado_ = false;
  instancia_->estado_ = TX_TIMEOUT;
  ++instancia_->timeouts_;
}
