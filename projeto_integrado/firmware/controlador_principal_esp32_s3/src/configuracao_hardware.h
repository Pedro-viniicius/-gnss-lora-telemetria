// configuracao_hardware.h
// -----------------------------------------------------------------------------
// FONTE UNICA DE VERDADE da pinagem e dos parametros de hardware do controlador
// principal (ESP32-S3, mestre do sistema).
//
// >>> ATENCAO: TODOS os pinos abaixo sao PREMISSAS a confirmar fisicamente na
// >>> sua placa. Ajuste aqui, em um unico lugar, sem espalhar numeros de pino
// >>> pelo codigo. Nesta versao simplificada o firmware usa apenas GNSS + UART
// >>> LoRa; teclado e cartao SD ficam desabilitados por padrao.
// -----------------------------------------------------------------------------
#ifndef CONFIGURACAO_HARDWARE_H
#define CONFIGURACAO_HARDWARE_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
// UART do GNSS UM980 (HardwareSerial dedicada). NAO use SoftwareSerial no S3.
// ESP32-S3 TX  -> UM980 RX ; ESP32-S3 RX <- UM980 TX ; GND comum.
// ----------------------------------------------------------------------------
#define GNSS_UART_NUM        1            // usa a instancia Serial1 (UART1)
#define GNSS_PINO_RX         43           // RX do ESP32-S3 (recebe TX do UM980)
#define GNSS_PINO_TX         44           // TX do ESP32-S3 (vai ao RX do UM980)
#define GNSS_BAUD            115200UL     // baud padrao do UM980 (confirmar)
#define GNSS_RX_BUFFER_BYTES 2048         // mesmo criterio do projeto de referencia
#define GNSS_TIMEOUT_INIT_MS 5000UL       // limite p/ detectar dados no boot
#define GNSS_IDADE_MAX_MS    3000UL       // acima disso, dados marcados ANTIGOS
#define GNSS_PINO_PPS        -1           // opcional; -1 = nao usado

#ifndef GNSS_CONFIGURAR_COM_SPARKFUN
#define GNSS_CONFIGURAR_COM_SPARKFUN 1    // usa biblioteca SparkFun para acordar/configurar UM980
#endif

#ifndef HABILITAR_VARREDURA_BAUD_GNSS
#define HABILITAR_VARREDURA_BAUD_GNSS 1   // 1 = testa bauds comuns se nao houver NMEA
#endif
#define GNSS_TEMPO_TESTE_BAUD_MS 3000UL   // tempo por baud durante a varredura

// ----------------------------------------------------------------------------
// UART do enlace LoRa (para a Heltec TX), full-duplex. HardwareSerial dedicada.
// ESP32-S3 TX  -> Heltec RX ; ESP32-S3 RX <- Heltec TX ; GND comum ; 3.3 V.
// ----------------------------------------------------------------------------
#define LORA_UART_NUM        2            // usa a instancia Serial2 (UART2)
#define LORA_PINO_RX         17           // RX do ESP32-S3 (recebe TX da Heltec)
#define LORA_PINO_TX         18           // TX do ESP32-S3 (vai ao RX da Heltec)
#define LORA_BAUD            115200UL     // enlace UART controlador <-> Heltec
#define LORA_TIMEOUT_MS      4000UL       // sem status da Heltec => desconectada

// ----------------------------------------------------------------------------
// USB Serial (depuracao/diagnostico).
// ----------------------------------------------------------------------------
#define USB_BAUD             115200UL

// ----------------------------------------------------------------------------
// Opcoes desta versao simplificada.
// ----------------------------------------------------------------------------
#ifndef HABILITAR_TECLADO
#define HABILITAR_TECLADO    0            // 0 = nao inicializa nem compila teclado
#endif

#ifndef HABILITAR_CARTAO_SD
#define HABILITAR_CARTAO_SD  0            // 0 = nao inicializa nem compila microSD
#endif

