// receptor_lora.cpp — Receptor LoRa
#include "receptor_lora.h"

#include <string.h>

#include "LoRaWan_APP.h"
#include "configuracao_radio.h"

using namespace protocolo;

static RadioEvents_t s_radioEvents;

ReceptorLora* ReceptorLora::instancia_ = nullptr;

ReceptorLora::ReceptorLora()
    : tam_(0),
      rssi_(0),
      snr_(0),
      tem_pacote_(false),
      precisa_rearmar_(true) {}

void ReceptorLora::iniciar() {
  instancia_ = this;

  // Nao chamamos Mcu.begin() aqui: nas versoes atuais da biblioteca Heltec essa
  // chamada dispara uma validacao de licenca e reinicia a placa quando a licenca
  // nao esta provisionada. Para LoRa P2P usamos diretamente a API Radio.*.

  s_radioEvents.RxDone = ReceptorLora::aoRxDone;
  Radio.Init(&s_radioEvents);
  Radio.SetChannel(RADIO_FREQUENCIA_HZ);
  Radio.SetRxConfig(MODEM_LORA, RADIO_LARGURA_BANDA, RADIO_SPREADING_FACTOR,
                    RADIO_CODING_RATE, 0, RADIO_PREAMBULO, RADIO_SYMBOL_TIMEOUT,
                    RADIO_PAYLOAD_FIXO, 0, true /*crcOn*/, 0, 0,
                    RADIO_IQ_INVERTIDO, true /*rxContinuous*/);
  precisa_rearmar_ = true;
}

void ReceptorLora::atualizar() {
  if (precisa_rearmar_) {
    precisa_rearmar_ = false;
    Radio.Rx(0);  // recepcao continua
  }
  Radio.IrqProcess();
}

void ReceptorLora::aoRxDone(uint8_t* payload, uint16_t size, int16_t rssi,
                            int8_t snr) {
  if (instancia_ == nullptr) {
    return;
  }
  // CORRECAO DE SEGURANCA: nunca copiar mais que a capacidade do buffer.
  uint16_t copiar = size;
  if (copiar > sizeof(instancia_->buf_)) {
    copiar = sizeof(instancia_->buf_);
  }
  memcpy(instancia_->buf_, payload, copiar);
  instancia_->tam_ = copiar;
  instancia_->rssi_ = rssi;
  instancia_->snr_ = snr;
  instancia_->tem_pacote_ = true;

  Radio.Sleep();
  instancia_->precisa_rearmar_ = true;  // volta ao Rx no proximo atualizar()
}

bool ReceptorLora::obterPacoteCru(uint8_t* destino, size_t capacidade,
                                  size_t& tamanho, int16_t& rssi, int8_t& snr) {
  if (!tem_pacote_) {
    return false;
  }
  uint16_t n = tam_;
  if (n > capacidade) {
    n = (uint16_t)capacidade;
  }
  memcpy(destino, buf_, n);
  tamanho = n;
  rssi = rssi_;
  snr = snr_;
  tem_pacote_ = false;
  return true;
}
