# Sistema Embarcado Multi-Placa — GNSS UM980 + Teclado 4x4 + LoRa P2P

Sistema de telemetria de campo composto por **três firmwares independentes** que
substituem o antigo pipeline DHT11 → ESP8266 → LoRa por uma cadeia
**GNSS (UM980) + teclado 4x4 + status/comando**, usando um **protocolo binário
versionado, verificado por CRC16** e enquadramento **COBS** no enlace UART.

> Todo o projeto (código, comentários, mensagens de Serial/OLED e documentação)
> está em **Português do Brasil**. Apenas nomes de APIs, hardware e constantes de
> framework permanecem no original.

---

## 1. Objetivo do projeto

Coletar posição GNSS (UM980) e comandos de um teclado matricial 4x4 em um
**controlador mestre ESP32-S3**, transmiti-los por **LoRa ponto-a-ponto** através
de uma **Heltec WiFi LoRa 32 V3** e recebê-los/decodificá-los em outra Heltec V3,
exibindo os dados em **OLED** e no **Monitor Serial**. O enlace UART entre o
mestre e a Heltec transmissora é **full-duplex**: a Heltec devolve status e
confirmações, mantendo o ESP32-S3 como unidade de comando.

## 2. Inventário de hardware

| Papel | Placa | Observação |
|---|---|---|
| Controlador mestre | ESP32-S3 (DevKitC-1) | leitura GNSS + teclado + enlace UART |
| Receptor GNSS | Unicore **UM980** | UART dedicada; saída NMEA por padrão |
| Teclado | Matricial **4x4** (16 teclas) | linhas = saídas, colunas = entradas pull-up |
| Transmissor LoRa | **Heltec WiFi LoRa 32 V3** | ESP32-S3 + SX1262 + OLED |
| Receptor LoRa | **Heltec WiFi LoRa 32 V3** | ESP32-S3 + SX1262 + OLED |

## 3. Modelos de hardware confirmados

- **Heltec WiFi LoRa 32 V3** (TX e RX): confirmado pelos códigos de referência
  (`LoRaWan_APP.h`, `HT_SSD1306Wire.h`, `Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE)`,
  `SDA_OLED`/`RST_OLED`, 915 MHz) e pelo board `heltec_wifi_lora_32_V3` do
  PlatformIO (ESP32-S3 + SX1262).

## 4. Premissas de hardware NÃO confirmadas

- **Controlador mestre = ESP32-S3**: o enunciado citava "ESP8266 S3", que **não
  existe**. Adotamos ESP32-S3 (default do enunciado). *ESP8266 não teria
  HardwareSerial suficiente para GNSS + LoRa + USB simultâneos.*
- **Pinos do GNSS, do enlace UART e do teclado** no mestre: são premissas,
  centralizadas em `configuracao_hardware.h` e marcadas como "CONFIRMAR NA PLACA
  FÍSICA".
- **Baud do UM980 = 115200** e **saída NMEA (GGA/RMC/GST)**: padrão de fábrica
  comum; confirme na sua unidade.

## 5. Arquitetura do sistema

```
                        ┌──────────────────────┐
                        │      UM980 GNSS       │
                        └──────────┬───────────┘
                                   │ UART (NMEA)
                                   ▼
┌───────────────────┐    ┌────────────────────────┐
│ Teclado matricial │───▶│ Controlador ESP32-S3   │
│      4x4          │    │ Mestre do sistema      │
└───────────────────┘    └────────────┬───────────┘
                                       │ UART full-duplex (COBS + CRC16)
                                       ▼
                           ┌────────────────────────┐
                           │ Heltec LoRa TX (V3)    │
                           │ ESP32-S3 + SX1262      │
                           └────────────┬───────────┘
                                        │ LoRa P2P (mesmo pacote binário)
                                        ▼
                           ┌────────────────────────┐
                           │ Heltec LoRa RX (V3)    │
                           │ OLED + Serial          │
                           └────────────────────────┘
```

O mestre envia `TELEMETRIA_GNSS` e `EVENTO_TECLADO`; a Heltec TX devolve
`STATUS_TRANSMISSOR` e `CONFIRMACAO_UART`. O mesmo pacote binário validado é
usado como carga LoRa. Detalhes em `docs/ARQUITETURA.md`.

## 6. Estrutura de diretórios

