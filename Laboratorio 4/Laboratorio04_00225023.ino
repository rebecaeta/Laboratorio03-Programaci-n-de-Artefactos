#include <WiFi.h>                   // Librería WiFi para ESP32
#include "Adafruit_MQTT.h"          // Librería MQTT
#include "Adafruit_MQTT_Client.h"   // Cliente MQTT para Adafruit IO

/************** CONFIGURACIÓN Wi-Fi **************/
#define WLAN_SSID     "ARTEFACTOS"       // Nombre de tu red WiFi
#define WLAN_PASS     "ARTEFACTOS"   // Contraseña de tu red WiFi

/************** CONFIGURACIÓN Adafruit IO **************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883          // Usa 8883 para SSL
#define AIO_USERNAME  ""
#define AIO_KEY       ""

/************** CLIENTE MQTT **************/
WiFiClient client;  
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/************** FEED DE PUBLICACIÓN **************/
Adafruit_MQTT_Publish pubTemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura");

/************** PIN DEL LM35 **************/
const int PIN_LM35 = 34; // ADC1 CH6 (solo lectura analógica en ESP32)

/************** CONFIGURACIÓN DEL ADC **************/
void configADC() {
  analogReadResolution(12); // Resolución de 12 bits → valores de 0 a 4095
  analogSetPinAttenuation(PIN_LM35, ADC_11db); // Rango ~0..3.3 V
}

/************** FUNCIÓN: Leer temperatura **************/
float leerTemperatura() {
  uint16_t raw = analogRead(PIN_LM35);         // Leer valor crudo (0..4095)
  float volt = (raw / 4095.0f) * 3.3f;         // Convertir a voltios
  return volt * 100.0f;                        // LM35: 10mV/°C → multiplicar por 100
}

/************** FUNCIÓN: Conexión a MQTT **************/
void MQTT_connect() {
  if (mqtt.connected()) return;
  Serial.print("Conectando a Adafruit IO... ");
  int8_t ret;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Reintento en 10 s...");
    mqtt.disconnect();
    delay(10000);
    if (--retries == 0) while (1) delay(1); // Queda esperando reset
  }
  Serial.println("¡Conectado!");
}

/************** SETUP **************/
void setup() {
  Serial.begin(115200);
  configADC();

  Serial.print("Conectando a WiFi...");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado");
}

/************** LOOP **************/
void loop() {
  // Mantener conexión MQTT
  MQTT_connect();
  mqtt.processPackets(10000);
  if (!mqtt.ping()) mqtt.disconnect();

  // Leer temperatura y publicar
  float tempC = leerTemperatura();
  if (pubTemp.publish(tempC)) {
    Serial.printf("Temperatura enviada: %.2f °C\n", tempC);
  } else {
    Serial.println("Error al publicar temperatura");
  }

  delay(2000); // Publicar cada 2 segundos
}