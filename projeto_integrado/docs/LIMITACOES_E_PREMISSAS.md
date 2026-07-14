# Limitações e premissas

## Premissas de hardware

1. **Controlador = ESP32-S3** (o "ESP8266 S3" do enunciado não existe). ESP8266
   não teria `HardwareSerial` suficiente para GNSS + LoRa + USB simultâneos, nem
   deveria misturar APIs ESP8266 com ESP32-S3.
2. **Board adotada**: `esp32-s3-devkitc-1` (genérica). Confirme a variante real.
3. **Pinos do mestre** são premissas centralizadas em
   `configuracao_hardware.h` e foram definidos pelo usuário: GNSS
   RX43/TX44, UART LoRa RX17/TX18, teclado linhas 20/21/47/48 e colunas
   37/36/35/0. **Não foram alterados** nesta integração. Atenção: GPIO0 é pino
   de *strapping* em ESP32-S3, GPIO20 pode estar ligado ao USB nativo em alguns
   módulos e GPIO48 costuma ser LED RGB em DevKitC-1. Confirme fisicamente.
4. **microSD no controlador**: está desabilitado nesta versão. Se for
   reabilitado, a pinagem opcional ficou em SCK5, MISO6, MOSI7, CS15 e
   card-detect opcional em GPIO16 (`SD_POSSUI_DETECCAO=0` por padrão), evitando
   os GPIOs 8-14 reservados ao rádio LoRa das Heltec V3.
5. **UM980**: assume-se saída **NMEA** (GGA/RMC/GST) a **115200 bps**. Ajuste
   `GNSS_BAUD` e confirme a configuração de saída do seu módulo.
6. **Heltec V3 confirmada** pelos códigos de referência; pinos internos ficam
   centralizados em `pinos_heltec_v3.h`. GPIO8-14 são exclusivos do rádio LoRa,
   GPIO17/18/21 do OLED integrado, GPIO36 do Vext, GPIO35 do LED, GPIO0 do botão,
   GPIO1 da leitura de bateria e GPIO37 do controle ADC. O USB/UART confirmado
   da Heltec usa `U0TXD=GPIO43` e `U0RXD=GPIO44`.

## Premissas de software / build

7. A biblioteca `heltecautomation/Heltec ESP32 Dev-Boards@2.1.7` (API
   `LoRaWan_APP.h`) foi escrita para o core **arduino-esp32 2.x (IDF4)**. Neste
   ambiente o `espressif32@7.0.1` fornece um core baseado em **IDF5**, no qual a
   macro `GPIO_PIN_COUNT` foi renomeada para `SOC_GPIO_PIN_COUNT`. Aplicamos o
   **shim** `-D GPIO_PIN_COUNT=SOC_GPIO_PIN_COUNT` nos `build_flags`, que resolve
   a incompatibilidade sem editar a biblioteca. Se, em outra máquina, um core
   genuíno 2.x estiver disponível, o shim é inofensivo.
8. As macros de placa Heltec (`WIFI_LORA_32_V3`, `HELTEC_BOARD=30`,
   `SLOW_CLK_TPYE=0`, `LoRaWAN_DEBUG_LEVEL=0`, `RADIO_CHIP_SX1262`) — normalmente
   injetadas pelo `boards.txt` do pacote Arduino da Heltec — são fornecidas via
   `build_flags` porque usamos a board genérica do PlatformIO.

## Limitações de validação (sem hardware)

Só foi possível validar **compilação** e **testes de host**. **Não** foram
verificados em bancada:

- comunicação UART real ESP32-S3 ↔ UM980 e ESP32-S3 ↔ Heltec;
- recepção real de NMEA e obtenção de fix;
- transmissão/recepção LoRa real, RSSI/SNR reais;
- varredura elétrica do teclado (níveis, ruído, debounce em campo);
- montagem, gravação, remoção a quente, rotação e recuperação real do microSD;
- comportamento do OLED e do Vext em placa;
- consumo de corrente e estabilidade de alimentação sob pico de TX.

## Limitações específicas do microSD

- O cartão é opcional. Estados `SEM_CARTAO`, `CHEIO` e `FALHA` não param o
  controlador, mas linhas podem ser descartadas quando a fila de RAM fica cheia.
- `SD.h` pode bloquear por curtos períodos durante operações internas do cartão;
  o projeto reduz o impacto gravando em blocos limitados e espaçados, mas a
  latência real depende do cartão, formatação e fiação.
- Frequência SPI inicial: 20 MHz. Se houver erro de montagem/gravação em bancada,
  reduza `FREQUENCIA_SPI_SD_HZ` para 10 MHz ou menos.
- Use cartão FAT32; exFAT não é assumido por este firmware.

## RTK e precisão

- A presença do UM980 **não** garante precisão centimétrica. O `TipoFix` reflete
  a solução real (`SEM_FIX`/`STANDALONE`/`DIFERENCIAL`/`RTK_FLOAT`/`RTK_FIXED`).
- **RTK Fixed** normalmente exige **correções RTCM válidas**, antena adequada e
  boa instalação/visada de céu. Sem correções, o comum é permanecer em
  `STANDALONE`.
- Há **ponto de extensão** `ServicoGnss::injetarRtcm()` para repassar RTCM cru ao
  UM980. **Não** há NTRIP, caster, usuário/senha nem serviço de correção — isso
  deve ser adicionado externamente pelo integrador.

## Segurança e regulamentação

- Frequência 915 MHz e potência 20 dBm devem obedecer à **regulamentação local**
  (Brasil: ISM 902–928 MHz) e à variante exata da placa/antena.
- Nunca transmita sem antena conectada (risco ao SX1262).
- Alimentação: use fonte estável com GND comum; pico de TX pode resetar a placa
  em regulador subdimensionado. Correntes exatas não foram medidas.
