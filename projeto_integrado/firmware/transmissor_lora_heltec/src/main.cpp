// main.cpp — Transmissor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// Recebe quadros validados do controlador pelo UART e os retransmite por LoRa
// P2P. Devolve STATUS_TRANSMISSOR e CONFIRMACAO_UART ao controlador. Mostra o
// estado no OLED. Agendamento cooperativo por millis(); sem delay no laco.
// -----------------------------------------------------------------------------
#include <Arduino.h>

#include "configuracao_hardware.h"
#include "diagnostico.h"
#include "display_oled.h"
#include "protocolo.h"
#include "receptor_uart.h"
#include "transmissor_lora.h"

using namespace protocolo;

static ReceptorUart uart;
static TransmissorLora radio;
static DisplayOled oled;
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
static DiagnosticoTx diag;
#endif

static uint32_t pacotes_descartados = 0;
static uint16_t seq_status = 0;
static uint32_t instante_status_ms = 0;
static uint32_t instante_oled_ms = 0;
static bool tem_ultima_gnss = false;
static TelemetriaGnss ultima_gnss;
static uint16_t sequencia_ultima_gnss = 0;
static uint32_t instante_ultima_gnss_ms = 0;

// Envia um CONFIRMACAO_UART (ACK) ao controlador com a sequencia confirmada.
static void enviarConfirmacao(uint16_t sequenciaConfirmada) {
  Pacote pkt;
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_CONFIRMACAO_UART;
  pkt.id_remetente = ID_TRANSMISSOR;
  pkt.sequencia = sequenciaConfirmada;
  pkt.flags = 0;
  pkt.tamanho_payload = 2;
  escreverU16LE(&pkt.payload[0], sequenciaConfirmada);

  uint8_t quadro[TAMANHO_MAXIMO_QUADRO_COBS];
  size_t n = enquadrarParaUart(pkt, quadro, sizeof(quadro));
  if (n > 0) {
    uart.enviarBrutos(quadro, n);
  }
}

// Envia o STATUS_TRANSMISSOR periodico de volta ao controlador.
static void enviarStatus() {
  StatusTransmissor s;
  s.uptime_ms = millis();
  s.quadros_uart_validos = uart.quadrosValidos();
  s.quadros_uart_invalidos = uart.quadrosInvalidos();
  s.transmissoes_lora = radio.transmissoes();
  s.timeouts_lora = radio.timeouts();
  s.pacotes_descartados = pacotes_descartados;
  s.ultima_sequencia = uart.ultimaSequencia();
  s.estado_tx = radio.estado();

  Pacote pkt;
  montarPacoteStatusTransmissor(pkt, ID_TRANSMISSOR, ++seq_status, s);

  uint8_t quadro[TAMANHO_MAXIMO_QUADRO_COBS];
  size_t n = enquadrarParaUart(pkt, quadro, sizeof(quadro));
  if (n > 0) {
    uart.enviarBrutos(quadro, n);
  }
}

void setup() {
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("==== Transmissor LoRa (Heltec V3) ===="));
#endif

  oled.iniciar();   // liga Vext e inicializa o OLED
  radio.iniciar();  // Mcu.begin + configuracao do SX1262
  uart.iniciar(Serial1);

#if HABILITAR_SERIAL_TX_DIAGNOSTICO
  Serial.println(F("Inicializacao concluida."));
#endif
}

void loop() {
  const uint32_t agora = millis();

  uart.atualizar();
  radio.atualizar();

  // Encaminha pacotes validos do controlador -> LoRa.
  Pacote pkt;
  while (uart.obterPacote(pkt)) {
    if (pkt.tipo == MSG_TELEMETRIA_GNSS) {
      TelemetriaGnss recebida;
      if (lerTelemetria(pkt, recebida)) {
        ultima_gnss = recebida;
        sequencia_ultima_gnss = pkt.sequencia;
        instante_ultima_gnss_ms = agora;
        tem_ultima_gnss = true;
      }
    }

    uint8_t buf[TAMANHO_MAXIMO_PACOTE];
    size_t n = codificarPacote(pkt, buf, sizeof(buf));
    if (n == 0) {
      continue;
    }
    if (radio.enviar(buf, n)) {
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
      Serial.print(F("[LoRa] enviado seq="));
      Serial.print(pkt.sequencia);
      Serial.print(F(" tipo="));
      Serial.println(pkt.tipo);
#endif
      enviarConfirmacao(pkt.sequencia);
    } else {
      ++pacotes_descartados;  // radio ocupado: nao sobrescreve TX ativa
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
      Serial.println(F("[LoRa] radio ocupado: pacote descartado"));
#endif
    }
  }

  // Status periodico -> controlador.
  if ((uint32_t)(agora - instante_status_ms) >= PERIODO_STATUS_MS) {
    instante_status_ms = agora;
    enviarStatus();
  }

  // OLED periodico.
  if ((uint32_t)(agora - instante_oled_ms) >= PERIODO_OLED_MS) {
    instante_oled_ms = agora;
    EstadoDisplayTx e;
    e.uartConectado = uart.controladorConectado();
    e.ultimaSequencia = uart.ultimaSequencia();
    e.ultimoTipo = uart.ultimoTipo();
    e.estadoTx = radio.estado();
    e.transmissoes = radio.transmissoes();
    e.timeouts = radio.timeouts();
    e.quadrosInvalidos = uart.quadrosInvalidos();
    e.idadeUltimoQuadroMs = uart.idadeUltimoQuadroMs();
    e.temGnss = tem_ultima_gnss;
    e.gnss = ultima_gnss;
    e.sequenciaGnss = sequencia_ultima_gnss;
    e.idadeGnssMs = tem_ultima_gnss
                        ? (uint32_t)(agora - instante_ultima_gnss_ms)
                        : UINT32_MAX;
    oled.mostrar(e);
  }

  // Diagnostico periodico pelo USB.
#if HABILITAR_SERIAL_TX_DIAGNOSTICO
  diag.atualizar(agora, uart.controladorConectado(), uart.quadrosValidos(),
                 uart.quadrosInvalidos(), radio.transmissoes(), radio.timeouts(),
                 pacotes_descartados, radio.estado(), uart.bytesRecebidos(),
                 uart.delimitadoresRecebidos(), uart.bytesNoBufferAtual(),
                 uart.idadeUltimoByteMs());
#endif
}
