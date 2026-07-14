// main.cpp — Controlador principal ESP32-S3 (mestre do sistema)
// -----------------------------------------------------------------------------
// Orquestra os servicos GNSS e enlace UART/LoRa, monta e envia telemetria GNSS
// e mantem a maquina de estados. Versao simplificada: sem teclado e sem cartao
// SD. Agendamento cooperativo por millis(); nenhum delay bloqueante no laco.
//
// Compile com -D MODO_DIAGNOSTICO para operar com GNSS simulado; os dados sao
// marcados como SIMULADOS.
// -----------------------------------------------------------------------------
#include <Arduino.h>

#include "configuracao_hardware.h"
#include "diagnostico.h"
#include "gerenciador_telemetria.h"
#include "maquina_estados.h"
#include "servico_gnss.h"
#include "servico_uart_lora.h"

using namespace protocolo;

// UART0/USB CDC = Serial (depuracao). Serial1 = GNSS. Serial2 = enlace Heltec.
static ServicoGnss gnss;
static ServicoUartLora enlace;
static GerenciadorTelemetria telemetria;
static MaquinaEstados maquina;
static Diagnostico diag;

static uint32_t instante_telemetria_ms = 0;
static uint32_t instante_heartbeat_ms = 0;
static uint32_t instante_led_ms = 0;
static bool led_ligado = false;

static void imprimirConfiguracaoHardware() {
  Serial.println(F("----- CONFIGURACAO DO CONTROLADOR -----"));
  Serial.print(F("USB Serial baud: "));
  Serial.println(USB_BAUD);
  Serial.print(F("GNSS UART"));
  Serial.print(GNSS_UART_NUM);
  Serial.print(F(" | RX GPIO"));
  Serial.print(GNSS_PINO_RX);
  Serial.print(F(" | TX GPIO"));
  Serial.print(GNSS_PINO_TX);
  Serial.print(F(" | baud "));
  Serial.println(GNSS_BAUD);
  Serial.print(F("LoRa UART"));
  Serial.print(LORA_UART_NUM);
  Serial.print(F(" | RX GPIO"));
  Serial.print(LORA_PINO_RX);
  Serial.print(F(" | TX GPIO"));
  Serial.print(LORA_PINO_TX);
  Serial.print(F(" | baud "));
  Serial.println(LORA_BAUD);
  Serial.println(F("Teclado: DESABILITADO nesta versao"));
  Serial.println(F("Cartao SD: DESABILITADO nesta versao"));
  Serial.print(F("Logs detalhados: "));
  Serial.print(HABILITAR_LOG_SERIAL_DETALHADO ? F("SIM") : F("NAO"));
  Serial.print(F(" | NMEA bruto: "));
  Serial.print(HABILITAR_LOG_NMEA_BRUTO ? F("SIM") : F("NAO"));
  Serial.print(F(" | UART LoRa: "));
  Serial.println(HABILITAR_LOG_UART_LORA ? F("SIM") : F("NAO"));
  Serial.println(F("----------------------------------------"));
}

static void enviarTelemetria(bool forcada) {
  TelemetriaGnss snap;
  gnss.obterSnapshot(snap);

  Pacote pkt;
  telemetria.montarTelemetria(snap, pkt);
  bool ok = enlace.enviarPacote(pkt);

  Serial.print(F("[TELEMETRIA] seq="));
  Serial.print(pkt.sequencia);
  Serial.print(forcada ? F(" (forcada)") : F(" (periodica)"));
  if (snap.flags & GNSS_FLAG_DADOS_ANTIGOS) {
    Serial.print(F(" [DADOS ANTIGOS]"));
  }
  if (snap.flags & GNSS_FLAG_DADOS_SIMULADOS) {
    Serial.print(F(" [SIMULADO]"));
  }
  Serial.println(ok ? F(" -> enviada") : F(" -> FALHA no envio"));
}

static void atualizarLed(const MaquinaEstados& m, uint32_t agora) {
  if (PINO_LED_STATUS < 0) {
    return;
  }
  // Operacional: LED aceso fixo. Demais estados: pisca (falha/atencao).
  if (m.estado() == EST_OPERACIONAL) {
    if (!led_ligado) {
      led_ligado = true;
      digitalWrite(PINO_LED_STATUS, HIGH);
    }
    return;
  }
  if ((uint32_t)(agora - instante_led_ms) >= 250UL) {
    instante_led_ms = agora;
    led_ligado = !led_ligado;
    digitalWrite(PINO_LED_STATUS, led_ligado ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(USB_BAUD);
  uint32_t inicioSerial = millis();
  while (!Serial && (uint32_t)(millis() - inicioSerial) < AGUARDAR_SERIAL_USB_MS) {
    delay(10);
  }
  delay(50);  // atraso curto e limitado para estabilizar o USB CDC no boot
  Serial.println();
  Serial.println(F("==== Controlador Principal ESP32-S3 (mestre) ===="));
  Serial.println(F("[BOOT] Serial USB ativo. Iniciando perifericos..."));
  imprimirConfiguracaoHardware();

  gnss.iniciar(Serial1);
  Serial.println(F("[BOOT] Servico GNSS inicializado"));
  enlace.iniciar(Serial2);
  Serial.println(F("[BOOT] Enlace UART LoRa inicializado"));
  Serial.println(F("[BOOT] Teclado e cartao SD desabilitados nesta versao"));
  maquina.iniciar(millis());
  Serial.println(F("[BOOT] Maquina de estados inicializada"));

  if (PINO_LED_STATUS >= 0) {
    pinMode(PINO_LED_STATUS, OUTPUT);
    digitalWrite(PINO_LED_STATUS, LOW);
  }

#ifdef MODO_DIAGNOSTICO
  gnss.habilitarSimulacao(true);
  Serial.println(F(">>> MODO_DIAGNOSTICO ATIVO: GNSS SIMULADO <<<"));
#endif

  Serial.println(F("Inicializacao concluida. Entrando no laco principal."));
}

void loop() {
  const uint32_t agora = millis();

  // 1) Atualiza servicos (todos nao-bloqueantes).
  gnss.atualizar();
  enlace.atualizar();

  // 2) Telemetria periodica.
  if ((uint32_t)(agora - instante_telemetria_ms) >= PERIODO_TELEMETRIA_MS) {
    instante_telemetria_ms = agora;
    enviarTelemetria(false);
  }

  // 3) Heartbeat periodico (sinal de vida do mestre).
  if ((uint32_t)(agora - instante_heartbeat_ms) >= PERIODO_HEARTBEAT_MS) {
    instante_heartbeat_ms = agora;
    Pacote hb;
    montarPacoteHeartbeat(hb, ID_CONTROLADOR, telemetria.proximaSequencia(),
                          agora);
    enlace.enviarPacote(hb);
  }

  // 4) Maquina de estados (continua operando mesmo com falha recuperavel).
  maquina.atualizar(agora, gnss.recebeuDados(), gnss.temFixValido(),
                    enlace.conectado(), GNSS_TIMEOUT_INIT_MS);

  // 5) LED de status.
  atualizarLed(maquina, agora);

  // 6) Diagnostico periodico pelo USB.
  TelemetriaGnss snap;
  gnss.obterSnapshot(snap);
  diag.atualizar(agora, maquina, snap, gnss.temFixValido(), gnss.idadeDadosMs(),
                 gnss, enlace, telemetria.sequenciaAtual());
}
