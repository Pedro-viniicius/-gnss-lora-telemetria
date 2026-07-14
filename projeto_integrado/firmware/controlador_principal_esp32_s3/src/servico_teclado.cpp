// servico_teclado.cpp
#include "servico_teclado.h"

#include <stdlib.h>
#include <string.h>

using protocolo::EventoTeclado;

ServicoTeclado::ServicoTeclado()
    : pinos_linha_{TECLADO_PINO_LINHA_0, TECLADO_PINO_LINHA_1,
                   TECLADO_PINO_LINHA_2, TECLADO_PINO_LINHA_3},
      pinos_coluna_{TECLADO_PINO_COLUNA_0, TECLADO_PINO_COLUNA_1,
                    TECLADO_PINO_COLUNA_2, TECLADO_PINO_COLUNA_3},
      mapa_{{'1', '2', '3', 'A'},
            {'4', '5', '6', 'B'},
            {'7', '8', '9', 'C'},
            {'*', '0', '#', 'D'}},
      buffer_len_(0),
      modo_atual_(0),
      ultimo_comando_(0),
      tecla_bruta_(0),
      tecla_estavel_(0),
      instante_mudanca_ms_(0),
      fila_ini_(0),
      fila_fim_(0),
      fila_qtd_(0) {
  buffer_[0] = '\0';
}

void ServicoTeclado::iniciar() {
  for (uint8_t l = 0; l < TECLADO_LINHAS; ++l) {
    pinMode(pinos_linha_[l], OUTPUT);
    digitalWrite(pinos_linha_[l], HIGH);
  }
  for (uint8_t c = 0; c < TECLADO_COLUNAS; ++c) {
    pinMode(pinos_coluna_[c], INPUT_PULLUP);
  }
}

char ServicoTeclado::varrer() {
  char pressionada = 0;
  for (uint8_t l = 0; l < TECLADO_LINHAS; ++l) {
    digitalWrite(pinos_linha_[l], LOW);
    // Pequeno tempo de assentamento eletrico sem bloquear a aplicacao.
    delayMicroseconds(5);
    for (uint8_t c = 0; c < TECLADO_COLUNAS; ++c) {
      if (digitalRead(pinos_coluna_[c]) == LOW) {
        pressionada = mapa_[l][c];
      }
    }
    digitalWrite(pinos_linha_[l], HIGH);
    if (pressionada != 0) {
      break;  // primeira tecla detectada basta
    }
  }
  return pressionada;
}

void ServicoTeclado::atualizar() {
  char lida = varrer();

  if (lida != tecla_bruta_) {
    tecla_bruta_ = lida;
    instante_mudanca_ms_ = millis();
    return;
  }

  // Estavel por tempo suficiente?
  if ((uint32_t)(millis() - instante_mudanca_ms_) < TECLADO_DEBOUNCE_MS) {
    return;
  }

  if (lida != tecla_estavel_) {
    tecla_estavel_ = lida;
    if (lida != 0) {
      tratarTecla(lida);  // borda de descida (nova tecla confirmada)
    }
  }
}

void ServicoTeclado::injetarTecla(char tecla) {
  if (tecla != 0) {
    tratarTecla(tecla);
  }
}

void ServicoTeclado::tratarTecla(char tecla) {
  Serial.print(F("[TECLADO] Tecla: "));
  Serial.println(tecla);

  if (tecla >= '0' && tecla <= '9') {
    if (buffer_len_ < TECLADO_MAX_DIGITOS) {
      buffer_[buffer_len_++] = tecla;
      buffer_[buffer_len_] = '\0';
      Serial.print(F("[TECLADO] Entrada: "));
      Serial.println(buffer_);
    } else {
      Serial.println(F("[TECLADO] Operacao invalida: buffer cheio"));
    }
    return;
  }

  switch (tecla) {
    case '*':  // cancelar/limpar
      buffer_len_ = 0;
      buffer_[0] = '\0';
      Serial.println(F("[TECLADO] Entrada cancelada"));
      enfileirar('*', protocolo::EVT_CANCELAMENTO, 0, 0);
      break;

    case '#': {  // confirmar
      if (buffer_len_ == 0) {
        Serial.println(F("[TECLADO] Operacao invalida: nada a confirmar"));
        break;
      }
      uint32_t valor = (uint32_t)strtoul(buffer_, nullptr, 10);
      ultimo_comando_ = (uint8_t)buffer_[buffer_len_ - 1];
      Serial.print(F("[TECLADO] Comando confirmado: "));
      Serial.println(valor);
      enfileirar('#', protocolo::EVT_CONFIRMACAO_NUMERO, valor, buffer_len_);
      buffer_len_ = 0;
      buffer_[0] = '\0';
      break;
    }

    case 'A':  // mudar modo
      modo_atual_ = (uint8_t)((modo_atual_ + 1) % QTD_MODOS);
      Serial.print(F("[TECLADO] Modo alterado para: "));
      Serial.println(modo_atual_);
      enfileirar('A', protocolo::EVT_MUDANCA_MODO, modo_atual_, 0);
      break;

    case 'B':  // apagar ultimo digito
      if (buffer_len_ > 0) {
        buffer_[--buffer_len_] = '\0';
        Serial.print(F("[TECLADO] Backspace, entrada: "));
        Serial.println(buffer_);
      } else {
        Serial.println(F("[TECLADO] Operacao invalida: buffer vazio"));
      }
      break;

    case 'C':  // limpar
      buffer_len_ = 0;
      buffer_[0] = '\0';
      Serial.println(F("[TECLADO] Entrada limpa"));
      break;

    case 'D':  // forcar telemetria
      Serial.println(F("[TECLADO] Telemetria forcada"));
      enfileirar('D', protocolo::EVT_TELEMETRIA_FORCADA, 0, 0);
      break;

    default:
      Serial.println(F("[TECLADO] Operacao invalida: tecla desconhecida"));
      break;
  }
}

void ServicoTeclado::enfileirar(char tecla, uint8_t tipoEvento, uint32_t valor,
                                uint8_t tamEntrada) {
  EventoTeclado e;
  e.uptime_ms = millis();
  e.tecla = (uint8_t)tecla;
  e.tipo_evento = tipoEvento;
  e.valor_numerico = valor;
  e.tamanho_entrada = tamEntrada;

  if (fila_qtd_ >= CAP_FILA) {
    // Fila cheia: descarta o evento mais antigo para nao travar.
    fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_FILA);
    --fila_qtd_;
  }
  fila_[fila_fim_] = e;
  fila_fim_ = (uint8_t)((fila_fim_ + 1) % CAP_FILA);
  ++fila_qtd_;
}

bool ServicoTeclado::temEvento() const {
  return fila_qtd_ > 0;
}

bool ServicoTeclado::obterEvento(EventoTeclado& saida) {
  if (fila_qtd_ == 0) {
    return false;
  }
  saida = fila_[fila_ini_];
  fila_ini_ = (uint8_t)((fila_ini_ + 1) % CAP_FILA);
  --fila_qtd_;
  return true;
}
