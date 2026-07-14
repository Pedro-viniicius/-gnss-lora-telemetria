// main.cpp — Receptor LoRa (Heltec WiFi LoRa 32 V3)
// -----------------------------------------------------------------------------
// Recebe o protocolo binario por LoRa P2P, valida (CRC/versao/tipo/tamanho),
// detecta duplicatas/faltantes, decodifica e exibe no OLED e no Serial. Mostra
// estado de timeout quando fica sem pacotes. Agendamento por millis().
// -----------------------------------------------------------------------------
#include <Arduino.h>

#include "configuracao_hardware.h"
#include "decodificador_pacotes.h"
#include "diagnostico.h"
#include "display_oled.h"
#include "protocolo.h"
#include "receptor_lora.h"

using namespace protocolo;

static ReceptorLora radio;
static DecodificadorPacotes decodificador;
static DisplayOled oled;
static DiagnosticoRx diag;

// Estado consolidado para exibicao.
static EstadoDisplayRx estado;
static uint32_t instante_ultimo_pacote_ms = 0;
static uint32_t instante_oled_ms = 0;
static uint32_t instante_diag_ocioso_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("==== Receptor LoRa (Heltec V3) ===="));

  oled.iniciar();
  radio.iniciar();

  estado.temDados = false;
  estado.timeout = false;
  memset(&estado.gnss, 0, sizeof(estado.gnss));
  estado.sequencia = 0;
  estado.rssi = 0;
  estado.snr = 0;
  estado.idadeUltimoPacoteMs = UINT32_MAX;

  Serial.println(F("Inicializacao concluida. Em modo de recepcao."));
}

void loop() {
  const uint32_t agora = millis();

  radio.atualizar();

  // Ha pacote cru novo?
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = 0;
  int16_t rssi = 0;
  int8_t snr = 0;
  if (radio.obterPacoteCru(buf, sizeof(buf), n, rssi, snr)) {
    Pacote pkt;
    bool faltante = false;
    DecodificadorPacotes::Resultado r =
        decodificador.processar(buf, n, pkt, faltante);

    if (r == DecodificadorPacotes::RES_INVALIDO) {
      diag.imprimirInvalido(decodificador.ultimoErro(), rssi, snr, decodificador);
    } else {
      // RES_OK ou RES_DUPLICADO: dados utilizaveis.
      bool duplicado = (r == DecodificadorPacotes::RES_DUPLICADO);
      diag.imprimirPacote(pkt, rssi, snr, duplicado, faltante, decodificador);

      instante_ultimo_pacote_ms = agora;
      estado.temDados = true;
      estado.sequencia = pkt.sequencia;
      estado.rssi = rssi;
      estado.snr = snr;

      if (pkt.tipo == MSG_TELEMETRIA_GNSS) {
        TelemetriaGnss t;
        if (lerTelemetria(pkt, t)) {
          estado.gnss = t;
        }
      }
    }
  }

  // Atualiza o estado de timeout (rollover-seguro).
  if (estado.temDados) {
    estado.idadeUltimoPacoteMs =
        (uint32_t)(agora - instante_ultimo_pacote_ms);
    estado.timeout = estado.idadeUltimoPacoteMs > RECEPTOR_TIMEOUT_MS;
  }

  // OLED periodico.
  if ((uint32_t)(agora - instante_oled_ms) >= PERIODO_OLED_MS) {
    instante_oled_ms = agora;
    oled.mostrar(estado);
  }

  // Aviso periodico de ociosidade/timeout no Serial.
  if (estado.temDados && estado.timeout &&
      (uint32_t)(agora - instante_diag_ocioso_ms) >= PERIODO_DIAGNOSTICO_MS) {
    instante_diag_ocioso_ms = agora;
    Serial.print(F("[TIMEOUT] Sem pacotes ha "));
    Serial.print(estado.idadeUltimoPacoteMs / 1000UL);
    Serial.println(F(" s"));
  }
}