#ifndef HABILITAR_LOG_SERIAL_DETALHADO
#define HABILITAR_LOG_SERIAL_DETALHADO 1  // logs de GNSS e estados principais
#endif

#ifndef HABILITAR_LOG_NMEA_BRUTO
#define HABILITAR_LOG_NMEA_BRUTO 0        // 1 gera muito texto; use so em bancada
#endif

#ifndef HABILITAR_LOG_UART_LORA
#define HABILITAR_LOG_UART_LORA 1         // logs de quadros UART para a Heltec TX
#endif

#define AGUARDAR_SERIAL_USB_MS 2500UL     // espera curta pelo Monitor Serial

// ----------------------------------------------------------------------------
// Teclado matricial 4x4. Linhas sao SAIDAS; colunas sao ENTRADAS com pull-up.
// Desabilitado nesta versao GPS-only.
// Layout fisico:
//     1 2 3 A
//     4 5 6 B
//     7 8 9 C
//     * 0 # D
// ----------------------------------------------------------------------------
#if HABILITAR_TECLADO
#define TECLADO_LINHAS       4
#define TECLADO_COLUNAS      4

// Pinos das 4 linhas (saidas).
#define TECLADO_PINO_LINHA_0 20
#define TECLADO_PINO_LINHA_1 21
#define TECLADO_PINO_LINHA_2 47
#define TECLADO_PINO_LINHA_3 48

// Pinos das 4 colunas (entradas com INPUT_PULLUP).
#define TECLADO_PINO_COLUNA_0 37
#define TECLADO_PINO_COLUNA_1 36
#define TECLADO_PINO_COLUNA_2 35
#define TECLADO_PINO_COLUNA_3 0

#define TECLADO_DEBOUNCE_MS   30          // tempo de estabilizacao de tecla
#define TECLADO_MAX_DIGITOS   9           // limite do buffer numerico (sem overflow)
#endif

// ----------------------------------------------------------------------------
// LED de status embarcado (opcional). -1 desabilita.
// ----------------------------------------------------------------------------
#ifdef LED_BUILTIN
#define PINO_LED_STATUS      LED_BUILTIN
#else
#define PINO_LED_STATUS      -1
#endif

// ----------------------------------------------------------------------------
// Cartao microSD (SPI). Mantido apenas como referencia de pinagem; esta versao
// simplificada compila com HABILITAR_CARTAO_SD=0 e nao usa o cartao.
//
// Todos os pinos abaixo sao PREMISSAS a confirmar fisicamente; foram escolhidos
// por estarem LIVRES no controlador generico e por NAO usar os GPIOs 8-14, que
// sao reservados ao radio LoRa nas placas Heltec V3.
//
// Ligacao (modulo microSD -> ESP32-S3), se reabilitado no futuro:
//   SCK -> PIN_SD_SCK ; MISO -> PIN_SD_MISO ; MOSI -> PIN_SD_MOSI ;
//   CS  -> PIN_SD_CS  ; VCC 3.3V ; GND comum.
// ----------------------------------------------------------------------------
#if HABILITAR_CARTAO_SD
#define PIN_SD_SCK           5            // clock SPI do cartao
#define PIN_SD_MISO          6            // MISO (dados do cartao -> ESP32-S3)
#define PIN_SD_MOSI          7            // MOSI (dados ESP32-S3 -> cartao)
#define PIN_SD_CS            15           // Chip Select do cartao
#define PIN_SD_DETECT        16           // card-detect (opcional)
#define SD_POSSUI_DETECCAO   0            // 0 = ignora PIN_SD_DETECT
#define FREQUENCIA_SPI_SD_HZ 20000000UL   // 20 MHz (reduza se houver instabilidade)
#define SD_SPI_HOST          FSPI         // host SPI2 do ESP32-S3

