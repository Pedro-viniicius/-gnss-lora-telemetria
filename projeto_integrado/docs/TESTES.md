# Testes

## Testes de host do protocolo (g++)

Compilam os **mesmos** `.cpp` usados no firmware, garantindo lógica idêntica.

```bash
cd testes/protocolo
make test        # compila e executa
make limpar      # remove o binário
```

Casos cobertos (73 asserções, resultado atual: **73/73 PASS**):

| Grupo | Casos |
|---|---|
| CRC16 | vetor `"123456789"`→0x29B1; init 0xFFFF; incremental == bloco |
| COBS | round-trip (zero no meio, só zeros, sem zeros); bloco longo (300 B); destino pequeno → falha |
| Pacote | round-trip encode/decode; magic; sequência/tipo/id preservados |
| Mínimo/Máximo | payload 0 e payload = `MAX_PAYLOAD` |
| Rejeições | muito curto, truncado, magic inválido, versão inválida, CRC inválido, tamanho inválido, tipo desconhecido (estrito vs. não-estrito) |
| Telemetria GNSS | round-trip com lat/lon negativas, altitude, fix, satélites, precisão |
| Evento de teclado | round-trip com valor no limite de uint32 |
| Status de armazenamento | round-trip de `STATUS_ARMAZENAMENTO`, aceito em modo estrito |
| CSV do microSD | cabeçalho, linha de telemetria, linha de evento, falha por buffer pequeno |
| UART | enquadramento COBS + delimitador + desenquadramento |
| Sequência | duplicada, +1 normal, salto (faltante), rollover 65535→0 |

## Compilação dos firmwares (PlatformIO)

```bash
cd firmware/controlador_principal_esp32_s3 && pio run -e controlador
cd firmware/controlador_principal_esp32_s3 && pio run -e controlador_diagnostico
cd firmware/controlador_principal_esp32_s3 && pio run -e controlador_sem_sd
cd firmware/transmissor_lora_heltec        && pio run -e transmissor
cd firmware/receptor_lora_heltec           && pio run -e receptor
```

Resultado atual: **os cinco ambientes compilam com [SUCCESS]** (ver tamanhos no
README/relatório).

## Modo de diagnóstico (sem periféricos)

- **Controlador**: `pio run -e controlador_diagnostico` habilita GNSS e teclado
  **simulados**. Os dados GNSS sintéticos são marcados com a flag
  `DADOS_SIMULADOS` e aparecem como `[SIMULADO]` no Serial e `ATENCAO: DADOS
  SIMULADOS` no receptor — **não podem ser confundidos com GNSS real**.
- O controlador ainda gera pacotes reais na UART, permitindo testar a cadeia sem
  UM980 nem teclado físicos.
- **Controlador sem SD**: `pio run -e controlador_sem_sd` compila com
  `HABILITAR_CARTAO_SD=0`. O firmware continua gerando GNSS/teclado/UART/LoRa e
  o status de armazenamento fica `DESABILITADO`.

## Verificações estáticas realizadas

- Sem `strcpy`/`strcat`/`sprintf`/`gets` inseguros.
- Sem placeholders (`TODO`/`[...]`/"restante do código") de código.
- Sem credenciais (NTRIP/caster/senha/token).
- Parâmetros de rádio não duplicados (apenas em `configuracao_radio.h`).
- `delay()` apenas em assentamentos únicos de boot (nenhum em laço/caminho crítico).
- Cálculos de tempo com subtração `unsigned` (seguros a rollover de `millis()`).
- Pinos do microSD (12/13/11/10/14) não colidem com GNSS, UART LoRa nem teclado;
  há `static_assert` em `configuracao_hardware.h`.

## Roteiro de teste em bancada (com hardware)

1. Grave os três firmwares (`-t upload`).
2. Abra o monitor serial de cada placa (`pio device monitor -b 115200`).
3. Sem GNSS: use `controlador_diagnostico` — confirme telemetria simulada
   chegando ao receptor.
4. Com GNSS: confirme transição `GNSS_SEM_DADOS` → `GNSS_SEM_FIX` →
   `OPERACIONAL` conforme o fix melhora.
5. Teclado: digite `123#` e confirme o `EVENTO_TECLADO` no receptor; `D` força
   telemetria; `*`/`C` limpam; `A` muda o modo.
6. microSD: com cartão FAT32 inserido, confirme no Serial `[SD] Cartao montado`
   e arquivos `LOG0001.CSV` com cabeçalho + linhas `TELEMETRIA`/`EVENTO`.
7. Remova o cartão durante a operação: o Serial deve marcar `FALHA`/`SEM_CARTAO`
   e o receptor deve receber `STATUS_ARMAZENAMENTO`, enquanto GNSS/LoRa continuam.
8. Desligue a UART: confirme `UART_LORA_DESCONECTADA` e recuperação ao religar.
