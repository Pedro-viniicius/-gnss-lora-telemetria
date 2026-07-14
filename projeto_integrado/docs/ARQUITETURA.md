# Arquitetura

## Visão geral

Três firmwares independentes, uma biblioteca compartilhada e um enlace UART
full-duplex entre o mestre e a Heltec transmissora:

```
UM980 ──UART(NMEA)──▶ Controlador ESP32-S3 ──UART(COBS+CRC16)──▶ Heltec TX ──LoRa P2P──▶ Heltec RX
Teclado 4x4 ────────▶ (mestre)             ◀──STATUS/ACK────────
```

O **controlador é o mestre**: origina telemetria e comandos, e recebe de volta
apenas status/confirmações. Todos os laços são cooperativos, agendados por
`millis()`, sem `delay()` bloqueante (exceto assentamentos únicos no boot).

## Biblioteca compartilhada `protocolo_comunicacao` (C++ puro)

| Arquivo | Responsabilidade |
|---|---|
| `crc16.{h,cpp}` | CRC16-CCITT (0x1021, init 0xFFFF) |
| `cobs.{h,cpp}` | enquadramento COBS com verificação de capacidade |
| `tipos_gnss.h` | `TelemetriaGnss`, `TipoFix`, flags |
| `tipos_comandos.h` | `TipoMensagem`, `EventoTeclado`, `StatusTransmissor` |
| `configuracao_radio.h` | **fonte única** dos parâmetros LoRa |
| `protocolo.{h,cpp}` | (des)serialização, encode/decode, enquadramento UART |

É compilada tanto pelos firmwares quanto pelos testes de host (g++), garantindo
lógica idêntica em todas as placas.

## Controlador principal (ESP32-S3) — módulos

| Módulo | Responsabilidade |
|---|---|
| `configuracao_hardware.h` | pinos/baud (UART GNSS, UART LoRa, teclado) |
| `servico_gnss` | UART GNSS, parser NMEA, frescor de dados, simulação |
| `servico_teclado` | varredura 4x4, debounce, buffer numérico, eventos |
| `servico_uart_lora` | envio/recepção COBS, status, saúde do enlace |
| `gerenciador_telemetria` | monta telemetria, contador de sequência global |
| `maquina_estados` | deriva o estado operacional, sem reboot por falha |
| `diagnostico` | impressão periódica pelo USB |
| `main.cpp` | scheduler cooperativo |

**Estados**: `INICIALIZANDO`, `OPERACIONAL`, `GNSS_SEM_DADOS`, `GNSS_SEM_FIX`,
`UART_LORA_DESCONECTADA`. O sistema continua lendo o teclado sem fix, continua o
GNSS com LoRa indisponível, marca dados antigos e não entra em ciclo de reboot.

## Transmissor LoRa (Heltec V3) — módulos

| Módulo | Responsabilidade |
|---|---|
| `configuracao_hardware.h` | UART com o controlador (GPIO5/6) e temporizações |
| `receptor_uart` | desenquadra COBS, valida, fila de pacotes, contadores |
| `transmissor_lora` | radio SX1262 (LoRaWan_APP.h), TX não-bloqueante, watchdog |
| `display_oled` | OLED com estado da UART/TX/erros |
| `diagnostico` | contadores pelo USB |
| `main.cpp` | encaminha pacotes → LoRa, devolve STATUS/ACK |

Regras de robustez: nunca sobrescreve uma transmissão ativa (`enviar()` recusa
enquanto ocupado, contando descarte); watchdog libera o rádio após
`TX_WATCHDOG_MS` sem callback.

## Receptor LoRa (Heltec V3) — módulos

| Módulo | Responsabilidade |
|---|---|
| `configuracao_hardware.h` | timeouts e temporizações |
| `receptor_lora` | Rx contínuo, `OnRxDone` com cópia limitada, re-arma Rx |
| `decodificador_pacotes` | validação + duplicata/faltante + contadores |
| `display_oled` | OLED com posição/fix/RSSI/SNR/timeout |
| `diagnostico` | impressão detalhada do pacote decodificado |
| `main.cpp` | orquestra recepção → decodificação → exibição |

**Correção do bug da referência**: `OnRxDone` limita a cópia à capacidade do
buffer antes de qualquer escrita (a referência fazia `rxpacket[size]='\0'` sem
checar, arriscando estouro). Usa-se buffer binário, sem terminador.

## Fluxo de sequência

O contador de sequência é global no controlador (telemetria, eventos e
heartbeat compartilham o mesmo espaço). O receptor detecta **duplicatas**
(sequência repetida) e **faltantes** (salto), tratando o rollover 65535→0 como
transição normal.
