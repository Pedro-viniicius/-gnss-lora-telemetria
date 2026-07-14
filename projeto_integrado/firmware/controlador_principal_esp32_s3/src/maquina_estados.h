// maquina_estados.h
// -----------------------------------------------------------------------------
// Maquina de estados do controlador. Deriva o estado operacional a partir da
// saude do GNSS e do enlace UART/LoRa. Nao provoca reboot: falhas recuperaveis
// apenas mudam o estado exibido.
// -----------------------------------------------------------------------------
#ifndef MAQUINA_ESTADOS_H
#define MAQUINA_ESTADOS_H

#include <stdint.h>

enum EstadoSistema : uint8_t {
  EST_INICIALIZANDO = 0,
  EST_OPERACIONAL,
  EST_GNSS_SEM_DADOS,
  EST_GNSS_SEM_FIX,
  EST_UART_LORA_DESCONECTADA
};

class MaquinaEstados {
 public:
  MaquinaEstados() : estado_(EST_INICIALIZANDO), boot_ms_(0) {}

  void iniciar(uint32_t agoraMs) { boot_ms_ = agoraMs; estado_ = EST_INICIALIZANDO; }

  // Recalcula o estado. 'timeoutInicialMs' e o tempo maximo em INICIALIZANDO
  // aguardando o primeiro dado de GNSS.
  void atualizar(uint32_t agoraMs, bool gnssRecebeu, bool gnssTemFix,
                 bool uartConectado, uint32_t timeoutInicialMs) {
    // Prioridade: enlace UART caido e a falha mais critica para o mestre.
    if (!uartConectado) {
      estado_ = EST_UART_LORA_DESCONECTADA;
      return;
    }
    if (!gnssRecebeu) {
      // Ainda dentro da janela de inicializacao?
      if ((uint32_t)(agoraMs - boot_ms_) < timeoutInicialMs) {
        estado_ = EST_INICIALIZANDO;
      } else {
        estado_ = EST_GNSS_SEM_DADOS;
      }
      return;
    }
    if (!gnssTemFix) {
      estado_ = EST_GNSS_SEM_FIX;
      return;
    }
    estado_ = EST_OPERACIONAL;
  }

  EstadoSistema estado() const { return estado_; }

  const char* nome() const {
    switch (estado_) {
      case EST_INICIALIZANDO:            return "INICIALIZANDO";
      case EST_OPERACIONAL:              return "OPERACIONAL";
      case EST_GNSS_SEM_DADOS:           return "GNSS_SEM_DADOS";
      case EST_GNSS_SEM_FIX:             return "GNSS_SEM_FIX";
      case EST_UART_LORA_DESCONECTADA:   return "UART_LORA_DESCONECTADA";
      default:                           return "DESCONHECIDO";
    }
  }

 private:
  EstadoSistema estado_;
  uint32_t boot_ms_;
};

#endif  // MAQUINA_ESTADOS_H
