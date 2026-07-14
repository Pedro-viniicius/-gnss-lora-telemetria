# Ligações (wiring)

> **Todos os pinos do lado ESP32-S3 (mestre) são PREMISSAS** definidas em
> `firmware/controlador_principal_esp32_s3/src/configuracao_hardware.h`.
> Confirme fisicamente e ajuste em um único lugar. Os pinos escolhidos evitam
> strapping (0/3/45/46), USB (19/20) e flash/PSRAM do ESP32-S3.

## 1. ESP32-S3 (mestre) ↔ UM980

| ESP32-S3 | Direção | UM980 | Macro |
|---|---|---|---|
| GPIO17 (TX1) | → | RX | `GNSS_PINO_TX` |
| GPIO18 (RX1) | ← | TX | `GNSS_PINO_RX` |
| GND | — | GND | — |
| VCC | — | VCC (3,3 V ou 5 V, conforme o breakout) | — |
| (opcional) GPIO — | ← | PPS | `GNSS_PINO_PPS` (=-1, desativado) |

- UART dedicada `Serial1`, `GNSS_BAUD = 115200` (confirmar no seu UM980).
- **GND comum obrigatório.** Tensão de alimentação conforme a placa/breakout do
  UM980 — não presuma 5 V nem 3,3 V sem checar.

## 2. ESP32-S3 (mestre) ↔ Heltec TX (enlace UART full-duplex)

| ESP32-S3 | Direção | Heltec TX | Macro (lado Heltec) |
|---|---|---|---|
| GPIO15 (TX2) | → | GPIO5 (RX) | `CONTROLADOR_PINO_RX` |
| GPIO16 (RX2) | ← | GPIO6 (TX) | `CONTROLADOR_PINO_TX` |
| GND | — | GND | — |

- UART dedicada `Serial2` no mestre; `Serial1` (GPIO5/6) na Heltec.
- **Mesmo baud nos dois lados**: `LORA_BAUD` == `CONTROLADOR_BAUD` == 115200.
- Lógica **3,3 V** nos dois lados. GPIO5/6 são livres na Heltec V3.

## 3. ESP32-S3 (mestre) ↔ Teclado 4x4

Layout físico das teclas:

```
1 2 3 A
4 5 6 B
7 8 9 C
* 0 # D
```

| Função | GPIO | Macro |
|---|---|---|
| Linha 0 (1 2 3 A) | GPIO4 | `TECLADO_PINO_LINHA_0` |
| Linha 1 (4 5 6 B) | GPIO5 | `TECLADO_PINO_LINHA_1` |
| Linha 2 (7 8 9 C) | GPIO6 | `TECLADO_PINO_LINHA_2` |
| Linha 3 (* 0 # D) | GPIO7 | `TECLADO_PINO_LINHA_3` |
| Coluna 0 (1 4 7 *) | GPIO9 | `TECLADO_PINO_COLUNA_0` |
| Coluna 1 (2 5 8 0) | GPIO10 | `TECLADO_PINO_COLUNA_1` |
| Coluna 2 (3 6 9 #) | GPIO11 | `TECLADO_PINO_COLUNA_2` |
| Coluna 3 (A B C D) | GPIO12 | `TECLADO_PINO_COLUNA_3` |

- Linhas = **saídas** (nível HIGH em repouso, LOW durante a varredura da linha).
- Colunas = **entradas com `INPUT_PULLUP`**; tecla pressionada leva a coluna a LOW.
- Sem conflito com as UARTs do mestre (15/16/17/18) nem com USB/flash/PSRAM.

> Se as teclas aparecerem trocadas/transpostas, inverta linhas↔colunas ou ajuste
> o `mapa_` em `servico_teclado.cpp`.

## 4. Recursos internos da Heltec V3 (fornecidos pela placa)

Definidos pela variante `WIFI_LORA_32_V3` (biblioteca Heltec) — **não fiar**:

| Recurso | Pino | Observação |
|---|---|---|
| OLED SDA | 17 | `SDA_OLED` |
| OLED SCL | 18 | `SCL_OLED` |
| OLED RST | 21 | `RST_OLED` |
| Vext (alim. OLED/periféricos) | 36 | LOW = ligado |
| SX1262 NSS | 8 | `RADIO_NSS` |
| SX1262 SCK | 9 | — |
| SX1262 MOSI | 10 | — |
| SX1262 MISO | 11 | — |
| SX1262 RST | 12 | `RADIO_RESET` |
| SX1262 BUSY | 13 | `RADIO_BUSY` |
| SX1262 DIO1 | 14 | `RADIO_DIO_1` |
| LED onboard | 35 | — |

Antena LoRa **sempre conectada** antes de transmitir (transmitir sem antena pode
danificar o SX1262).