```
projeto_integrado/
├── README.md
├── docs/                     ARQUITETURA, LIGACOES, PROTOCOLO, TESTES, LIMITACOES
├── firmware/
│   ├── bibliotecas/protocolo_comunicacao/   biblioteca compartilhada (C++ puro)
│   ├── controlador_principal_esp32_s3/
│   ├── transmissor_lora_heltec/
│   └── receptor_lora_heltec/
├── testes/protocolo/         testes de host (g++)
└── referencias/              cópias somente-leitura dos .ino originais
```

## 7. Bibliotecas necessárias

| Firmware | Plataforma | Bibliotecas |
|---|---|---|
| Controlador | `espressif32@7.0.1` (Arduino) | `protocolo_comunicacao` (local) |
| Transmissor | `espressif32@7.0.1` (Arduino) | `protocolo_comunicacao` + `heltecautomation/Heltec ESP32 Dev-Boards@2.1.7` |
| Receptor | `espressif32@7.0.1` (Arduino) | `protocolo_comunicacao` + `heltecautomation/Heltec ESP32 Dev-Boards@2.1.7` |

> A biblioteca Heltec (API `LoRaWan_APP.h`) exige as macros de placa injetadas
> via `build_flags` (ver item 23 e os `platformio.ini`). O shim
> `-D GPIO_PIN_COUNT=SOC_GPIO_PIN_COUNT` resolve a renomeação IDF4→IDF5.

## 8. Comandos de compilação

```bash
# Controlador (normal e modo diagnóstico)
cd firmware/controlador_principal_esp32_s3
pio run -e controlador
pio run -e controlador_diagnostico     # GNSS/teclado simulados

# Transmissor
cd ../transmissor_lora_heltec
pio run -e transmissor

# Receptor
cd ../receptor_lora_heltec
pio run -e receptor

# Testes de host do protocolo
cd ../../testes/protocolo
make test
```

## 9. Comandos de gravação (upload)

```bash
cd firmware/controlador_principal_esp32_s3 && pio run -e controlador -t upload
cd firmware/transmissor_lora_heltec        && pio run -e transmissor  -t upload
cd firmware/receptor_lora_heltec           && pio run -e receptor     -t upload
# Especifique a porta se necessário: --upload-port /dev/ttyUSB0
# Monitor serial:
pio device monitor -b 115200
```

## 10. Tabelas de ligação (resumo)

Tabelas completas em `docs/LIGACOES.md`.

**ESP32-S3 ↔ UM980** (pinos do mestre, premissa — confirmar):

| ESP32-S3 | Direção | UM980 |
|---|---|---|
| GPIO17 (TX1) | → | RX |
| GPIO18 (RX1) | ← | TX |
| GND | — | GND |
| 3V3/5V* | — | VCC |

\* Conforme a placa/breakout do UM980. **GND comum é obrigatório.**

**ESP32-S3 ↔ Heltec TX** (enlace UART full-duplex):

| ESP32-S3 | Direção | Heltec |
|---|---|---|
| GPIO15 (TX2) | → | GPIO5 (RX) |
| GPIO16 (RX2) | ← | GPIO6 (TX) |
| GND | — | GND |

**ESP32-S3 ↔ Teclado 4x4:** linhas GPIO4/5/6/7 (saídas), colunas GPIO9/10/11/12
(entradas pull-up).

## 11. Comportamento do teclado

| Tecla | Ação |
|---|---|
| 0–9 | acumula dígitos no buffer numérico (limite `TECLADO_MAX_DIGITOS`) |
| `*` | cancela/limpa a entrada (gera `EVENTO_TECLADO`) |
| `#` | confirma o número (gera `EVENTO_TECLADO` e atualiza o último comando) |
| `A` | muda o modo de operação |
| `B` | apaga o último dígito |
| `C` | limpa a entrada |
| `D` | força telemetria imediata |

Debounce por tempo (`TECLADO_DEBOUNCE_MS`) e exigência de soltar a tecla antes de
registrar a próxima evitam repetições acidentais. Feedback no Serial para tecla
pressionada, entrada atual, confirmação, cancelamento e operação inválida.

## 12. Comportamento do GNSS

- UART dedicada (`HardwareSerial`), leitura **não-bloqueante**, parser NMEA
  próprio (GGA/RMC/GST) com **validação de checksum** (sentenças corrompidas são
  descartadas).
