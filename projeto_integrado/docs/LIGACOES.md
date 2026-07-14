# Ligações (wiring)

> **Todos os pinos do lado ESP32-S3 (mestre) são PREMISSAS** definidas em
> `firmware/controlador_principal_esp32_s3/src/configuracao_hardware.h`.
> Confirme fisicamente e ajuste em um único lugar. A pinagem de GNSS, LoRa e
> teclado foi definida pelo usuário; os pinos do cartão SD foram escolhidos por
> estarem livres (SPI2/FSPI nativo) e não colidirem com os demais.

## 1. ESP32-S3 (mestre) ↔ UM980

| ESP32-S3 | Direção | UM980 | Macro |
|---|---|---|---|
| GPIO43 (TX1) | → | RX | `GNSS_PINO_TX` |
| GPIO44 (RX1) | ← | TX | `GNSS_PINO_RX` |
| GND | — | GND | — |
| VCC | — | VCC (3,3 V ou 5 V, conforme o breakout) | — |
| (opcional) GPIO — | ← | PPS | `GNSS_PINO_PPS` (=-1, desativado) |

- UART dedicada `Serial1`, `GNSS_BAUD = 115200` (confirmar no seu UM980).
- **GND comum obrigatório.** Tensão de alimentação conforme a placa/breakout do
  UM980 — não presuma 5 V nem 3,3 V sem checar.

## 2. ESP32-S3 (mestre) ↔ Heltec TX (enlace UART full-duplex)

| ESP32-S3 | Direção | Heltec TX | Macro (lado Heltec) |
|---|---|---|---|
| GPIO18 (TX2) | → | GPIO44 (U0RXD/RX) | `CONTROLADOR_PINO_RX` |
| GPIO17 (RX2) | ← | GPIO43 (U0TXD/TX) | `CONTROLADOR_PINO_TX` |
| GND | — | GND | — |

- UART dedicada `Serial2` no mestre (`LORA_PINO_TX=18`, `LORA_PINO_RX=17`);
  `Serial1` remapeada para GPIO44/43 na Heltec.
- **Mesmo baud nos dois lados**: `LORA_BAUD` == `CONTROLADOR_BAUD` == 115200.
- Lógica **3,3 V** nos dois lados. Na Heltec LoRa, os pinos certos confirmados
  são `U0TXD=GPIO43` e `U0RXD=GPIO44`.
- GPIO1 é reservado para leitura de bateria na Heltec V3 confirmada. Não use
  GPIO1 como TX externo da Heltec.

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
| Linha 0 (1 2 3 A) | GPIO20 | `TECLADO_PINO_LINHA_0` |
| Linha 1 (4 5 6 B) | GPIO21 | `TECLADO_PINO_LINHA_1` |
| Linha 2 (7 8 9 C) | GPIO47 | `TECLADO_PINO_LINHA_2` |
| Linha 3 (* 0 # D) | GPIO48 | `TECLADO_PINO_LINHA_3` |
| Coluna 0 (1 4 7 *) | GPIO37 | `TECLADO_PINO_COLUNA_0` |
| Coluna 1 (2 5 8 0) | GPIO36 | `TECLADO_PINO_COLUNA_1` |
| Coluna 2 (3 6 9 #) | GPIO35 | `TECLADO_PINO_COLUNA_2` |
| Coluna 3 (A B C D) | GPIO0  | `TECLADO_PINO_COLUNA_3` |

- Linhas = **saídas** (nível HIGH em repouso, LOW durante a varredura da linha).
- Colunas = **entradas com `INPUT_PULLUP`**; tecla pressionada leva a coluna a LOW.
- **Atenção (pinagem do usuário)**: GPIO0 é pino de *strapping* e GPIO20 é
  compartilhado com o USB nativo em alguns módulos; GPIO48 costuma ser o LED RGB
  (`LED_BUILTIN`) no DevKitC-1. Confirme na sua placa e ajuste se necessário.

> Se as teclas aparecerem trocadas/transpostas, inverta linhas↔colunas ou ajuste
> o `mapa_` em `servico_teclado.cpp`.

## 4. ESP32-S3 (mestre) ↔ Cartão microSD (SPI2/FSPI)

Módulo microSD ligado **apenas ao controlador**. Barramento SPI2 (FSPI) nos
pinos IOMUX nativos do ESP32-S3, todos livres (sem colisão com GNSS 43/44, LoRa
17/18 e teclado). **PREMISSA — confirmar fisicamente.**

| ESP32-S3 | Direção | microSD | Macro |
|---|---|---|---|
| GPIO5 (SCK) | → | SCK/CLK | `PIN_SD_SCK` |
| GPIO7 (MOSI) | → | MOSI/DI | `PIN_SD_MOSI` |
| GPIO6 (MISO) | ← | MISO/DO | `PIN_SD_MISO` |
| GPIO15 (CS) | → | CS/SS | `PIN_SD_CS` |
| GPIO16 (opcional) | ← | card-detect (CD) | `PIN_SD_DETECT` (`SD_POSSUI_DETECCAO=0`) |
| 3V3 | — | VCC | alimentar em **3,3 V** |
| GND | — | GND | GND comum |

- `FREQUENCIA_SPI_SD_HZ = 20 MHz` (reduza para ~10 MHz se houver instabilidade de
  fiação/cartão). `SD_SPI_HOST = FSPI`. Ponto de montagem VFS: `PONTO_MONTAGEM_SD`.
- Use cartão **FAT32** (≤ 32 GB recomendado). O card-detect vem **desativado** por
  padrão; muitos módulos microSD não possuem esse contato.
- O cartão é **opcional**: sua ausência/falha não impede GNSS, teclado, UART ou
  LoRa. Para compilar sem SD: `pio run -e controlador_sem_sd`.

## 5. Recursos internos da Heltec V3 (fornecidos pela placa)

Definidos pela variante `WIFI_LORA_32_V3` (biblioteca Heltec) — **não trate
como pinos livres para periféricos externos**:

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
| Botão do usuário | 0 | reservado |
| Leitura de bateria | 1 | reservado; não usar como TX externo |
| Controle ADC | 37 | reservado |
| USB/UART U0TXD | 43 | TX correto da Heltec para o controlador |
| USB/UART U0RXD | 44 | RX correto da Heltec vindo do controlador |

> Os pinos acima são **internos da Heltec** e não têm relação com os pinos do
> cartão SD nesta arquitetura (que fica no ESP32-S3 controlador, outra placa).

Antena LoRa **sempre conectada** antes de transmitir (transmitir sem antena pode
danificar o SX1262).
