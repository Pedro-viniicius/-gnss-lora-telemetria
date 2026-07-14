// pinos_heltec_v3.h
// -----------------------------------------------------------------------------
// Mapa centralizado dos pinos internos confirmados da Heltec WiFi LoRa 32 V3.
// Estes pinos pertencem aos recursos embarcados da propria Heltec e nao devem
// ser reutilizados para UART externa, GNSS, teclado, microSD, display externo
// ou qualquer outro periferico.
// -----------------------------------------------------------------------------
#ifndef PINOS_HELTEC_V3_H
#define PINOS_HELTEC_V3_H

// Radio LoRa SX1262 integrado. Uso exclusivo do radio.
constexpr int PIN_LORA_NSS  = 8;
constexpr int PIN_LORA_SCK  = 9;
constexpr int PIN_LORA_MOSI = 10;
constexpr int PIN_LORA_MISO = 11;
constexpr int PIN_LORA_RST  = 12;
constexpr int PIN_LORA_BUSY = 13;
constexpr int PIN_LORA_DIO1 = 14;

// OLED integrado. Reservado quando o OLED estiver habilitado.
constexpr int PIN_OLED_SDA = 17;
constexpr int PIN_OLED_SCL = 18;
constexpr int PIN_OLED_RST = 21;

// Outros recursos internos da placa.
constexpr int PIN_VEXT_CONTROLE   = 36;
constexpr int PIN_LED_CONTROLE    = 35;
constexpr int PIN_BOTAO_USUARIO   = 0;
constexpr int PIN_LEITURA_BATERIA = 1;
constexpr int PIN_CONTROLE_ADC    = 37;

// Interface USB/UART confirmada da placa Heltec WiFi LoRa 32 V3.
constexpr int PIN_UART_USB_TX = 43;
constexpr int PIN_UART_USB_RX = 44;

constexpr bool pinoReservadoRadioHeltec(int pino) {
  return pino == PIN_LORA_NSS || pino == PIN_LORA_SCK ||
         pino == PIN_LORA_MOSI || pino == PIN_LORA_MISO ||
         pino == PIN_LORA_RST || pino == PIN_LORA_BUSY ||
         pino == PIN_LORA_DIO1;
}

constexpr bool pinoReservadoOledHeltec(int pino) {
  return pino == PIN_OLED_SDA || pino == PIN_OLED_SCL ||
         pino == PIN_OLED_RST;
}

constexpr bool pinoReservadoSistemaHeltec(int pino) {
  return pino == PIN_VEXT_CONTROLE || pino == PIN_LED_CONTROLE ||
         pino == PIN_BOTAO_USUARIO || pino == PIN_LEITURA_BATERIA ||
         pino == PIN_CONTROLE_ADC;
}

constexpr bool pinoReservadoUsbUartHeltec(int pino) {
  return pino == PIN_UART_USB_TX || pino == PIN_UART_USB_RX;
}

constexpr bool pinoReservadoHeltecV3ComOled(int pino) {
  return pinoReservadoRadioHeltec(pino) || pinoReservadoOledHeltec(pino) ||
         pinoReservadoSistemaHeltec(pino) || pinoReservadoUsbUartHeltec(pino);
}

#endif  // PINOS_HELTEC_V3_H
