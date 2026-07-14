# Protocolo de comunicação

Protocolo binário versionado, compartilhado pelos três firmwares. Substitui a
string frágil `"T:25.0 H:50.0"` da referência.

## Enquadramento UART

- **COBS** (Consistent Overhead Byte Stuffing) + **delimitador `0x00`**.
- O COBS remove todos os `0x00` do interior do pacote, permitindo usar `0x00`
  como fronteira de quadro inequívoca. Codificação/decodificação com verificação
  rígida de capacidade de destino (nunca estoura).
- No LoRa, o **mesmo pacote binário** (sem COBS) é a carga útil.

## Cabeçalho (10 bytes, little-endian)

| Offset | Tam | Campo |
|---|---|---|
| 0 | 1 | `magic[0]` = `0xAA` |
| 1 | 1 | `magic[1]` = `0x55` |
| 2 | 1 | `versao` (=1) |
| 3 | 1 | `tipo` (ver abaixo) |
| 4 | 1 | `id_remetente` |
| 5 | 2 | `sequencia` (uint16) |
| 7 | 2 | `tamanho_payload` (uint16) |
| 9 | 1 | `flags` |
| 10 | N | `payload` |
| 10+N | 2 | `crc16` (CRC16-CCITT sobre os bytes 0..10+N-1) |

- `MAX_PAYLOAD` = 64 bytes → `TAMANHO_MAXIMO_PACOTE` = 76 bytes.
- Serialização **explícita byte-a-byte** (não depende de padding/alinhamento/
  endianness; nunca transmite struct cru).

## CRC

- **CRC16-CCITT** ("FALSE"): polinômio `0x1021`, valor inicial `0xFFFF`, sem
  reflexão, sem XOR final. Vetor de teste: `"123456789"` → `0x29B1`.
- Cobre cabeçalho + payload; validado antes de aceitar qualquer pacote.

## Números de sequência

- Contador global de 16 bits no controlador (telemetria, eventos e heartbeat
  compartilham o mesmo espaço). Rollover 65535→0 é tratado como transição normal.
- O receptor detecta **duplicatas** (sequência igual à anterior) e **faltantes**
  (salto de sequência), mantendo contadores.

## Tipos de mensagem

| Valor | Tipo | Sentido |
|---|---|---|
| 0x01 | `TELEMETRIA_GNSS` | controlador → LoRa |
| 0x02 | `EVENTO_TECLADO` | controlador → LoRa |
| 0x03 | `COMANDO` | reservado |
| 0x04 | `STATUS_TRANSMISSOR` | Heltec TX → controlador (UART) |
| 0x05 | `CONFIRMACAO_UART` | Heltec TX → controlador (UART, ACK) |
| 0x06 | `ERRO` | reservado |
| 0x07 | `HEARTBEAT` | controlador → LoRa |
| 0x08 | `STATUS_ARMAZENAMENTO` | controlador → LoRa |

## Payload — TELEMETRIA_GNSS (22 bytes)

| Offset | Tam | Campo |
|---|---|---|
| 0 | 4 | `uptime_ms` (uint32) |
| 4 | 4 | `latitude_1e7` (int32, graus×1e7) |
| 8 | 4 | `longitude_1e7` (int32, graus×1e7) |
| 12 | 4 | `altitude_mm` (int32) |
| 16 | 1 | `tipo_fix` (0..4) |
| 17 | 1 | `satelites` |
| 18 | 2 | `precisao_horizontal_mm` (uint16) |
| 20 | 1 | `ultimo_comando_teclado` (ASCII) |
| 21 | 1 | `flags` (GNSS) |

`TipoFix`: 0=SEM_FIX, 1=STANDALONE, 2=DIFERENCIAL, 3=RTK_FLOAT, 4=RTK_FIXED.
Flags GNSS: `DADOS_SIMULADOS(0x01)`, `DADOS_ANTIGOS(0x02)`, `TEM_ALTITUDE(0x04)`,
`TEM_PRECISAO(0x08)`, `SD_ATIVO(0x10)`, `SD_FALHA(0x20)`.

As flags de SD indicam apenas o estado do armazenamento local no controlador.
Elas não bloqueiam a aceitação da telemetria: `SD_FALHA` significa cartão
ausente, cheio ou com erro de I/O, mas GNSS/teclado/UART/LoRa seguem operando.

