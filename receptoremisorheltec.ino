// --- EMISOR (SENDER) LoRa: Recibe Serial1 de ESP8266 y Transmite por LoRa ---

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h" 

// [ ... CONFIGURACIÓN LORA (sin cambios) ... ]
#define RF_FREQUENCY 915000000 
#define TX_OUTPUT_POWER 20 
#define LORA_BANDWIDTH 0 
#define LORA_SPREADING_FACTOR 7 
#define LORA_CODINGRATE 1 
#define LORA_PREAMBLE_LENGTH 8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 30 
// [ ... FIN CONFIGURACIÓN LORA ... ]

// -------------------------------------------------------------------
// CONFIGURACIÓN SERIAL (Para recibir de la ESP8266) y OLED
// -------------------------------------------------------------------
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); 

// Variables para la recepción Serial1
String serialBuffer = "";
String datoListoParaTx = "";// NUEVO: Buffer temporal para el dato a enviar.
unsigned long ultimoDatoSerial = 0; 
const unsigned long serialTimeout = 5000; 
bool mostrandoErrorSerial = false;
bool datoNuevoListo = false;// Flag para indicar que hay un nuevo paquete para enviar por LoRa

// -------------------------------------------------------------------
// VARIABLES GLOBALES LO-RA
// -------------------------------------------------------------------
char txpacket[BUFFER_SIZE];
bool lora_idle=true; 
unsigned long ultimaTransmision = 0;

// --- DECLARACIONES DE EVENTOS y FUNCIONES ---
static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void mostrarDatos(const String& dataMsg, const String& statusMsg);
void VextON(void);
void VextOFF(void);
void leerSerial(); 
void mostrarErrorConexion(const String& errorMsg);

void setup() {
// [ ... SETUP (sin cambios) ... ]
Serial.begin(115200);
Serial1.begin(9600, SERIAL_8N1, 5, 6); 
delay(100);

Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE); 
VextON();
delay(100);
display.init();
display.setContrast(255);
display.setFont(ArialMT_Plain_10);
display.setTextAlignment(TEXT_ALIGN_CENTER);

RadioEvents.TxDone = OnTxDone;
RadioEvents.TxTimeout = OnTxTimeout; 
Radio.Init( &RadioEvents );
Radio.SetChannel( RF_FREQUENCY );
Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
LORA_SPREADING_FACTOR, LORA_CODINGRATE,
LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

mostrarDatos("Esperando datos Serial...", "INICIADO (Serial->LoRa)");
ultimoDatoSerial = millis(); 
}


void loop()
{
// 1. Leer y procesar los datos de Serial1
leerSerial();

// 2. Verificar timeout de Serial1
if ((millis() - ultimoDatoSerial > serialTimeout) && !mostrandoErrorSerial) {
mostrarErrorConexion("Sin datos de ESP8266");
}

// 3. Lógica de Transmisión LoRa
// Solo intenta enviar si LoRa está libre Y hay un dato nuevo listo
if(lora_idle == true && datoNuevoListo == true) 
{
// Usamos el dato temporalmente almacenado para TX
strncpy(txpacket, datoListoParaTx.c_str(), BUFFER_SIZE - 1);
txpacket[BUFFER_SIZE - 1] = '\0'; 

Serial.printf("\r\nEnviando por LoRa: \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); 
lora_idle = false; 
datoNuevoListo = false; // DESACTIVAR el flag después de iniciar la transmisión
ultimaTransmision = millis();
mostrarDatos(datoListoParaTx, "TX ENVIANDO...");
}
 
// Procesa interrupciones de radio (IMPORTANTE)
Radio.IrqProcess( );
}


// --- FUNCIONES DE SERIAL1 (Ajustadas) ---

void leerSerial() {
while (Serial1.available()) {
  char c = Serial1.read();

if (c == '\n' || c == '\r') {
// Fin de línea detectado
serialBuffer.trim();

// Si recibimos un buffer válido (no vacío)
if (serialBuffer.length() > 0) {
Serial.print("Serial1 Recibido: ");
Serial.println(serialBuffer); 
// 1. Transferir el dato al buffer de TX
datoListoParaTx = serialBuffer; 
 
// 2. Activar el flag
datoNuevoListo = true; 

ultimoDatoSerial = millis();
mostrandoErrorSerial = false;
mostrarDatos(datoListoParaTx, "Serial RX OK");
}

// 3. IMPORTANTE: Limpiar el buffer para la próxima línea.
serialBuffer = "";
} else {
// Construir la cadena
serialBuffer += c;
}
}
}
// [ ... RESTO DE FUNCIONES (OnTxDone, OnTxTimeout, mostrarErrorConexion, VextON/OFF) ... ]

void mostrarErrorConexion(const String& errorMsg) {
 display.clear();
display.setTextAlignment(TEXT_ALIGN_CENTER);
display.setFont(ArialMT_Plain_16);
display.drawString(64, 20, "ERROR SERIAL");
display.setFont(ArialMT_Plain_10);
display.drawString(64, 45, errorMsg);
display.display();

Serial.println("⚠️ ERROR: No se reciben datos por Serial1");
mostrandoErrorSerial = true;
}

void OnTxDone( void )
{
Serial.println("TX done......");
mostrarDatos(datoListoParaTx, "LoRa TX OK");
lora_idle = true;
}

void OnTxTimeout( void )
{
Radio.Sleep( );
Serial.println("TX Timeout......");
mostrarDatos(datoListoParaTx, "LoRa TX FALLO (Timeout)");
lora_idle = true;
}

void mostrarDatos(const String& dataMsg, const String& statusMsg) {
if (mostrandoErrorSerial) return; 

display.clear();
display.setTextAlignment(TEXT_ALIGN_LEFT);

display.setFont(ArialMT_Plain_10);
display.drawString(0, 0, statusMsg);

display.setFont(ArialMT_Plain_16);
display.drawString(0, 30, dataMsg);

display.display(); 
}

void VextON(void) {
pinMode(Vext, OUTPUT);
digitalWrite(Vext, LOW);
}

void VextOFF(void) {
pinMode(Vext, OUTPUT);
digitalWrite(Vext, HIGH);
}