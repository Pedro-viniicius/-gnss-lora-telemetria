// main.cpp — Controlador principal ESP32-S3 (mestre do sistema)
// -----------------------------------------------------------------------------
// Orquestra os servicos (GNSS, teclado, enlace UART/LoRa), monta e envia
// telemetria/eventos e mantem a maquina de estados. Agendamento cooperativo
// por millis(); nenhum delay bloqueante no laco principal.
//
// Compile com -D MODO_DIAGNOSTICO para operar sem periféricos reais (GNSS e
// teclado simulados; dados marcados como SIMULADOS).
// -----------------------------------------------------------------------------
#include <Arduino.h>

#include "configuracao_hardware.h"
#include "diagnostico.h"
#include "gerenciador_telemetria.h"
#include "maquina_estados.h"
#include "servico_gnss.h"
#include "servico_teclado.h"
#include "servico_uart_lora.h"

using namespace protocolo;

// UART0/USB CDC = Serial (depuracao). Serial1 = GNSS. Serial2 = enlace Heltec.
static ServicoGnss gnss;
static ServicoTeclado teclado;
static ServicoUartLora enlace;
static GerenciadorTelemetria telemetria;
static MaquinaEstados maquina;
static Diagnostico diag;

static uint32_t instante_telemetria_ms = 0;
static uint32_t instante_heartbeat_ms = 0;
static uint32_t instante_led_ms = 0;
static bool led_ligado = false;

#ifdef MODO_DIAGNOSTICO
static uint32_t instante_sim_teclado_ms = 0;
static const char kSequenciaTeclasSimuladas[] = {'1', '2', '3', '#', 'A', 'D'};
static uint8_t idx_tecla_simulada = 0;
#endif

static void enviarTelemetria(bool forcada) {
  TelemetriaGnss snap;
  gnss.obterSnapshot(snap);

  Pacote pkt;
  telemetria.montarTelemetria(snap, teclado.ultimoComandoConfirmado(), pkt);
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

static void enviarEventoTeclado(const EventoTeclado& ev) {
  Pacote pkt;
  montarPacoteEventoTeclado(pkt, ID_CONTROLADOR, telemetria.proximaSequencia(),
                            ev);
  bool ok = enlace.enviarPacote(pkt);
  Serial.print(F("[EVENTO] tecla='"));
  Serial.print((char)ev.tecla);
  Serial.print(F("' seq="));
  Serial.print(pkt.sequencia);
  Serial.println(ok ? F(" -> enviado") : F(" -> FALHA no envio"));
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
  delay(50);  // unico atraso: estabiliza o USB CDC no boot
  Serial.println();
  Serial.println(F("==== Controlador Principal ESP32-S3 (mestre) ===="));

  gnss.iniciar(Serial1);
  enlace.iniciar(Serial2);
  teclado.iniciar();
  maquina.iniciar(millis());

  if (PINO_LED_STATUS >= 0) {
    pinMode(PINO_LED_STATUS, OUTPUT);
    digitalWrite(PINO_LED_STATUS, LOW);
  }

#ifdef MODO_DIAGNOSTICO
  gnss.habilitarSimulacao(true);
  Serial.println(F(">>> MODO_DIAGNOSTICO ATIVO: GNSS e teclado SIMULADOS <<<"));
#endif

  Serial.println(F("Inicializacao concluida. Entrando no laco principal."));
}

void loop() {
  const uint32_t agora = millis();

  // 1) Atualiza servicos (todos nao-bloqueantes).
  gnss.atualizar();
  teclado.atualizar();
  enlace.atualizar();

#ifdef MODO_DIAGNOSTICO
  // Injeta teclas simuladas periodicamente.
  if ((uint32_t)(agora - instante_sim_teclado_ms) >= 4000UL) {
    instante_sim_teclado_ms = agora;
    char t = kSequenciaTeclasSimuladas[idx_tecla_simulada];
    idx_tecla_simulada =
        (uint8_t)((idx_tecla_simulada + 1) %
                  (sizeof(kSequenciaTeclasSimuladas) / sizeof(char)));
    Serial.print(F("[SIM] Injetando tecla: "));
    Serial.println(t);
    teclado.injetarTecla(t);
  }
#endif

  // 2) Processa eventos de teclado (envio imediato).
  EventoTeclado ev;
  while (teclado.obterEvento(ev)) {
    if (ev.tipo_evento == EVT_TELEMETRIA_FORCADA) {
      enviarTelemetria(true);
    } else {
      enviarEventoTeclado(ev);
    }
  }

  // 3) Telemetria periodica.
  if ((uint32_t)(agora - instante_telemetria_ms) >= PERIODO_TELEMETRIA_MS) {
    instante_telemetria_ms = agora;
    enviarTelemetria(false);
  }

  // 4) Heartbeat periodico (sinal de vida do mestre).
  if ((uint32_t)(agora - instante_heartbeat_ms) >= PERIODO_HEARTBEAT_MS) {
    instante_heartbeat_ms = agora;
    Pacote hb;
    montarPacoteHeartbeat(hb, ID_CONTROLADOR, telemetria.proximaSequencia(),
                          agora);
    enlace.enviarPacote(hb);
  }

  // 5) Maquina de estados (continua operando mesmo com falha recuperavel).
  maquina.atualizar(agora, gnss.recebeuDados(), gnss.temFixValido(),
                    enlace.conectado(), GNSS_TIMEOUT_INIT_MS);

  // 6) LED de status.
  atualizarLed(maquina, agora);

  // 7) Diagnostico periodico pelo USB.
  TelemetriaGnss snap;
  gnss.obterSnapshot(snap);
  diag.atualizar(agora, maquina, snap, gnss.temFixValido(), gnss.idadeDadosMs(),
                 enlace, teclado.modoAtual(), telemetria.sequenciaAtual());
}