#define PONTO_MONTAGEM_SD              "/sd"           // ponto de montagem VFS
#define INTERVALO_FLUSH_SD_MS         2000UL          // periodo de flush ao cartao
#define INTERVALO_VERIFICACAO_SD_MS   5000UL          // periodo de checagem de saude
#define TAMANHO_MAXIMO_ARQUIVO_SD_BYTES  (5UL*1024UL*1024UL)  // 5 MB -> rotaciona
#define ESPACO_LIVRE_MINIMO_SD_BYTES     (1UL*1024UL*1024UL)  // 1 MB -> CHEIO
#define PERIODO_STATUS_ARMAZENAMENTO_MS  5000UL        // envio de STATUS_ARMAZENAMENTO
#endif

// ----------------------------------------------------------------------------
// Periodos de agendamento cooperativo (millis()).
// ----------------------------------------------------------------------------
#define PERIODO_TELEMETRIA_MS   2000UL    // envio periodico de telemetria
#define PERIODO_HEARTBEAT_MS    5000UL    // heartbeat quando ocioso
#define PERIODO_DIAGNOSTICO_MS  1000UL    // impressao de diagnostico no USB

// Validacoes basicas de coerencia de pinagem (nao pega tudo, mas evita erros
// grosseiros de duplicidade entre UARTs).
static_assert(GNSS_PINO_RX != GNSS_PINO_TX, "GNSS RX e TX nao podem ser iguais");
static_assert(LORA_PINO_RX != LORA_PINO_TX, "LoRa RX e TX nao podem ser iguais");
static_assert(GNSS_UART_NUM != LORA_UART_NUM, "GNSS e LoRa em UARTs distintas");

// Coerencia opcional do cartao SD, somente se ele for reabilitado no futuro.
#if HABILITAR_CARTAO_SD
static_assert(PIN_SD_SCK != PIN_SD_MISO && PIN_SD_SCK != PIN_SD_MOSI &&
                  PIN_SD_SCK != PIN_SD_CS && PIN_SD_MISO != PIN_SD_MOSI &&
                  PIN_SD_MISO != PIN_SD_CS && PIN_SD_MOSI != PIN_SD_CS,
              "Pinos do barramento SPI do SD devem ser distintos entre si");
static_assert(PIN_SD_DETECT != PIN_SD_SCK && PIN_SD_DETECT != PIN_SD_MISO &&
                  PIN_SD_DETECT != PIN_SD_MOSI && PIN_SD_DETECT != PIN_SD_CS,
              "Pino de deteccao do SD nao pode colidir com o barramento SPI");
static_assert(PIN_SD_SCK < 8 || PIN_SD_SCK > 14,
              "SD nao deve usar GPIO8-GPIO14 reservados ao radio LoRa Heltec V3");
static_assert(PIN_SD_MISO < 8 || PIN_SD_MISO > 14,
              "SD nao deve usar GPIO8-GPIO14 reservados ao radio LoRa Heltec V3");
static_assert(PIN_SD_MOSI < 8 || PIN_SD_MOSI > 14,
              "SD nao deve usar GPIO8-GPIO14 reservados ao radio LoRa Heltec V3");
static_assert(PIN_SD_CS < 8 || PIN_SD_CS > 14,
              "SD nao deve usar GPIO8-GPIO14 reservados ao radio LoRa Heltec V3");
static_assert(PIN_SD_DETECT < 8 || PIN_SD_DETECT > 14,
              "SD nao deve usar GPIO8-GPIO14 reservados ao radio LoRa Heltec V3");
static_assert(PIN_SD_SCK != GNSS_PINO_RX && PIN_SD_SCK != GNSS_PINO_TX &&
                  PIN_SD_SCK != LORA_PINO_RX && PIN_SD_SCK != LORA_PINO_TX &&
                  PIN_SD_MISO != GNSS_PINO_RX && PIN_SD_MISO != GNSS_PINO_TX &&
                  PIN_SD_MISO != LORA_PINO_RX && PIN_SD_MISO != LORA_PINO_TX &&
                  PIN_SD_MOSI != GNSS_PINO_RX && PIN_SD_MOSI != GNSS_PINO_TX &&
                  PIN_SD_MOSI != LORA_PINO_RX && PIN_SD_MOSI != LORA_PINO_TX &&
                  PIN_SD_CS != GNSS_PINO_RX && PIN_SD_CS != GNSS_PINO_TX &&
                  PIN_SD_CS != LORA_PINO_RX && PIN_SD_CS != LORA_PINO_TX &&
                  PIN_SD_DETECT != GNSS_PINO_RX && PIN_SD_DETECT != GNSS_PINO_TX &&
                  PIN_SD_DETECT != LORA_PINO_RX && PIN_SD_DETECT != LORA_PINO_TX,
              "Pinos do SD colidem com as UARTs GNSS/LoRa");
