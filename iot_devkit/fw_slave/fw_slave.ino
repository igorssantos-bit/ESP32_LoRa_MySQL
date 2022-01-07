/*******************************************************************************
   fw_slave

   Codigo usado para envio dos dados de leitura de sensores de modulos escravos
   para um modulo mestre (master) usando a tecnologia LoRa ponto-a-ponto (P2P).

   O Hardware utilizado tanto para os modulos escravos quanto para o mestre e
   o IoT DevKit LoRaWAN. Especificacoes:
      Processador principal ESP-32 (WiFi & Bluetooth)
      Módulo LoRaWAN Bee V2 (Transceptor LoRa: SMW-SX1276M0)
      Sensores:   Temperatura e Humidade (DHT11)
                  Acelerometro (MMA8452Q)
                  LDR

   Escrito por Igor Santos (06/01/2022)
 ******************************************************************************/
#if !defined(ARDUINO_ESP32_DEV)
#error Use this example with the ESP32
#endif

/* Includes ------------------------------------------------------------------*/
#include "RoboCore_SMW_SX1276M0.h"
#include "HardwareSerial.h"
#include "ArduinoJson.h"
#include "DHT.h"
#include "RoboCore_MMA8452Q.h"

/* Defines -------------------------------------------------------------------*/
#define LORA_RXD2_PIN   16
#define LORA_TXD2_PIN   17

/* Variables -----------------------------------------------------------------*/
// Declaracao do GPIO
const int pinoDHT = 12;
const int pinoBotao = 4;
const int pinoLDR = 15;
const int statusLED = 13;
const int sendLED = 2;

// Criamos as instancias que serao usadas no decorrer do codigo
HardwareSerial LoRaSerial(2);       // Pinos de Comunicacao LoRa
SMW_SX1276M0 lorawan(LoRaSerial);   // Objeto lora da biblioteca SMW_SX1276
DHT dht(pinoDHT, DHT11);
MMA8452Q acel;
CommandResponse resposta;

// Declaracao das variaveis que armazenam as informacoes da rede
const char DADDR[] = "00000002";
const char P2PDA[] = "00000001";
const char APPSKEY[] = "0000000000000000000000000000000c";
const char NWSKEY[] = "0000000000000000000000000000000b";

// Declaracao das variaveis que armazenam estado ou leitura dos sensores
int valorLDR = 0;
int estado_botao = LOW;

// Declaracao das variaveis de tempo
const unsigned long PAUSE_TIME = 300000; // [ms] (5 min)
unsigned long timeout;

/* Prototypes  ----------------------------------------------------------------*/
void lora_network_init(void);
void event_handler(Event);
void envia_dados(void);

/* Functions  ----------------------------------------------------------------*/

/*!
   Funcao de inicializacao do kit de desenvolvimento
   @param None
   @return None
*/
void setup() {
   // Inicializacao Serial
   Serial.begin(9600);
   Serial.println(F("Lora_Slave_v1.0"));

   // Inicializacao GPIO
   pinMode(pinoBotao, INPUT);
   pinMode(statusLED, OUTPUT);
   pinMode(sendLED, OUTPUT);

   // Inicializacao Lora
   lorawan.setPinReset(5);                       // Define pino
   lorawan.reset();                              // Realiza um reset no modulo
   LoRaSerial.begin(115200, SERIAL_8N1, LORA_RXD2_PIN, LORA_TXD2_PIN);
   lorawan.event_listener = &event_handler;      // Declaracao do handler
   lora_network_init();

   // Inicializacao sensores (DHT11 e Acelerometro)
   dht.begin();
   acel.init();
}

/*!
   Funcao loop que permite o programa realizar acoes sequenciais
   @param None
   @return None
*/
void loop() {
   // Habilita eventos vindos do modulo lora
   lorawan.listen();

   estado_botao = digitalRead(pinoBotao);

   // Envia dados a cada PAUSE_TIMES ms ou ou pressionar o botao
   if (timeout < millis()) {
      envia_dados();
      timeout = millis() + PAUSE_TIME;
   } else if (estado_botao == LOW) {
      delay(30);
      estado_botao = digitalRead(pinoBotao);
      envia_dados();
      // Espero soltar o botao
      while (digitalRead(pinoBotao) == LOW);
   }
}