- Coordenadas em inteiros escalados: **latitude/longitude × 1e7**, **altitude em
  mm**. Precisão horizontal (mm) via GST quando disponível.
- **Frescor dos dados**: cada fix registra `millis()`; dados acima de
  `GNSS_IDADE_MAX_MS` são marcados `DADOS_ANTIGOS`. Nunca reaproveitamos uma
  coordenada antiga como atual.
- Tipos de fix distintos: `SEM_FIX`, `STANDALONE`, `DIFERENCIAL`, `RTK_FLOAT`,
  `RTK_FIXED`. **Precisão RTK não é presumida** só por o UM980 estar conectado.

## 13. Configuração LoRa

Fonte única: `firmware/bibliotecas/protocolo_comunicacao/configuracao_radio.h`.

| Parâmetro | Valor |
|---|---|
| Frequência | 915 MHz |
| Largura de banda | 125 kHz |
| Spreading Factor | SF7 |
| Coding Rate | 4/5 |
| Preâmbulo | 8 símbolos |
| Potência TX | 20 dBm |
| Inversão IQ | desabilitada |

Transmissor e receptor **compilam com os mesmos parâmetros** (incluem o mesmo
header). Há `static_assert` de coerência. **Respeite a regulamentação local**
(no Brasil, 915 MHz está na faixa ISM 902–928 MHz) e a antena/variante da placa.

## 14. Protocolo de comunicação

Cabeçalho fixo de 10 bytes: `magic(0xAA55)`, versão, tipo, id_remetente,
sequência, tamanho_payload, flags; seguido do payload e de **CRC16-CCITT**.
Enquadramento UART por **COBS + delimitador 0x00**. Tipos: `TELEMETRIA_GNSS`,
`EVENTO_TECLADO`, `COMANDO`, `STATUS_TRANSMISSOR`, `CONFIRMACAO_UART`, `ERRO`,
`HEARTBEAT`. Detalhes e layout de bytes em `docs/PROTOCOLO.md`.

## 15. Como testar cada placa separadamente

1. **Controlador sem periféricos**: compile e grave `controlador_diagnostico`.
   Ele gera GNSS e teclado **simulados** (marcados `SIMULADO`) e transmite
   pacotes reais pela UART. Observe o Serial (115200).
2. **Transmissor isolado**: alimente-o e ligue a UART ao controlador (ou a um
   gerador de quadros). O OLED mostra estado da UART, sequência, estado de TX e
   contadores; o Serial imprime cada envio LoRa.
3. **Receptor isolado**: ligue-o e observe "AGUARDANDO 1º pacote" no OLED e o
   estado de `TIMEOUT` no Serial quando não há recepção.

## 16. Como testar a cadeia completa

1. Grave os três firmwares.
2. Ligue GNSS e teclado ao controlador; ligue a UART controlador↔Heltec TX.
3. Ligue a Heltec RX (mesma frequência).
4. No Serial do controlador, confirme telemetria periódica e eventos de teclado.
5. No Serial/OLED do receptor, confirme posição, tipo de fix, sequência,
   RSSI/SNR e o último comando de teclado.
6. Pressione `D` no teclado: deve aparecer uma telemetria forçada no receptor.

## 17. Saída esperada no Monitor Serial

- **Controlador**: blocos `----- DIAGNOSTICO CONTROLADOR -----` com estado, fix,
  satélites, lat/lon/alt, idade do GNSS, contadores do enlace e sequência; linhas
  `[TELEMETRIA]`, `[EVENTO]`, `[TECLADO]`.
- **Transmissor**: `----- DIAGNOSTICO TRANSMISSOR -----`, `[LoRa] enviado seq=…`.
- **Receptor**: blocos `===== PACOTE RECEBIDO =====` com tipo, seq, RSSI/SNR e os
  campos GNSS decodificados; `[PACOTE INVALIDO] …` e `[TIMEOUT] …` quando aplicável.

## 18. Saída esperada no OLED

- **Transmissor**: `UART: CONECTADO/SEM SINAL`, `Seq/Tipo`, `TX:estado ok/to`,
  quadros inválidos, idade do último pacote.
- **Receptor**: `OK RSSI/SNR` ou `TIMEOUT`, `Fix/Sat`, `La:`, `Lo:`, `Seq/Cmd`.

