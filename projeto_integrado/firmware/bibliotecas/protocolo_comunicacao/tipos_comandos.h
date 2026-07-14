// tipos_comandos.h
// -----------------------------------------------------------------------------
// Tipos de mensagem do protocolo e estruturas de evento de teclado / status do
// transmissor. Todas serializadas explicitamente (ver protocolo.h).
// -----------------------------------------------------------------------------
#ifndef PROTOCOLO_TIPOS_COMANDOS_H
#define PROTOCOLO_TIPOS_COMANDOS_H

#include <stdint.h>

namespace protocolo {

// Tipos de mensagem (campo 'tipo' do cabecalho).
enum TipoMensagem : uint8_t {
  MSG_TELEMETRIA_GNSS    = 0x01,  // telemetria periodica de posicao
  MSG_EVENTO_TECLADO     = 0x02,  // evento imediato de tecla confirmada
  MSG_COMANDO            = 0x03,  // comando generico
  MSG_STATUS_TRANSMISSOR = 0x04,  // status da Heltec TX -> controlador (UART)
  MSG_CONFIRMACAO_UART   = 0x05,  // ACK de quadro recebido pela UART
  MSG_ERRO               = 0x06,  // notificacao de erro
  MSG_HEARTBEAT          = 0x07,  // sinal de vida periodico
  MSG_STATUS_ARMAZENAMENTO = 0x08 // status do cartao microSD do controlador
};

// Retorna true se 'tipo' e um TipoMensagem conhecido.
inline bool tipoMensagemConhecido(uint8_t tipo) {
  return tipo >= MSG_TELEMETRIA_GNSS && tipo <= MSG_STATUS_ARMAZENAMENTO;
}

// Sub-tipo de um evento de teclado.
enum TipoEventoTeclado : uint8_t {
  EVT_CONFIRMACAO_NUMERO   = 0,  // usuario confirmou um numero (#)
  EVT_MUDANCA_MODO         = 1,  // usuario mudou o modo (A)
  EVT_TELEMETRIA_FORCADA   = 2,  // usuario forcou telemetria (D)
  EVT_CANCELAMENTO         = 3   // usuario cancelou a entrada (*)
};

// Evento de teclado. Payload serializado = 4 + 1 + 1 + 4 + 1 = 11 bytes.
struct EventoTeclado {
  uint32_t uptime_ms;         // tempo desde o boot do controlador
  uint8_t  tecla;             // ASCII da tecla que gerou o evento
  uint8_t  tipo_evento;       // ver TipoEventoTeclado
  uint32_t valor_numerico;    // valor numerico confirmado (0 se nao aplicavel)
  uint8_t  tamanho_entrada;   // quantidade de digitos digitados
};

static const uint16_t TAMANHO_PAYLOAD_EVENTO_TECLADO = 11;

// Estado reportado pelo transmissor de volta ao controlador.
enum EstadoTransmissor : uint8_t {
  TX_OCIOSO       = 0,  // radio livre
  TX_TRANSMITINDO = 1,  // transmissao em andamento
  TX_TIMEOUT      = 2   // ultima transmissao expirou
};

// Status do transmissor. Payload serializado:
// 4 + 4 + 4 + 4 + 4 + 4 + 2 + 1 = 27 bytes.
struct StatusTransmissor {
  uint32_t uptime_ms;
  uint32_t quadros_uart_validos;
  uint32_t quadros_uart_invalidos;
  uint32_t transmissoes_lora;
  uint32_t timeouts_lora;
  uint32_t pacotes_descartados;
  uint16_t ultima_sequencia;
  uint8_t  estado_tx;          // ver EstadoTransmissor
};

static const uint16_t TAMANHO_PAYLOAD_STATUS_TRANSMISSOR = 27;

// Estado do subsistema de armazenamento (cartao microSD).
enum EstadoArmazenamento : uint8_t {
  SD_DESABILITADO     = 0,  // compilado sem SD (HABILITAR_CARTAO_SD=0)
  SD_PRONTO           = 1,  // cartao montado e gravando
  SD_SEM_CARTAO       = 2,  // nenhum cartao detectado/montado
  SD_CHEIO            = 3,  // espaco livre abaixo do minimo
  SD_FALHA            = 4   // erro de I/O (removido/corrompido)
};

// Status do armazenamento reportado pelo controlador. Payload serializado:
// 4 + 1 + 1 + 4 + 4 + 4 + 4 = 22 bytes.
struct StatusArmazenamento {
  uint32_t uptime_ms;
  uint8_t  estado;               // ver EstadoArmazenamento
  uint8_t  presente;             // 1 = cartao presente/montado, 0 = nao
  uint32_t kb_livres;            // espaco livre em kilobytes
  uint32_t arquivos_escritos;    // numero de arquivos de log criados
  uint32_t bytes_escritos;       // total de bytes gravados desde o boot
  uint32_t registros_descartados;// linhas perdidas por buffer cheio/falha
};

static const uint16_t TAMANHO_PAYLOAD_STATUS_ARMAZENAMENTO = 22;

}  // namespace protocolo

#endif  // PROTOCOLO_TIPOS_COMANDOS_H
