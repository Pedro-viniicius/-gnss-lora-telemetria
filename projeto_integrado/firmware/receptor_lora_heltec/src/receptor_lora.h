// receptor_lora.h — Receptor LoRa
// -----------------------------------------------------------------------------
// Encapsula o radio SX1262 da Heltec V3 em modo de recepcao continua, com a API
// de baixo nivel (LoRaWan_APP.h). Preserva o padrao da referencia:
//   Radio.Init -> SetChannel -> SetRxConfig -> Rx(0) -> IrqProcess.
//
// Correcao de seguranca: no callback OnRxDone, o tamanho recebido e limitado a
// capacidade do buffer ANTES de copiar (a referencia fazia rxpacket[size]='\0'
// sem checar, arriscando estouro). Usamos buffer binario (sem terminador).
// Volta corretamente ao modo Rx apos cada callback.
// -----------------------------------------------------------------------------
#ifndef RECEPTOR_LORA_H
#define RECEPTOR_LORA_H

#include <Arduino.h>

#include "protocolo.h"

class ReceptorLora {
 public:
  ReceptorLora();

  void iniciar();

  // Processa IRQs e (re)arma o modo Rx quando necessario. Nao bloqueia.
  void atualizar();

  // Copia o ultimo pacote cru recebido, se houver um novo. Limita a copia a
  // 'capacidade'. Retorna true e limpa o flag de "novo" quando entrega um pacote.
  bool obterPacoteCru(uint8_t* destino, size_t capacidade, size_t& tamanho,
                      int16_t& rssi, int8_t& snr);

 private:
  static ReceptorLora* instancia_;

  uint8_t buf_[protocolo::TAMANHO_MAXIMO_PACOTE];
  volatile uint16_t tam_;
  volatile int16_t rssi_;
  volatile int8_t snr_;
  volatile bool tem_pacote_;
  volatile bool precisa_rearmar_;

  static void aoRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
};

#endif  // RECEPTOR_LORA_H