/*!
   Funcao loop que permite o programa realizar acoes sequenciais
   @param None
   @return None
*/
void lora_network_init(void) {
   //Requisita e imprime o Device EUI do modulo
   char deveui[16];
   resposta = lorawan.get_DevEUI(deveui);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("DevEUI: "));
      Serial.write((uint8_t *)deveui, 16);
      Serial.println();
   } else {
      Serial.println(F("Erro ao obter o Device EUI"));
   }

   // set the Device Address
   resposta = lorawan.set_DevAddr(DADDR);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Device Address set: "));
      Serial.write((uint8_t *)DADDR, 8);
      Serial.println();
   } else {
      Serial.println(F("Error setting the Device Address"));
   }

   // set P2P Device Address
   resposta = lorawan.set_P2PDevAddr(P2PDA);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("P2P Device Address set "));
      Serial.write((uint8_t *)P2PDA, 8);
      Serial.println();
   } else {
      Serial.println(F("Error setting the Device Address"));
   }

   //Configura o Application Session Key no modulo
   resposta = lorawan.set_AppSKey(APPSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Application Session Key configurado "));
      Serial.write((uint8_t *)APPSKEY, 32);
      Serial.println();
   } else {
      Serial.println(F("Erro ao configurar o Application Session Key"));
   }

   //Configura o Network Session Key no modulo
   resposta = lorawan.set_NwkSKey(NWSKEY);
   if (resposta == CommandResponse::OK) {
      Serial.print(F("Network Session Key configurado "));
      Serial.write((uint8_t *)NWSKEY, 32);
      Serial.println();
   } else {
      Serial.println(F("Erro ao configurar o Network Session Key"));
   }

   //Condfigura o metodo de conexao como P2P
   resposta = lorawan.set_JoinMode(SMW_SX1276M0_JOIN_MODE_P2P);
   if (resposta == CommandResponse::OK) {
      Serial.println(F("Metodo de Conexao Configurado como P2P"));
   } else {
      Serial.println(F("Erro ao configurar o metodo P2P"));
   }
}

/*!
   Funcao que trata os eventos do lora
   @param type Tipo de evento
   @return None
*/
void event_handler(Event type) {
   switch(type){
      case (Event::JOINED):{
         Serial.println(F("Conectado!"));
      }
      break;
      
      default:{}
      break;
   }
}

/*!
   Funcao que trata os dados a ser enviados no formato json
   @param None
   @return None
*/
void envia_dados(void) {
   // Acendemos o LED azul do ESP32 quando iniciamos a leitura e envio dos dados
   digitalWrite(sendLED, HIGH);

   // Fazemos a leitura do sensor de luminosidade LDR e mapeamos o valor lido
   valorLDR = analogRead(pinoLDR);
   valorLDR = map(valorLDR, 4095, 2500, 0, 100);

   // Fazemos a leitura da temperatura e umidade
   float temperatura = dht.readTemperature();
   float umidade = dht.readHumidity();

   // Fazemos a leitura do acelerometro e multiplicamos os valores por 100,
   // para torna-los valores inteiros e facilitar o envio para a plataforma
   acel.read();
   int x = acel.x * 100;
   int y = acel.y * 100;
   int z = acel.z * 100;

   // Criamos uma variavel JSON que ira conter os valores das variaveis
   DynamicJsonDocument json(JSON_OBJECT_SIZE(6));

   // Configuramos a variavel JSON com os Alias criados na plataforma ProIoT
   // e salvamos em cada componente o respectivo valor da variavel lida
   json["T"] = temperatura;
   json["U"] = umidade;
   json["L"] = valorLDR;
   json["X"] = x;
   json["Y"] = y;
   json["Z"] = z;

   // Criamos uma String chamada payload que ira conter todas as informacoes do JSON
   String payload = "";
   serializeJson(json, payload);

   Serial.print("Valores enviados: ");
   Serial.println(payload);

   // Enviamos todas as informações via LoRaWAN
   resposta = lorawan.sendT(1, payload);

   // Apagamos o LED azul do ESP32 quando terminamos de enviar
   digitalWrite(sendLED, LOW);
}
