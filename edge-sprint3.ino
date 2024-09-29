#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Definindo componentes
#define TAMANHO 50
#define trigger01 13
#define echo01 12
#define trigger02 33
#define echo02 32
#define ledWiFiPin 2
#define ledMQTTPin 15

// Configurações de rede e MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "test.mosquitto.org";
// const char* mqtt_server = "broker.hivemq.com";

WiFiClient WOKWI_Client;
PubSubClient client(WOKWI_Client);

// Declarando variáveis
float leitura01 = 0;
float leitura02 = 0;
float cm01 = 0;
float cm02 = 0;

bool leitura01Iniciou = false;
bool leitura02Iniciou = false;

unsigned long tempoInicial = 0;
unsigned long tempoFinal = 0;
float intervalo = 0;
float distancia = 100; // Distancia entre os dois sensores (em metros)
float velocidadeMps = 0;
float velocidadeKph = 0;


// FUNÇÕES

// Função para estabelecer conexão com wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando com: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

// Função para estabelecer conexão com MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Tentando conexão MQTT...");
    if (client.connect("WOKWI_Client")) {
      Serial.println("Conectado");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando Novamente em 5 Segundos");
      delay(5000);
    }
  }
}

// Função para feedback de conexão com wifi
void ledWiFi() {
  if (WiFi.status()) {
    digitalWrite(ledWiFiPin, HIGH);
  } else {
    digitalWrite(ledWiFiPin, LOW);
  }
}

// Função para feedback de conexão com MQTT
void ledMQTT() {
  if (client.connected()) {
    digitalWrite(ledMQTTPin, HIGH);
  } else {
    digitalWrite(ledMQTTPin, LOW);
  }
}

void converterJSON() {
  // Iniciando buffer JSON e atribuindo variaveis ao JSON
  StaticJsonDocument<TAMANHO>radar;
  radar["Intervalo_Tempo"] = intervalo;
  radar["Velocidade_Mps"] = velocidadeMps;
  radar["Velocidade_Kph"] = velocidadeKph;
}

// Setup
void setup() {
  // Iniciando o serial
  Serial.begin(115200);

  // Declarando os pinModes dos componentes
  pinMode(trigger01, OUTPUT);
  pinMode(echo01, INPUT);
  pinMode(trigger02, OUTPUT);
  pinMode(echo02, INPUT);
  pinMode(ledWiFiPin, OUTPUT);
  pinMode(ledMQTTPin, OUTPUT);

  // Iniciando WiFi e Cliente MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("Pronto para iniciar medições");
}

// Loop
void loop() {
  // Reconectando WiFi
  reconnect();
  ledWiFi();
  ledMQTT();

  // Iniciando o sensor de distancia ultrasonico 01
  digitalWrite(trigger01, LOW);
  delayMicroseconds(5);
  digitalWrite(trigger01, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger01, LOW);

  // Captando dados dos sensores 01
  leitura01 = pulseIn(echo01, HIGH);

  // Calculando e transformando unidades de medida do sensor 01
  cm01 = leitura01 / 58.0;

  // Checando se a leitura foi iniciada no sensor 01
  if(cm01 <= 300 && !leitura01Iniciou) {
    delay(50);
    if(cm01 <= 300) {
      tempoInicial = millis();
      leitura01Iniciou = true;
      Serial.println("Leitura do Sensor 01 Iniciada");
    }
  }

  // Iniciando o sensor de distancia ultrasonico 02
  digitalWrite(trigger02, LOW);
  delayMicroseconds(5);
  digitalWrite(trigger02, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigger02, LOW);

  // Captando dados do sensor 02
  leitura02 = pulseIn(echo02, HIGH);

  // Calculando e transformando unidades de medida do sensor 02
  cm02 = leitura02 / 58.0;

  // Checando se a leitura foi iniciada no sensor 02
  if(cm02 <= 300 && leitura01Iniciou && !leitura02Iniciou) {
    delay(50);
    if(cm02 <= 300) {
      tempoFinal = millis();
      leitura02Iniciou = true;
      Serial.println("Leitura do Sensor 02 Iniciada");

      // Calculo do intervalo de tempo
      intervalo = (tempoFinal - tempoInicial) / 1000.0;
      Serial.print("Intervalo de Tempo: ");
      Serial.print(intervalo);
      Serial.println(" segundos");

      // Calculo da velocidade
      velocidadeMps = distancia / intervalo;
      velocidadeKph = (float)(distancia / 1000.0) / (float)(intervalo / 3600.0);
      Serial.print("Velociade (m/s): ");
      Serial.print(velocidadeMps);
      Serial.println(" m/s");
      Serial.print("Velociade (km/h): ");
      Serial.print(velocidadeKph);
      Serial.println(" km/h");

      // Resetar estados das leituras
      leitura01Iniciou = false;
      leitura02Iniciou = false;
      delay(500);

      // converterJSON();
      client.publish("challenge/radar/intervalo", String(intervalo).c_str());
      client.publish("challenge/radar/velocidadeMps", String(velocidadeMps).c_str());
      client.publish("challenge/radar/velocidadeKph", String(velocidadeKph).c_str());
    }
  }
  // serializeJson(radar, Serial);
  // Serial.println();
  // delay(1000);
}