#if HABILITAR_TECLADO
static_assert(PIN_SD_SCK != TECLADO_PINO_LINHA_0 && PIN_SD_SCK != TECLADO_PINO_LINHA_1 &&
                  PIN_SD_SCK != TECLADO_PINO_LINHA_2 && PIN_SD_SCK != TECLADO_PINO_LINHA_3 &&
                  PIN_SD_SCK != TECLADO_PINO_COLUNA_0 && PIN_SD_SCK != TECLADO_PINO_COLUNA_1 &&
                  PIN_SD_SCK != TECLADO_PINO_COLUNA_2 && PIN_SD_SCK != TECLADO_PINO_COLUNA_3 &&
                  PIN_SD_MISO != TECLADO_PINO_LINHA_0 && PIN_SD_MISO != TECLADO_PINO_LINHA_1 &&
                  PIN_SD_MISO != TECLADO_PINO_LINHA_2 && PIN_SD_MISO != TECLADO_PINO_LINHA_3 &&
                  PIN_SD_MISO != TECLADO_PINO_COLUNA_0 && PIN_SD_MISO != TECLADO_PINO_COLUNA_1 &&
                  PIN_SD_MISO != TECLADO_PINO_COLUNA_2 && PIN_SD_MISO != TECLADO_PINO_COLUNA_3 &&
                  PIN_SD_MOSI != TECLADO_PINO_LINHA_0 && PIN_SD_MOSI != TECLADO_PINO_LINHA_1 &&
                  PIN_SD_MOSI != TECLADO_PINO_LINHA_2 && PIN_SD_MOSI != TECLADO_PINO_LINHA_3 &&
                  PIN_SD_MOSI != TECLADO_PINO_COLUNA_0 && PIN_SD_MOSI != TECLADO_PINO_COLUNA_1 &&
                  PIN_SD_MOSI != TECLADO_PINO_COLUNA_2 && PIN_SD_MOSI != TECLADO_PINO_COLUNA_3 &&
                  PIN_SD_CS != TECLADO_PINO_LINHA_0 && PIN_SD_CS != TECLADO_PINO_LINHA_1 &&
                  PIN_SD_CS != TECLADO_PINO_LINHA_2 && PIN_SD_CS != TECLADO_PINO_LINHA_3 &&
                  PIN_SD_CS != TECLADO_PINO_COLUNA_0 && PIN_SD_CS != TECLADO_PINO_COLUNA_1 &&
                  PIN_SD_CS != TECLADO_PINO_COLUNA_2 && PIN_SD_CS != TECLADO_PINO_COLUNA_3 &&
                  PIN_SD_DETECT != TECLADO_PINO_LINHA_0 && PIN_SD_DETECT != TECLADO_PINO_LINHA_1 &&
                  PIN_SD_DETECT != TECLADO_PINO_LINHA_2 && PIN_SD_DETECT != TECLADO_PINO_LINHA_3 &&
                  PIN_SD_DETECT != TECLADO_PINO_COLUNA_0 && PIN_SD_DETECT != TECLADO_PINO_COLUNA_1 &&
                  PIN_SD_DETECT != TECLADO_PINO_COLUNA_2 && PIN_SD_DETECT != TECLADO_PINO_COLUNA_3,
              "Pinos do SD colidem com o teclado");
#endif
#endif

#endif  // CONFIGURACAO_HARDWARE_H
