// transmissor_lora.h — Transmissor LoRa
// -----------------------------------------------------------------------------
// Encapsula o radio SX1262 da Heltec V3 usando a API de baixo nivel
// (LoRaWan_APP.h), preservando o padrao dos codigos de referencia:
//   Mcu.begin -> Radio.Init -> SetChannel -> SetTxConfig -> Send -> IrqProcess.
//
// Regras de robustez:
//   - transmissao nao-bloqueante via callbacks (OnTxDone/OnTxTimeout);
//   - NUNCA sobrescreve o buffer de uma transmissao ativa (enviar() recusa
//     enquanto ocupado);
//   - watchdog libera o radio se um callback nao vier a tempo (recuperacao).
// Todos os parametros de radio vem de configuracao_radio.h (fonte unica).
// -----------------------------------------------------------------------------
#ifndef TRANSMISSOR_LORA_H
#define TRANSMISSOR_LORA_H

#include <Arduino.h>

#include "protocolo.h"

class TransmissorLora {
 public:
  TransmissorLora();

  void iniciar();

  // Processa IRQs do radio e o watchdog. Chamar com frequencia no loop.
  void atualizar();

  // Envia 'tamanho' bytes crus por LoRa. Retorna false se o radio estiver
  // ocupado (nesse caso o chamador deve contar como pacote descartado).
  bool enviar(const uint8_t* dados, size_t tamanho);

  bool ocupado() const { return ocupado_; }
  uint8_t estado() const { return estado_; }  // ver protocolo::EstadoTransmissor
  uint32_t transmissoes() const { return transmissoes_; }
  uint32_t timeouts() const { return timeouts_; }

 private:
  uint8_t tx_[protocolo::TAMANHO_MAXIMO_PACOTE];
  size_t tx_len_;
  volatile bool ocupado_;
  volatile uint8_t estado_;
  volatile uint32_t transmissoes_;
  volatile uint32_t timeouts_;
  uint32_t instante_envio_ms_;

  // Trampolins estaticos para os callbacks C da Heltec.
  static TransmissorLora* instancia_;
  static void aoTxDone();
  static void aoTxTimeout();
};

#endif  // TRANSMISSOR_LORA_H
