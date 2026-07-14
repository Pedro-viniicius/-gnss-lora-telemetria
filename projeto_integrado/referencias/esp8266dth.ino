#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN 2       // GPIO2 (D4 en NodeMCU)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

SoftwareSerial espSerial(13, 15); // RX=5(D1), TX=4(D2)

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  dht.begin();
  delay(2000);
  Serial.println("Iniciando sensor DHT11...");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error de lectura DHT11");
    return;
  }

  String mensaje = "T:" + String(t, 1) + "C H:" + String(h, 1) + "%\n";
  espSerial.print(mensaje);
  Serial.print("Datos enviados: ");
  Serial.println(mensaje);

  delay(3000);
}