## 19. Solução de problemas (troubleshooting)

| Sintoma | Causa provável | Ação |
|---|---|---|
| UM980 não produz dados no Serial | RX/TX trocados, baud errado, sem GND comum | inverter RX/TX; conferir `GNSS_BAUD`; unir GNDs |
| UM980 tem dados mas sem fix | sem visão de céu, antena ruim, tempo insuficiente | levar a céu aberto; aguardar; verificar antena |
| Fica só em `STANDALONE` | sem correções RTCM/RTK, sem base | RTK exige correções válidas (ver item 24) |
| Teclas invertidas | linhas/colunas trocadas | ajustar pinos/mapa em `configuracao_hardware.h` |
| Teclas duplicadas | debounce insuficiente | aumentar `TECLADO_DEBOUNCE_MS` |
| Controlador não fala com a Heltec TX | UART trocada, baud diferente, sem GND | conferir 15↔RX/16↔TX; `LORA_BAUD`==`CONTROLADOR_BAUD`; GND |
| Heltec recebe UART mas não transmite | radio ocupado/timeout, frequência | ver contadores `to`/descartados; conferir rádio |
| Receptor não recebe nada | frequência/parâmetros diferentes, antena | mesma `configuracao_radio.h`; conferir antena |
| Receptor acusa CRC inválido | interferência, parâmetros divergentes, ruído | aproximar; conferir SF/BW/CR idênticos |
| OLED em branco | Vext desligado, endereço I2C, board errada | conferir `VextON`, 0x3c, board V3 |
| Placa reinicia ao transmitir | alimentação fraca (pico de TX) | fonte estável; capacitor de desacoplamento |
| Board Heltec errada selecionada | env/board incorreto | usar `heltec_wifi_lora_32_V3` |
| Frequência LoRa errada | valor fora da regulamentação/plano | ajustar `RADIO_FREQUENCIA_HZ` |
| RX e TX invertidos (UART) | fiação | TX de um lado no RX do outro |
| Falta de GND comum | referência de terra ausente | unir todos os GNDs |
| Só o receptor liga, sem dados | transmissor sem carga (UART do mestre off) | verificar cadeia completa |

## 20. Notas de segurança e alimentação

- Não alimente GNSS + Heltec + teclado por um regulador subdimensionado. O pico
  de corrente na transmissão LoRa pode causar reset. Use fonte estável e, se
  necessário, alimentações separadas com **GND comum**.
- Lógica **3,3 V** em todas as UARTs (ESP32-S3 e Heltec). Não aplique 5 V nos
  pinos de dados.
- As correntes exatas dependem das variantes de placa e não foram medidas aqui.

## 21. Limitações de RTK e precisão

Ver `docs/LIMITACOES_E_PREMISSAS.md`. Em resumo: RTK **fixed** normalmente exige
correções válidas (RTCM), antena adequada e boa instalação. O sistema **marca**
o tipo de fix real e não afirma precisão centimétrica sem RTK confirmado.

## 22. Como trocar os pinos

Edite **apenas** o `configuracao_hardware.h` de cada firmware. Todos os pinos
(GNSS, enlace UART, teclado) e baud rates estão centralizados lá, com validações
`static_assert` básicas.

## 23. Como trocar frequência e parâmetros de rádio

Edite **apenas** `firmware/bibliotecas/protocolo_comunicacao/configuracao_radio.h`.
Transmissor e receptor herdam automaticamente. As macros de placa Heltec ficam em
`build_flags` dos `platformio.ini` (`WIFI_LORA_32_V3`, `HELTEC_BOARD=30`,
`SLOW_CLK_TPYE=0`, `LoRaWAN_DEBUG_LEVEL=0`, `RADIO_CHIP_SX1262`,
`GPIO_PIN_COUNT=SOC_GPIO_PIN_COUNT`).

## 24. Como adicionar correções RTCM no futuro

O `ServicoGnss` expõe `injetarRtcm(dados, tamanho)`, que repassa bytes RTCM crus
ao UART do UM980 — **ponto de extensão** para RTK. Este projeto **não** inclui
NTRIP, endereço de caster, usuário/senha nem serviço de correção; a fonte de
RTCM (base própria ou serviço) deve ser adicionada externamente.