## Payload — EVENTO_TECLADO (11 bytes)

| Offset | Tam | Campo |
|---|---|---|
| 0 | 4 | `uptime_ms` |
| 4 | 1 | `tecla` (ASCII) |
| 5 | 1 | `tipo_evento` (0=confirmação, 1=modo, 2=telemetria forçada, 3=cancelamento) |
| 6 | 4 | `valor_numerico` (uint32) |
| 10 | 1 | `tamanho_entrada` |

## Payload — STATUS_TRANSMISSOR (27 bytes)

`uptime_ms`, `quadros_uart_validos`, `quadros_uart_invalidos`,
`transmissoes_lora`, `timeouts_lora`, `pacotes_descartados` (uint32 cada),
`ultima_sequencia` (uint16), `estado_tx` (uint8).

## Payload — STATUS_ARMAZENAMENTO (22 bytes)

| Offset | Tam | Campo |
|---|---|---|
| 0 | 4 | `uptime_ms` (uint32) |
| 4 | 1 | `estado` (`EstadoArmazenamento`) |
| 5 | 1 | `presente` (1=cartão montado/presente, 0=não) |
| 6 | 4 | `kb_livres` (uint32) |
| 10 | 4 | `arquivos_escritos` (uint32) |
| 14 | 4 | `bytes_escritos` (uint32) |
| 18 | 4 | `registros_descartados` (uint32) |

`EstadoArmazenamento`: 0=`DESABILITADO`, 1=`PRONTO`, 2=`SEM_CARTAO`,
3=`CHEIO`, 4=`FALHA`.

## Formato CSV no microSD

O CSV é gravado apenas no controlador, em arquivos `LOG0001.CSV`,
`LOG0002.CSV` etc. Separador: `;`.

Cabeçalho:

```text
tipo;uptime_ms;sequencia;latitude;longitude;altitude_m;fix;satelites;precisao_mm;comando_tecla;valor_ou_flags
```

Linhas `TELEMETRIA` carregam os campos GNSS e usam `valor_ou_flags` para as
flags da telemetria. Linhas `EVENTO` deixam campos GNSS vazios e usam
`comando_tecla`/`valor_ou_flags` para o evento do teclado. A formatação é feita
por `formato_registro.{h,cpp}`, sem `String` e com verificação de capacidade via
`snprintf`.

## Regras de rejeição do decodificador

O decodificador rejeita, com resultado específico: pacote menor que o mínimo,
`magic` inválido, versão desconhecida, `tamanho_payload` impossível (>
`MAX_PAYLOAD`), payload truncado, CRC inválido e — em modo estrito — tipo
desconhecido.

## Comportamento de timeout

- **Controlador ↔ Heltec (UART)**: sem quadro válido por `LORA_TIMEOUT_MS` →
  enlace "desconectado" (estado `UART_LORA_DESCONECTADA`).
- **Receptor (LoRa)**: sem pacote por `RECEPTOR_TIMEOUT_MS` → estado de
  `TIMEOUT` no OLED e aviso periódico no Serial.
- **Transmissor (rádio)**: ocupado por mais de `TX_WATCHDOG_MS` sem callback →
  watchdog libera o rádio e conta um timeout.
- **microSD (controlador)**: flush a cada `INTERVALO_FLUSH_SD_MS`; verificação
  de saúde/remontagem a cada `INTERVALO_VERIFICACAO_SD_MS`; status enviado por
  `STATUS_ARMAZENAMENTO` a cada `PERIODO_STATUS_ARMAZENAMENTO_MS`.

## Funções testáveis no host

`calcularCrc16`, `codificarPacote`, `decodificarPacote`, `cobsEncode`,
`cobsDecode`, `enquadrarParaUart`, `desenquadrarDeUart`, `montarPacoteTelemetria`
/`lerTelemetria`, `montarPacoteEventoTeclado`/`lerEventoTeclado`,
`montarPacoteStatusArmazenamento`/`lerStatusArmazenamento`,
`formatarLinhaTelemetriaCsv` e `formatarLinhaEventoCsv`. Ver `docs/TESTES.md`.
