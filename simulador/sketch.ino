/*
  Contenedor inteligente - llenado + bateria + envio a ThingSpeak (canal 3486377)
  HC-SR04: TRIG -> GPIO 5, ECHO -> GPIO 18, VCC -> 5V, GND -> GND
  Potenciometro (hace de bateria): SIG -> GPIO 34, VCC -> 3V3, GND -> GND
  Envia field1 = llenado (%) y field2 = bateria (%).
  Simulacion: cada 20 s (ThingSpeak gratis acepta como mucho uno cada 15 s).
  Equipo real: cada 15 min, con MODO_SIMULACION en false.
*/

#include <WiFi.h>
#include <HTTPClient.h>

const int PIN_TRIG = 5;
const int PIN_ECHO = 18;
const int PIN_BATERIA = 34;

const float ALTURA_CONTENEDOR_CM = 40.0;
const float UMBRAL_ALERTA_PORCENTAJE = 80.0;
const float HISTERESIS = 3.0;

const bool  MODO_SIMULACION = true;   // true en Wokwi, false en el equipo real
const float UMBRAL_BATERIA_BAJA = 20.0;

// En Wokwi la red "Wokwi-GUEST" no tiene contrasena.
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
// Write API Key del canal (ThingSpeak > tu canal > "API Keys"). No la publiques.
const char* THINGSPEAK_WRITE_KEY = "TU_WRITE_API_KEY";   // en Wokwi esta la clave real
const unsigned long INTERVALO_ENVIO_MS = MODO_SIMULACION ? 20000UL : 15UL * 60UL * 1000UL;

bool alertaActiva = false;
unsigned long ultimoEnvio = 0;
bool primerEnvio = true;

// ---------- Sensor de llenado ----------

float leerDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracion == 0) {
    return -1.0;
  }
  return duracion * 0.0343 / 2.0;
}

float calcularPorcentajeLlenado(float distanciaCm) {
  if (distanciaCm < 0) {
    return -1.0;
  }
  float porcentaje = 100.0 * (1.0 - (distanciaCm / ALTURA_CONTENEDOR_CM));
  if (porcentaje < 0) porcentaje = 0;
  if (porcentaje > 100) porcentaje = 100;
  return porcentaje;
}

bool evaluarAlerta(float porcentaje) {
  if (porcentaje < 0) {
    return alertaActiva;
  }
  if (!alertaActiva && porcentaje >= UMBRAL_ALERTA_PORCENTAJE) {
    alertaActiva = true;
  } else if (alertaActiva && porcentaje < (UMBRAL_ALERTA_PORCENTAJE - HISTERESIS)) {
    alertaActiva = false;
  }
  return alertaActiva;
}

// ---------- Bateria ----------

float leerTensionBateria() {
  int lectura = analogRead(PIN_BATERIA);           // 0 a 4095
  if (MODO_SIMULACION) {
    // tope a la izquierda = 3,0 V (vacia), tope a la derecha = 4,2 V (llena)
    return 3.0 + (lectura / 4095.0) * 1.2;
  }
  // Equipo real: divisor 100k/100k, la bateria es el doble de lo que ve el pin
  float tensionPin = (lectura / 4095.0) * 3.3;
  return tensionPin * 2.0;
}

// Tension -> carga no es lineal: tabla aproximada e interpolacion.
float porcentajeBateria(float tension) {
  const float V[]   = {3.00, 3.30, 3.50, 3.60, 3.70, 3.80, 3.90, 4.00, 4.10, 4.20};
  const float PCT[] = {   0,    3,   10,   22,   38,   52,   65,   78,   90,  100};
  const int N = 10;

  if (tension <= V[0]) return 0;
  if (tension >= V[N - 1]) return 100;
  for (int i = 0; i < N - 1; i++) {
    if (tension < V[i + 1]) {
      float t = (tension - V[i]) / (V[i + 1] - V[i]);
      return PCT[i] + t * (PCT[i + 1] - PCT[i]);
    }
  }
  return 100;
}

// ---------- WiFi y envio ----------

bool conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.print("# Conectando al WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);   // canal 6: en Wokwi conecta mas rapido
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 10000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " conectado" : " sin conexion");
  return WiFi.status() == WL_CONNECTED;
}

void enviarThingSpeak(float porcentaje, float bateria) {
  if (!conectarWiFi()) {
    Serial.println("# Envio omitido: sin WiFi");
    return;
  }
  String url = "http://api.thingspeak.com/update?api_key=";
  url += THINGSPEAK_WRITE_KEY;
  if (porcentaje >= 0) {                 // SIN_DATO: no se manda field1
    url += "&field1=" + String(porcentaje, 1);
  }
    url += "&field2=" + String((int)(bateria + 0.5));   // entero redondeado, sin espacios

  Serial.println("# Largo de la clave: " + String(strlen(THINGSPEAK_WRITE_KEY)) + " (tiene que ser 16)");
  Serial.println("# Datos enviados: " + url.substring(url.indexOf("&")));

  HTTPClient http;
  http.begin(url);
  int codigo = http.GET();
  String respuesta = http.getString();   // numero de entrada, o 0 si rechazo el envio
  http.end();

  if (codigo == 200 && respuesta != "0") {
    Serial.println("# Enviado a ThingSpeak, entrada " + respuesta);
  } else {
    Serial.println("# Error al enviar a ThingSpeak (HTTP " + String(codigo) + ", respuesta " + respuesta + ")");
  }
}

// ---------- Programa principal ----------

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  analogReadResolution(12);
  conectarWiFi();
  Serial.println("timestamp_ms,distancia_cm,porcentaje_llenado,alerta,bateria_v,bateria_pct,bateria_baja");
}

void loop() {
  float distancia = leerDistanciaCm();
  float porcentaje = calcularPorcentajeLlenado(distancia);
  bool alerta = evaluarAlerta(porcentaje);

  float tension = leerTensionBateria();
  float bateria = porcentajeBateria(tension);
  bool bateriaBaja = bateria < UMBRAL_BATERIA_BAJA;

  Serial.print(millis());
  Serial.print(",");

  if (distancia < 0) {
    Serial.print("SIN_DATO,SIN_DATO,");
  } else {
    Serial.print(distancia);
    Serial.print(",");
    Serial.print(porcentaje);
    Serial.print(",");
  }

  Serial.print(alerta ? "SI" : "NO");
  Serial.print(",");
  Serial.print(tension, 2);
  Serial.print(",");
  Serial.print(bateria, 0);
  Serial.print(",");
  Serial.println(bateriaBaja ? "SI" : "NO");

  if (primerEnvio || millis() - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    enviarThingSpeak(porcentaje, bateria);
    ultimoEnvio = millis();
    primerEnvio = false;
  }

  delay(1000); // en el equipo real: deep sleep de 15 minutos entre mediciones
}