# Arquitetura

## Visão geral

Três firmwares independentes, uma biblioteca compartilhada e um enlace UART
full-duplex entre o mestre e a Heltec transmissora:

```
UM980 ──UART(NMEA)──▶ Controlador ESP32-S3 ──UART(COBS+CRC16)──▶ Heltec TX ──LoRa P2P──▶ Heltec RX
Teclado 4x4 ────────▶ (mestre)             ◀──STATUS/ACK────────
microSD ──SPI2/FSPI◀▶ (log CSV local, opcional e tolerante a falha)
```

O **controlador é o mestre**: origina telemetria e comandos, e recebe de volta
apenas status/confirmações. Todos os laços são cooperativos, agendados por
`millis()`, sem `delay()` bloqueante (exceto assentamentos únicos no boot).
O cartão microSD fica **somente no controlador** e não é requisito para GNSS,
teclado, UART ou LoRa: se falhar, o sistema continua transmitindo.

## Biblioteca compartilhada `protocolo_comunicacao` (C++ puro)

| Arquivo | Responsabilidade |
|---|---|
| `crc16.{h,cpp}` | CRC16-CCITT (0x1021, init 0xFFFF) |
| `cobs.{h,cpp}` | enquadramento COBS com verificação de capacidade |
| `tipos_gnss.h` | `TelemetriaGnss`, `TipoFix`, flags |
| `tipos_comandos.h` | `TipoMensagem`, `EventoTeclado`, `StatusTransmissor`, `StatusArmazenamento` |
| `configuracao_radio.h` | **fonte única** dos parâmetros LoRa |
| `protocolo.{h,cpp}` | (des)serialização, encode/decode, enquadramento UART |
| `formato_registro.{h,cpp}` | formatação CSV de telemetria/eventos para o microSD |

É compilada tanto pelos firmwares quanto pelos testes de host (g++), garantindo
lógica idêntica em todas as placas.

## Controlador principal (ESP32-S3) — módulos

| Módulo | Responsabilidade |
|---|---|
| `configuracao_hardware.h` | pinos/baud (UART GNSS, UART LoRa, teclado, microSD SPI) |
| `servico_gnss` | UART GNSS, parser NMEA, frescor de dados, simulação |
| `servico_teclado` | varredura 4x4, debounce, buffer numérico, eventos |
| `servico_uart_lora` | envio/recepção COBS, status, saúde do enlace |
| `servico_cartao_sd` | log CSV em microSD, buffer em RAM, flush limitado, recuperação |
| `gerenciador_telemetria` | monta telemetria, contador de sequência global |
| `maquina_estados` | deriva o estado operacional, sem reboot por falha |
| `diagnostico` | impressão periódica pelo USB |
| `main.cpp` | scheduler cooperativo |

**Estados**: `INICIALIZANDO`, `OPERACIONAL`, `GNSS_SEM_DADOS`, `GNSS_SEM_FIX`,
`UART_LORA_DESCONECTADA`. O sistema continua lendo o teclado sem fix, continua o
GNSS com LoRa indisponível, marca dados antigos e não entra em ciclo de reboot.

### Armazenamento local no controlador

O `servico_cartao_sd` usa `SD.h` + `SPI.h` do core Arduino-ESP32 com `SPIClass`
no host `FSPI`. As chamadas `registrarTelemetria()` e `registrarEvento()` apenas
formatam linhas CSV e as colocam em uma fila circular de RAM; a escrita real no
cartão ocorre em `atualizar()`, em blocos limitados por `LINHAS_POR_FLUSH` e por
`INTERVALO_FLUSH_SD_MS`.

Falhas de cartão são informativas: estados `SEM_CARTAO`, `CHEIO` e `FALHA` não
alteram a máquina de estados principal. O controlador envia periodicamente
`STATUS_ARMAZENAMENTO` por LoRa e também embute flags `SD_ATIVO`/`SD_FALHA` na
telemetria GNSS. O ambiente `controlador_sem_sd` compila o mesmo firmware com
`HABILITAR_CARTAO_SD=0`.

## Transmissor LoRa (Heltec V3) — módulos

| Módulo | Responsabilidade |
|---|---|
| `configuracao_hardware.h` | mapa interno Heltec V3, UART com o controlador (`U0RXD=GPIO44`, `U0TXD=GPIO43`) e temporizações |
| `receptor_uart` | desenquadra COBS, valida, fila de pacotes, contadores |
| `transmissor_lora` | radio SX1262 (LoRaWan_APP.h), TX não-bloqueante, watchdog |
| `display_oled` | OLED com a última telemetria GPS recebida do controlador e resumo UART/TX |
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

O contador de sequência é global no controlador (telemetria, eventos, heartbeat
e status de armazenamento compartilham o mesmo espaço). O receptor detecta **duplicatas**
(sequência repetida) e **faltantes** (salto), tratando o rollover 65535→0 como
transição normal.
