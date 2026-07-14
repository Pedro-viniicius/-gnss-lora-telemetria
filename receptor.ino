// --- RECEPTOR (RECEIVER) LoRa + OLED (HT_SSD1306Wire) ---

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h" // TU librería OLED para Heltec

// -------------------------------------------------------------------
// CONFIGURACIÓN LO-RA (API de bajo nivel)
// -------------------------------------------------------------------
#define RF_FREQUENCY 915000000 // Hz
#define LORA_BANDWIDTH 0 // 125 kHz
#define LORA_SPREADING_FACTOR 7 // SF7
#define LORA_CODINGRATE 1 // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 30

// -------------------------------------------------------------------
// CONFIGURACIÓN OLED (Tus definiciones)
// -------------------------------------------------------------------
// Definición de OLED
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); 

// -------------------------------------------------------------------
// VARIABLES GLOBALES
// -------------------------------------------------------------------
char rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
int16_t rxSize;
int16_t rssi = 0; // Inicializado para evitar problemas
bool lora_idle = true;

// Variables de datos y estado
float receivedTemp = 0.0;
float receivedHum = 0.0;
unsigned long lastReceivedTime = 0;
const long timeout = 10000; // Si no hay datos en 10s, muestra "ESPERANDO"

// --- DECLARACIONES DE EVENTOS y FUNCIONES ---
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi_val, int8_t snr );
void parseData(String data);
void VextON(void);
void VextOFF(void);
void mostrarDatos(float temp, float hum, bool isReceiving);


void setup() {
    Serial.begin(115200);
    delay(100);
    
    // Inicialización de la MCU y OLED (Usando tu lógica)
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE); 
    VextON();
    delay(100);
    display.init();
    display.setContrast(255);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    
    // Configuración de eventos de radio
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    
    // Configuración de la radio con la API de bajo nivel
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
                       
    mostrarDatos(receivedTemp, receivedHum, false); // Muestra estado inicial
}


void loop()
{
    if(lora_idle)
    {
        lora_idle = false;
        Serial.println("into RX mode");
        Radio.Rx(0); // Pone la radio en modo Recepción infinita
    }
    
    // Procesa interrupciones de radio
    Radio.IrqProcess( );
    
    // Comprobación de timeout: si pasó mucho tiempo desde la última recepción
    if (lastReceivedTime != 0 && millis() - lastReceivedTime > timeout) {
        mostrarDatos(receivedTemp, receivedHum, false); 
    }
    
    delay(10); 
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi_val, int8_t snr )
{
    rssi=rssi_val; // Usa el valor RSSI recibido
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0'; // Asegura que la cadena esté terminada
    Radio.Sleep( );
    
    Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
    
    // Procesa los datos y actualiza la pantalla
    parseData(String(rxpacket));
    lastReceivedTime = millis(); 
    mostrarDatos(receivedTemp, receivedHum, true);
    
    lora_idle = true;
}

// Funciones de control de energía Vext
void VextON(void) {
 pinMode(Vext, OUTPUT);
 digitalWrite(Vext, LOW);
}

void VextOFF(void) {
 pinMode(Vext, OUTPUT);
 digitalWrite(Vext, HIGH);
}

// Función para analizar la cadena "T:XX.X H:YY.Y"
void parseData(String data) {
  int tIndex = data.indexOf("T:");
  int hIndex = data.indexOf("H:");

  if (tIndex != -1 && hIndex != -1) {
    // Extrae la temperatura (desde T:+2 hasta H:-1)
    String tempStr = data.substring(tIndex + 2, hIndex - 1);
    // Extrae la humedad (desde H:+2 hasta el final)
    String humStr = data.substring(hIndex + 2);

    receivedTemp = tempStr.toFloat();
    receivedHum = humStr.toFloat();
  }
}

// Función unificada para mostrar datos y estado en OLED (Adaptada de tu código DHT)
void mostrarDatos(float temp, float hum, bool isReceiving) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    String statusMsg = isReceiving ? "OK (RSSI: " + String(rssi) + ")" : "CON: ESPERANDO (Timeout)";
    
    // Estado de la comunicación LoRa
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, statusMsg);
    
    // Muestra Temperatura
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 15, String(temp, 1) + (char)0xB0 + "C"); 

    // Muestra Humedad
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 45, "Humedad: " + String(hum, 1) + "%");
    
    display.display();
}

